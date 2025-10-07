// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "spotify.h"
#include <QObject>
#include <albert/item.h>
#include <memory>
#include <set>
class QJsonObject;
namespace albert { class Icon; }
namespace albert::util { class Download; }

class SpotifyItem : public QObject,
                    public albert::Item
{
    Q_OBJECT
public:
    SpotifyItem(spotify::RestApi &api,
                const QString &spotify_id,
                const QString &title,
                const QString &description,
                const QString &icon_url);
    ~SpotifyItem();

    QString id() const override;
    QString text() const override;
    QString subtext() const override;
    std::unique_ptr<albert::Icon> icon() const override;

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
    mutable std::unique_ptr<albert::Icon> icon_;
    mutable std::shared_ptr<albert::util::Download> download_;

};


class TrackItem : public SpotifyItem
{
public:
    TrackItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class ArtistItem : public SpotifyItem
{
public:
    ArtistItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class AlbumItem : public SpotifyItem
{
public:
    AlbumItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class PlaylistItem : public SpotifyItem
{
public:
    PlaylistItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class ShowItem : public SpotifyItem
{
public:
    ShowItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class EpisodeItem : public SpotifyItem
{
public:
    EpisodeItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};


class AudiobookItem : public SpotifyItem
{
public:
    AudiobookItem(spotify::RestApi&, const QJsonObject&);
    spotify::SearchType type() const override final;
    std::vector<albert::Action> actions() const override;
};
