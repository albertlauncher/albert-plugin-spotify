// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "api.h"
#include "handlers.h"
#include <albert/extensionplugin.h>
#include <albert/urlhandler.h>
#include <vector>

class Plugin final : public albert::ExtensionPlugin,
                     private albert::UrlHandler
{
    ALBERT_PLUGIN

public:

    Plugin();
    ~Plugin() override;

    QWidget* buildConfigWidget() override;
    std::vector<albert::Extension*> extensions() override;
    void handle(const QUrl &) override;

private:

    RestApi api;

    TrackSearchHandler track_search_handler;
    ArtistSearchHandler artist_search_hanlder;
    AlbumSearchHandler album_search_handler;
    PlaylistSearchHandler playlist_search_handler;
    ShowSearchHandler show_search_handler;
    EpisodeSearchHandler episode_search_handler;
    AudiobookSearchHandler audiobook_search_handler;

};
