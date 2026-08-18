#include "comm.h"
#include "protocol.h"
#include <pebble.h>

#define INBOX_SIZE 2048
#define OUTBOX_SIZE 256

static CommCallbacks s_callbacks;
static int s_next_req_id = 1;
static int s_current_req_id = 0;

static void send_request(int req_type, const char *notebook_id, const char *note_id) {
  s_current_req_id = s_next_req_id++;

  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_begin failed: %d", (int)result);
    return;
  }

  dict_write_int32(iter, MESSAGE_KEY_req, req_type);
  dict_write_int32(iter, MESSAGE_KEY_reqId, s_current_req_id);
  if (notebook_id) {
    dict_write_cstring(iter, MESSAGE_KEY_notebookId, notebook_id);
  }
  if (note_id) {
    dict_write_cstring(iter, MESSAGE_KEY_noteId, note_id);
  }

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_send failed: %d", (int)result);
  }
}

static bool req_id_matches(DictionaryIterator *iter) {
  Tuple *req_id_tuple = dict_find(iter, MESSAGE_KEY_reqId);
  if (!req_id_tuple) return false;
  return req_id_tuple->value->int32 == s_current_req_id;
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *resp_tuple = dict_find(iter, MESSAGE_KEY_resp);
  if (!resp_tuple) return;

  // Errors and stale-response filtering: everything else is scoped to the request that's
  // currently on screen, so a slow LIST_NOTES reply after the user already opened a note
  // can't clobber what's being shown.
  int resp_type = (int)resp_tuple->value->int32;

  if (resp_type == RESP_ERROR) {
    Tuple *msg_tuple = dict_find(iter, MESSAGE_KEY_errorMessage);
    if (s_callbacks.on_error) {
      s_callbacks.on_error(msg_tuple ? msg_tuple->value->cstring : "Unknown error");
    }
    return;
  }

  if (!req_id_matches(iter)) {
    return;
  }

  switch (resp_type) {
    case RESP_NOTEBOOK_ITEM:
    case RESP_NOTE_ITEM: {
      Tuple *id_tuple = dict_find(iter, MESSAGE_KEY_itemId);
      Tuple *title_tuple = dict_find(iter, MESSAGE_KEY_itemTitle);
      if (s_callbacks.on_item && id_tuple && title_tuple) {
        s_callbacks.on_item(id_tuple->value->cstring, title_tuple->value->cstring);
      }
      break;
    }
    case RESP_NOTEBOOK_LIST_END:
    case RESP_NOTE_LIST_END: {
      Tuple *count_tuple = dict_find(iter, MESSAGE_KEY_itemCount);
      if (s_callbacks.on_list_end) {
        s_callbacks.on_list_end(count_tuple ? (int)count_tuple->value->int32 : 0);
      }
      break;
    }
    case RESP_NOTE_CONTENT_CHUNK: {
      Tuple *title_tuple = dict_find(iter, MESSAGE_KEY_itemTitle);
      Tuple *chunk_tuple = dict_find(iter, MESSAGE_KEY_chunkText);
      Tuple *index_tuple = dict_find(iter, MESSAGE_KEY_chunkIndex);
      Tuple *count_tuple = dict_find(iter, MESSAGE_KEY_chunkCount);
      if (s_callbacks.on_content_chunk && chunk_tuple) {
        s_callbacks.on_content_chunk(title_tuple ? title_tuple->value->cstring : NULL,
                                      chunk_tuple->value->cstring,
                                      index_tuple ? (int)index_tuple->value->int32 : 0,
                                      count_tuple ? (int)count_tuple->value->int32 : 1);
      }
      break;
    }
    case RESP_NOTE_CONTENT_END: {
      if (s_callbacks.on_content_end) {
        s_callbacks.on_content_end();
      }
      break;
    }
    default:
      APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown resp type: %d", resp_type);
      break;
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox dropped: %d", (int)reason);
  if (s_callbacks.on_error) {
    s_callbacks.on_error("Lost part of the reply. Try again.");
  }
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason,
                                   void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed: %d", (int)reason);
  if (s_callbacks.on_error) {
    s_callbacks.on_error("Couldn't reach the phone.");
  }
}

void comm_init(void) {
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(INBOX_SIZE, OUTBOX_SIZE);
}

void comm_set_callbacks(CommCallbacks callbacks) {
  s_callbacks = callbacks;
}

void comm_request_notebooks(void) {
  send_request(REQ_LIST_NOTEBOOKS, NULL, NULL);
}

void comm_request_notes(const char *notebook_id) {
  send_request(REQ_LIST_NOTES, notebook_id, NULL);
}

void comm_request_note(const char *note_id) {
  send_request(REQ_GET_NOTE, NULL, note_id);
}
