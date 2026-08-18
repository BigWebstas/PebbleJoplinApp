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
