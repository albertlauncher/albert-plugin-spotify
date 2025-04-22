// Copyright (c) 2025-2025 Manuel Schneider

#include "handlers.h"
#include "items.h"
#include "plugin.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>
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
    return StandardItem::make(u"notify"_s, u"Spotify"_s, error, Plugin::error_icon_urls);
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

SpotifySearchHandler::SpotifySearchHandler(Plugin &plugin,
                                           const QString &id,
                                           const QString &name,
                                           const QString &description) :
    plugin_(plugin),
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

void SpotifySearchHandler::search(
    albert::Query &q,
    spotify::SearchType type,
    std::shared_ptr<albert::Item> (*item_parser)(Plugin &, const QJsonObject &)) const
{
    if (!debounce(q.isValid()))
        return;

    else if (const auto var = RestApi::parseJson(await(plugin_.api.search(q, type)));
             holds_alternative<QString>(var))
        q.add(makeErrorItem(get<QString>(var)));
    else
    {
        vector<shared_ptr<Item>> r;
        const auto key = u"%1s"_s.arg(typeString(type));
        const auto a = get<QJsonDocument>(var)[key][items].toArray();
        for (const auto &v : a)
            if (!v.isNull())
                if (auto o = v.toObject(); plugin_.show_explicit_content() || !o["explicit"_L1].toBool())
                    r.emplace_back(item_parser(plugin_, o));
        q.add(::move(r));
    }
}

//--------------------------------------------------------------------------------------------------

TrackSearchHandler::TrackSearchHandler(Plugin &plugin) :
    SpotifySearchHandler(plugin,
                         typeString(type()),
                         Plugin::tr("Spotify tracks"),
                         Plugin::tr("Search Spotify tracks"))
{}

void TrackSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return plugin_.api.userTracks(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                        r.emplace_back(TrackItem::make(plugin_, v[typeString(type())].toObject()));
                });
    else
        search(q, type(), TrackItem::make);
}

SearchType TrackSearchHandler::type() const { return Track; }

//--------------------------------------------------------------------------------------------------

ArtistSearchHandler::ArtistSearchHandler(Plugin &plugin) :
    SpotifySearchHandler(plugin,
                         typeString(type()),
                         Plugin::tr("Spotify artists"),
                         Plugin::tr("Search Spotify artists"))
{}

void ArtistSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        q.add(StandardItem::make(u"notify"_s, name_,
                                 Plugin::tr("Enter a query"), Plugin::icon_urls));
    else
        search(q, type(), ArtistItem::make);
}

SearchType ArtistSearchHandler::type() const { return Artist; }

//--------------------------------------------------------------------------------------------------

AlbumSearchHandler::AlbumSearchHandler(Plugin &plugin) :
    SpotifySearchHandler(plugin,
                         typeString(type()),
                         Plugin::tr("Spotify albums"),
                         Plugin::tr("Search Spotify albums"))
{}

void AlbumSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return plugin_.api.userAlbums(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                        r.emplace_back(AlbumItem::make(plugin_, v[typeString(type())].toObject()));
                });
    else
        search(q, type(), AlbumItem::make);
}

SearchType AlbumSearchHandler::type() const { return Album; }

//--------------------------------------------------------------------------------------------------

PlaylistSearchHandler::PlaylistSearchHandler(Plugin &plugin) :
    SpotifySearchHandler(plugin,
                         typeString(type()),
                         Plugin::tr("Spotify playlists"),
                         Plugin::tr("Search Spotify playlists"))
{}

void PlaylistSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return plugin_.api.userPlaylists(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                        r.emplace_back(PlaylistItem::make(plugin_, v.toObject()));
                });
    else
        search(q, type(), PlaylistItem::make);
}

SearchType PlaylistSearchHandler::type() const { return Playlist; }

//--------------------------------------------------------------------------------------------------

ShowSearchHandler::ShowSearchHandler(Plugin &plugin) :
    SpotifySearchHandler(plugin,
                         typeString(type()),
                         Plugin::tr("Spotify shows"),
                         Plugin::tr("Search Spotify shows"))
{}

void ShowSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return plugin_.api.userShows(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                        r.emplace_back(ShowItem::make(plugin_, v[typeString(type())].toObject()));
                });
    else
        search(q, type(), ShowItem::make);
}

SearchType ShowSearchHandler::type() const { return Show; }

//--------------------------------------------------------------------------------------------------

EpisodeSearchHandler::EpisodeSearchHandler(Plugin &plugin) :
    SpotifySearchHandler(plugin,
                         typeString(type()),
                         Plugin::tr("Spotify episodes"),
                         Plugin::tr("Search Spotify episodes"))
{}

void EpisodeSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return plugin_.api.userEpisodes(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                        if (const auto &o = v[typeString(type())];
                            !o["id"_L1].isNull())
                            r.emplace_back(EpisodeItem::make(plugin_, v[typeString(type())].toObject()));
                });
    else
        search(q, type(), EpisodeItem::make);
}

SearchType EpisodeSearchHandler::type() const { return Episode; }

//--------------------------------------------------------------------------------------------------

AudiobookSearchHandler::AudiobookSearchHandler(Plugin &plugin) :
    SpotifySearchHandler(plugin,
                         typeString(type()),
                         Plugin::tr("Spotify audiobooks"),
                         Plugin::tr("Search Spotify audiobooks"))
{}

void AudiobookSearchHandler::handleTriggerQuery(albert::Query &q)
{
    if (q.string().isEmpty())
        apiCall(q,
                [this]{ return plugin_.api.userAudiobooks(); },
                [this](const QJsonDocument &doc, vector<shared_ptr<Item>> &r) {
                    for (const auto &v : doc[items].toArray())
                        r.emplace_back(AudiobookItem::make(plugin_, v.toObject()));
                });
    else
        search(q, type(), AudiobookItem::make);
}

SearchType AudiobookSearchHandler::type() const { return Audiobook; }
