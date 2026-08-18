const { execFile } = require('child_process');
const config = require('./config');
const state = require('./state');

/**
 * Runs `joplin sync` against the profile that was already configured (see docs/SETUP.md)
 * to point at the user's Joplin Server, including the E2EE master password. Reusing the
 * real Joplin CLI means we never re-implement Joplin's sync protocol or decryption
 * ourselves - the CLI's local database.sqlite ends up holding plaintext, decrypted rows
 * exactly like the desktop/mobile apps produce.
 */
function runSync() {
  if (state.syncInProgress) {
    return state.syncPromise;
  }

  state.syncInProgress = true;
  state.syncPromise = new Promise((resolve) => {
    execFile(
      config.joplinBin,
      ['--profile', config.profileDir, 'sync'],
      { timeout: config.syncTimeoutMs },
      (error, stdout, stderr) => {
        state.syncInProgress = false;
        state.lastSyncAt = new Date().toISOString();
        if (error) {
          state.lastSyncError = stderr?.trim() || error.message;
          console.error('[joplin sync] failed:', state.lastSyncError);
          resolve({ ok: false, error: state.lastSyncError });
        } else {
          state.lastSyncError = null;
          console.log('[joplin sync] completed');
          resolve({ ok: true, output: stdout?.trim() });
        }
      }
    );
  });

  return state.syncPromise;
}

module.exports = { runSync };
