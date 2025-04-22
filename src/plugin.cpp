// Copyright (c) 2025-2025 Manuel Schneider

#include "configwidget.h"
#include "items.h"
#include "plugin.h"
#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>
#include <albert/desktoputil.h>
#include <albert/logging.h>
#include <albert/networkutil.h>
#include <albert/rankitem.h>
#include <albert/standarditem.h>
#include <ranges>
ALBERT_LOGGING_CATEGORY("spotify")
using namespace albert;
using namespace spotify;
using namespace std;
using namespace util;

const QStringList Plugin::icon_urls{":spotify"};
const QStringList Plugin::error_icon_urls{"comp:?src1=%3Aspotify&src2=qsp%3ASP_MessageBoxWarning"};

struct {
    const char *kck_secrets         = "secrets";
    const char *ck_show_explicit    = "show_explicit";
    const char *sk_token_expiration = "token_expiration";
    const char *sk_last_device      = "last_device";
    const char *oauth_auth_url      = "https://accounts.spotify.com/authorize";
    const char *oauth_token_url     = "https://accounts.spotify.com/api/token";
    const char *oauth_scope         = "user-library-read "
                                      "user-modify-playback-state "
                                      "user-read-playback-state "
                                      "user-read-private";
} const strings ;


struct Device
{
    QString id;
    QString name;
    QString type;
    bool isActive;

    static Device fromJson(const QJsonObject &json)
    {
        return Device{
            .id=json["id"].toString(),
            .name=json["name"].toString(),
            .type=json["type"].toString(),
            .isActive=json["is_active"].toBool()
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


Plugin::Plugin()
{
    const auto s = settings();
    restore_show_explicit_content(s);
    restore_search_types(s);

    oauth.setAuthUrl(strings.oauth_auth_url);
    oauth.setScope(strings.oauth_scope);
    oauth.setTokenUrl(strings.oauth_token_url);
    oauth.setRedirectUri(QString("%1://%2/").arg(qApp->applicationName(), id()));
    oauth.setPkceEnabled(true);

    connect(&oauth, &OAuth2::tokensChanged, this, [this] {
        writeSecrets();
        state()->setValue(strings.sk_token_expiration, oauth.tokenExpiration());
        api.setBearerToken(oauth.accessToken().toUtf8());
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

bool Plugin::premium() const { return premium_; }

static const bool &debounce(const bool &valid)
{
    static mutex m;
    static long long block_until = 0;
    auto now = QDateTime::currentMSecsSinceEpoch();

    unique_lock lock(m);

    while (block_until > QDateTime::currentMSecsSinceEpoch())
        if (valid)
            QThread::msleep(10);
        else
            return valid;

    block_until = now + 1000;

    return valid;
}

static shared_ptr<Item> makeErrorItem(const QString &error)
{
    WARN << error;
    return StandardItem::make("notify", "Spotify", error, Plugin::error_icon_urls);
}

void Plugin::handleTriggerQuery(Query &query)
{
    if (!debounce(query.isValid()))
        return;

    // Drop first word if it matches a media type and put the type to the search type flags
    auto query_string = query.string();
    SearchTypeFlags types = None;
    while (1)
    {
        const auto prefix = query_string.section(QChar::Space, 0, 0, QString::SectionSkipEmpty);
        if (auto it = ranges::find_if(type_strings, [&](auto &type_string){ return type_string == prefix; });
            it != type_strings.end())
        {
            auto index = distance(type_strings.begin(), it);
            types = static_cast<SearchTypeFlags>(types | (1 << index));
            query_string = query_string.section(QChar::Space, 1, -1, QString::SectionSkipEmpty);
        }
        else
            break;
    }
    if (!types)
        types = search_types();


    if (query.string().trimmed().isEmpty())
    {
        if (const auto var = RestApi::parseJson(await(api.userMusic()));
            holds_alternative<QString>(var))
            query.add(makeErrorItem(get<QString>(var)));
        else
        {
            vector<shared_ptr<Item>> results;
            auto v = get<QJsonDocument>(var)["items"].toArray()
                     | views::filter([](const auto val){ return !val.isNull(); })
                     | views::transform([](const auto val){ return val.toObject()["track"].toObject(); });

            for (const auto &json_object : v)
                results.emplace_back(TrackItem::make(*this, json_object));

            query.add(::move(results));
        }
    }

    else if (const auto var = RestApi::parseJson(await(api.search(query_string, types)));
             holds_alternative<QString>(var))
        query.add(makeErrorItem(get<QString>(var)));
    else
    {
        const auto json_data = get<QJsonDocument>(var);
        vector<shared_ptr<Item>> results;

        const auto filter = [this](const auto result_category){
            return result_category["items"].toArray()
                   | views::filter([](const auto v){ return !v.isNull(); })
                   | views::transform([](const auto v){ return v.toObject(); })
                   | views::filter([this](const auto o){ return show_explicit_content()
                                                                 || !o["explicit"].toBool(); });
        };

        for (const auto &json_object : filter(json_data["tracks"]))
            results.emplace_back(TrackItem::make(*this, json_object));

        for (const auto &json_object : filter(json_data["artists"]))
            results.emplace_back(ArtistItem::make(*this, json_object));

        for (const auto &json_object : filter(json_data["albums"]))
            results.emplace_back(AlbumItem::make(*this, json_object));

        for (const auto &json_object : filter(json_data["playlists"]))
            results.emplace_back(PlayListItem::make(*this, json_object));

        for (const auto &json_object : filter(json_data["shows"]))
            results.emplace_back(ShowItem::make(*this, json_object));

        for (const auto &json_object : filter(json_data["episodes"]))
            results.emplace_back(EpisodeItem::make(*this, json_object));

        for (const auto &json_object : filter(json_data["audiobooks"]))
            results.emplace_back(AudiobookItem::make(*this, json_object));

        query.add(::move(results));
    }
}

void Plugin::play(const MediaItem &media_item)
{
    if (premium_)
    {
        if (const auto var = RestApi::parseJson(await(api.getDevices()));
            holds_alternative<QString>(var))
        {
            const auto error = get<QString>(var);
            WARN << "Failed fetching devices:" << error;
            INFO << "Open local Spotify to run" << media_item.uri();
        }

        // If we have no devices run local Spotify client
        else if (const auto devices = Device::parseDevices(
                     get<QJsonDocument>(var)["devices"].toArray());
                 devices.empty())
        {
            DEBG << "Spotify API returned no devices";
            INFO << "Open local Spotify to run" << media_item.uri();
            openUrl(media_item.uri());
        }

        // If available, use an active device and play the track.
        else if (auto it = ranges::find_if(devices, &Device::isActive); it != devices.cend())
        {
            const auto activeDevice = *it;
            api.play(media_item.uri(), it->id)->deleteLater();
            INFO << "Playing on active device:" << it->name;
            state()->setValue(strings.sk_last_device, it->id);
        }

        // If available, use the last-used device.
        else if (it = ranges::find_if(devices,
                                      [id = state()->value(strings.sk_last_device).toString()](
                                          const auto &dev) { return dev.id == id; });
                 it != devices.end())
        {
            api.play(media_item.uri(), it->id)->deleteLater();
            INFO << "Playing on last used device:" << it->name;
        }

        // Otherwise Use the first available device.
        else
        {
            api.play(media_item.uri(), devices[0].id)->deleteLater();
            INFO << "Playing on:" << devices[0].id;
            state()->setValue(strings.sk_last_device, devices[0].id);
        }
    }
    else
    {
        if (media_remote)
            media_remote->pause();
        INFO << "Open local Spotify to run" << media_item.uri();
        openUrl(media_item.uri());
    }
}

void Plugin::addToQueue(const MediaItem &)
{

}

void Plugin::downloadAlbumCover(const QString &album_id,
                                const QUrl &url,
                                function<void(const QString &)> onFinished)
{
    auto album_covers_location = cacheLocation() / "album_covers";
    if (!is_directory(album_covers_location))
        tryCreateDirectory(album_covers_location);

    auto icon_path = QDir(album_covers_location).filePath(album_id + ".jpeg");

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
            const auto product_string = profile["product"].toString();
            INFO << "Product type:" << product_string;
            premium_ = profile["product"].toString() == "premium";

            disconnect(&oauth, &OAuth2::tokensChanged, this, &Plugin::initializeAccountType);
        }
    });
}

void Plugin::readSecrets()
{
    if (auto secrets = readKeychain(strings.kck_secrets).split(QChar::Tabulation);
        secrets.size() == 4)
    {
        oauth.setClientId(secrets[0]);
        oauth.setClientSecret(secrets[1]);
        oauth.setTokens(
            secrets[2],
            secrets[3],
            state()->value(strings.sk_token_expiration).toDateTime()
        );
    }
}

void Plugin::writeSecrets() const
{
    writeKeychain(strings.kck_secrets,
                  QStringList{
                      oauth.clientId(),
                      oauth.clientSecret(),
                      oauth.accessToken(),
                      oauth.refreshToken()
                  }.join(QChar::Tabulation));

    state()->setValue(strings.sk_token_expiration, oauth.tokenExpiration());
}

spotify::SearchTypeFlags Plugin::search_types() const { return search_types_; }

void Plugin::set_search_types_(spotify::SearchTypeFlags search_types)
{ search_types_ = search_types == None ? All : search_types; }

void Plugin::handle(const QUrl &url)
{
    oauth.handleCallback(url);
    showSettings(id());
}
