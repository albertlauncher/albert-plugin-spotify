// Copyright (c) 2025-2025 Manuel Schneider

#include "items.h"
#include "plugin.h"
#include "spotify.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <ranges>
using namespace std;
using namespace albert;
using namespace spotify;

MediaItem::MediaItem(Plugin &plugin,
                     const QJsonObject &json,
                     const QString &spotify_id,
                     const QString &title,
                     const QString &description,
                     const QString &icon_url
                     ) :
    plugin_(plugin),
    json_(json),
    spotify_id_(spotify_id),
    title_(title),
    description_(description),
    icon_url_(icon_url)
{
}

MediaItem::~MediaItem() = default;

const QJsonObject &MediaItem::json() const { return json_; }

QString MediaItem::id() const { return spotify_id_; }

QString MediaItem::text() const { return title_; }

QString MediaItem::subtext() const { return description_; }

QString MediaItem::uri() const
{ return QStringLiteral("spotify:%1:%2").arg(spotify::typeString(mediaType()), id()); }

QStringList MediaItem::iconUrls() const
{
    if (icon_.isNull())
    {
        icon_ = ""; // indicates that a download is awaited, order matters!
        plugin_.downloadAlbumCover(spotify_id_, QUrl(icon_url_), [this](const QString &path) {
            icon_ = path;
            for (auto observer : observers)
                observer->notify(this);;
        });
        return {};
    }
    else if (icon_.isEmpty())
        return {};
    else
        return {
            QStringLiteral("mask:?src=%1&radius=5")
                .arg(QUrl::toPercentEncoding(icon_))
        };
}

vector<Action> MediaItem::actions() const
{
    vector<Action> actions{{
        "play", Plugin::tr("Play on Spotify"),
        [this]{ const_cast<Plugin&>(plugin_).play(*this); }
    }};

    if (plugin_.premium())
        actions.emplace_back(
            "queue", Plugin::tr("Add to the Spotify queue"),
            [this]{ const_cast<Plugin&>(plugin_).addToQueue(*this); }
        );
    return actions;
}

void MediaItem::addObserver(Observer *observer) { observers.insert(observer); }

void MediaItem::removeObserver(Observer *observer) { observers.erase(observer); }

//  {
//  "album": {
//    "album_type": "compilation",
//    "total_tracks": 9,
//    "available_markets": ["CA", "BR", "IT"],
//    "external_urls": {
//      "spotify": "string"
//    },
//    "href": "string",
//    "id": "2up3OPMp9Tb4dAKM2erWXQ",
//    "images": [
//      {
//        "url": "https://i.scdn.co/image/ab67616d00001e02ff9ca10b55ce82ae553c8228",
//        "height": 300,
//        "width": 300
//      }
//    ],
//    "name": "string",
//    "release_date": "1981-12",
//    "release_date_precision": "year",
//    "restrictions": {
//      "reason": "market"
//    },
//    "type": "album",
//    "uri": "spotify:album:2up3OPMp9Tb4dAKM2erWXQ",
//    "artists": [
//      {
//        "external_urls": {
//          "spotify": "string"
//        },
//        "href": "string",
//        "id": "string",
//        "name": "string",
//        "type": "artist",
//        "uri": "string"
//      }
//    ]
//  },
//  "artists": [
//    {
//      "external_urls": {
//        "spotify": "string"
//      },
//      "href": "string",
//       "id": "string",
//       "name": "string",
//       "type": "artist",
//       "uri": "string"
//     }
//   ],
//   "available_markets": ["string"],
//   "disc_number": 0,
//   "duration_ms": 0,
//   "explicit": false,
//   "external_ids": {
//     "isrc": "string",
//     "ean": "string",
//     "upc": "string"
//   },
//   "external_urls": {
//     "spotify": "string"
//   },
//   "href": "string",
//   "id": "string",
//   "is_playable": false,
//   "linked_from": {
//   },
//   "restrictions": {
//     "reason": "string"
//   },
//   "name": "string",
//   "popularity": 0,
//   "preview_url": "string",
//   "track_number": 0,
//   "type": "track",
//   "uri": "string",
//   "is_local": false
// }

shared_ptr<Item> TrackItem::make(Plugin &plugin, const QJsonObject &json)
{
    QStringList description_tokens;
    description_tokens << spotify::localizedTypeString(spotify::Track);

    if (auto view = json.value("artists").toArray()
                    | views::transform([](const QJsonValue& a){ return a["name"].toString(); });
        !view.empty())
        description_tokens << QStringList(view.begin(), view.end()).join(", ");

    description_tokens << json["album"]["name"].toString();

    return make_shared<TrackItem>(
        plugin,
        json,
        json["id"].toString(),
        json["name"].toString(),
        description_tokens.join(QStringLiteral(" · ")),
        json["album"]["images"].toArray().last()["url"].toString()
    );
}

spotify::SearchTypeFlags TrackItem::mediaType() const { return spotify::Track; }

// {
//   "audio_preview_url": "https://p.scdn.co/mp3-preview/2f37da1d4221f40b9d1a98cd191f4d6f1646ad17",
//   "description": "A Spotify podcast sharing fresh insights on important topics of the moment—in a way only Spotify can. You’ll hear from experts in the music, podcast and tech industries as we discover and uncover stories about our work and the world around us.",
//   "html_description": "<p>A Spotify podcast sharing fresh insights on important topics of the moment—in a way only Spotify can. You’ll hear from experts in the music, podcast and tech industries as we discover and uncover stories about our work and the world around us.</p>",
//   "duration_ms": 1686230,
//   "explicit": false,
//   "external_urls": {
//     "spotify": "string"
//   },
//   "href": "https://api.spotify.com/v1/episodes/5Xt5DXGzch68nYYamXrNxZ",
//   "id": "5Xt5DXGzch68nYYamXrNxZ",
//   "images": [
//     {
//       "url": "https://i.scdn.co/image/ab67616d00001e02ff9ca10b55ce82ae553c8228",
//       "height": 300,
//       "width": 300
//     }
//   ],
//   "is_externally_hosted": false,
//   "is_playable": false,
//   "language": "en",
//   "languages": ["fr", "en"],
//   "name": "Starting Your Own Podcast: Tips, Tricks, and Advice From Anchor Creators",
//   "release_date": "1981-12-15",
//   "release_date_precision": "day",
//   "resume_point": {
//     "fully_played": false,
//     "resume_position_ms": 0
//   },
//   "type": "episode",
//   "uri": "spotify:episode:0zLhl3WsOCQHbe1BPTiHgr",
//   "restrictions": {
//     "reason": "string"
//   }
// }

shared_ptr<Item> EpisodeItem::make(Plugin &plugin, const QJsonObject &json)
{
    auto description = spotify::localizedTypeString(spotify::Episode);
    if (const auto d = json["description"].toString();
        !d.isEmpty())
        description += QStringLiteral(" · %1").arg(d);

    return make_shared<EpisodeItem>(
        plugin,
        json,
        json["id"].toString(),
        json["name"].toString(),
        description,
        json["images"].toArray().last()["url"].toString()
    );
}

spotify::SearchTypeFlags EpisodeItem::mediaType() const { return spotify::Episode; }

// {
//   "external_urls": {
//     "spotify": "string"
//   },
//   "followers": {
//     "href": "string",
//     "total": 0
//   },
//   "genres": ["Prog rock", "Grunge"],
//   "href": "string",
//   "id": "string",
//   "images": [
//     {
//       "url": "https://i.scdn.co/image/ab67616d00001e02ff9ca10b55ce82ae553c8228",
//       "height": 300,
//       "width": 300
//     }
//   ],
//   "name": "string",
//   "popularity": 0,
//   "type": "artist",
//   "uri": "string"
// }

shared_ptr<Item> ArtistItem::make(Plugin &plugin, const QJsonObject &json)
{
    auto description = spotify::localizedTypeString(spotify::Artist);

    // if (auto view = json["genres"].toArray()
    //             | views::transform([](const auto& i){ return i.toString(); });
    //     !view.empty())
    //     description += QStringLiteral(" · %1")
    //                        .arg(QStringList(view.begin(), view.end()).join(", "));

    // if (const auto followers = json["followers"]["total"].toInt();
    //     followers > 0)
    //     description += QStringLiteral(" · ✨ %1").arg(followers);

    return make_shared<ArtistItem>(
        plugin,
        json,
        json["id"].toString(),
        json["name"].toString(),
        description,
        json["images"].toArray().last()["url"].toString()
    );
}

spotify::SearchTypeFlags ArtistItem::mediaType() const { return spotify::Artist; }

// {
//   "album_type": "compilation",
//   "total_tracks": 9,
//   "available_markets": [
//     "CA",
//     "BR",
//     "IT"
//   ],
//   "external_urls": {
//     "spotify": "string"
//   },
//   "href": "string",
//   "id": "2up3OPMp9Tb4dAKM2erWXQ",
//   "images": [
//     {
//       "url": "https://i.scdn.co/image/ab67616d00001e02ff9ca10b55ce82ae553c8228",
//       "height": 300,
//       "width": 300
//     }
//   ],
//   "name": "string",
//   "release_date": "1981-12",
//   "release_date_precision": "year",
//   "restrictions": {
//     "reason": "market"
//   },
//   "type": "album",
//   "uri": "spotify:album:2up3OPMp9Tb4dAKM2erWXQ",
//   "artists": [
//     {
//       "external_urls": {
//         "spotify": "string"
//       },
//       "href": "string",
//       "id": "string",
//       "name": "string",
//       "type": "artist",
//       "uri": "string"
//     }
//   ]
// }

std::shared_ptr<Item> AlbumItem::make(Plugin &plugin, const QJsonObject &json)
{
    auto v = json.value("artists").toArray()
             | views::transform([](const QJsonValue& a){ return a["name"].toString(); });

    return make_shared<AlbumItem>(
        plugin,
        json,
        json["id"].toString(),
        json["name"].toString(),
        QString("%1 · %2").arg(spotify::localizedTypeString(spotify::Album),
                               QStringList{begin(v), end(v)}.join(", ")),
        json["images"].toArray().last()["url"].toString()
    );
}

SearchTypeFlags AlbumItem::mediaType() const { return spotify::Album; }

// {
//   "collaborative": false,
//   "description": "string",
//   "external_urls": {
//     "spotify": "string"
//   },
//   "href": "string",
//   "id": "string",
//   "images": [
//     {
//       "url": "https://i.scdn.co/image/ab67616d00001e02ff9ca10b55ce82ae553c8228",
//       "height": 300,
//       "width": 300
//     }
//   ],
//   "name": "string",
//   "owner": {
//     "external_urls": {
//       "spotify": "string"
//     },
//     "href": "string",
//     "id": "string",
//     "type": "user",
//     "uri": "string",
//     "display_name": "string"
//   },
//   "public": false,
//   "snapshot_id": "string",
//   "tracks": {
//     "href": "string",
//     "total": 0
//   },
//   "type": "string",
//   "uri": "string"
// }

shared_ptr<Item> PlayListItem::make(Plugin &plugin, const QJsonObject &json)
{
    return make_shared<PlayListItem>(
        plugin,
        json,
        json["id"].toString(),
        json["name"].toString(),
        QString("%1 · %2")
            .arg(spotify::localizedTypeString(spotify::Playlist),
                 json["owner"]["display_name"].toString()),
        json["images"].toArray().last()["url"].toString()
    );
}

SearchTypeFlags PlayListItem::mediaType() const { return spotify::Playlist; }

// {
//   "available_markets": [
//     "string"
//   ],
//   "copyrights": [
//     {
//       "text": "string",
//       "type": "string"
//     }
//   ],
//   "description": "string",
//   "html_description": "string",
//   "explicit": false,
//   "external_urls": {
//     "spotify": "string"
//   },
//   "href": "string",
//   "id": "string",
//   "images": [
//     {
//       "url": "https://i.scdn.co/image/ab67616d00001e02ff9ca10b55ce82ae553c8228",
//       "height": 300,
//       "width": 300
//     }
//   ],
//   "is_externally_hosted": false,
//   "languages": [
//     "string"
//   ],
//   "media_type": "string",
//   "name": "string",
//   "publisher": "string",
//   "type": "show",
//   "uri": "string",
//   "total_episodes": 0
// }

shared_ptr<Item> ShowItem::make(Plugin &plugin, const QJsonObject &json)
{
    return make_shared<ShowItem>(
        plugin,
        json,
        json["id"].toString(),
        json["name"].toString(),
        QString("%1 · %2")
            .arg(spotify::localizedTypeString(spotify::Show),
                 json["publisher"].toString()),
        json["images"].toArray().last()["url"].toString()
    );
}

SearchTypeFlags ShowItem::mediaType() const { return spotify::Show; }

// {
//   "authors": [
//     {
//       "name": "string"
//     }
//   ],
//   "available_markets": [
//     "string"
//   ],
//   "copyrights": [
//     {
//       "text": "string",
//       "type": "string"
//     }
//   ],
//   "description": "string",
//   "html_description": "string",
//   "edition": "Unabridged",
//   "explicit": false,
//   "external_urls": {
//     "spotify": "string"
//   },
//   "href": "string",
//   "id": "string",
//   "images": [
//     {
//       "url": "https://i.scdn.co/image/ab67616d00001e02ff9ca10b55ce82ae553c8228",
//       "height": 300,
//       "width": 300
//     }
//   ],
//   "languages": [
//     "string"
//   ],
//   "media_type": "string",
//   "name": "string",
//   "narrators": [
//     {
//       "name": "string"
//     }
//   ],
//   "publisher": "string",
//   "type": "audiobook",
//   "uri": "string",
//   "total_chapters": 0
// }

shared_ptr<Item> AudiobookItem::make(Plugin &plugin, const QJsonObject &json)
{
    QStringList description_tokens;
    description_tokens << spotify::localizedTypeString(spotify::Audiobook);

    auto view = json.value("authors").toArray()
                | views::transform([](const QJsonValue& a){ return a["name"].toString(); });

    return make_shared<AudiobookItem>(
        plugin,
        json,
        json["id"].toString(),
        json["name"].toString(),
        QString("%1 · %2")
            .arg(spotify::localizedTypeString(spotify::Audiobook),
                 QStringList(view.begin(), view.end()).join(", ")),
        json["images"].toArray().last()["url"].toString()
    );
}

SearchTypeFlags AudiobookItem::mediaType() const { return spotify::Audiobook; }
