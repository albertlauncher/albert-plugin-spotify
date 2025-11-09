// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "api.h"
#include <QObject>
#include <albert/item.h>
#include <memory>
class QJsonObject;
namespace albert { class Download; class Icon; }

class SpotifyItem : public QObject, public albert::detail::DynamicItem
{
    Q_OBJECT
public:
    SpotifyItem(const RestApi &api,
                const QString &spotify_id,
                const QString &title,
                const QString &description,
                const QString &icon_url);
    ~SpotifyItem();

    QString id() const override;
    QString text() const override;
    QString subtext() const override;
    std::unique_ptr<albert::Icon> icon() const override;

    virtual SearchType type() const = 0;
    QString uri() const;

protected:

    static QString tr_show_in();
    static QString tr_play_in();
    static QString tr_play_on();
    static QString tr_queue();

    const RestApi &api_;
    QString spotify_id_;
    QString title_;
    QString description_;
    QString icon_url_;
    mutable std::unique_ptr<albert::Icon> icon_;
    mutable std::shared_ptr<albert::Download> download_;

};


class TrackItem : public SpotifyItem
{
public:
    TrackItem(const RestApi&, const QJsonObject&);
    SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class ArtistItem : public SpotifyItem
{
public:
    ArtistItem(const RestApi&, const QJsonObject&);
    SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class AlbumItem : public SpotifyItem
{
public:
    AlbumItem(const RestApi&, const QJsonObject&);
    SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class PlaylistItem : public SpotifyItem
{
public:
    PlaylistItem(const RestApi&, const QJsonObject&);
    SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class ShowItem : public SpotifyItem
{
public:
    ShowItem(const RestApi&, const QJsonObject&);
    SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class EpisodeItem : public SpotifyItem
{
public:
    EpisodeItem(const RestApi&, const QJsonObject&);
    SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class AudiobookItem : public SpotifyItem
{
public:
    AudiobookItem(const RestApi&, const QJsonObject&);
    SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};
