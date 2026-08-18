#include "item_list.h"
#include <pebble.h>

static char *pjl_strdup(const char *s) {
  if (!s) return NULL;
  size_t len = strlen(s) + 1;
  char *copy = malloc(len);
  if (copy) memcpy(copy, s, len);
  return copy;
}

void item_list_init(ItemList *list) {
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

void item_list_add(ItemList *list, const char *id, const char *title) {
  if (list->count >= list->capacity) {
    int new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    ListItem *resized = realloc(list->items, (size_t)new_capacity * sizeof(ListItem));
    if (!resized) return;
    list->items = resized;
    list->capacity = new_capacity;
  }
  list->items[list->count].id = pjl_strdup(id);
  list->items[list->count].title = pjl_strdup(title);
  list->count++;
}

void item_list_clear(ItemList *list) {
  for (int i = 0; i < list->count; i++) {
    free(list->items[i].id);
    free(list->items[i].title);
  }
  free(list->items);
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}
