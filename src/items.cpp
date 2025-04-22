// Copyright (c) 2025-2025 Manuel Schneider

#include "items.h"
#include "plugin.h"
#include "spotify.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <albert/systemutil.h>
#include <ranges>
using namespace Qt::StringLiterals;
using namespace albert::util;
using namespace albert;
using namespace spotify;
using namespace std;

MediaItem::MediaItem(Plugin &plugin,
                     const QString &uri,
                     const QString &title,
                     const QString &description,
                     const QString &icon_url) :
    plugin_(plugin),
    uri_(uri),
    title_(title),
    description_(description),
    icon_url_(icon_url)
{
}

MediaItem::~MediaItem() = default;

QString MediaItem::id() const { return uri_; }

QString MediaItem::text() const { return title_; }

QString MediaItem::subtext() const { return description_; }

QStringList MediaItem::iconUrls() const
{
    if (icon_.isNull())
    {
        icon_ = ""_L1; // indicates that a download is awaited, order matters!
        plugin_.downloadAlbumCover(uri_, QUrl(icon_url_), [this](const QString &path) {
            icon_ = path;
            for (auto observer : observers)
                observer->notify(this);
        });
        return {};
    }
    else if (icon_.isEmpty())
        return {};
    else
        return {u"mask:?src=%1&radius=5"_s.arg(QString::fromUtf8(QUrl::toPercentEncoding(icon_)))};
}

void MediaItem::addObserver(Observer *observer) { observers.insert(observer); }

void MediaItem::removeObserver(Observer *observer) { observers.erase(observer); }

// -------------------------------------------------------------------------------------------------


static void play(Plugin &plugin, const QString &uri)
{
    const auto reply = plugin.api.play({uri});
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, uri]{
        const auto var = RestApi::parseJson(reply);
        if (holds_alternative<QString>(var))
        {
            const auto error = get<QString>(var);
            DEBG << "Failed to play" << uri << error;
            DEBG << "Open local Spotify to run" << uri;
            openUrl(uri);
        }
        else
            DEBG << "Successfully played" << uri;
    });
}

static void queue(Plugin &plugin, const QString &uri)
{
    const auto reply = plugin.api.queue({uri});
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, uri]{
        const auto var = RestApi::parseJson(reply);
        if (holds_alternative<QString>(var))
            WARN << "Failed to queue" << uri;
        else
            DEBG << "Successfully queued" << uri;
    });
}

// -------------------------------------------------------------------------------------------------

// See https://developer.spotify.com/documentation/web-api/reference/search
shared_ptr<Item> TrackItem::make(Plugin &plugin, const QJsonObject &json)
{
    QStringList description_tokens;
    description_tokens << localizedTypeString(Track);

    if (auto view = json["artists"_L1].toArray()
                    | views::transform([](const QJsonValue& a){ return a["name"_L1].toString(); });
        !view.empty())
        description_tokens << QStringList(view.begin(), view.end()).join(u", "_s);

    description_tokens << json["album"_L1]["name"_L1].toString();

    return make_shared<TrackItem>(
        plugin,
        json["uri"_L1].toString(),
        json["name"_L1].toString(),
        description_tokens.join(u" · "_s),
        json["album"_L1]["images"_L1].toArray().last()["url"_L1].toString()
        );
}

vector<Action> TrackItem::actions() const
{
    vector<Action> actions;

    if (plugin_.premium())
    {
        actions.emplace_back(u"play"_s, plugin_.ui_strings.play_on_spotify,
                             [this] { play(plugin_, id()); });
        actions.emplace_back(u"queue"_s, plugin_.ui_strings.add_to_queue,
                             [this] { ::queue(plugin_, id()); });
    }
    else if (plugin_.media_remote)
    {
        actions.emplace_back(u"playlocal"_s, plugin_.ui_strings.play_in_spotify, [this]{
            if (plugin_.media_remote)
                plugin_.media_remote->pause();
            INFO << "Open local Spotify with" << id();
            openUrl(id());
        });
    }
    else
    {
        actions.emplace_back(u"show"_s, plugin_.ui_strings.show_in_spotify,
                             [this]{ openUrl(id()); });
    }

    return actions;
}

// -------------------------------------------------------------------------------------------------

// See https://developer.spotify.com/documentation/web-api/reference/search
shared_ptr<Item> ArtistItem::make(Plugin &plugin, const QJsonObject &json)
{
    return make_shared<ArtistItem>(
        plugin,
        json["uri"_L1].toString(),
        json["name"_L1].toString(),
        localizedTypeString(Artist),
        json["images"_L1].toArray().last()["url"_L1].toString()
        );
}

vector<Action> ArtistItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, plugin_.ui_strings.show_in_spotify, [this]{ openUrl(id()); });
    return actions;
}

// -------------------------------------------------------------------------------------------------

// See https://developer.spotify.com/documentation/web-api/reference/search
shared_ptr<Item> AlbumItem::make(Plugin &plugin, const QJsonObject &json)
{
    auto v = json["artists"_L1].toArray()
             | views::transform([](const QJsonValue& a){ return a["name"_L1].toString(); });

    return make_shared<AlbumItem>(
        plugin,
        json["uri"_L1].toString(),
        json["name"_L1].toString(),
        u"%1 · %2"_s.arg(localizedTypeString(Album),
                         QStringList{begin(v), end(v)}.join(u", "_s)),
        json["images"_L1].toArray().last()["url"_L1].toString()
        );
}

vector<Action> AlbumItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, plugin_.ui_strings.show_in_spotify, [this]{ openUrl(id()); });
    return actions;
}

// -------------------------------------------------------------------------------------------------

// See https://developer.spotify.com/documentation/web-api/reference/search
shared_ptr<Item> PlaylistItem::make(Plugin &plugin, const QJsonObject &json)
{
    return make_shared<PlaylistItem>(
        plugin,
        json["uri"_L1].toString(),
        json["name"_L1].toString(),
        u"%1 · %2"_s.arg(localizedTypeString(Playlist),
                         json["owner"_L1]["display_name"_L1].toString()),
        json["images"_L1].toArray().last()["url"_L1].toString()
        );
}

vector<Action> PlaylistItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, plugin_.ui_strings.show_in_spotify, [this]{ openUrl(id()); });
    return actions;
}

// -------------------------------------------------------------------------------------------------

// See https://developer.spotify.com/documentation/web-api/reference/search
shared_ptr<Item> ShowItem::make(Plugin &plugin, const QJsonObject &json)
{
    return make_shared<ShowItem>(
        plugin,
        json["uri"_L1].toString(),
        json["name"_L1].toString(),
        u"%1 · %2"_s.arg(localizedTypeString(Show),
                         json["publisher"_L1].toString()),
        json["images"_L1].toArray().last()["url"_L1].toString()
        );
}

vector<Action> ShowItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, plugin_.ui_strings.show_in_spotify, [this]{ openUrl(id()); });
    return actions;
}

// -------------------------------------------------------------------------------------------------

// See https://developer.spotify.com/documentation/web-api/reference/search
shared_ptr<Item> EpisodeItem::make(Plugin &plugin, const QJsonObject &json)
{
    auto description = localizedTypeString(Episode);
    if (const auto d = json["description"_L1].toString();
        !d.isEmpty())
        description += u" · %1"_s.arg(d);

    return make_shared<EpisodeItem>(
        plugin,
        json["uri"_L1].toString(),
        json["name"_L1].toString(),
        description,
        json["images"_L1].toArray().last()["url"_L1].toString()
        );
}

vector<Action> EpisodeItem::actions() const
{
    vector<Action> actions;

    if (plugin_.premium())
    {
        actions.emplace_back(u"play"_s, plugin_.ui_strings.play_on_spotify,
                             [this] { play(plugin_, id()); });
        actions.emplace_back(u"queue"_s, plugin_.ui_strings.add_to_queue,
                             [this] { ::queue(plugin_, id()); });
    }
    else
    {
        actions.emplace_back(u"show"_s, plugin_.ui_strings.show_in_spotify,
                             [this]{ openUrl(id()); });
    }

    return actions;
}

// -------------------------------------------------------------------------------------------------

// See https://developer.spotify.com/documentation/web-api/reference/search
shared_ptr<Item> AudiobookItem::make(Plugin &plugin, const QJsonObject &json)
{
    QStringList description_tokens;
    description_tokens << localizedTypeString(Audiobook);

    auto view = json["authors"_L1].toArray()
                | views::transform([](const QJsonValue& a){ return a["name"_L1].toString(); });

    return make_shared<AudiobookItem>(
        plugin,
        json["uri"_L1].toString(),
        json["name"_L1].toString(),
        u"%1 · %2"_s.arg(localizedTypeString(Audiobook),
                         QStringList(view.begin(), view.end()).join(u", "_s)),
        json["images"_L1].toArray().last()["url"_L1].toString()
        );
}

vector<Action> AudiobookItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, plugin_.ui_strings.show_in_spotify, [this]{ openUrl(id()); });
    return actions;
}
