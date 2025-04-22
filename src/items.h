// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "spotify.h"
#include <albert/item.h>
#include <QDir>
#include <QCoreApplication>
#include <QJsonObject>
#include <memory>
#include <set>
namespace albert::util { class FileDownloader; }
class Plugin;

class MediaItem : public albert:: Item
{
public:
    MediaItem(Plugin &plugin,
              const QJsonObject &json,
              const QString &spotify_id,
              const QString &title,
              const QString &description,
              const QString &icon_url);
    ~MediaItem();

    const QJsonObject &json() const;
    QString uri() const;
    virtual spotify::SearchTypeFlags mediaType() const = 0;

    QString id() const override;
    QString text() const override;
    QString subtext() const override;
    QStringList iconUrls() const override;
    std::vector<albert::Action> actions() const override;

    void addObserver(Observer *observer) override;
    void removeObserver(Observer *observer) override;

protected:

    std::set<Item::Observer*> observers;
    Plugin &plugin_;
    QJsonObject json_;
    QString spotify_id_;
    QString title_;
    QString description_;
    QString icon_url_;
    mutable QString icon_;
    std::unique_ptr<albert::util::FileDownloader> file_downloader_;

};


class TrackItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    spotify::SearchTypeFlags mediaType() const override;
};


class EpisodeItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    spotify::SearchTypeFlags mediaType() const override;
};


class ArtistItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    spotify::SearchTypeFlags mediaType() const override;
};


class AlbumItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    spotify::SearchTypeFlags mediaType() const override;
};


class PlayListItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    spotify::SearchTypeFlags mediaType() const override;
};


class ShowItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    spotify::SearchTypeFlags mediaType() const override;
};


class AudiobookItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    spotify::SearchTypeFlags mediaType() const override;
};
