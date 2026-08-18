#pragma once

// Small growable array of (id, title) pairs shared by the notebooks and notes windows.
// Owns copies of the strings it's given, so callers can free/reuse their source buffers.

typedef struct {
  char *id;
  char *title;
} ListItem;

typedef struct {
  ListItem *items;
  int count;
  int capacity;
} ItemList;

void item_list_init(ItemList *list);
void item_list_add(ItemList *list, const char *id, const char *title);
void item_list_clear(ItemList *list);
