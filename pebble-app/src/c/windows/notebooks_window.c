#include "notebooks_window.h"
#include "notes_window.h"
#include "../comm.h"
#include "../item_list.h"
#include <pebble.h>

static Window *s_window;
static MenuLayer *s_menu_layer;
static TextLayer *s_status_layer;
static ItemList s_notebooks;
static bool s_loading;

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index,
                                       void *context) {
  return (uint16_t)s_notebooks.count;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index,
                               void *context) {
  ListItem *item = &s_notebooks.items[cell_index->row];
  menu_cell_basic_draw(ctx, cell_layer, item->title, NULL, NULL);
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  ListItem *item = &s_notebooks.items[cell_index->row];
  notes_window_push(item->id, item->title);
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
  item_list_add(&s_notebooks, id, title);
}

static void on_list_end(int count) {
  s_loading = false;
  if (s_notebooks.count == 0) {
    set_status("No notebooks found");
  } else {
    show_menu();
  }
}

static void on_error(const char *message) {
  s_loading = false;
  set_status(message);
}

static void request_notebooks(void) {
  item_list_clear(&s_notebooks);
  s_loading = true;
  set_status("Loading notebooks...");

  CommCallbacks callbacks = {
    .on_item = on_item,
    .on_list_end = on_list_end,
    .on_content_chunk = NULL,
    .on_content_end = NULL,
    .on_error = on_error,
  };
  comm_set_callbacks(callbacks);
  comm_request_notebooks();
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

  request_notebooks();
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_status_layer);
  item_list_clear(&s_notebooks);
  window_destroy(s_window);
  s_window = NULL;
}

void notebooks_window_push(void) {
  item_list_init(&s_notebooks);
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
