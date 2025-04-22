// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include <albert/triggerqueryhandler.h>
#include "spotify.h"
class Plugin;
class QJsonArray;
class MediaItem;

class SpotifySearchHandler : public albert::TriggerQueryHandler
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
    virtual spotify::SearchType type() const = 0;

    void apiCall(albert::Query &q,
                 std::function<QNetworkReply*()> api_call,
                 std::function<void(const QJsonDocument&,
                                    std::vector<std::shared_ptr<albert::Item>>&)> success_handler) const;

    spotify::RestApi &api_;
    const QString id_;
    const QString name_;
    const QString description_;

};

class TrackSearchHandler : public SpotifySearchHandler
{
public:
    TrackSearchHandler(spotify::RestApi &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class ArtistSearchHandler : public SpotifySearchHandler
{
public:
    ArtistSearchHandler(spotify::RestApi &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class AlbumSearchHandler : public SpotifySearchHandler
{
public:
    AlbumSearchHandler(spotify::RestApi &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class  PlaylistSearchHandler : public SpotifySearchHandler
{
public:
    PlaylistSearchHandler(spotify::RestApi &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class ShowSearchHandler : public SpotifySearchHandler
{
public:
    ShowSearchHandler(spotify::RestApi &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class EpisodeSearchHandler : public SpotifySearchHandler
{
public:
    EpisodeSearchHandler(spotify::RestApi &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class AudiobookSearchHandler : public SpotifySearchHandler
{
public:
    AudiobookSearchHandler(spotify::RestApi &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};
