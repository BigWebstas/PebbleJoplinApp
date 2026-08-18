# PebbleJoplinApp

A read-only Joplin notes browser for Pebble watches (including the open-source PebbleOS),
syncing from a self-hosted **Joplin Server** with end-to-end encryption support.

Since a Pebble has no network of its own and Joplin Server's sync data is E2EE-encrypted,
this is three pieces:

- **`bridge/`** - a small Node.js service you self-host. It shells out to the official
  Joplin CLI (which correctly implements Joplin's sync protocol and E2EE decryption) and
  serves the resulting notes as plain JSON over a token-authed REST API.
- **`pebble-app/`** - the watchapp (C) plus its PebbleKit JS phone companion, which relays
  AppMessage requests from the watch to the bridge over HTTP and streams responses back.
- **`docs/SETUP.md`** - full setup instructions, architecture notes, and known limitations
  (this couldn't be tested end-to-end against a live Joplin Server or real hardware).

See [`docs/SETUP.md`](docs/SETUP.md) to get started.

## Running the bridge with Docker

A prebuilt image is published to Docker Hub: [`webstas/pebblejoplinbridge`](https://hub.docker.com/r/webstas/pebblejoplinbridge).

```sh
docker pull webstas/pebblejoplinbridge:latest
# or pin a version:
docker pull webstas/pebblejoplinbridge:0.1.0

docker run -d \
  --name pebble-joplin-bridge \
  -p 8077:8077 \
  -e BRIDGE_TOKEN=changeme \
  -v joplin-profile:/data/joplin-profile \
  webstas/pebblejoplinbridge:latest
```

The Joplin CLI profile must already be configured to sync with your Joplin Server before
the bridge can serve notes - see [`docs/SETUP.md`](docs/SETUP.md).

### Environment variables

| Variable | Default | Description |
| --- | --- | --- |
| `BRIDGE_TOKEN` | *(required)* | Shared secret the Pebble phone companion sends as `Authorization: Bearer <token>`. Generate one with `node -e "console.log(require('crypto').randomBytes(24).toString('hex'))"`. |
| `PORT` | `8077` | Port the bridge HTTP server listens on. |
| `JOPLIN_PROFILE_DIR` | `/data/joplin-profile` (set in the image) | Directory of the Joplin CLI profile synced with your Joplin Server. Mount a volume here so it persists across container restarts. |
| `JOPLIN_BIN` | `joplin` | Path/command used to invoke the Joplin CLI. |
| `SYNC_INTERVAL_MINUTES` | `15` | How often the bridge runs `joplin sync` in the background. Set to `0` to disable and sync only via `POST /api/sync`. |
| `SYNC_TIMEOUT_MS` | `120000` | Timeout for a single `joplin sync` invocation, in milliseconds. |

Profile Sync Directory info can be found here https://joplinapp.org/help/dev/spec/user_profile/
