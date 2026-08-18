const express = require('express');
const { runSync } = require('../joplinCli');
const state = require('../state');

const router = express.Router();

router.post('/sync', async (req, res) => {
  const result = await runSync();
  res.status(result.ok ? 200 : 502).json(result);
});

router.get('/status', (req, res) => {
  res.json({
    syncInProgress: state.syncInProgress,
    lastSyncAt: state.lastSyncAt,
    lastSyncError: state.lastSyncError,
  });
});

module.exports = router;
