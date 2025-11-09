// Copyright (c) 2025-2025 Manuel Schneider

#include "items.h"
#include "spotify.h"
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <albert/app.h>
#include <albert/download.h>
#include <albert/iconutil.h>
#include <albert/logging.h>
#include <albert/networkutil.h>
#include <albert/systemutil.h>
#include <ranges>
using namespace Qt::StringLiterals;
using namespace albert;
using namespace spotify;
using namespace std;

#if defined Q_OS_MAC
static void pauseSpotify()
{

    try {
        runAppleScript(uR"(tell application "Spotify" to pause)"_s);
    } catch (const std::runtime_error &e) {
        WARN << e.what();
    }
}
#elif defined Q_OS_LINUX
#include <QDBusInterface>
void pauseSpotify()
{
    QDBusInterface(
        u"org.mpris.MediaPlayer2.spotify"_s,
        u"/org/mpris/MediaPlayer2"_s,
        u"org.mpris.MediaPlayer2.Player"_s,
        QDBusConnection::sessionBus()
    ).call(u"Pause"_s);
}
#endif

SpotifyItem::SpotifyItem(const RestApi &api,
                         const QString &spotify_id,
                         const QString &title,
                         const QString &description,
                         const QString &icon_url) :
    api_(api),
    spotify_id_(spotify_id),
    title_(title),
    description_(description),
    icon_url_(icon_url)
{
}

SpotifyItem::~SpotifyItem() = default;

QString SpotifyItem::id() const { return spotify_id_; }

QString SpotifyItem::text() const { return title_; }

QString SpotifyItem::subtext() const { return description_; }

std::unique_ptr<Icon> SpotifyItem::icon() const
{
    if (!icon_)  // lazy, first request
    {
        const auto icons_location = App::cacheLocation() / "spotify" / "icons";

        if (const auto icon_path = QDir(icons_location).filePath(id() + u".jpeg"_s);
            QFile::exists(icon_path))
            icon_ = makeIconifiedIcon(makeImageIcon(icon_path), iconifiedIconDefaultColor(), .4);

        else if (!download_)
        {
            download_ = Download::unique(icon_url_, icon_path);

            connect(download_.get(), &Download::finished, this, [=, this]{
                if (const auto error = download_->error();
                    error.isNull())
                    icon_ = makeIconifiedIcon(makeImageIcon(download_->path()), iconifiedIconDefaultColor(), .4);
                else
                {
                    WARN << "Failed to download icon:" << error;
                    icon_ = makeThemeIcon(u"spotify"_s);
                }

                dataChanged();
            });
        }
    }

    return icon_ ? icon_->clone() : nullptr;  // awaiting if null
}

QString SpotifyItem::uri() const { return u"spotify:%1:%2"_s.arg(typeString(type()), id()); }

QString SpotifyItem::tr_show_in() { return tr("Show in Spotify"); }

QString SpotifyItem::tr_play_in()  { return tr("Play in Spotify"); }

QString SpotifyItem::tr_play_on() { return tr("Play on Spotify"); }

QString SpotifyItem::tr_queue()  { return tr("Add to queue"); }

// -------------------------------------------------------------------------------------------------

static void play(const spotify::RestApi &api, const QString &uri)
{
    const auto reply = api.play({uri});
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, uri]{
        const auto var = RestApi::parseJson(reply);
        if (holds_alternative<QString>(var))
        {
            const auto error = get<QString>(var);
            DEBG << "Failed to play" << uri << error;
            DEBG << "Open local Spotify to run" << uri;
            openUrl(uri);
        }
        else
            DEBG << "Successfully played" << uri;
    });
}

static void queue(const spotify::RestApi &api, const QString &uri)
{
    const auto reply = api.queue({uri});
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, uri]{
        const auto var = RestApi::parseJson(reply);
        if (holds_alternative<QString>(var))
            WARN << "Failed to queue" << uri;
        else
            DEBG << "Successfully queued" << uri;
    });
}

static QString pickImageUrl(const QJsonArray &arr)
{
    if (arr.isEmpty())
        return {};

    static const auto target_size = 128;

    auto reference = arr.begin()->toObject();
    auto rw = reference["width"_L1].toInt();

    for (auto i = next(arr.begin()); i != arr.end(); ++i)
        if (const auto cw = i->toObject()["width"_L1].toInt();
            rw < target_size)
        {
            if (rw < cw)
            {
                reference = i->toObject();
                rw = cw;
            }
        }
        else if (cw < rw && cw >= target_size)
        {
            reference = i->toObject();
            rw = cw;
        }

    return reference["url"_L1].toString();  // TODO
}

// -------------------------------------------------------------------------------------------------

static QString makeTrackDescription(const QJsonObject &json)
{
    auto view = json["artists"_L1].toArray()
                | views::transform([](const QJsonValue &a) { return a["name"_L1].toString(); });

    return QStringList(view.begin(), view.end()).join(u", "_s);
}

TrackItem::TrackItem(const spotify::RestApi &api, const QJsonObject &json) :
    SpotifyItem(api,
                json["id"_L1].toString(),
                json["name"_L1].toString(),
                makeTrackDescription(json),
                pickImageUrl(json["album"_L1]["images"_L1].toArray()))
{}

SearchType TrackItem::type() const { return Track; }

vector<Action> TrackItem::actions() const
{
    vector<Action> actions;

    if (api_.isPremium())
    {
        actions.emplace_back(u"play"_s, tr_play_on(), [this] { ::play(api_, uri()); });
        actions.emplace_back(u"queue"_s, tr_queue(), [this] { ::queue(api_, uri()); });
    }
    else
    {
        pauseSpotify();
        actions.emplace_back(u"playlocal"_s, tr_play_in(), [this] { openUrl(uri()); });
    }

    return actions;
}

// -------------------------------------------------------------------------------------------------

static QString makeArtistDescription(const QJsonObject &json)
{
    const auto view = json["genres"_L1].toArray()
                      | views::transform([](const QJsonValue &a) {
                          auto s = a.toString();

                          // Capitalize each word
                          bool first = true;
                          for (auto &c : s)
                              if (c.isLetter())
                              {
                                  if (first)
                                  {
                                      c = c.toUpper();
                                      first = false;
                                  }
                              }
                              else
                                  first = true;
                          return s;
                      });


    QString sfollowers;
    if (const auto followers = json["followers"_L1]["total"_L1].toInt();
        followers > 1'000'000)
        sfollowers = u"✨%1M"_s.arg(followers / 1'000'000);
    else if (followers > 1'000)
        sfollowers = u"✨%1k"_s.arg(followers / 1'000);
    else
        sfollowers = u"✨%1"_s.arg(followers);

    if (view.size() == 0)
        return sfollowers;
    else
        return u"%1 · %2"_s.arg(sfollowers, QStringList(view.begin(), view.end()).join(u", "_s));

}

ArtistItem::ArtistItem(const spotify::RestApi &api, const QJsonObject &json) :
    SpotifyItem(api,
                json["id"_L1].toString(),
                json["name"_L1].toString(),
                makeArtistDescription(json),
                pickImageUrl(json["images"_L1].toArray()))
{}

SearchType ArtistItem::type() const { return Artist; }

vector<Action> ArtistItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, tr_show_in(), [this] { openUrl(uri()); });
    return actions;
}

// -------------------------------------------------------------------------------------------------

static QString makeAlbumDescription(const QJsonObject &json)
{
    auto v = json["artists"_L1].toArray()
             | views::transform([](const QJsonValue &a) { return a["name"_L1].toString(); });

    return QStringList{begin(v), end(v)}.join(u", "_s);
}

AlbumItem::AlbumItem(const spotify::RestApi &api, const QJsonObject &json) :
    SpotifyItem(api,
                json["id"_L1].toString(),
                json["name"_L1].toString(),
                makeAlbumDescription(json),
                pickImageUrl(json["images"_L1].toArray()))
{}

SearchType AlbumItem::type() const { return Album; }

vector<Action> AlbumItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, tr_show_in(), [this] { openUrl(uri()); });
    return actions;
}

// -------------------------------------------------------------------------------------------------

PlaylistItem::PlaylistItem(const spotify::RestApi &api, const QJsonObject &json) :
    SpotifyItem(api,
                json["id"_L1].toString(),
                json["name"_L1].toString(),
                json["owner"_L1]["display_name"_L1].toString(),
                pickImageUrl(json["images"_L1].toArray()))
{}

SearchType PlaylistItem::type() const { return Playlist; }

vector<Action> PlaylistItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, tr_show_in(), [this] { openUrl(uri()); });
    return actions;
}

// -------------------------------------------------------------------------------------------------

ShowItem::ShowItem(const spotify::RestApi &api, const QJsonObject &json) :
    SpotifyItem(api,
                json["id"_L1].toString(),
                json["name"_L1].toString(),
                json["publisher"_L1].toString(),
                pickImageUrl(json["images"_L1].toArray()))
{}

SearchType ShowItem::type() const { return Show; }

vector<Action> ShowItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, tr_show_in(), [this] { openUrl(uri()); });
    return actions;
}

// -------------------------------------------------------------------------------------------------

EpisodeItem::EpisodeItem(const spotify::RestApi &api, const QJsonObject &json) :
    SpotifyItem(api,
                json["id"_L1].toString(),
                json["name"_L1].toString(),
                json["description"_L1].toString(),
                pickImageUrl(json["images"_L1].toArray()))
{}

SearchType EpisodeItem::type() const { return Episode; }

vector<Action> EpisodeItem::actions() const
{
    vector<Action> actions;

    if (api_.isPremium())
    {
        actions.emplace_back(u"play"_s, tr_play_on(), [this] { ::play(api_, uri()); });
        actions.emplace_back(u"queue"_s, tr_queue(), [this] { ::queue(api_, uri()); });
    }
    else
    {
        actions.emplace_back(u"show"_s, tr_show_in(), [this] { openUrl(uri()); });
    }

    return actions;
}

// -------------------------------------------------------------------------------------------------

static QString makeAudiobookDescription(const QJsonObject &json)
{
    auto v = json["authors"_L1].toArray()
             | views::transform([](const QJsonValue& a){ return a["name"_L1].toString(); });

    return u"%1 · %2"_s.arg(localizedTypeString(Audiobook),
                            QStringList(v.begin(), v.end()).join(u", "_s));
}

AudiobookItem::AudiobookItem(const spotify::RestApi &api, const QJsonObject &json) :
    SpotifyItem(api,
              json["id"_L1].toString(),
              json["name"_L1].toString(),
              makeAudiobookDescription(json),
              pickImageUrl(json["images"_L1].toArray())) { }

SearchType AudiobookItem::type() const { return Audiobook; }

vector<Action> AudiobookItem::actions() const
{
    vector<Action> actions;
    actions.emplace_back(u"show"_s, tr_show_in(),[this]{ openUrl(uri()); });
    return actions;
}
