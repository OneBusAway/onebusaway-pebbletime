#include "manage_stops.h"
#include "add_routes.h"
#include "utility.h"
#include "buses.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Stops s_stops;

static uint16_t GetNumSectionsCallback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t GetNumRowsCallback(MenuLayer *menu_layer, 
                                   uint16_t section_index, 
                                   void *context) {
  return (s_stops.total_size > 0) ? s_stops.total_size : 1;
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
  MenuCellDrawHeader(ctx, cell_layer, "Favorite stops");
}

static void DrawRowCallback(GContext *ctx, 
                            const Layer *cell_layer,
                            MenuIndex *cell_index,
                            void *context) {
  if(cell_index->row <= s_stops.total_size) {
    if(s_stops.total_size == 0) {
      menu_cell_basic_draw(ctx, cell_layer, "Sorry", "No favorite stops", NULL);
    }
    else {
      int16_t index = cell_index->row;
      if ((index >= 0) && (index < MemListCount(s_stops.memlist))) {
        // TODO: arbitrary constant - consider removing
        char stopInfo[55];
        Stop* s = MemListGet(s_stops.memlist, index);
        if(strlen(s->direction) > 0) {
          snprintf(stopInfo, 
                  sizeof(stopInfo),
                  "(%s) %s",
                  s->direction,
                  s->stop_name);
        }
        else {
          snprintf(stopInfo, sizeof(stopInfo), "%s", s->stop_name);
        }
        
        MenuCellDraw(ctx, cell_layer, s->detail_string, stopInfo);
      }
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Request for more stops than exist!");
  }
}

static int16_t GetCellHeightCallback(struct MenuLayer *menu_layer, 
                                     MenuIndex *cell_index,
                                     void *context) {
  return PBL_IF_ROUND_ELSE(
      menu_layer_is_index_selected(menu_layer, cell_index) ?
          MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : 
          MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT,
      MENU_CELL_HEIGHT_BUS);
}

static void SelectCallback(struct MenuLayer *menu_layer, 
                           MenuIndex *cell_index, 
                           void *context) {
  if(cell_index->row <= s_stops.total_size) {
    if(s_stops.total_size != 0) {
      Stop *stop = MemListGet(s_stops.memlist, 
                              cell_index->row);
      AppData* appdata = (AppData*)context;
      AddRoutesInit(*stop, &appdata->buses);
    }
    else {
      // nudge - no action to take
      VibeMicroPulse();
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "SelectCallback: too many stops");
  }
}

static void WindowAppear(Window* window) {
  AppData* appdata = window_get_user_data(window);

  StopsDestructor(&s_stops);
  StopsConstructor(&s_stops);
  CreateStopsFromBuses(&appdata->buses, &s_stops);

  // refresh the menu
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  menu_layer_reload_data(s_menu_layer);    
}

static void WindowLoad(Window* window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  AppData* appdata = window_get_user_data(window);

  StopsConstructor(&s_stops);

  s_menu_layer = menu_layer_create(bounds);
  if(s_menu_layer == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL MENU LAYER");
  }

#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlue, GColorWhite);
#endif

  menu_layer_set_callbacks(s_menu_layer, appdata, (MenuLayerCallbacks) {
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
  StopsDestructor(&s_stops);
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void ManageStopsInit(AppData* appdata) {
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

  window_set_user_data(s_window, appdata);
  window_stack_push(s_window, true);
}
