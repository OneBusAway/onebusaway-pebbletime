// Original Source: https://github.com/pebble-examples/ui-patterns

#ifndef PROGRESS_WINDOW_H
#define PROGRESS_WINDOW_H

#include <pebble.h>
#include "appdata.h"

#define PROGRESS_LAYER_WINDOW_DELTA 33
#define PROGRESS_LAYER_WINDOW_WIDTH 80

void ProgressWindowPush(AppData* appdata);
void ProgressWindowRemove();

#endif /* end of include guard: PROGRESS_WINDOW_H */
