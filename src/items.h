// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include <albert/item.h>
#include <memory>
#include <set>
class Plugin;
class QJsonObject;
namespace albert::util { class FileDownloader; }

class MediaItem : public albert:: Item
{
public:
    MediaItem(Plugin &plugin,
              const QString &uri,
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

protected:

    std::set<Item::Observer*> observers;
    Plugin &plugin_;
    QString uri_;
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
    std::vector<albert::Action> actions() const override;
};


class ArtistItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    std::vector<albert::Action> actions() const override;
};


class AlbumItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    std::vector<albert::Action> actions() const override;
};


class PlaylistItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    std::vector<albert::Action> actions() const override;
};


class ShowItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    std::vector<albert::Action> actions() const override;
};


class EpisodeItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    std::vector<albert::Action> actions() const override;
};


class AudiobookItem : public MediaItem
{
public:
    using MediaItem::MediaItem;
    static std::shared_ptr<Item> make(Plugin&, const QJsonObject&);
    std::vector<albert::Action> actions() const override;
};
