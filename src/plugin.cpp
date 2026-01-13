// Copyright (c) 2025-2025 Manuel Schneider

#include "plugin.h"
#include <QSettings>
#include <albert/app.h>
#include <albert/logging.h>
#include <albert/oauthconfigwidget.h>
#include <qt6keychain/keychain.h>
ALBERT_LOGGING_CATEGORY("spotify")
using namespace Qt::StringLiterals;
using namespace albert;
using namespace std;

namespace
{
static const auto keychain_service = u"albert.spotify"_s;
static const auto keychain_key = u"secrets"_s;
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
{}

Plugin::~Plugin() = default;

void Plugin::initialize()
{
    auto *job = new QKeychain::ReadPasswordJob(keychain_service, this);  // Deletes itself
    job->setKey(keychain_key);

    connect(job, &QKeychain::ReadPasswordJob::finished, this, [this, job] {
        if (job->error())
            WARN << "Failed to read secrets from keychain:" << job->errorString();
        else if (auto secrets = job->textData().split(QChar::Tabulation);
                 secrets.size() != 4)
            WARN << "Unexpected format of the secrets read from keychain.";
        else
        {
            api.oauth.setClientId(secrets[0]);
            api.oauth.setClientSecret(secrets[1]);
            api.oauth.setTokens(
                secrets[2],
                secrets[3],
                state()->value(sk_token_expiration).toDateTime()
                );

            DEBG << "Successfully read secrets from keychain.";
        }

        connect(&api.oauth, &OAuth2::clientIdChanged,     this, &Plugin::writeSecrets);
        connect(&api.oauth, &OAuth2::clientSecretChanged, this, &Plugin::writeSecrets);
        connect(&api.oauth, &OAuth2::tokensChanged,       this, &Plugin::writeSecrets);

        emit initialized();
    });

    job->start();
}

void Plugin::writeSecrets()
{
    state()->setValue(sk_token_expiration, api.oauth.tokenExpiration());

    QStringList secrets = {api.oauth.clientId(),
                           api.oauth.clientSecret(),
                           api.oauth.accessToken(),
                           api.oauth.refreshToken()};

    auto job = new QKeychain::WritePasswordJob(keychain_service, this);  // Deletes itself
    job->setKey(keychain_key);
    job->setTextData(secrets.join(QChar::Tabulation));

    connect(job, &QKeychain::Job::finished, this, [=] {
        if (job->error())
            WARN << "Failed to write secrets to keychain:" << job->errorString();
        else
            DEBG << "Successfully wrote secrets to keychain.";
    });

    job->start();
}

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
