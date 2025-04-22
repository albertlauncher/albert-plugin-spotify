// Copyright (c) 2025-2025 Manuel Schneider

#pragma once
#include "items.h"
#include "spotify.h"
#include <albert/extensionplugin.h>
#include <albert/plugin/mediaremote.h>
#include <albert/plugindependency.h>
#include <albert/property.h>
#include <albert/triggerqueryhandler.h>
#include <albert/urlhandler.h>
#include <albert/filedownloader.h>
#include <albert/oauth.h>
#include <memory>
#include <mutex>
#include <vector>

class Plugin final : public albert::ExtensionPlugin,
                     public albert::TriggerQueryHandler,
                     private albert::UrlHandler
{
    ALBERT_PLUGIN

public:

    Plugin();
    ~Plugin() override;

    void handleTriggerQuery(albert::Query&) override;
    QWidget* buildConfigWidget() override;

    void authorizedInitialization();

    bool premium() const;
    void play(const MediaItem&);
    void addToQueue(const MediaItem&);

    void downloadAlbumCover(const QString &album_id, const QUrl &url,
                            std::function<void(const QString&)> onFinished);

    albert::util::OAuth2 oauth;
    spotify::RestApi api;
    albert::WeakDependency<albert::plugin::mediaremote::IPlugin> media_remote{"mediaremote"};

private:

    void initializeAccountType();
    void readSecrets();
    void writeSecrets() const;

    void handle(const QUrl &) override;

    bool premium_ = false;
    spotify::SearchTypeFlags search_types_;

    std::mutex downloads_mutex;

    struct Download {
        std::vector<std::function<void(const QString&)>> callbacks;
        std::unique_ptr<albert::util::FileDownLoader> downloader;
    };

    std::map<QString, Download> downloads;

    ALBERT_PLUGIN_PROPERTY(bool, show_explicit_content, true)
    ALBERT_PLUGIN_PROPERTY_GETSET(spotify::SearchTypeFlags, search_types, spotify::SearchTypeFlags::All)

};
