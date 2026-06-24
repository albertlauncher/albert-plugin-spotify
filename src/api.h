// Copyright (c) 2025-2026 Manuel Schneider

#pragma once
#include <QJsonDocument>
#include <QString>
#include <albert/oauth.h>
#include <albert/ratelimiter.h>
#include <expected>
class QNetworkReply;
class QNetworkRequest;
class QUrlQuery;

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

class API
{
public:

    API();

    [[nodiscard]] const QString &username() const;

    [[nodiscard]] bool isPremium() const;

    [[nodiscard]] QNetworkReply *getDevices();

    [[nodiscard]] QNetworkReply *userProfile();

    [[nodiscard]] QNetworkReply *search(const QString &query, SearchType types,
                                        uint limit, uint offset);

    [[nodiscard]] QNetworkReply *userTopTracks(uint limit, uint offset);

    [[nodiscard]] QNetworkReply *userTopArtists(uint limit, uint offset);

    [[nodiscard]] QNetworkReply *userAlbums(uint limit, uint offset);

    [[nodiscard]] QNetworkReply *userPlaylists(uint limit, uint offset);

    [[nodiscard]] QNetworkReply *userShows(uint limit, uint offset);

    [[nodiscard]] QNetworkReply *userEpisodes(uint limit, uint offset);

    [[nodiscard]] QNetworkReply *userAudiobooks(uint limit, uint offset);


    [[nodiscard]] QNetworkReply *play(const QStringList &uris, const QString& deviceId = {});

    [[nodiscard]] QNetworkReply *pause(const QString &deviceId = {});

    [[nodiscard]] QNetworkReply *queue(const QString &uri, const QString& deviceId = {});

    static std::expected<QJsonDocument, QString> parseJson(QNetworkReply *reply);

    albert::OAuth2 oauth;
    albert::detail::RateLimiter rate_limiter;

private:

    QNetworkRequest request(const QString &, const QUrlQuery &);
    void updateAccountInformatoin();

    QString username_;
    bool is_premium_ = false;

};
