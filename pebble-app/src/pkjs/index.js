// PebbleKit JS companion. Runs inside the Pebble mobile app on the phone (not on the
// watch), where it has real network access. Relays AppMessage requests from the watch to
// the bridge service's HTTP API and streams the JSON responses back over Bluetooth.
//
// Keep REQ_/RESP_ constants in sync with pebble-app/src/c/protocol.h.

var REQ_LIST_NOTEBOOKS = 0;
var REQ_LIST_NOTES = 1;
var REQ_GET_NOTE = 2;

var RESP_NOTEBOOK_ITEM = 10;
var RESP_NOTEBOOK_LIST_END = 11;
var RESP_NOTE_ITEM = 12;
var RESP_NOTE_LIST_END = 13;
var RESP_NOTE_CONTENT_CHUNK = 14;
var RESP_NOTE_CONTENT_END = 15;
var RESP_ERROR = 99;

// Must comfortably fit alongside the other keys in a single AppMessage dictionary within
// the watch's 2048-byte inbox buffer (see comm.c INBOX_SIZE).
var CHUNK_SIZE = 400;
var XHR_TIMEOUT_MS = 10000;

var sendQueue = [];
var sending = false;

function getSettings() {
  return {
    bridgeUrl: (localStorage.getItem('bridgeUrl') || '').replace(/\/+$/, ''),
    bridgeToken: localStorage.getItem('bridgeToken') || '',
  };
}

function enqueueMessage(dict) {
  sendQueue.push(dict);
  processQueue();
}

function processQueue() {
  if (sending || sendQueue.length === 0) return;
  sending = true;
  var dict = sendQueue.shift();
  Pebble.sendAppMessage(
    dict,
    function () {
      sending = false;
      processQueue();
    },
    function (e) {
      console.log('sendAppMessage failed: ' + JSON.stringify(e));
      sending = false;
      processQueue();
    }
  );
}

function sendError(reqId, message) {
  enqueueMessage({
    resp: RESP_ERROR,
    reqId: reqId,
    errorMessage: String(message).substring(0, 100),
  });
}

function apiGet(path, callback) {
  var settings = getSettings();
  if (!settings.bridgeUrl) {
    callback('Set the bridge URL in app settings');
    return;
  }

  var xhr = new XMLHttpRequest();
  xhr.timeout = XHR_TIMEOUT_MS;
  xhr.onload = function () {
    if (xhr.status >= 200 && xhr.status < 300) {
      try {
        callback(null, JSON.parse(xhr.responseText));
      } catch (e) {
        callback('Bad response from bridge');
      }
    } else if (xhr.status === 401) {
      callback('Bridge rejected token');
    } else {
      callback('Bridge error: ' + xhr.status);
    }
  };
  xhr.onerror = function () {
    callback('Could not reach bridge');
  };
  xhr.ontimeout = function () {
    callback('Bridge timed out');
  };

  xhr.open('GET', settings.bridgeUrl + path, true);
  xhr.setRequestHeader('Authorization', 'Bearer ' + settings.bridgeToken);
  xhr.send();
}

function chunkString(str, size) {
  var chunks = [];
  for (var i = 0; i < str.length; i += size) {
    chunks.push(str.substring(i, i + size));
  }
  return chunks;
}

function fetchNotebooks(reqId) {
  apiGet('/api/notebooks', function (err, body) {
    if (err) return sendError(reqId, err);
    var notebooks = body.notebooks || [];
    notebooks.forEach(function (nb) {
      enqueueMessage({
        resp: RESP_NOTEBOOK_ITEM,
        reqId: reqId,
        itemId: nb.id,
        itemTitle: nb.title || '(untitled)',
      });
    });
    enqueueMessage({ resp: RESP_NOTEBOOK_LIST_END, reqId: reqId, itemCount: notebooks.length });
  });
}

function fetchNotes(reqId, notebookId) {
  apiGet('/api/notebooks/' + encodeURIComponent(notebookId) + '/notes', function (err, body) {
    if (err) return sendError(reqId, err);
    var notes = body.notes || [];
    notes.forEach(function (note) {
      enqueueMessage({
        resp: RESP_NOTE_ITEM,
        reqId: reqId,
        itemId: note.id,
        itemTitle: note.title || '(untitled)',
      });
    });
    enqueueMessage({ resp: RESP_NOTE_LIST_END, reqId: reqId, itemCount: notes.length });
  });
}

function fetchNote(reqId, noteId) {
  apiGet('/api/notes/' + encodeURIComponent(noteId), function (err, body) {
    if (err) return sendError(reqId, err);
    var title = body.title || '(untitled)';
    var chunks = chunkString(body.body || '', CHUNK_SIZE);
    if (chunks.length === 0) chunks = [''];

    chunks.forEach(function (chunk, idx) {
      var msg = {
        resp: RESP_NOTE_CONTENT_CHUNK,
        reqId: reqId,
        chunkText: chunk,
        chunkIndex: idx,
        chunkCount: chunks.length,
      };
      if (idx === 0) msg.itemTitle = title;
      enqueueMessage(msg);
    });
    enqueueMessage({ resp: RESP_NOTE_CONTENT_END, reqId: reqId, chunkCount: chunks.length });
  });
}

Pebble.addEventListener('ready', function () {
  console.log('Pebble Joplin companion ready');
});

Pebble.addEventListener('appmessage', function (e) {
  var payload = e.payload;
  switch (payload.req) {
    case REQ_LIST_NOTEBOOKS:
      fetchNotebooks(payload.reqId);
      break;
    case REQ_LIST_NOTES:
      fetchNotes(payload.reqId, payload.notebookId);
      break;
    case REQ_GET_NOTE:
      fetchNote(payload.reqId, payload.noteId);
      break;
    default:
      console.log('Unknown request type: ' + payload.req);
  }
});

// Clay (https://github.com/pebble/clay) generates and hosts the config webview locally as
// a data URI, which is the officially maintained path for local config pages - the
// hand-rolled data:text/html approach this used to use isn't part of the documented/
// supported API and failed to open at all on the CoreDevices Pebble app.
var Clay = require('pebble-clay');
var clayConfig = require('./config.json');

// autoHandleEvents is off because Clay defaults to sending saved values to the watch as an
// AppMessage; bridgeUrl/bridgeToken are only ever used here in pkjs, so they're persisted to
// localStorage directly instead.
function buildClayConfig() {
  var settings = getSettings();
  var config = JSON.parse(JSON.stringify(clayConfig));
  config.forEach(function (item) {
    if (item.messageKey === 'bridgeUrl') item.defaultValue = settings.bridgeUrl;
    if (item.messageKey === 'bridgeToken') item.defaultValue = settings.bridgeToken;
  });
  return config;
}

Pebble.addEventListener('showConfiguration', function () {
  var clay = new Clay(buildClayConfig(), null, { autoHandleEvents: false });
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e.response) return;
  try {
    var clay = new Clay(clayConfig, null, { autoHandleEvents: false });
    var settings = clay.getSettings(e.response, false);
    if (settings.bridgeUrl) localStorage.setItem('bridgeUrl', settings.bridgeUrl.value || '');
    if (settings.bridgeToken) localStorage.setItem('bridgeToken', settings.bridgeToken.value || '');
  } catch (ex) {
    console.log('Failed to parse configuration response: ' + ex);
  }
});
