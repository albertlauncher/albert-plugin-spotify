// Copyright (c) 2025-2026 Manuel Schneider

#pragma once
#include "api.h"
#include <albert/asyncgeneratorqueryhandler.h>
#include <albert/networkutil.h>
class Plugin;
class QJsonArray;
class SpotifyItem;

class SpotifySearchHandler : public albert::AsyncGeneratorQueryHandler
{
public:
    SpotifySearchHandler(RestApi &api,
                         SearchType type,
                         const QString &name,
                         const QString &description);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString defaultTrigger() const override;
    albert::AsyncItemGenerator items(albert::QueryContext &) override;

    virtual QNetworkReply *fetch(albert::QueryContext &ctx, uint page) const = 0;
    virtual std::vector<std::shared_ptr<albert::Item>>
    handleReply(albert::QueryContext &ctx, const QJsonDocument &doc) = 0;

protected:
    RestApi &api_;
    const SearchType type_;
    const QString name_;
    const QString description_;
};

class TrackSearchHandler : public SpotifySearchHandler
{
public:
    TrackSearchHandler(RestApi&);
    QNetworkReply *fetch(albert::QueryContext &ctx, uint page) const override;
    std::vector<std::shared_ptr<albert::Item>>
    handleReply(albert::QueryContext &ctx, const QJsonDocument &doc) override;
};

class ArtistSearchHandler : public SpotifySearchHandler
{
public:
    ArtistSearchHandler(RestApi&);
    QNetworkReply *fetch(albert::QueryContext &ctx, uint page) const override;
    std::vector<std::shared_ptr<albert::Item>>
    handleReply(albert::QueryContext &ctx, const QJsonDocument &doc) override;
};

class AlbumSearchHandler : public SpotifySearchHandler
{
public:
    AlbumSearchHandler(RestApi&);
    QNetworkReply *fetch(albert::QueryContext &ctx, uint page) const override;
    std::vector<std::shared_ptr<albert::Item>>
    handleReply(albert::QueryContext &ctx, const QJsonDocument &doc) override;
};

class  PlaylistSearchHandler : public SpotifySearchHandler
{
public:
    PlaylistSearchHandler(RestApi&);
    QNetworkReply *fetch(albert::QueryContext &ctx, uint page) const override;
    std::vector<std::shared_ptr<albert::Item>>
    handleReply(albert::QueryContext &ctx, const QJsonDocument &doc) override;
};

class ShowSearchHandler : public SpotifySearchHandler
{
public:
    ShowSearchHandler(RestApi&);
    QNetworkReply *fetch(albert::QueryContext &ctx, uint page) const override;
    std::vector<std::shared_ptr<albert::Item>>
    handleReply(albert::QueryContext &ctx, const QJsonDocument &doc) override;
};

class EpisodeSearchHandler : public SpotifySearchHandler
{
public:
    EpisodeSearchHandler(RestApi&);
    QNetworkReply *fetch(albert::QueryContext &ctx, uint page) const override;
    std::vector<std::shared_ptr<albert::Item>>
    handleReply(albert::QueryContext &ctx, const QJsonDocument &doc) override;
};

class AudiobookSearchHandler : public SpotifySearchHandler
{
public:
    AudiobookSearchHandler(RestApi&);
    QNetworkReply *fetch(albert::QueryContext &ctx, uint page) const override;
    std::vector<std::shared_ptr<albert::Item>>
    handleReply(albert::QueryContext &ctx, const QJsonDocument &doc) override;
};
