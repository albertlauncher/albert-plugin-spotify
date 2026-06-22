// Copyright (c) 2025-2026 Manuel Schneider

#include "handlers.h"
#include "items.h"
#include "plugin.h"
#include <QCoreApplication>
#include <QCoroAsyncGenerator>
#include <QCoroNetworkReply>
#include <QCoroSignal>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QThread>
#include <albert/app.h>
#include <albert/icon.h>
#include <albert/logging.h>
#include <albert/queryexecution.h>
#include <albert/queryresults.h>
#include <albert/standarditem.h>
#include <ranges>
using namespace Qt::StringLiterals;
using namespace albert::detail;
using namespace albert;
using namespace std;

static const auto items_key = "items"_L1;
static const auto batch_size = 10u;

static auto makeErrorItem(const QString &error)
{
    WARN << error;
    auto ico_fac = [] {
        return Icon::composed(Icon::theme(u"spotify"_s), Icon::standard(Icon::MessageBoxWarning));
    };
    return StandardItem::make(u"notify"_s,
                              u"Spotify"_s,
                              error,
                              ::move(ico_fac),
                              {{u"settings"_s, Plugin::tr("Open settings"),
                                [] { app().showSettings(u"spotify"_s); }}});
}

SpotifySearchHandler::SpotifySearchHandler(API &api,
                                           SearchType type,
                                           const QString &name,
                                           const QString &description) :
    api_(api),
    type_(type),
    name_(name),
    description_(description)
{}

QString SpotifySearchHandler::id() const { return typeString(type_); }

QString SpotifySearchHandler::name() const { return name_; }

QString SpotifySearchHandler::description() const { return description_; }

QString SpotifySearchHandler::defaultTrigger() const
{ return localizedTypeString(type_).toLower() + QChar::Space; }

AsyncItemGenerator SpotifySearchHandler::items(QueryContext ctx)
{
    try {
        for (auto page = 0;; ++page)
        {
            co_await api_.rate_limiter.acquire();

            if (!ctx.isValid())
                co_return;

            unique_ptr<QNetworkReply> reply{fetch(ctx, page)};

            co_await qCoro(reply.get()).waitForFinished();  // TODO: QCoro>13 QCoroNetworkReply

            if (const auto exp_doc = API::parseJson(reply.get()); exp_doc)
            {
                // TODO: GCC>13 yieling temporaries is fine
                auto v = handleReply(ctx, *exp_doc);
                co_yield ::move(v);
            }
            else
            {
                // TODO: GCC>13 yieling temporaries is fine
                vector<shared_ptr<Item>> v{makeErrorItem(exp_doc.error())};
                co_yield ::move(v);
                co_return;
            }
        }
    }
    catch (const exception &e) { CRIT << "Exception while fetching data:" << e.what(); }
    catch (...) { CRIT << "Unknown exception while fetching data."; }
}

//--------------------------------------------------------------------------------------------------

TrackSearchHandler::TrackSearchHandler(API &api) :
    SpotifySearchHandler(api,
                         Track,
                         Plugin::tr("Spotify tracks"),
                         Plugin::tr("Search Spotify tracks"))
{}

QNetworkReply *TrackSearchHandler::fetch(QueryContext ctx, uint page) const
{
    return ctx.query().isEmpty()
               ? api_.userTopTracks(batch_size, page * batch_size)
               : api_.search(ctx, Track, batch_size, page * batch_size);
}

vector<shared_ptr<Item>>
TrackSearchHandler::handleReply(QueryContext ctx, const QJsonDocument &doc)
{
    const auto items = ctx.query().isEmpty()
                           ? doc[items_key]
                           : doc[u"%1s"_s.arg(typeString(type_))][items_key];

    auto v = items.toArray()
           | views::filter([](const auto &val) { return !val.isNull(); })
           | views::transform([this](const auto &val) {
                 return make_shared<TrackItem>(api_, val.toObject());
             });

    return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
}

//--------------------------------------------------------------------------------------------------

ArtistSearchHandler::ArtistSearchHandler(API &api) :
    SpotifySearchHandler(api,
                         Artist,
                         Plugin::tr("Spotify artists"),
                         Plugin::tr("Search Spotify artists"))
{}

QNetworkReply *ArtistSearchHandler::fetch(QueryContext ctx, uint page) const
{
    return ctx.query().isEmpty()
               ? api_.userTopArtists(batch_size, page * batch_size)
               : api_.search(ctx, type_, batch_size, page * batch_size);
}

vector<shared_ptr<Item>>
ArtistSearchHandler::handleReply(QueryContext ctx, const QJsonDocument &doc)
{
    const auto items = ctx.query().isEmpty() ? doc[items_key]
                                             : doc[u"%1s"_s.arg(typeString(type_))][items_key];

    auto v = items.toArray()
             | views::filter([](const auto &val) { return !val.isNull(); })
             | views::transform([this](const auto &val) {
                     return make_shared<ArtistItem>(api_, val.toObject());
               });

    return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
}

//--------------------------------------------------------------------------------------------------

AlbumSearchHandler::AlbumSearchHandler(API &api) :
    SpotifySearchHandler(api,
                         Album,
                         Plugin::tr("Spotify albums"),
                         Plugin::tr("Search Spotify albums"))
{}

QNetworkReply *AlbumSearchHandler::fetch(QueryContext ctx, uint page) const
{
    return ctx.query().isEmpty()
               ? api_.userAlbums(batch_size, page * batch_size)
               : api_.search(ctx, type_, batch_size, page * batch_size);
}

vector<shared_ptr<Item>>
AlbumSearchHandler::handleReply(QueryContext ctx, const QJsonDocument &doc)
{
    if (ctx.query().isEmpty())
    {
        auto v = doc[items_key].toArray()
                 | views::filter([](const auto &val) { return !val.isNull(); })
                 | views::transform([this](const auto &val) {
                       const auto album = val[typeString(type_)].toObject();
                       return make_shared<AlbumItem>(api_, album);
                   });
        return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
    }
    else
    {
        auto v = doc[u"%1s"_s.arg(typeString(type_))][items_key].toArray()
                 | views::filter([](const auto &val) { return !val.isNull(); })
                 | views::transform([this](const auto &val) {
                       return make_shared<AlbumItem>(api_, val.toObject());
                   });
        return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
    }
}

//--------------------------------------------------------------------------------------------------

PlaylistSearchHandler::PlaylistSearchHandler(API &api) :
    SpotifySearchHandler(api,
                         Playlist,
                         Plugin::tr("Spotify playlists"),
                         Plugin::tr("Search Spotify playlists"))
{}

QNetworkReply *PlaylistSearchHandler::fetch(QueryContext ctx, uint page) const
{
    return ctx.query().isEmpty()
               ? api_.userPlaylists(batch_size, page * batch_size)
               : api_.search(ctx, type_, batch_size, page * batch_size);
}

vector<shared_ptr<Item>>
PlaylistSearchHandler::handleReply(QueryContext ctx, const QJsonDocument &doc)
{
    const auto items = ctx.query().isEmpty()
                           ? doc[items_key]
                           : doc[u"%1s"_s.arg(typeString(type_))][items_key];

    auto v = items.toArray()
             | views::filter([](const auto &val) { return !val.isNull(); })
             | views::transform([this](const auto &val) {
                   return make_shared<PlaylistItem>(api_, val.toObject());
               });

    return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
}

//--------------------------------------------------------------------------------------------------

ShowSearchHandler::ShowSearchHandler(API &api) :
    SpotifySearchHandler(api,
                         Show,
                         Plugin::tr("Spotify shows"),
                         Plugin::tr("Search Spotify shows"))
{}

QNetworkReply *ShowSearchHandler::fetch(QueryContext ctx, uint page) const
{
    return ctx.query().isEmpty()
               ? api_.userShows(batch_size, page * batch_size)
               : api_.search(ctx, type_, batch_size, page * batch_size);
}

vector<shared_ptr<Item>>
ShowSearchHandler::handleReply(QueryContext ctx, const QJsonDocument &doc)
{
    if (ctx.query().isEmpty())
    {
        auto v = doc[items_key].toArray()
                 | views::filter([](const auto &val) { return !val.isNull(); })
                 | views::transform([this](const auto &val) {
                       const auto album = val[typeString(type_)].toObject();
                       return make_shared<ShowItem>(api_, album);
                   });
        return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
    }
    else
    {
        auto v = doc[u"%1s"_s.arg(typeString(type_))][items_key].toArray()
                 | views::filter([](const auto &val) { return !val.isNull(); })
                 | views::transform([this](const auto &val) {
                       return make_shared<ShowItem>(api_, val.toObject());
                   });
        return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
    }
}

//--------------------------------------------------------------------------------------------------

EpisodeSearchHandler::EpisodeSearchHandler(API &api) :
    SpotifySearchHandler(api,
                         Episode,
                         Plugin::tr("Spotify episodes"),
                         Plugin::tr("Search Spotify episodes"))
{}

QNetworkReply *EpisodeSearchHandler::fetch(QueryContext ctx, uint page) const
{
    return ctx.query().isEmpty()
               ? api_.userEpisodes(batch_size, page * batch_size)
               : api_.search(ctx, type_, batch_size, page * batch_size);
}

vector<shared_ptr<Item>>
EpisodeSearchHandler::handleReply(QueryContext ctx, const QJsonDocument &doc)
{
    if (ctx.query().isEmpty())
    {
       // endpoint beta and buggy af random null and non null but null filled items
       auto v = doc[items_key].toArray()
                | views::filter([](const auto &val) { return !val.isNull(); })
                | views::transform([this](const auto &val) { return val[typeString(type_)]; })
                | views::filter([](const auto &val) { return !val["id"_L1].isNull(); })
                | views::transform([this](const auto &val) {
                      return make_shared<EpisodeItem>(api_, val.toObject());
                  });
       return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
    }
    else
    {
        auto v = doc[u"%1s"_s.arg(typeString(type_))][items_key].toArray()
                 | views::filter([](const auto &val) { return !val.isNull(); })
                 | views::transform([this](const auto &val) {
                       return make_shared<EpisodeItem>(api_, val.toObject());
                   });
        return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
    }
}

//--------------------------------------------------------------------------------------------------

AudiobookSearchHandler::AudiobookSearchHandler(API &api) :
    SpotifySearchHandler(api,
                         Audiobook,
                         Plugin::tr("Spotify audiobooks"),
                         Plugin::tr("Search Spotify audiobooks"))
{}

QNetworkReply *AudiobookSearchHandler::fetch(QueryContext ctx, uint page) const
{
    return ctx.query().isEmpty()
               ? api_.userAudiobooks(batch_size, page * batch_size)
               : api_.search(ctx, type_, batch_size, page * batch_size);
}

vector<shared_ptr<Item>>
AudiobookSearchHandler::handleReply(QueryContext ctx, const QJsonDocument &doc)
{
    const auto items = ctx.query().isEmpty()
                           ? doc[items_key]
                           : doc[u"%1s"_s.arg(typeString(type_))][items_key];

    auto v = items.toArray()
             | views::filter([](const auto &val) { return !val.isNull(); })
             | views::transform([this](const auto &val) {
                   return make_shared<AudiobookItem>(api_, val.toObject());
               });

    return vector<shared_ptr<Item>>{begin(v), end(v)};  // ranges::to
}
