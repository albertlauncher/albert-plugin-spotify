// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "spotify.h"
#include <albert/triggerqueryhandler.h>
class Plugin;
class QJsonArray;

class SpotifySearchHandler : public albert::TriggerQueryHandler
{
protected:
    SpotifySearchHandler(Plugin &plugin,
                         const QString &id,
                         const QString &name,
                         const QString &description);
    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString defaultTrigger() const override;
    virtual spotify::SearchType type() const = 0;

    void search(albert::Query &query,
                spotify::SearchType type,
                std::shared_ptr<albert::Item> (*item_parser)(Plugin&, const QJsonObject&)) const;

    void apiCall(albert::Query &q,
                 std::function<QNetworkReply*()> api_call,
                 std::function<void(const QJsonDocument&,
                                    std::vector<std::shared_ptr<albert::Item>>&)> success_handler) const;

    Plugin &plugin_;
    const QString id_;
    const QString name_;
    const QString description_;

};

class TrackSearchHandler : public SpotifySearchHandler
{
public:
    TrackSearchHandler(Plugin &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class ArtistSearchHandler : public SpotifySearchHandler
{
public:
    ArtistSearchHandler(Plugin &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class AlbumSearchHandler : public SpotifySearchHandler
{
public:
    AlbumSearchHandler(Plugin &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class  PlaylistSearchHandler : public SpotifySearchHandler
{
public:
    PlaylistSearchHandler(Plugin &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class ShowSearchHandler : public SpotifySearchHandler
{
public:
    ShowSearchHandler(Plugin &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class EpisodeSearchHandler : public SpotifySearchHandler
{
public:
    EpisodeSearchHandler(Plugin &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};

class AudiobookSearchHandler : public SpotifySearchHandler
{
public:
    AudiobookSearchHandler(Plugin &);
    void handleTriggerQuery(albert::Query &q) override;
    spotify::SearchType type() const override final;
};
