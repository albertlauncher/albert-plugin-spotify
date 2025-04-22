// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "handlers.h"
#include "spotify.h"
#include <albert/extensionplugin.h>
#include <albert/filedownloader.h>
#include <albert/oauth.h>
#include <albert/plugin/mediaremote.h>
#include <albert/plugindependency.h>
#include <albert/property.h>
#include <albert/urlhandler.h>
#include <memory>
#include <mutex>
#include <vector>

class Plugin final : public albert::ExtensionPlugin,
                     private albert::UrlHandler
{
    ALBERT_PLUGIN

public:

    Plugin();
    ~Plugin() override;

    // void handleTriggerQuery(albert::Query&) override;
    QWidget* buildConfigWidget() override;
    std::vector<albert::Extension*> extensions() override;

    void authorizedInitialization();

    bool premium() const;
    // void play(const MediaItem&);
    // void addToQueue(const MediaItem&);

    void downloadAlbumCover(const QString &album_id, const QUrl &url,
                            std::function<void(const QString&)> onFinished);

    albert::util::OAuth2 oauth;
    spotify::RestApi api;
    albert::WeakDependency<albert::plugin::mediaremote::IPlugin> media_remote{QStringLiteral("mediaremote")};

    static const QStringList icon_urls;
    static const QStringList error_icon_urls;
    struct {
        QString play_on_spotify = tr("Play on Spotify");
        QString play_in_spotify = tr("Play in Spotify");
        QString show_in_spotify = tr("Show in Spotify");
        QString add_to_queue = tr("Add to queue");
    } const ui_strings;

private:

    void initializeAccountType();
    void readSecrets();
    void writeSecrets() const;

    void handle(const QUrl &) override;

    bool premium_ = false;
    spotify::SearchType search_types_;

    std::mutex downloads_mutex;

    struct Download {
        std::vector<std::function<void(const QString&)>> callbacks;
        std::unique_ptr<albert::util::FileDownloader> downloader;
    };

    std::map<QString, Download> downloads;

    ALBERT_PLUGIN_PROPERTY(bool, show_explicit_content, true)

    TrackSearchHandler track_search_handler;
    ArtistSearchHandler artist_search_hanlder;
    AlbumSearchHandler album_search_handler;
    PlaylistSearchHandler playlist_search_handler;
    ShowSearchHandler show_search_handler;
    EpisodeSearchHandler episode_search_handler;
    AudiobookSearchHandler audiobook_search_handler;

};
