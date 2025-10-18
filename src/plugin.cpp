// Copyright (c) 2025-2025 Manuel Schneider

#include "plugin.h"
#include <QSettings>
#include <albert/app.h>
#include <albert/logging.h>
#include <albert/oauthconfigwidget.h>
ALBERT_LOGGING_CATEGORY("spotify")
using namespace Qt::StringLiterals;
using namespace albert;
using namespace std;

namespace
{
static const auto kck_secrets         = u"secrets"_s;
static const auto sk_token_expiration = u"token_expiration"_s;
}


// struct Device
// {
//     QString id;
//     QString name;
//     QString type;
//     bool isActive;

//     static Device fromJson(const QJsonObject &json)
//     {
//         return Device{
//             .id=json["id"_L1].toString(),
//             .name=json["name"_L1].toString(),
//             .type=json["type"_L1].toString(),
//             .isActive=json["is_active"_L1].toBool()
//         };
//     }

//     static vector<Device> parseDevices(const QJsonArray &array)
//     {
//         vector<Device> devices;
//         for (const auto value : array)
//             devices.emplace_back(Device::fromJson(value.toObject()));
//         return devices;
//     }
// };

Plugin::Plugin() :
    track_search_handler(api),
    artist_search_hanlder(api),
    album_search_handler(api),
    playlist_search_handler(api),
    show_search_handler(api),
    episode_search_handler(api),
    audiobook_search_handler(api)
{
    // Read tokens
    readKeychain(kck_secrets,
                 [this](const QString &value){
                     if (auto secrets = value.split(QChar::Tabulation);
                         secrets.size() == 4)
                     {
                         api.oauth.setClientId(secrets[0]);
                         api.oauth.setClientSecret(secrets[1]);
                         api.oauth.setTokens(
                             secrets[2],
                             secrets[3],
                             state()->value(sk_token_expiration).toDateTime()
                             );
                         DEBG << "Successfully read Spotify OAuth credentials to keychain.";
                     }
                     else
                         WARN << "Unexpected format of the Spotify OAuth credentials read from keychain.";
                 },
                 [](const QString & error){
                     WARN << "Failed to read Spotify OAuth credentials to keychain:" << error;
                 });


    const auto writeAuthConfig = [this]{
        state()->setValue(sk_token_expiration, api.oauth.tokenExpiration());
        writeKeychain(kck_secrets,
                      QStringList{
                          api.oauth.clientId(),
                          api.oauth.clientSecret(),
                          api.oauth.accessToken(),
                          api.oauth.refreshToken()
                      }.join(QChar::Tabulation),
                      [] {
                          DEBG << "Successfully wrote Spotify OAuth credentials to keychain.";
                      },
                      [](const QString &error){
                          WARN << "Failed to write Spotify OAuth credentials to keychain:" << error;
                      });
    };

    connect(&api.oauth, &OAuth2::clientIdChanged, this, writeAuthConfig);
    connect(&api.oauth, &OAuth2::clientSecretChanged, this, writeAuthConfig);
    connect(&api.oauth, &OAuth2::tokensChanged, this, writeAuthConfig);
}

Plugin::~Plugin() = default;

QWidget *Plugin::buildConfigWidget()
{
    return new OAuthConfigWidget(api.oauth);
}

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


void Plugin::handle(const QUrl &url)
{
    api.oauth.handleCallback(url);
    App::instance().showSettings(id());
}
