const express = require('express');
const db = require('../db');
const { stripMarkdown } = require('../markdown');

const router = express.Router();

router.get('/notes/:id', (req, res) => {
  try {
    const note = db.getNote(req.params.id);
    if (!note) {
      return res.status(404).json({ error: 'note not found' });
    }
    const raw = req.query.raw === '1';
    res.json({
      id: note.id,
      parentId: note.parentId,
      title: note.title,
      updatedTime: note.updatedTime,
      body: raw ? note.body : stripMarkdown(note.body),
    });
  } catch (err) {
    console.error('GET /api/notes/:id failed:', err);
    res.status(500).json({ error: 'failed to read note' });
  }
});

module.exports = router;
