// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include <albert/threadedqueryhandler.h>
#include <albert/networkutil.h>
#include <albert/ratelimiter.h>
#include "spotify.h"
class Plugin;
class QJsonArray;
class SpotifyItem;

class SpotifySearchHandler : public albert::ThreadedQueryHandler
{
protected:
    SpotifySearchHandler(spotify::RestApi &api,
                         const QString &id,
                         const QString &name,
                         const QString &description);
    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString defaultTrigger() const override;
    void handleThreadedQuery(albert::ThreadedQuery &) override;

    virtual spotify::SearchType type() const = 0;
    virtual std::shared_ptr<SpotifyItem> parseItem(const QJsonObject &) const = 0;

    void apiCall(albert::ThreadedQuery &q,
                 std::function<QNetworkReply*()> api_call,
                 std::function<void(const QJsonDocument&,
                                    std::vector<std::shared_ptr<albert::Item>>&)> success_handler) const;

    spotify::RestApi &api_;
    albert::detail::RateLimiter limiter_;
    const QString id_;
    const QString name_;
    const QString description_;

};

class TrackSearchHandler : public SpotifySearchHandler
{
public:
    TrackSearchHandler(spotify::RestApi&);
    void handleThreadedQuery(albert::ThreadedQuery &) override;
    spotify::SearchType type() const override final;
    std::shared_ptr<SpotifyItem> parseItem(const QJsonObject &) const override;
};

class ArtistSearchHandler : public SpotifySearchHandler
{
public:
    ArtistSearchHandler(spotify::RestApi&);
    void handleThreadedQuery(albert::ThreadedQuery &) override;
    spotify::SearchType type() const override final;
    std::shared_ptr<SpotifyItem> parseItem(const QJsonObject &) const override;
};

class AlbumSearchHandler : public SpotifySearchHandler
{
public:
    AlbumSearchHandler(spotify::RestApi&);
    void handleThreadedQuery(albert::ThreadedQuery &) override;
    spotify::SearchType type() const override final;
    std::shared_ptr<SpotifyItem> parseItem(const QJsonObject &) const override;
};

class  PlaylistSearchHandler : public SpotifySearchHandler
{
public:
    PlaylistSearchHandler(spotify::RestApi&);
    void handleThreadedQuery(albert::ThreadedQuery &) override;
    spotify::SearchType type() const override final;
    std::shared_ptr<SpotifyItem> parseItem(const QJsonObject &) const override;
};

class ShowSearchHandler : public SpotifySearchHandler
{
public:
    ShowSearchHandler(spotify::RestApi&);
    void handleThreadedQuery(albert::ThreadedQuery &) override;
    spotify::SearchType type() const override final;
    std::shared_ptr<SpotifyItem> parseItem(const QJsonObject &) const override;
};

class EpisodeSearchHandler : public SpotifySearchHandler
{
public:
    EpisodeSearchHandler(spotify::RestApi&);
    void handleThreadedQuery(albert::ThreadedQuery &) override;
    spotify::SearchType type() const override final;
    std::shared_ptr<SpotifyItem> parseItem(const QJsonObject &) const override;
};

class AudiobookSearchHandler : public SpotifySearchHandler
{
public:
    AudiobookSearchHandler(spotify::RestApi&);
    void handleThreadedQuery(albert::ThreadedQuery &) override;
    spotify::SearchType type() const override final;
    std::shared_ptr<SpotifyItem> parseItem(const QJsonObject &) const override;
};
