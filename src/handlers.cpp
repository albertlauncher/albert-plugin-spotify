// Copyright (c) 2025-2025 Manuel Schneider

#include "handlers.h"
#include "items.h"
#include "plugin.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QThread>
#include <albert/iconutil.h>
#include <albert/logging.h>
#include <albert/queryexecution.h>
#include <albert/queryresults.h>
#include <albert/standarditem.h>
#include <ranges>
using namespace Qt::StringLiterals;
using namespace albert::detail;
using namespace albert;
using namespace spotify;
using namespace std;

static const auto items_key = "items"_L1;
static const auto batch_size = 10u;

static auto makeErrorItem(const QString &error)
{
    WARN << error;
    auto ico_fac = [] { return makeComposedIcon(makeThemeIcon(u"spotify"_s),
                                                makeStandardIcon(MessageBoxWarning));};
    return StandardItem::make(u"notify"_s, u"Spotify"_s, error, ::move(ico_fac));
}

SpotifySearchHandler::SpotifySearchHandler(const RestApi &api_,
                                           SearchType type_,
                                           const QString &name,
                                           const QString &description) :
    api(api_),
    type(type_),
    name_(name),
    description_(description),
    rate_limiter_(1000)
{}

QString SpotifySearchHandler::id() const { return typeString(type); }

QString SpotifySearchHandler::name() const { return name_; }

QString SpotifySearchHandler::description() const { return description_; }

QString SpotifySearchHandler::defaultTrigger() const
{ return localizedTypeString(type).toLower() + QChar::Space; }


class SpotifySearchHandler::QueryExecution : public albert::QueryExecution
{
public:

    SpotifySearchHandler &handler;
    unique_ptr<Acquire> acquire;
    unique_ptr<QNetworkReply> reply;
    int batch;  // also valid flag
    bool active;

    QueryExecution(SpotifySearchHandler &h, Query &q)
        : albert::QueryExecution(q)
        , handler(h)
        , acquire(nullptr)
        , reply(nullptr)
        , batch(0)
        , active(false)
    {
        fetchMore();
    }

    ~QueryExecution() override {}

    void cancel() override final
    {
        acquire.reset();
        reply.reset();
        emit activeChanged(active = false);
        batch = -1;
    }

    void fetchMore() override final
    {
        if (isActive() || !canFetchMore())
            return;

        emit activeChanged(active = true);
        acquire = handler.rate_limiter_.acquire();
        connect(acquire.get(), &Acquire::granted, this, [this]
        {
            reply.reset(fetch(batch++));
            connect(reply.get(), &QNetworkReply::finished, this, [this]
            {
                if (const auto var = RestApi::parseJson(reply.get());
                    holds_alternative<QJsonDocument>(var))
                    handleReply(get<QJsonDocument>(var));
                else
                {
                    results.add(handler, makeErrorItem(get<QString>(var)));
                    batch = -1;
                }
                reply.reset();
                emit activeChanged(active = false);
            }, Qt::SingleShotConnection);
        }, Qt::SingleShotConnection);
    }

    bool canFetchMore() const override final { return batch >= 0; }

    bool isActive() const override final { return active; }

    virtual QNetworkReply *fetch(uint page) const = 0;

    virtual void handleReply(const QJsonDocument &) = 0;
};

//--------------------------------------------------------------------------------------------------

TrackSearchHandler::TrackSearchHandler(RestApi &api) :
    SpotifySearchHandler(api,
                         Track,
                         Plugin::tr("Spotify tracks"),
                         Plugin::tr("Search Spotify tracks"))
{}

unique_ptr<QueryExecution> TrackSearchHandler::execution(Query &q)
{
    struct Execution : public SpotifySearchHandler::QueryExecution
    {
        using QueryExecution::QueryExecution;

        QNetworkReply *fetch(uint batch) const override
        {
            return query.string().isEmpty()
                            ? handler.api.userTopTracks(batch_size, batch * batch_size)
                            : handler.api.search(query, Track, batch_size, batch * batch_size);
        }

        void handleReply(const QJsonDocument &doc) override
        {
            const auto items = query.string().isEmpty()
                                   ? doc[items_key]
                                   : doc[u"%1s"_s.arg(typeString(handler.type))][items_key];
            results.add(handler,
                        items.toArray()
                        | views::filter([](const auto &val) { return !val.isNull(); })
                        | views::transform([this](const auto &val) {
                            return make_shared<TrackItem>(handler.api, val.toObject());
                        }));
        }
    };

    return make_unique<Execution>(*this, q);
}

//--------------------------------------------------------------------------------------------------

ArtistSearchHandler::ArtistSearchHandler(RestApi &api) :
    SpotifySearchHandler(api,
                         Artist,
                         Plugin::tr("Spotify artists"),
                         Plugin::tr("Search Spotify artists"))
{}

unique_ptr<QueryExecution> ArtistSearchHandler::execution(Query &q)
{
    struct Execution : public SpotifySearchHandler::QueryExecution
    {
        using QueryExecution::QueryExecution;

        QNetworkReply *fetch(uint batch) const override
        {
            return query.string().isEmpty()
                       ? handler.api.userTopArtists(batch_size, batch * batch_size)
                       : handler.api.search(query, handler.type, batch_size, batch * batch_size);
        }

        void handleReply(const QJsonDocument &doc) override
        {
            const auto items = query.string().isEmpty()
                                   ? doc[items_key]
                                   : doc[u"%1s"_s.arg(typeString(handler.type))][items_key];
            results.add(handler,
                        items.toArray()
                        | views::filter([](const auto &val) { return !val.isNull(); })
                        | views::transform([this](const auto &val) {
                            return make_shared<ArtistItem>(handler.api, val.toObject());
                        }));
        }
    };

    return make_unique<Execution>(*this, q);
}

//--------------------------------------------------------------------------------------------------

AlbumSearchHandler::AlbumSearchHandler(RestApi &api) :
    SpotifySearchHandler(api,
                         Album,
                         Plugin::tr("Spotify albums"),
                         Plugin::tr("Search Spotify albums"))
{}

unique_ptr<QueryExecution> AlbumSearchHandler::execution(Query &q)
{
    struct Execution : public SpotifySearchHandler::QueryExecution
    {
        using QueryExecution::QueryExecution;

        QNetworkReply *fetch(uint batch) const override
        {
            return query.string().isEmpty()
                       ? handler.api.userAlbums(batch_size, batch * batch_size)
                       : handler.api.search(query, handler.type, batch_size, batch * batch_size);
        }

        void handleReply(const QJsonDocument &doc) override
        {
            if (query.string().isEmpty())
                results.add(handler,
                            doc[items_key].toArray()
                            | views::filter([](const auto &val) { return !val.isNull();})
                            | views::transform([this](const auto &val) {
                                const auto album = val[typeString(handler.type)].toObject();
                                return make_shared<AlbumItem>(handler.api, album);
                            }));
            else
                results.add(handler,
                            doc[u"%1s"_s.arg(typeString(handler.type))][items_key].toArray()
                            | views::filter([](const auto &val) { return !val.isNull(); })
                            | views::transform([this](const auto &val) {
                                return make_shared<AlbumItem>(handler.api, val.toObject());
                            }));
        }
    };

    return make_unique<Execution>(*this, q);
}

//--------------------------------------------------------------------------------------------------

PlaylistSearchHandler::PlaylistSearchHandler(RestApi &api) :
    SpotifySearchHandler(api,
                         Playlist,
                         Plugin::tr("Spotify playlists"),
                         Plugin::tr("Search Spotify playlists"))
{}

unique_ptr<QueryExecution> PlaylistSearchHandler::execution(Query &q)
{
    struct Execution : public SpotifySearchHandler::QueryExecution
    {
        using QueryExecution::QueryExecution;

        QNetworkReply *fetch(uint batch) const override
        {
            return query.string().isEmpty()
                       ? handler.api.userPlaylists(batch_size, batch * batch_size)
                       : handler.api.search(query, handler.type, batch_size, batch * batch_size);
        }

        void handleReply(const QJsonDocument &doc) override
        {
            const auto items = query.string().isEmpty()
                                   ? doc[items_key]
                                   : doc[u"%1s"_s.arg(typeString(handler.type))][items_key];

            results.add(handler,
                        items.toArray()
                        | views::filter([](const auto &val) { return !val.isNull(); })
                        | views::transform([this](const auto &val) {
                            return make_shared<PlaylistItem>(handler.api, val.toObject());
                        }));
        }
    };

    return make_unique<Execution>(*this, q);
}

//--------------------------------------------------------------------------------------------------

ShowSearchHandler::ShowSearchHandler(RestApi &api) :
    SpotifySearchHandler(api,
                         Show,
                         Plugin::tr("Spotify shows"),
                         Plugin::tr("Search Spotify shows"))
{}

unique_ptr<QueryExecution> ShowSearchHandler::execution(Query &q)
{
    struct Execution : public SpotifySearchHandler::QueryExecution
    {
        using QueryExecution::QueryExecution;

        QNetworkReply *fetch(uint batch) const override
        {
            return query.string().isEmpty()
                       ? handler.api.userShows(batch_size, batch * batch_size)
                       : handler.api.search(query, handler.type, batch_size, batch * batch_size);
        }

        void handleReply(const QJsonDocument &doc) override
        {
            if (query.string().isEmpty())
                results.add(handler,
                            doc[items_key].toArray()
                            | views::filter([](const auto &val) { return !val.isNull(); })
                            | views::transform([this](const auto &val) {
                                const auto album = val[typeString(handler.type)].toObject();
                                return make_shared<ShowItem>(handler.api, album);
                            }));
            else
                results.add(handler,
                            doc[u"%1s"_s.arg(typeString(handler.type))][items_key].toArray()
                            | views::filter([](const auto &val) { return !val.isNull(); })
                            | views::transform([this](const auto &val) {
                                return make_shared<ShowItem>(handler.api, val.toObject());
                            }));
        }
    };

    return make_unique<Execution>(*this, q);
}

//--------------------------------------------------------------------------------------------------

EpisodeSearchHandler::EpisodeSearchHandler(RestApi &api) :
    SpotifySearchHandler(api,
                         Episode,
                         Plugin::tr("Spotify episodes"),
                         Plugin::tr("Search Spotify episodes"))
{}

unique_ptr<QueryExecution> EpisodeSearchHandler::execution(Query &q)
{
    struct Execution : public SpotifySearchHandler::QueryExecution
    {
        using QueryExecution::QueryExecution;

        QNetworkReply *fetch(uint batch) const override
        {
            return query.string().isEmpty()
                       ? handler.api.userEpisodes(batch_size, batch * batch_size)
                       : handler.api.search(query, handler.type, batch_size, batch * batch_size);
        }

        void handleReply(const QJsonDocument &doc) override
        {
            if (query.string().isEmpty())
                // endpoint beta and buggy af random null and non null but null filled items
                results.add(handler,
                            doc[items_key].toArray()
                            | views::filter([](const auto &val) { return !val.isNull(); })
                            | views::transform([this](const auto &val) {
                                  return val[typeString(handler.type)];
                            })
                            | views::filter([](const auto &val) { return !val["id"_L1].isNull(); })
                            | views::transform([this](const auto &val) {
                                return make_shared<EpisodeItem>(handler.api, val.toObject());
                            }));
            else
                results.add(handler,
                            doc[u"%1s"_s.arg(typeString(handler.type))][items_key].toArray()
                            | views::filter([](const auto &val) { return !val.isNull(); })
                            | views::transform([this](const auto &val) {
                                return make_shared<EpisodeItem>(handler.api, val.toObject());
                            }));
        }
    };

    return make_unique<Execution>(*this, q);
}

//--------------------------------------------------------------------------------------------------

AudiobookSearchHandler::AudiobookSearchHandler(RestApi &api) :
    SpotifySearchHandler(api,
                         Audiobook,
                         Plugin::tr("Spotify audiobooks"),
                         Plugin::tr("Search Spotify audiobooks"))
{}

unique_ptr<QueryExecution> AudiobookSearchHandler::execution(Query &q)
{
    struct Execution : public SpotifySearchHandler::QueryExecution
    {
        using QueryExecution::QueryExecution;

        QNetworkReply *fetch(uint batch) const override
        {
            return query.string().isEmpty()
                       ? handler.api.userAudiobooks(batch_size, batch * batch_size)
                       : handler.api.search(query, handler.type, batch_size, batch * batch_size);
        }

        void handleReply(const QJsonDocument &doc) override
        {
            const auto items = query.string().isEmpty()
                                   ? doc[items_key]
                                   : doc[u"%1s"_s.arg(typeString(handler.type))][items_key];
            results.add(handler,
                        items.toArray()
                        | views::filter([](const auto &val) { return !val.isNull(); })
                        | views::transform([this](const auto &val) {
                            return make_shared<AudiobookItem>(handler.api, val.toObject());
                        }));
        }
    };

    return make_unique<Execution>(*this, q);
}
