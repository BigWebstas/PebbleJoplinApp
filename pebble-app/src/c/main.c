#include "comm.h"
#include "windows/notebooks_window.h"
#include <pebble.h>

static void init(void) {
  comm_init();
  notebooks_window_push();
}

static void deinit(void) {
  // Windows free themselves via their own unload handlers as the stack unwinds.
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
