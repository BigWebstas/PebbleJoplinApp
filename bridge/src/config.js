require('dotenv').config();
const path = require('path');

function required(name, fallback) {
  const value = process.env[name] || fallback;
  if (!value) {
    throw new Error(`Missing required environment variable: ${name}`);
  }
  return value;
}

const profileDir = required('JOPLIN_PROFILE_DIR');

module.exports = {
  port: parseInt(process.env.PORT || '8077', 10),
  token: required('BRIDGE_TOKEN'),
  joplinBin: process.env.JOPLIN_BIN || 'joplin',
  profileDir,
  dbPath: path.join(profileDir, 'database.sqlite'),
  syncIntervalMinutes: parseInt(process.env.SYNC_INTERVAL_MINUTES || '15', 10),
  syncTimeoutMs: parseInt(process.env.SYNC_TIMEOUT_MS || '120000', 10),
};
