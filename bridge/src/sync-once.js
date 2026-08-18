// Convenience CLI entrypoint: `npm run sync-once` triggers a single sync and exits,
// useful for testing the JOPLIN_PROFILE_DIR / JOPLIN_BIN config before starting the server.
const { runSync } = require('./joplinCli');

runSync().then((result) => {
  console.log(JSON.stringify(result, null, 2));
  process.exit(result.ok ? 0 : 1);
});
