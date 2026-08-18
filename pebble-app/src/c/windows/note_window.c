#include "note_window.h"
#include "../comm.h"
#include <pebble.h>

static Window *s_window;
static ScrollLayer *s_scroll_layer;
static TextLayer *s_text_layer;
static TextLayer *s_status_layer;
static char s_note_id[64];
static char s_title[64];

static char *s_content_buffer;
static size_t s_content_len;
static size_t s_content_capacity;

static void reset_content_buffer(void) {
  free(s_content_buffer);
  s_content_buffer = malloc(1);
  s_content_buffer[0] = '\0';
  s_content_len = 0;
  s_content_capacity = 1;
}

static void append_content(const char *chunk) {
  size_t chunk_len = strlen(chunk);
  size_t needed = s_content_len + chunk_len + 1;
  if (needed > s_content_capacity) {
    size_t new_capacity = s_content_capacity;
    while (new_capacity < needed) {
      new_capacity *= 2;
    }
    char *resized = realloc(s_content_buffer, new_capacity);
    if (!resized) return;
    s_content_buffer = resized;
    s_content_capacity = new_capacity;
  }
  memcpy(s_content_buffer + s_content_len, chunk, chunk_len + 1);
  s_content_len += chunk_len;
}

static void set_status(const char *text) {
  text_layer_set_text(s_status_layer, text);
  layer_set_hidden(text_layer_get_layer(s_status_layer), false);
  layer_set_hidden(scroll_layer_get_layer(s_scroll_layer), true);
}

static void show_content(void) {
  layer_set_hidden(text_layer_get_layer(s_status_layer), true);
  layer_set_hidden(scroll_layer_get_layer(s_scroll_layer), false);

  text_layer_set_text(s_text_layer, s_content_buffer);

  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  GSize content_size = text_layer_get_content_size(s_text_layer);
  content_size.h += 8;
  text_layer_set_size(s_text_layer, GSize(bounds.size.w, content_size.h));
  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, content_size.h));
}

static void on_content_chunk(const char *title, const char *chunk, int chunk_index,
                              int chunk_count) {
  if (chunk_index == 0) {
    reset_content_buffer();
    if (title) {
      strncpy(s_title, title, sizeof(s_title) - 1);
      s_title[sizeof(s_title) - 1] = '\0';
    }
  }
  append_content(chunk);
}

static void on_content_end(void) {
  if (s_content_len == 0) {
    set_status("(empty note)");
  } else {
    show_content();
  }
}

static void on_error(const char *message) {
  set_status(message);
}

static void request_note(void) {
  char loading_text[96];
  snprintf(loading_text, sizeof(loading_text), "Loading %s...", s_title);
  set_status(loading_text);

  CommCallbacks callbacks = {
    .on_item = NULL,
    .on_list_end = NULL,
    .on_content_chunk = on_content_chunk,
    .on_content_end = on_content_end,
    .on_error = on_error,
  };
  comm_set_callbacks(callbacks);
  comm_request_note(s_note_id);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorWhite);

  s_status_layer = text_layer_create(bounds);
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

  s_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
  layer_set_hidden(scroll_layer_get_layer(s_scroll_layer), true);

  s_text_layer = text_layer_create(GRect(0, 0, bounds.size.w, 4000));
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));

  request_note();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  scroll_layer_destroy(s_scroll_layer);
  text_layer_destroy(s_status_layer);
  free(s_content_buffer);
  s_content_buffer = NULL;
  window_destroy(s_window);
  s_window = NULL;
}

void note_window_push(const char *note_id, const char *title_hint) {
  strncpy(s_note_id, note_id, sizeof(s_note_id) - 1);
  s_note_id[sizeof(s_note_id) - 1] = '\0';
  strncpy(s_title, title_hint ? title_hint : "Note", sizeof(s_title) - 1);
  s_title[sizeof(s_title) - 1] = '\0';

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
