const Database = require('better-sqlite3');
const config = require('./config');

/**
 * Opens a fresh read-only connection per call instead of keeping one long-lived handle.
 * `joplin sync` (a separate process) rewrites database.sqlite periodically, and this
 * keeps the bridge from ever holding a stale handle or fighting the CLI for a write lock.
 */
function withDb(fn) {
  const db = new Database(config.dbPath, { readonly: true, fileMustExist: true });
  try {
    return fn(db);
  } finally {
    db.close();
  }
}

function listNotebooks() {
  return withDb((db) =>
    db
      .prepare(
        `SELECT id, title, parent_id AS parentId
         FROM folders
         ORDER BY title COLLATE NOCASE ASC`
      )
      .all()
  );
}

function listNotesInNotebook(notebookId) {
  return withDb((db) =>
    db
      .prepare(
        `SELECT id, title, updated_time AS updatedTime
         FROM notes
         WHERE parent_id = ? AND is_conflict = 0
         ORDER BY updated_time DESC`
      )
      .all(notebookId)
  );
}

function getNote(noteId) {
  return withDb((db) =>
    db
      .prepare(
        `SELECT id, parent_id AS parentId, title, body, updated_time AS updatedTime
         FROM notes
         WHERE id = ? AND is_conflict = 0`
      )
      .get(noteId)
  );
}

module.exports = { listNotebooks, listNotesInNotebook, getNote };
