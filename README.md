# albert-plugin-spotify

## Features

- Search media on spotify
- Set default search media types
- Change query media type by prefixing the query with one of:
  - *album*, *artist*, *playlist*, *track*, *show*, *episode*, *audiobook*
- Play/queue media on your Spotify clients
  
## Limitations

The web API works for premium accounts only. 
Free accounts use only the local `spotify:` scheme handler.

Note: If you have enabled the media remote plugin, playback will be paused before opening a URI.
This will start playback of opened URIs.

## Setup

1. Create account at https://developer.spotify.com/dashboard/applications.
1. Create an app.
1. Add Redirect URI `albert://spotify/`.
1. Tick checkbox *Web API*.
1. Add your name and mail under *User Management*.
1. Insert *Client ID* and *Client secret* in the plugin settings and click the authorize button.

## Technical notes

- Uses the [Spotfiy Web API](https://developer.spotify.com/documentation/web-api).
- See `spotify.h` for the endpoints used.
