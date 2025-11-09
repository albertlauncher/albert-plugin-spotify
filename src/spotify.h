// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include <QJsonDocument>
#include <QString>
#include <albert/oauth.h>
class QNetworkReply;
class QNetworkRequest;
class QUrlQuery;

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
public:

    RestApi();

    static std::variant<QJsonDocument, QString> parseJson(QNetworkReply *reply);

    [[nodiscard]] const QString username() const;

    [[nodiscard]] bool isPremium() const;


    [[nodiscard]] QNetworkReply *getDevices() const;

    [[nodiscard]] QNetworkReply *userProfile() const;


    [[nodiscard]] QNetworkReply *search(const QString &query, SearchType types,
                                        uint limit, uint offset) const;


    [[nodiscard]] QNetworkReply *userTopTracks(uint limit, uint offset) const;

    [[nodiscard]] QNetworkReply *userTopArtists(uint limit, uint offset) const;

    [[nodiscard]] QNetworkReply *userAlbums(uint limit, uint offset) const;

    [[nodiscard]] QNetworkReply *userPlaylists(uint limit, uint offset) const;

    [[nodiscard]] QNetworkReply *userShows(uint limit, uint offset) const;

    [[nodiscard]] QNetworkReply *userEpisodes(uint limit, uint offset) const;

    [[nodiscard]] QNetworkReply *userAudiobooks(uint limit, uint offset) const;


    [[nodiscard]] QNetworkReply *play(const QStringList &uris, const QString& deviceId = {}) const;

    [[nodiscard]] QNetworkReply *pause(const QString &deviceId = {}) const;

    [[nodiscard]] QNetworkReply *queue(const QString &uri, const QString& deviceId = {}) const;


    albert::OAuth2 oauth;

private:

    QNetworkRequest request(const QString &, const QUrlQuery &) const;
    void updateAccountInformatoin();

    QString username_;
    bool is_premium_ = false;

};
}
