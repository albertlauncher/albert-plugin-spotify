// Copyright (c) 2025-2025 Manuel Schneider

#include "spotify.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>
#include <albert/networkutil.h>
using namespace Qt::StringLiterals;
using namespace albert::util;
using namespace spotify;
using namespace std;

static const std::array<const char*, 7> type_strings {
    //: Not literally, use Spotify client translations.
    QT_TRANSLATE_NOOP("spotify", "track"),
    //: Not literally, use Spotify client translations.
    QT_TRANSLATE_NOOP("spotify", "artist"),
    //: Not literally, use Spotify client translations.
    QT_TRANSLATE_NOOP("spotify", "album"),
    //: Not literally, use Spotify client translations.
    QT_TRANSLATE_NOOP("spotify", "playlist"),
    //: Not literally, use Spotify client translations.
    QT_TRANSLATE_NOOP("spotify", "show"),
    //: Not literally, use Spotify client translations.
    QT_TRANSLATE_NOOP("spotify", "episode"),
    //: Not literally, use Spotify client translations.
    QT_TRANSLATE_NOOP("spotify", "audiobook")
};

QString spotify::typeString(SearchType type)
{ return QString::fromUtf8(type_strings[static_cast<int>(type)]); }

QString spotify::localizedTypeString(SearchType type)
{ return QCoreApplication::translate("spotify", type_strings[static_cast<int>(type)]); }

// -------------------------------------------------------------------------------------------------

void RestApi::setBearerToken(const QString &bearer_token)
{
    auth_header_ = "Bearer " + bearer_token.toUtf8();
}

variant<QJsonDocument, QString> RestApi::parseJson(QNetworkReply *reply)
{
    reply->deleteLater();
    const QByteArray data = reply->readAll();
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (reply->error() == QNetworkReply::NoError) {
        if (parseError.error == QJsonParseError::NoError)
            return doc;
        return u"JSON parse error: "_s + parseError.errorString();
    }

    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject obj = doc.object();
        if (obj.contains("error"_L1)) {
            const QJsonValue errVal = obj["error"_L1];
            if (errVal.isObject()) {
                const QJsonObject errObj = errVal.toObject();
                QString message = errObj.value("message"_L1).toString();
                int status = errObj.value("status"_L1).toInt();
                return u"Spotify API error %1: %2"_s.arg(status).arg(message);
            } else if (errVal.isString()) {
                return errVal.toString();
            }
        }
    }

    return u"%1: %2"_s.arg(reply->errorString(), QString::fromUtf8(data));
}

static QNetworkRequest request(const QString &p, const QUrlQuery &q, const QByteArray &h)
{ return makeRestRequest(u"https://api.spotify.com"_s, p, q, h); }

// -------------------------------------------------------------------------------------------------

QNetworkReply *RestApi::userProfile() const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-current-users-profile
    return network().get(request(u"/v1/me"_s,
                                 {},
                                 auth_header_));
}

QNetworkReply *RestApi::userTracks(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-tracks
    return network().get(request(u"/v1/me/tracks"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}},
                                 auth_header_));
}

QNetworkReply *RestApi::userAlbums(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-albums
    return network().get(request(u"/v1/me/albums"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}},
                                 auth_header_));
}

QNetworkReply *RestApi::userPlaylists(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-a-list-of-current-users-playlists
    return network().get(request(u"/v1/me/playlists"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}},
                                 auth_header_));
}

QNetworkReply *RestApi::userShows(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-shows
    return network().get(request(u"/v1/me/shows"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}},
                                 auth_header_));
}

QNetworkReply *RestApi::userEpisodes(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-episodes
    return network().get(request(u"/v1/me/episodes"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}},
                                 auth_header_));
}

QNetworkReply *RestApi::userAudiobooks(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-audiobooks
    return network().get(request(u"/v1/me/audiobooks"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}},
                                 auth_header_));
}

QNetworkReply *RestApi::getDevices() const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-a-users-available-devices
    return network().get(request(u"/v1/me/player/devices"_s,
                                 {},
                                 auth_header_));
}

QNetworkReply *RestApi::search(const QString &query,
                               spotify::SearchType type,
                               uint limit,
                               uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/search
    // QStringList types;
    // for (const auto type_flag : {Album, Artist, Playlist, Track, Show, Episode, Audiobook})
    //     if (type_flags & type_flag)
    //         types << type_strings[std::countr_zero(static_cast<unsigned>(type_flag))];
    // if (types.isEmpty())
    //     types = {type_strings.begin(), type_strings.end()};

    return network().get(request(u"/v1/search"_s,
                                 {
                                  {u"q"_s, QString::fromUtf8(QUrl::toPercentEncoding(query))},
                                  {u"type"_s, typeString(type)},
                                  {u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}
                                 },
                                 auth_header_));
}

QNetworkReply *RestApi::play(const QStringList &uris, const QString& deviceId) const
{
    // https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback
    auto params = deviceId.isNull() ? QUrlQuery{} : QUrlQuery{{u"device_id"_s, deviceId}};
    auto body = QJsonObject{{u"uris"_s, QJsonArray::fromStringList(uris)}};

    return network().put(request(u"/v1/me/player/play"_s,
                                 params,
                                 auth_header_),
                         QJsonDocument(body).toJson());
}

QNetworkReply *RestApi::pause(const QString& deviceId) const
{
    // https://developer.spotify.com/documentation/web-api/reference/pause-a-users-playback
    auto params = deviceId.isNull() ? QUrlQuery{} : QUrlQuery{{u"device_id"_s, deviceId}};
    return network().put(request(u"/v1/me/player/pause"_s,
                                 params,
                                 auth_header_),
                         QByteArray{});
}

QNetworkReply *RestApi::queue(const QString &uri, const QString& device_id) const
{
    // https://developer.spotify.com/documentation/web-api/reference/add-to-queue
    QUrlQuery params{{{u"uri"_s, uri}}};
    if (!device_id.isNull())
        params.addQueryItem(u"device_id"_s, device_id);

    return network().post(request(u"/v1/me/player/queue"_s,
                                  params,
                                  auth_header_),
                          QByteArray{});
}
