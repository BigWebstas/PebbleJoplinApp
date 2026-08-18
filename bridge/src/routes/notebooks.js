const express = require('express');
const db = require('../db');

const router = express.Router();

// GET /api/notebooks -> flat list; the watch app builds any hierarchy client-side
// using parentId (root notebooks have parentId === '').
router.get('/notebooks', (req, res) => {
  try {
    res.json({ notebooks: db.listNotebooks() });
  } catch (err) {
    console.error('GET /api/notebooks failed:', err);
    res.status(500).json({ error: 'failed to read notebooks' });
  }
});

router.get('/notebooks/:id/notes', (req, res) => {
  try {
    res.json({ notes: db.listNotesInNotebook(req.params.id) });
  } catch (err) {
    console.error('GET /api/notebooks/:id/notes failed:', err);
    res.status(500).json({ error: 'failed to read notes' });
  }
});

module.exports = router;
