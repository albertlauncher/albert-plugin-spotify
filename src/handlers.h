// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "spotify.h"
#include <albert/networkutil.h>
#include <albert/ratelimiter.h>
#include <albert/threadedqueryhandler.h>
class Plugin;
class QJsonArray;
class SpotifyItem;

class SpotifySearchHandler : public albert::QueryHandler
{
public:
    SpotifySearchHandler(const spotify::RestApi &api,
                         spotify::SearchType type,
                         const QString &name,
                         const QString &description);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString defaultTrigger() const override;

    const spotify::RestApi &api;
    const spotify::SearchType type;

protected:

    const QString name_;
    const QString description_;
    albert::detail::RateLimiter rate_limiter_;

    class QueryExecution;

};

class TrackSearchHandler : public SpotifySearchHandler
{
public:
    TrackSearchHandler(spotify::RestApi&);
    std::unique_ptr<albert::QueryExecution> execution(albert::Query &query) override;
};

class ArtistSearchHandler : public SpotifySearchHandler
{
public:
    ArtistSearchHandler(spotify::RestApi&);
    std::unique_ptr<albert::QueryExecution> execution(albert::Query &query) override;
};

class AlbumSearchHandler : public SpotifySearchHandler
{
public:
    AlbumSearchHandler(spotify::RestApi&);
    std::unique_ptr<albert::QueryExecution> execution(albert::Query &query) override;
};

class  PlaylistSearchHandler : public SpotifySearchHandler
{
public:
    PlaylistSearchHandler(spotify::RestApi&);
    std::unique_ptr<albert::QueryExecution> execution(albert::Query &query) override;
};

class ShowSearchHandler : public SpotifySearchHandler
{
public:
    ShowSearchHandler(spotify::RestApi&);
    std::unique_ptr<albert::QueryExecution> execution(albert::Query &query) override;
};

class EpisodeSearchHandler : public SpotifySearchHandler
{
public:
    EpisodeSearchHandler(spotify::RestApi&);
    std::unique_ptr<albert::QueryExecution> execution(albert::Query &query) override;
};

class AudiobookSearchHandler : public SpotifySearchHandler
{
public:
    AudiobookSearchHandler(spotify::RestApi&);
    std::unique_ptr<albert::QueryExecution> execution(albert::Query &query) override;
};
