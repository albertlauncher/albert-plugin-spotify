// Copyright (c) 2025-2025 Manuel Schneider

#include "configwidget.h"
#include "plugin.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <albert/logging.h>
#include <albert/systemutil.h>
ALBERT_LOGGING_CATEGORY("spotify")
using namespace Qt::StringLiterals;
using namespace albert::util;
using namespace albert;
using namespace spotify;
using namespace std;

const QStringList Plugin::icon_urls{u":spotify"_s};
const QStringList Plugin::error_icon_urls{u"comp:?src1=%3Aspotify&src2=qsp%3ASP_MessageBoxWarning"_s};

namespace
{
static const auto kck_secrets         = u"secrets"_s;
static const auto ck_show_explicit    = u"show_explicit"_s;
static const auto sk_token_expiration = u"token_expiration"_s;
static const auto sk_last_device      = u"last_device"_s;
static const auto oauth_auth_url      = u"https://accounts.spotify.com/authorize"_s;
static const auto oauth_token_url     = u"https://accounts.spotify.com/api/token"_s;
static const auto oauth_scope         = u"playlist-read-collaborative "_s
                                        u"playlist-read-private "_s
                                        u"user-modify-playback-state "_s
                                        u"user-read-playback-state "_s
                                        u"user-read-private "_s
                                        u"user-library-read"_s;
}


struct Device
{
    QString id;
    QString name;
    QString type;
    bool isActive;

    static Device fromJson(const QJsonObject &json)
    {
        return Device{
            .id=json["id"_L1].toString(),
            .name=json["name"_L1].toString(),
            .type=json["type"_L1].toString(),
            .isActive=json["is_active"_L1].toBool()
        };
    }

    static vector<Device> parseDevices(const QJsonArray &array)
    {
        vector<Device> devices;
        for (const auto value : array)
            devices.emplace_back(Device::fromJson(value.toObject()));
        return devices;
    }
};

Plugin::Plugin() :
    track_search_handler(*this),
    artist_search_hanlder(*this),
    album_search_handler(*this),
    playlist_search_handler(*this),
    show_search_handler(*this),
    episode_search_handler(*this),
    audiobook_search_handler(*this)
{
    const auto s = settings();
    restore_show_explicit_content(s);

    oauth.setAuthUrl(oauth_auth_url);
    oauth.setScope(oauth_scope);
    oauth.setTokenUrl(oauth_token_url);
    oauth.setRedirectUri("%1://%2/"_L1.arg(qApp->applicationName(), id()));
    oauth.setPkceEnabled(true);

    connect(&oauth, &OAuth2::tokensChanged, this, [this] {
        writeSecrets();
        state()->setValue(sk_token_expiration, oauth.tokenExpiration());
        api.setBearerToken(oauth.accessToken());
        if (oauth.error().isEmpty())
            INFO << "Tokens updated.";
        else
            WARN << oauth.error();
    });

    connect(&oauth, &OAuth2::tokensChanged,
            this, &Plugin::initializeAccountType);

    readSecrets();

    connect(&oauth, &OAuth2::clientIdChanged, this, &Plugin::writeSecrets);
    connect(&oauth, &OAuth2::clientSecretChanged, this, &Plugin::writeSecrets);
}

Plugin::~Plugin() = default;

QWidget *Plugin::buildConfigWidget() { return new ConfigWidget(*this); }

vector<Extension*> Plugin::extensions() {
    return {
        this,
        &track_search_handler,
        &artist_search_hanlder,
        &album_search_handler,
        &playlist_search_handler,
        &show_search_handler,
        &episode_search_handler,
        &audiobook_search_handler
    };
}

bool Plugin::premium() const { return premium_; }

void Plugin::downloadAlbumCover(const QString &album_id,
                                const QUrl &url,
                                function<void(const QString &)> onFinished)
{
    auto album_covers_location = cacheLocation() / "album_covers";
    if (!is_directory(album_covers_location))
        tryCreateDirectory(album_covers_location);

    auto icon_path = QDir(album_covers_location).filePath(album_id + u".jpeg"_s);

    if (QFile::exists(icon_path))
    {
        onFinished(icon_path);
    }
    else
    {
        unique_lock lock(downloads_mutex);

        if (auto it = downloads.find(album_id); it != downloads.end())
        {
            it->second.callbacks.emplace_back(onFinished);
        }
        else
        {
            auto p = downloads.emplace(
                piecewise_construct,
                forward_as_tuple(album_id),
                forward_as_tuple(vector<function<void(const QString &)>>{onFinished},
                                 make_unique<FileDownloader>(url, icon_path)));

            auto on_finish = [this, album_id](bool success, const QString &path_or_error)
            {
                unique_lock alock(downloads_mutex);
                const auto ait = downloads.find(album_id);
                if (ait != downloads.end())
                {
                    if (success)
                        for (auto &callback : ait->second.callbacks)
                            callback(path_or_error);
                    else
                        WARN << path_or_error;
                    downloads.erase(ait);
                }
                else
                    CRIT << "Logic error in Plugin::downloadAlbumCover";
            };

            auto downloader = p.first->second.downloader.get();
            downloader->moveToThread(thread());
            QObject::connect(downloader, &FileDownloader::finished,
                             this, on_finish, Qt::QueuedConnection);
            downloader->start();
        }
    }
}

void Plugin::initializeAccountType()
{
    if (oauth.state() != OAuth2::State::Granted)
        return;

    auto reply = api.userProfile();
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        auto var = RestApi::parseJson(reply);
        if (holds_alternative<QString>(var))
        {
            const auto error = get<QString>(var);
            WARN << "Failed fetching user profile:" << error;
            return;
        }
        else
        {
            const auto profile = get<QJsonDocument>(var);
            const auto product_string = profile["product"_L1].toString();
            INFO << "Product type:" << product_string;
            premium_ = profile["product"_L1].toString() == u"premium"_s;

            disconnect(&oauth, &OAuth2::tokensChanged, this, &Plugin::initializeAccountType);
        }
    });
}

void Plugin::readSecrets()
{
    if (auto secrets = readKeychain(kck_secrets).split(QChar::Tabulation);
        secrets.size() == 4)
    {
        oauth.setClientId(secrets[0]);
        oauth.setClientSecret(secrets[1]);
        oauth.setTokens(
            secrets[2],
            secrets[3],
            state()->value(sk_token_expiration).toDateTime()
        );
    }
}

void Plugin::writeSecrets() const
{
    writeKeychain(kck_secrets,
                  QStringList{
                      oauth.clientId(),
                      oauth.clientSecret(),
                      oauth.accessToken(),
                      oauth.refreshToken()
                  }.join(QChar::Tabulation));

    state()->setValue(sk_token_expiration, oauth.tokenExpiration());
}

void Plugin::handle(const QUrl &url)
{
    oauth.handleCallback(url);
    showSettings(id());
}
