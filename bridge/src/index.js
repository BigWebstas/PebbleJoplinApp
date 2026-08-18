const express = require('express');
const config = require('./config');
const { requireToken } = require('./auth');
const { runSync } = require('./joplinCli');
const notebooksRouter = require('./routes/notebooks');
const notesRouter = require('./routes/notes');
const syncRouter = require('./routes/sync');

const app = express();
app.use(express.json());

// Unauthenticated liveness probe only - no note data.
app.get('/healthz', (req, res) => res.json({ ok: true }));

app.use('/api', requireToken, notebooksRouter);
app.use('/api', requireToken, notesRouter);
app.use('/api', requireToken, syncRouter);

app.listen(config.port, () => {
  console.log(`pebble-joplin-bridge listening on :${config.port}`);
  console.log(`Joplin profile: ${config.profileDir}`);
});

if (config.syncIntervalMinutes > 0) {
  const intervalMs = config.syncIntervalMinutes * 60 * 1000;
  console.log(`Background sync every ${config.syncIntervalMinutes} minute(s)`);
  setInterval(() => {
    runSync().catch((err) => console.error('background sync error:', err));
  }, intervalMs);
}

// Sync once at startup so a freshly (re)started bridge isn't serving a stale cache.
runSync().catch((err) => console.error('initial sync error:', err));
