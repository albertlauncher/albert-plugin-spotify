# Albert plugin: Spotify

## Features

Provides triggered query handlers for Spotify searches. 

Besides their obvious primary purpose, the empty query for the handler with id …

- *track* returns the users top tracks.
- *artist* returns the users top artists.
- *album* returns the users saved albums.
- *playlist* returns the users saved playlists.
- *show* returns the users saved shows.
- *episode* returns the users saved episodes.
- *audiobook* returns the users saved audiobooks.

The "modify-playback-state" scope of the web API works for premium accounts only. 
Free accounts can use the feature set of the local `spotify:` scheme handler though.

This plugin is work in progress. Currently the items support these actions:

- *track* 
  - Premium: Play/Queue on active device.
  - Free: Play in local spotify client.
- *artist* 
  - Show in local spotify client.
- *album*
  - Show in local spotify client.
- *playlist*
  - Show in local spotify client.
- *show*
  - Show in local spotify client.
- *episode*
  - Premium: Play/Queue on active device.
  - Free: Show in local spotify client.
- *audiobook*
  - Show in local spotify client.

## Setup

1. Create account at https://developer.spotify.com/dashboard/applications.
1. Create an app.
1. Add Redirect URI `albert://spotify/`.
1. Tick checkbox *Web API*.
1. Add your name and mail under *User Management*.
1. Insert *Client ID* in the plugin settings and click the authorize button.

## Technical notes

- Uses the [Spotfiy Web API](https://developer.spotify.com/documentation/web-api).
- See `spotify.h` for the endpoints used.
