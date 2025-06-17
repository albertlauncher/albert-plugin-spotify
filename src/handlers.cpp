// Copyright (c) 2025-2025 Manuel Schneider

#include "handlers.h"
#include "items.h"
#include "plugin.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>
#include <albert/logging.h>
#include <albert/standarditem.h>
using namespace Qt::StringLiterals;
using namespace albert::util;
using namespace albert;
using namespace spotify;
using namespace std;

static const auto items = u"items";

static shared_ptr<Item> makeErrorItem(const QString &error)
{
    static const QStringList icon{u"comp:?src1=%3Aspotify&src2=qsp%3ASP_MessageBoxWarning"_s};
    WARN << error;
    return StandardItem::make(u"notify"_s, u"Spotify"_s, error, icon);
}

SpotifySearchHandler::SpotifySearchHandler(RestApi &api,
                                           const QString &id,
                                           const QString &name,
                                           const QString &description):
    api_(api),
    limiter_(1000),
    id_(id),
    name_(name),
    description_(description)
{}

QString SpotifySearchHandler::id() const { return id_; }

QString SpotifySearchHandler::name() const { return name_; }

QString SpotifySearchHandler::description() const { return description_; }

QString SpotifySearchHandler::defaultTrigger() const
{ return localizedTypeString(type()).toLower() + QChar::Space; }

void SpotifySearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        return;

    else if (!limiter_.debounce(q.isValid()))
        return;

    else if (const auto var = RestApi::parseJson(await(api_.search(q, type())));
             holds_alternative<QString>(var))
        q.add(makeErrorItem(get<QString>(var)));

    else
    {
        vector<shared_ptr<Item>> r;
        const auto key = u"%1s"_s.arg(typeString(type()));
        const auto a = get<QJsonDocument>(var)[key][items].toArray();
        for (const auto &v : a)
            if (!v.isNull())
            {
                auto item = parseItem(v.toObject());
                item->moveToThread(qApp->thread());
                r.emplace_back(::move(item));
            }
        q.add(::move(r));
    }
}

void SpotifySearchHandler::apiCall(
    albert::Query &q,
    function<QNetworkReply*()> api_call,
    function<void(const QJsonDocument&, vector<shared_ptr<Item>>&)> success_handler) const
{
    if (static auto limiter = albert::detail::RateLimiter(1000);
        !limiter.debounce(q.isValid()))
        return;

    else if (const auto var = RestApi::parseJson(await(api_call()));
             holds_alternative<QString>(var))
        q.add(makeErrorItem(get<QString>(var)));

    else
    {
        const auto json = get<QJsonDocument>(var);
        vector<shared_ptr<Item>> r;
        success_handler(json, r);
        q.add(::move(r));
    }
}

//--------------------------------------------------------------------------------------------------

TrackSearchHandler::TrackSearchHandler(RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify tracks"),
                         Plugin::tr("Search Spotify tracks"))
{}

SearchType TrackSearchHandler::type() const { return Track; }

std::shared_ptr<SpotifyItem> TrackSearchHandler::parseItem(const QJsonObject &o) const
{ return make_shared<TrackItem>(api_, o); }

void TrackSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return api_.userTopTracks(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                    {
                        auto item = make_shared<TrackItem>(api_, v.toObject());
                        item->moveToThread(qApp->thread());
                        r.emplace_back(::move(item));
                    }
                });
    else
        SpotifySearchHandler::handleTriggerQuery(q);
}

//--------------------------------------------------------------------------------------------------

ArtistSearchHandler::ArtistSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify artists"),
                         Plugin::tr("Search Spotify artists"))
{}

SearchType ArtistSearchHandler::type() const { return Artist; }

std::shared_ptr<SpotifyItem> ArtistSearchHandler::parseItem(const QJsonObject &o) const
{ return make_shared<ArtistItem>(api_, o); }

void ArtistSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return api_.userTopArtists(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                    {
                        auto item = make_shared<ArtistItem>(api_, v.toObject());
                        item->moveToThread(qApp->thread());
                        r.emplace_back(::move(item));
                    }
                });
    else
        SpotifySearchHandler::handleTriggerQuery(q);
}


//--------------------------------------------------------------------------------------------------

AlbumSearchHandler::AlbumSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify albums"),
                         Plugin::tr("Search Spotify albums"))
{}

SearchType AlbumSearchHandler::type() const { return Album; }

std::shared_ptr<SpotifyItem> AlbumSearchHandler::parseItem(const QJsonObject &o) const
{ return make_shared<AlbumItem>(api_, o); }

void AlbumSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return api_.userAlbums(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                    {
                        auto item = make_shared<AlbumItem>(api_, v[typeString(type())].toObject());
                        item->moveToThread(qApp->thread());
                        r.emplace_back(::move(item));
                    }
                });
    else
        SpotifySearchHandler::handleTriggerQuery(q);
}

//--------------------------------------------------------------------------------------------------

PlaylistSearchHandler::PlaylistSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify playlists"),
                         Plugin::tr("Search Spotify playlists"))
{}

SearchType PlaylistSearchHandler::type() const { return Playlist; }

std::shared_ptr<SpotifyItem> PlaylistSearchHandler::parseItem(const QJsonObject &o) const
{ return make_shared<PlaylistItem>(api_, o); }

void PlaylistSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return api_.userPlaylists(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                    {
                        auto item = make_shared<PlaylistItem>(api_, v.toObject());
                        item->moveToThread(qApp->thread());
                        r.emplace_back(::move(item));
                    }
                });
    else
        SpotifySearchHandler::handleTriggerQuery(q);
}

//--------------------------------------------------------------------------------------------------

ShowSearchHandler::ShowSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify shows"),
                         Plugin::tr("Search Spotify shows"))
{}

SearchType ShowSearchHandler::type() const { return Show; }

std::shared_ptr<SpotifyItem> ShowSearchHandler::parseItem(const QJsonObject &o) const
{ return make_shared<ShowItem>(api_, o); }

void ShowSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return api_.userShows(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                    {
                        auto item = make_shared<ShowItem>(api_, v[typeString(type())].toObject());
                        item->moveToThread(qApp->thread());
                        r.emplace_back(::move(item));
                    }
                });
    else
        SpotifySearchHandler::handleTriggerQuery(q);
}

//--------------------------------------------------------------------------------------------------

EpisodeSearchHandler::EpisodeSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify episodes"),
                         Plugin::tr("Search Spotify episodes"))
{}

SearchType EpisodeSearchHandler::type() const { return Episode; }

std::shared_ptr<SpotifyItem> EpisodeSearchHandler::parseItem(const QJsonObject &o) const
{ return make_shared<EpisodeItem>(api_, o); }

void EpisodeSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return api_.userEpisodes(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                        if (const auto &o = v[typeString(type())];
                            !o["id"_L1].isNull())
                        {
                            auto item = make_shared<EpisodeItem>(api_, v[typeString(type())].toObject());
                            item->moveToThread(qApp->thread());
                            r.emplace_back(::move(item));
                        }
                });
    else
        SpotifySearchHandler::handleTriggerQuery(q);
}

//--------------------------------------------------------------------------------------------------

AudiobookSearchHandler::AudiobookSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify audiobooks"),
                         Plugin::tr("Search Spotify audiobooks"))
{}

SearchType AudiobookSearchHandler::type() const { return Audiobook; }

std::shared_ptr<SpotifyItem> AudiobookSearchHandler::parseItem(const QJsonObject &o) const
{ return make_shared<AudiobookItem>(api_, o); }

void AudiobookSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return api_.userAudiobooks(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                    {
                        auto item = make_shared<AudiobookItem>(api_, v.toObject());
                        item->moveToThread(qApp->thread());
                        r.emplace_back(::move(item));
                    }
                });
    else
        SpotifySearchHandler::handleTriggerQuery(q);
}
