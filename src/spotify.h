// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include <QNetworkReply>
#include <QString>
#include <QString>
#include <QUrlQuery>
class QNetworkRequest;

namespace spotify
{

// Shift must match type strings index
enum SearchTypeFlags {
    None      = 0,
    Album     = 1 << 0,
    Artist    = 1 << 1,
    Audiobook = 1 << 2,
    Episode   = 1 << 3,
    Playlist  = 1 << 4,
    Show      = 1 << 5,
    Track     = 1 << 6,
    All       = Album | Artist | Audiobook | Episode | Playlist | Show | Track
};

// Index must match SearchTypeFlag shift
extern const std::array<const char*, 7> type_strings;

const char* typeString(SearchTypeFlags type);

QString localizedTypeString(SearchTypeFlags type);

class RestApi
{
    QByteArray auth_header_;

public:

    void setBearerToken(const QString&);

    static std::variant<QJsonDocument, QString> parseJson(QNetworkReply *reply);

    // https://developer.spotify.com/documentation/web-api/reference/get-current-users-profile
    [[nodiscard]] QNetworkReply *userProfile() const;

    // https://developer.spotify.com/documentation/web-api/reference/get-users-saved-tracks
    [[nodiscard]] QNetworkReply *userMusic(int limit = 50) const;

    // https://developer.spotify.com/documentation/web-api/reference/get-a-users-available-devices
    [[nodiscard]] QNetworkReply *getDevices() const;

    // https://developer.spotify.com/documentation/web-api/reference/search
    [[nodiscard]] QNetworkReply *search(const QString &query,
                                        SearchTypeFlags types,
                                        int limit = 10,
                                        int offset = 0) const;

    // https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback
    [[nodiscard]] QNetworkReply *play(const QString &uri, const QString& deviceId = {}) const;

    // https://developer.spotify.com/documentation/web-api/reference/pause-a-users-playback
    [[nodiscard]] QNetworkReply *pause(const QString &deviceId = {}) const;

    // https://developer.spotify.com/documentation/web-api/reference/add-to-queue
    [[nodiscard]] QNetworkReply *addToQueue(const QString &uri, const QString& deviceId = {}) const;

};
}
