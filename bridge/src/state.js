// Small shared in-memory status, read by GET /api/status and used to dedupe concurrent syncs.
module.exports = {
  syncInProgress: false,
  syncPromise: null,
  lastSyncAt: null,
  lastSyncError: null,
};
