// Copyright (c) 2025-2025 Manuel Schneider

#include "spotify.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSaveFile>
#include <QUrlQuery>
#include <albert/albert.h>
#include <albert/logging.h>
#include <albert/networkutil.h>
using namespace albert;
using namespace spotify;
using namespace std;
using namespace util;

const auto api_base_url = "https://api.spotify.com";

// Index must match SearchTypeFlag shift
const std::array<const char*, 7> spotify::type_strings {
    QT_TRANSLATE_NOOP("spotify", "album"),
    QT_TRANSLATE_NOOP("spotify", "artist"),
    QT_TRANSLATE_NOOP("spotify", "audiobook"),
    QT_TRANSLATE_NOOP("spotify", "episode"),
    QT_TRANSLATE_NOOP("spotify", "playlist"),
    QT_TRANSLATE_NOOP("spotify", "show"),
    QT_TRANSLATE_NOOP("spotify", "track")
};

const char *spotify::typeString(SearchTypeFlags type)
{ return type_strings[std::countr_zero(static_cast<unsigned>(type))]; }

QString spotify::localizedTypeString(SearchTypeFlags type)
{ return QCoreApplication::translate("spotify", typeString(type)); }

// -------------------------------------------------------------------------------------------------

void RestApi::setBearerToken(const QString &bearer_token)
{
    auth_header_ = "Bearer " + bearer_token.toUtf8();
}

static QNetworkRequest request(const QString &p, const QUrlQuery &q, const QByteArray &h)
{ return makeRestRequest(api_base_url, p, q, h); }

QNetworkReply *RestApi::userProfile() const
{
    return network().get(request("/v1/me", {}, auth_header_));
}

QNetworkReply *RestApi::getDevices() const
{
    return network().get(request("/v1/me/player/devices", {}, auth_header_));
}

QNetworkReply *RestApi::search(const QString &query,
                               spotify::SearchTypeFlags type_flags,
                               int limit,
                               int offset) const
{
    QStringList types;
    for (const auto type_flag : {Album, Artist, Playlist, Track, Show, Episode, Audiobook})
        if (type_flags & type_flag)
            types << type_strings[std::countr_zero(static_cast<unsigned>(type_flag))];
    if (types.isEmpty())
        types = {type_strings.begin(), type_strings.end()};

    return network().get(request("/v1/search",
                                 {{"q", QUrl::toPercentEncoding(query)},
                                  {"type", types.join(',')},
                                  {"limit", QString::number(limit)},
                                  {"offset", QString::number(offset)}},
                                 auth_header_));
}

QNetworkReply *RestApi::play(const QString& uri, const QString& deviceId) const
{
    auto params = deviceId.isNull() ? QUrlQuery{} : QUrlQuery{{"device_id", deviceId}};
    auto body = QJsonObject{{"uris", QJsonArray{uri}}};
    return network().put(request("/v1/me/player/play", params, auth_header_),
                         QJsonDocument(body).toJson());
}

QNetworkReply *RestApi::pause(const QString& deviceId) const
{
    auto params = deviceId.isNull() ? QUrlQuery{} : QUrlQuery{{"device_id", deviceId}};
    return network().put(request("/v1/me/player/pause", params, auth_header_), QByteArray{});
}

QNetworkReply *RestApi::addToQueue(const QString &uri, const QString& device_id) const
{
    QUrlQuery params{{{"uri", uri}}};
    if (!device_id.isNull())
        params.addQueryItem("device_id", device_id);
    return network().post(request("/v1/me/player/queue", params, auth_header_), QByteArray{});
}
