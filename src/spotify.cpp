// Copyright (c) 2025-2025 Manuel Schneider

#include "spotify.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>
#include <albert/logging.h>
#include <albert/networkutil.h>
using namespace Qt::StringLiterals;
using namespace albert::util;
using namespace spotify;
using namespace std;

static const auto oauth_auth_url = u"https://accounts.spotify.com/authorize"_s;
static const auto oauth_token_url = u"https://accounts.spotify.com/api/token"_s;
static const auto oauth_scope = u"playlist-read-collaborative "_s
                                u"playlist-read-private "_s
                                u"user-follow-read "_s
                                u"user-modify-playback-state "_s
                                u"user-read-playback-state "_s
                                u"user-read-private "_s
                                u"user-top-read "_s
                                u"user-library-read"_s;


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

RestApi::RestApi()
{
    oauth.setAuthUrl(oauth_auth_url);
    oauth.setScope(oauth_scope);
    oauth.setTokenUrl(oauth_token_url);
    oauth.setRedirectUri("%1://spotify/"_L1.arg(qApp->applicationName()));
    oauth.setPkceEnabled(true);

    QObject::connect(&oauth, &OAuth2::tokensChanged, &oauth, [this] {
        if (oauth.error().isEmpty())
            DEBG << "Tokens updated.";
        else
            WARN << oauth.error();
    });

    QObject::connect(&oauth, &OAuth2::stateChanged, &oauth, [this] {
        updateAccountInformatoin();
    });
}

void RestApi::updateAccountInformatoin()
{
    if (oauth.state() != OAuth2::State::Granted)
        return;

    auto reply = userProfile();

    QObject::connect(reply, &QNetworkReply::finished, &oauth, [this, reply]  // use oauth as context to avoid having inherit qobject
    {
        auto var = RestApi::parseJson(reply);
        if (holds_alternative<QString>(var))
        {
            const auto error = get<QString>(var);
            WARN << "Failed fetching user profile:" << error;
            return;
        }
        else
        {
            const auto profile = get<QJsonDocument>(var);

            username_ = profile["display_name"_L1].toString();
            if (username_.isEmpty())
                username_ = profile["id"_L1].toString();
            INFO << "Username:" << username_;

            const auto product_string = profile["product"_L1].toString();
            is_premium_ = profile["product"_L1].toString() == u"premium"_s;
            INFO << "Product type:" << product_string;
        }
    });
}

const QString RestApi::username() const { return username_; }

bool RestApi::isPremium() const { return is_premium_; }

// -------------------------------------------------------------------------------------------------

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

QNetworkRequest RestApi::request(const QString &path, const QUrlQuery &query) const
{
    QUrl url(u"https://api.spotify.com"_s);
    url.setPath(path);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    if (oauth.state() == OAuth2::State::Granted)
        request.setRawHeader("Authorization", "Bearer " + oauth.accessToken().toUtf8());

    return request;
}

// -------------------------------------------------------------------------------------------------

QNetworkReply *RestApi::userProfile() const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-current-users-profile
    return network().get(request(u"/v1/me"_s, {}));
}

// QNetworkReply *RestApi::userTracks(uint limit, uint offset) const
// {
//     // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-tracks
//     return network().get(request(u"/v1/me/tracks"_s,
//                                  {{u"limit"_s, QString::number(limit)},
//                                   {u"offset"_s, QString::number(offset)}}));
// }

QNetworkReply *RestApi::userTopTracks(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-top-artists-and-tracks
    return network().get(request(u"/v1/me/top/tracks"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}}));
}

QNetworkReply *RestApi::userTopArtists(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-top-artists-and-tracks
    return network().get(request(u"/v1/me/top/artists"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}}));
}

// QNetworkReply *RestApi::userArtists(uint limit) const
// {
//     // https://developer.spotify.com/documentation/web-api/reference/get-followed
//     return network().get(request(u"/v1/me/following"_s,
//                                  {{u"limit"_s, QString::number(limit)},
//                                   {u"type"_s, typeString(Artist)}}));
// }

QNetworkReply *RestApi::userAlbums(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-albums
    return network().get(request(u"/v1/me/albums"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}}));
}

QNetworkReply *RestApi::userPlaylists(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-a-list-of-current-users-playlists
    return network().get(request(u"/v1/me/playlists"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}}));
}

QNetworkReply *RestApi::userShows(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-shows
    return network().get(request(u"/v1/me/shows"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}}));
}

QNetworkReply *RestApi::userEpisodes(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-episodes
    return network().get(request(u"/v1/me/episodes"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}}));
}

QNetworkReply *RestApi::userAudiobooks(uint limit, uint offset) const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-audiobooks
    return network().get(request(u"/v1/me/audiobooks"_s,
                                 {{u"limit"_s, QString::number(limit)},
                                  {u"offset"_s, QString::number(offset)}}));
}

QNetworkReply *RestApi::getDevices() const
{
    // https://developer.spotify.com/documentation/web-api/reference/get-a-users-available-devices
    return network().get(request(u"/v1/me/player/devices"_s,
                                 {}));
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
                                 }));
}

QNetworkReply *RestApi::play(const QStringList &uris, const QString& deviceId) const
{
    // https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback
    auto params = deviceId.isNull() ? QUrlQuery{} : QUrlQuery{{u"device_id"_s, deviceId}};
    auto body = QJsonObject{{u"uris"_s, QJsonArray::fromStringList(uris)}};

    return network().put(request(u"/v1/me/player/play"_s, params),
                         QJsonDocument(body).toJson());
}

QNetworkReply *RestApi::pause(const QString& deviceId) const
{
    // https://developer.spotify.com/documentation/web-api/reference/pause-a-users-playback
    auto params = deviceId.isNull() ? QUrlQuery{} : QUrlQuery{{u"device_id"_s, deviceId}};
    return network().put(request(u"/v1/me/player/pause"_s,
                                 params),
                         QByteArray{});
}

QNetworkReply *RestApi::queue(const QString &uri, const QString& device_id) const
{
    // https://developer.spotify.com/documentation/web-api/reference/add-to-queue
    QUrlQuery params{{{u"uri"_s, uri}}};
    if (!device_id.isNull())
        params.addQueryItem(u"device_id"_s, device_id);

    return network().post(request(u"/v1/me/player/queue"_s, params),
                          QByteArray{});
}

