// Copyright (c) 2025-2025 Manuel Schneider

#include "handlers.h"
#include "items.h"
#include "plugin.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>
#include <albert/logging.h>
#include <albert/networkutil.h>
#include <albert/standarditem.h>
using namespace Qt::StringLiterals;
using namespace albert::util;
using namespace albert;
using namespace spotify;
using namespace std;

static const auto items = u"items";

static shared_ptr<Item> makeErrorItem(const QString &error)
{
    WARN << error;
    return StandardItem::make(u"notify"_s, u"Spotify"_s, error,
                              u"comp:?src1=%3Aspotify&src2=qsp%3ASP_MessageBoxWarning"_s);
}

static const bool &debounce(const bool &valid)
{
    static mutex m;
    static long long block_until = 0;
    auto now = QDateTime::currentMSecsSinceEpoch();

    unique_lock lock(m);

    while (block_until > QDateTime::currentMSecsSinceEpoch())
        if (valid)
            QThread::msleep(10);
        else
            return valid;

    block_until = now + 1000;

    return valid;
}

SpotifySearchHandler::SpotifySearchHandler(RestApi &api,
                                           const QString &id,
                                           const QString &name,
                                           const QString &description) :
    api_(api),
    id_(id),
    name_(name),
    description_(description)
{}

QString SpotifySearchHandler::id() const { return id_; }

QString SpotifySearchHandler::name() const { return name_; }

QString SpotifySearchHandler::description() const { return description_; }

QString SpotifySearchHandler::defaultTrigger() const
{ return localizedTypeString(type()).toLower() + QChar::Space; }


void SpotifySearchHandler::apiCall(
    albert::Query &q,
    function<QNetworkReply*()> api_call,
    function<void(const QJsonDocument&, vector<shared_ptr<Item>>&)> success_handler) const
{
    if (!debounce(q.isValid()))
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

template<typename T>
static void search(Query &q, RestApi &api, SearchType type)
{
    if (!debounce(q.isValid()))
        return;

    else if (const auto var = RestApi::parseJson(await(api.search(q, type)));
             holds_alternative<QString>(var))
        q.add(makeErrorItem(get<QString>(var)));
    else
    {
        vector<shared_ptr<Item>> r;
        const auto key = u"%1s"_s.arg(typeString(type));
        const auto a = get<QJsonDocument>(var)[key][items].toArray();
        for (const auto &v : a)
            if (!v.isNull())
            {
                auto item = make_shared<T>(api, v.toObject());
                item->moveToThread(qApp->thread());
                r.emplace_back(::move(item));
            }
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

void TrackSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return api_.userTracks(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                    {
                        auto item = make_shared<TrackItem>(api_, v[typeString(type())].toObject());
                        item->moveToThread(qApp->thread());
                        r.emplace_back(::move(item));
                    }
                });
    else
        search<TrackItem>(q, api_, type());
}

SearchType TrackSearchHandler::type() const { return Track; }

//--------------------------------------------------------------------------------------------------

ArtistSearchHandler::ArtistSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify artists"),
                         Plugin::tr("Search Spotify artists"))
{}

void ArtistSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        q.add(StandardItem::make(u"notify"_s, name_,
                                 Plugin::tr("Enter a query"), {u":spotify"_s}));
    else
        search<ArtistItem>(q, api_, type());
}

SearchType ArtistSearchHandler::type() const { return Artist; }

//--------------------------------------------------------------------------------------------------

AlbumSearchHandler::AlbumSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify albums"),
                         Plugin::tr("Search Spotify albums"))
{}

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
        search<AlbumItem>(q, api_, type());
}

SearchType AlbumSearchHandler::type() const { return Album; }

//--------------------------------------------------------------------------------------------------

PlaylistSearchHandler::PlaylistSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify playlists"),
                         Plugin::tr("Search Spotify playlists"))
{}

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
        search<PlaylistItem>(q, api_, type());
}

SearchType PlaylistSearchHandler::type() const { return Playlist; }

//--------------------------------------------------------------------------------------------------

ShowSearchHandler::ShowSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify shows"),
                         Plugin::tr("Search Spotify shows"))
{}

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
        search<ShowItem>(q, api_, type());
}

SearchType ShowSearchHandler::type() const { return Show; }

//--------------------------------------------------------------------------------------------------

EpisodeSearchHandler::EpisodeSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify episodes"),
                         Plugin::tr("Search Spotify episodes"))
{}

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
        search<EpisodeItem>(q, api_, type());
}

SearchType EpisodeSearchHandler::type() const { return Episode; }

//--------------------------------------------------------------------------------------------------

AudiobookSearchHandler::AudiobookSearchHandler(spotify::RestApi &api) :
    SpotifySearchHandler(api,
                         typeString(type()),
                         Plugin::tr("Spotify audiobooks"),
                         Plugin::tr("Search Spotify audiobooks"))
{}

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
        search<AudiobookItem>(q, api_, type());
}

SearchType AudiobookSearchHandler::type() const { return Audiobook; }
