#pragma once

// AppMessage transport to the phone-side PebbleKit JS companion (src/pkjs/index.js),
// which in turn talks to the bridge service. Every request carries a monotonically
// increasing reqId; responses whose reqId doesn't match the most recently sent request
// are dropped, so a screen switch never renders a stale in-flight response.

typedef void (*CommItemCallback)(const char *id, const char *title);
typedef void (*CommListEndCallback)(int count);
typedef void (*CommContentChunkCallback)(const char *title, const char *chunk, int chunk_index,
                                          int chunk_count);
typedef void (*CommContentEndCallback)(void);
typedef void (*CommErrorCallback)(const char *message);

typedef struct {
  CommItemCallback on_item;
  CommListEndCallback on_list_end;
  CommContentChunkCallback on_content_chunk;
  CommContentEndCallback on_content_end;
  CommErrorCallback on_error;
} CommCallbacks;

void comm_init(void);
void comm_set_callbacks(CommCallbacks callbacks);

void comm_request_notebooks(void);
void comm_request_notes(const char *notebook_id);
void comm_request_note(const char *note_id);
