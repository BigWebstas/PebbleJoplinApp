#include "notes_window.h"
#include "note_window.h"
#include "../comm.h"
#include "../item_list.h"
#include <pebble.h>

static Window *s_window;
static MenuLayer *s_menu_layer;
static TextLayer *s_status_layer;
static ItemList s_notes;
static char s_notebook_id[64];
static char s_loading_text[96];

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index,
                                       void *context) {
  return (uint16_t)s_notes.count;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                               void *context) {
  ListItem *item = &s_notes.items[cell_index->row];
  menu_cell_basic_draw(ctx, cell_layer, item->title, NULL, NULL);
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  ListItem *item = &s_notes.items[cell_index->row];
  note_window_push(item->id, item->title);
}

static void set_status(const char *text) {
  text_layer_set_text(s_status_layer, text);
  layer_set_hidden(text_layer_get_layer(s_status_layer), false);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), true);
}

static void show_menu(void) {
  layer_set_hidden(text_layer_get_layer(s_status_layer), true);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), false);
  menu_layer_reload_data(s_menu_layer);
}

static void on_item(const char *id, const char *title) {
  item_list_add(&s_notes, id, title);
}

static void on_list_end(int count) {
  if (s_notes.count == 0) {
    set_status("No notes in this notebook");
  } else {
    show_menu();
  }
}

static void on_error(const char *message) {
  set_status(message);
}

static void request_notes(void) {
  item_list_clear(&s_notes);
  set_status(s_loading_text);

  CommCallbacks callbacks = {
    .on_item = on_item,
    .on_list_end = on_list_end,
    .on_content_chunk = NULL,
    .on_content_end = NULL,
    .on_error = on_error,
  };
  comm_set_callbacks(callbacks);
  comm_request_notes(s_notebook_id);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_status_layer = text_layer_create(bounds);
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows_callback,
    .draw_row = draw_row_callback,
    .select_click = select_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), true);

  request_notes();
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_status_layer);
  item_list_clear(&s_notes);
  window_destroy(s_window);
  s_window = NULL;
}

void notes_window_push(const char *notebook_id, const char *notebook_title) {
  strncpy(s_notebook_id, notebook_id, sizeof(s_notebook_id) - 1);
  s_notebook_id[sizeof(s_notebook_id) - 1] = '\0';
  snprintf(s_loading_text, sizeof(s_loading_text), "Loading %s...", notebook_title);

  item_list_init(&s_notes);
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
