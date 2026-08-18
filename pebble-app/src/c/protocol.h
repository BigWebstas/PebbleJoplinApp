#pragma once

// Shared with src/pkjs/index.js - keep both sides in sync if these change.

// Request types (watch -> phone), sent in the "req" key.
#define REQ_LIST_NOTEBOOKS 0
#define REQ_LIST_NOTES 1
#define REQ_GET_NOTE 2

// Response types (phone -> watch), sent in the "resp" key.
#define RESP_NOTEBOOK_ITEM 10
#define RESP_NOTEBOOK_LIST_END 11
#define RESP_NOTE_ITEM 12
#define RESP_NOTE_LIST_END 13
#define RESP_NOTE_CONTENT_CHUNK 14
#define RESP_NOTE_CONTENT_END 15
#define RESP_ERROR 99
