# Setup

## Architecture

```
Joplin Server  <--official sync protocol, E2EE-->  Joplin CLI (headless profile)
                                                            |
                                                   reads decrypted database.sqlite
                                                            |
                                                     bridge/ (Node.js REST API)
                                                            |
                                                     HTTP over your LAN/VPN
                                                            |
                                            PebbleKit JS companion (runs in Pebble
                                            phone app, src/pkjs/index.js)
                                                            |
                                                    Bluetooth AppMessage
                                                            |
                                              Pebble watchapp (src/c, read-only
                                              notebook/note browser)
```

The watch itself never talks to the network - Pebble hardware has no Wi-Fi/cellular. The
phone's Pebble app relays AppMessages to `src/pkjs/index.js`, which makes the actual HTTP
calls to the bridge. The bridge is the only component that speaks to Joplin Server, and it
does so by shelling out to the real `joplin` CLI rather than reimplementing Joplin's sync
protocol or E2EE decryption.

**Why a bridge instead of talking to Joplin Server directly from the phone?** Joplin
Server's sync API stores items as encrypted, serialized blobs. Decrypting them correctly
requires Joplin's exact key-derivation and chunked-cipher format. The Joplin CLI already
implements that correctly and is maintained upstream - the bridge just reuses it instead of
re-deriving that crypto by hand in the Pebble/PebbleKit JS environment, which has no crypto
primitives available anyway.

## Prerequisites

- Node.js 18+ on the machine that will run the bridge (a home server/NAS/Raspberry Pi
  that can stay on and reach both your Joplin Server and your phone's network is ideal).
- The official Joplin CLI: `npm install -g joplin` (this installs the `joplin` command).
- [pebble-tool](https://github.com/pebble-dev/pebble-tool) (or the Core Devices PebbleOS
  SDK fork) to build and install the watchapp.
- A Joplin Server instance you can already sync other Joplin clients against, plus your
  sync username/password and your E2EE master password.

## Part A: Configure a dedicated Joplin CLI profile

Use a profile directory dedicated to the bridge (don't reuse your desktop's profile):

```sh
mkdir -p ~/joplin-bridge-profile
joplin --profile ~/joplin-bridge-profile config sync.target 9
joplin --profile ~/joplin-bridge-profile config sync.9.path https://your-joplin-server.example.com
joplin --profile ~/joplin-bridge-profile config sync.9.username you@example.com
joplin --profile ~/joplin-bridge-profile config sync.9.password yourpassword
```

> `sync.target 9` is Joplin Server (`10` is Joplin Cloud). Run
> `joplin --profile ~/joplin-bridge-profile sync --help` (or check
> `joplin help config` in the version you installed) to confirm this hasn't changed -
> these IDs come from Joplin's `SyncTargetRegistry` and this repo couldn't be validated
> against a live server while writing it.

Run a first sync **interactively**, once, from a terminal:

```sh
joplin --profile ~/joplin-bridge-profile sync
```

If your notes are end-to-end encrypted, the CLI will prompt for your E2EE master password
to decrypt the master key. On a headless Linux box without an OS keychain, Joplin CLI
typically falls back to caching the decrypted key material in that profile's
`settings.json` in plaintext so future automated syncs don't re-prompt - this is what lets
the bridge sync unattended, but it also means **anyone with filesystem read access to
`~/joplin-bridge-profile` can read your notes**. Lock down permissions on that directory
(`chmod 700`), and treat it like any other credentials store.

## Part B: Run the bridge

```sh
cd bridge
npm install
cp .env.example .env
# edit .env: set JOPLIN_PROFILE_DIR to the profile from Part A, and set BRIDGE_TOKEN
# to a random secret (see the comment in .env.example for how to generate one)
npm run sync-once   # sanity check: confirms the CLI/profile config works
npm start
```

The bridge listens on `PORT` (default `8077`) and exposes:

- `GET /api/notebooks`
- `GET /api/notebooks/:id/notes`
- `GET /api/notes/:id` (add `?raw=1` to skip the Markdown-stripping cleanup)
- `POST /api/sync` - trigger an immediate `joplin sync`
- `GET /api/status` - last sync time/error
- `GET /healthz` - unauthenticated liveness probe

All `/api/*` routes require `Authorization: Bearer <BRIDGE_TOKEN>`.

Run it as a persistent service (systemd unit, `pm2`, or the included `Dockerfile`) so it
survives reboots - see `bridge/Dockerfile` for a container build. If you use Docker, mount
a volume at `/data/joplin-profile` and run Part A's config against that mounted path first.

## Part C: Build and install the watchapp

```sh
cd pebble-app
pebble build
pebble install --phone <phone-ip-or-emulator>   # or --emulator basalt while testing
```

After installing, open the app once on the watch so it registers with the phone, then open
the app's settings from the Pebble phone app and fill in:

- **Bridge URL** - e.g. `http://192.168.1.20:8077` (must be reachable from your phone;
  use a Tailscale/VPN address if the bridge isn't on the same LAN as your phone).
- **Bridge Token** - must match `BRIDGE_TOKEN` in `bridge/.env`.

Reopen the watchapp; it requests the notebook list over AppMessage, which PebbleKit JS
forwards to the bridge.

## Known limitations / unverified assumptions

This was built without access to a running Joplin Server or physical Pebble hardware, so
the following are informed-but-unverified and are the first things to check if something
doesn't work:

- The `sync.target` id (`9`) for Joplin Server, and the exact `joplin config` key names.
- The `folders`/`notes` SQLite column names the bridge queries in `bridge/src/db.js`
  (`id`, `parent_id`, `title`, `body`, `updated_time`) - stable in recent Joplin versions,
  but worth checking with `sqlite3 database.sqlite '.schema notes'` against your profile.
- Exact E2EE-password-caching behavior (whether it prompts again after a restart) varies
  by Joplin CLI version and platform keychain availability.
- AppMessage buffer sizing (`INBOX_SIZE`/`OUTBOX_SIZE` in `comm.c`, `CHUNK_SIZE` in
  `pkjs/index.js`) is a conservative guess; if you see dropped messages on very long notes,
  lower `CHUNK_SIZE` first.
