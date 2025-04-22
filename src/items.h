// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "spotify.h"
#include <QObject>
#include <albert/item.h>
#include <memory>
#include <set>
class QJsonObject;
namespace albert::util { class Download; }

class MediaItem : public QObject,
                  public albert::Item
{
    Q_OBJECT
public:
    MediaItem(spotify::RestApi &api,
              const QString &spotify_id,
              const QString &title,
              const QString &description,
              const QString &icon_url);
    ~MediaItem();

    QString id() const override;
    QString text() const override;
    QString subtext() const override;
    QStringList iconUrls() const override;

    void addObserver(Observer *observer) override;
    void removeObserver(Observer *observer) override;

    virtual spotify::SearchType type() const = 0;
    QString uri() const;

protected:

    static QString tr_show_in();
    static QString tr_play_in();
    static QString tr_play_on();
    static QString tr_queue();

    std::set<Item::Observer*> observers;
    spotify::RestApi &api_;
    QString spotify_id_;
    QString title_;
    QString description_;
    QString icon_url_;
    mutable QString icon_;
    mutable std::shared_ptr<albert::util::Download> download_;

};


class TrackItem : public MediaItem
{
public:
    TrackItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class ArtistItem : public MediaItem
{
public:
    ArtistItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class AlbumItem : public MediaItem
{
public:
    AlbumItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class PlaylistItem : public MediaItem
{
public:
    PlaylistItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class ShowItem : public MediaItem
{
public:
    ShowItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class EpisodeItem : public MediaItem
{
public:
    EpisodeItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class AudiobookItem : public MediaItem
{
public:
    AudiobookItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};
