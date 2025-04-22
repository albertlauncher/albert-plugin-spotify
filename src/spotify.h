// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include <QJsonDocument>
#include <QString>
class QNetworkReply;
class QNetworkRequest;

namespace spotify
{

enum SearchType {
    Track,
    Artist,
    Album,
    Playlist,
    Show,
    Episode,
    Audiobook,
};

QString typeString(SearchType type);

QString localizedTypeString(SearchType type);

class RestApi
{
    QByteArray auth_header_;

public:

    void setBearerToken(const QString&);

    static std::variant<QJsonDocument, QString> parseJson(QNetworkReply *reply);

    [[nodiscard]] QNetworkReply *userProfile() const;

    [[nodiscard]] QNetworkReply *getDevices() const;


    [[nodiscard]] QNetworkReply *userTracks(uint limit = 50, uint offset = 0) const;

    [[nodiscard]] QNetworkReply *userAlbums(uint limit = 50, uint offset = 0) const;

    [[nodiscard]] QNetworkReply *userPlaylists(uint limit = 50, uint offset = 0) const;

    [[nodiscard]] QNetworkReply *userShows(uint limit = 50, uint offset = 0) const;

    [[nodiscard]] QNetworkReply *userEpisodes(uint limit = 50, uint offset = 0) const;

    [[nodiscard]] QNetworkReply *userAudiobooks(uint limit = 50, uint offset = 0) const;


    [[nodiscard]] QNetworkReply *search(const QString &query, SearchType types,
                                        uint limit = 50, uint offset = 0) const;


    [[nodiscard]] QNetworkReply *play(const QStringList &uris, const QString& deviceId = {}) const;

    [[nodiscard]] QNetworkReply *pause(const QString &deviceId = {}) const;

    [[nodiscard]] QNetworkReply *queue(const QString &uri, const QString& deviceId = {}) const;

};
}
