/** 
 * Copyright 2016 Kevin Michael Woley (kmwoley@gmail.com)
 * All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#include <pebble.h>
#include "persistence.h"
#include "radius_window.h"
#include "utility.h"

#define MENU_CELL_HEIGHT 44

static Window *s_window;
static MenuLayer *s_menu_layer;
static NumberWindow *s_number_window;

static uint16_t GetNumSectionsCallback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t GetNumRowsCallback(MenuLayer *menu_layer, 
                                   uint16_t section_index, 
                                   void *context) {
  return 3;
}

static int16_t GetHeaderHeightCallback(MenuLayer *menu_layer, 
                                       uint16_t section_index, 
                                       void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void DrawHeaderCallback(GContext* ctx, 
                               const Layer *cell_layer, 
                               uint16_t section_index, 
                               void *data) {
  MenuCellDrawHeader(ctx, cell_layer, "Search radius");
}

static void DrawRowCallback(GContext *ctx, 
                            const Layer *cell_layer,
                            MenuIndex *cell_index,
                            void *context) {
  switch(cell_index->row) {
    char buffer[20];
    case 0:
      snprintf(buffer, 
               sizeof(buffer),
               "%u meters",
               PersistReadArrivalRadius());
      menu_cell_basic_draw(ctx, cell_layer, "Favorites nearby", buffer, NULL);
      break;
    case 1:
      snprintf(buffer, 
              sizeof(buffer),
              "%u meters",
              PersistReadSearchRadius());
      menu_cell_basic_draw(ctx, cell_layer, "Adding favorites", buffer, NULL);
      break;
    case 2:
      menu_cell_basic_draw(ctx, cell_layer, "Reset", "Restore default values", NULL);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Menu error - too many options");
      break;  
  }
}

static int16_t GetCellHeightCallback(struct MenuLayer *menu_layer, 
                                     MenuIndex *cell_index,
                                     void *context) {
  return PBL_IF_ROUND_ELSE(
      menu_layer_is_index_selected(menu_layer, cell_index) ?
          MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : 
          MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT,
      MENU_CELL_HEIGHT);
}

static void NumberWindowSelectCallback(NumberWindow *number_window, 
                                       void *context) {
  int32_t value = number_window_get_value(number_window);
  switch(((MenuIndex*)context)->row) {
    case 0:
      PersistWriteArrivalRadius(value);
      break;
    case 1:
      PersistWriteSearchRadius(value);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Menu error - too many options");
      break;
  }
  window_stack_remove(number_window_get_window(s_number_window), true);
}

static void SelectCallback(struct MenuLayer *menu_layer, 
                           MenuIndex *cell_index, 
                           void *context) {
  if(cell_index->row == 2) {
    // reset to defaults
    PersistWriteArrivalRadius(DEFAULT_ARRIVAL_RADIUS);
    PersistWriteSearchRadius(DEFAULT_SEARCH_RADIUS);
    menu_layer_reload_data(s_menu_layer);
    return;
  }

  uint current = 0;
  switch(cell_index->row) {
    case 0:
      current = PersistReadArrivalRadius();
      break;
    case 1:
      current = PersistReadSearchRadius();
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Menu error - too many options");
      break;
  }

  s_number_window = number_window_create(
    "Radius (meters)", 
    (NumberWindowCallbacks) {
      .incremented = NULL,
      .decremented = NULL,
      .selected = NumberWindowSelectCallback
    }, 
    (void*)cell_index);

  number_window_set_value(s_number_window, current);
  number_window_set_min(s_number_window, 100);
  number_window_set_max(s_number_window, 2500);
  number_window_set_step_size(s_number_window, 100);

  window_stack_push(number_window_get_window(s_number_window), true);
}

static void WindowAppear(Window* window) {
  // refresh the menu
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  menu_layer_reload_data(s_menu_layer);

  if(s_number_window) {
    number_window_destroy(s_number_window);
    s_number_window = NULL;
  }
}

static void WindowLoad(Window* window) {
  s_number_window = NULL;
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  if(s_menu_layer == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL MENU LAYER");
  }

#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlue, GColorWhite);
#endif

  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = GetNumSectionsCallback,
    .get_num_rows = GetNumRowsCallback,
    .get_header_height = GetHeaderHeightCallback,
    .draw_header = DrawHeaderCallback,
    .draw_row = DrawRowCallback,
    .get_cell_height = GetCellHeightCallback,
    .select_click = SelectCallback
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void WindowUnload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void RadiusWindowInit() {
  s_menu_layer = NULL;
  s_window = window_create();
  if(s_window == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL WINDOW LAYER");
    return;
  }

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = WindowLoad,
    .unload = WindowUnload,
    .appear = WindowAppear
  });

  window_stack_push(s_window, true);
}
