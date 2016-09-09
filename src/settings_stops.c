#include "settings_stops.h"
#include "utility.h"
#include "progress_window.h"
#include "communication.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Stops s_nearby_stops;
static Routes s_nearby_routes;

static uint16_t GetNumSectionsCallback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t GetNumRowsCallback(MenuLayer *menu_layer, 
                                   uint16_t section_index, 
                                   void *context) {
  return (s_nearby_stops.count > 0) ? s_nearby_stops.count : 1;
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
  MenuCellDrawHeader(ctx, cell_layer, "Stops nearby");
}

static void DrawRowCallback(GContext *ctx, 
                            const Layer *cell_layer,
                            MenuIndex *cell_index,
                            void *context) {
  if(cell_index->row <= s_nearby_stops.count) {
    if(s_nearby_stops.count == 0) {
      menu_cell_basic_draw(ctx, cell_layer, "Sorry!", "No stops nearby.", NULL);
    }
    else {
      // TODO: arbitrary constant - consider removing
      char stopInfo[55];
      Stop* s = MemListGet(s_nearby_stops.memlist, cell_index->row);
      if(strlen(s->direction) > 0) {
        snprintf(stopInfo, 
                 sizeof(stopInfo),
                 "(%s) %s",
                 s->direction,
                 s->stop_name);
      }
      else {
        snprintf(stopInfo, sizeof(stopInfo), "%s",
          s->stop_name);
      }
      
      MenuCellDraw(ctx, cell_layer, s->detail_string, stopInfo);
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
  if(cell_index->row <= s_nearby_stops.count) {
    if(s_nearby_stops.count != 0) {
      Stop stop = *(Stop*)MemListGet(s_nearby_stops.memlist, cell_index->row);
      SettingsRoutesInit(stop, (Buses*)context);
    }
    // no selection action when there are no stops found
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "SelectCallback: too many buses");
  }
}

static void WindowLoad(Window* window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  Buses* buses = window_get_user_data(window);

  s_menu_layer = menu_layer_create(bounds);
  if(s_menu_layer == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL MENU LAYER");
  }

#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlue, GColorWhite);
#endif

  menu_layer_set_callbacks(s_menu_layer, buses, (MenuLayerCallbacks) {
    .get_num_sections = GetNumSectionsCallback,
    .get_num_rows = GetNumRowsCallback,
    .get_header_height = GetHeaderHeightCallback,
    .draw_header = DrawHeaderCallback,
    .draw_row = DrawRowCallback,
    .get_cell_height = GetCellHeightCallback,
    .select_click = SelectCallback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  vibes_short_pulse();
}

static void WindowUnload(Window *window) {
  SettingsRoutesDeinit();
  menu_layer_destroy(s_menu_layer);
}

void SettingsStopsUpdate(Stops stops, Buses* buses) {
  s_nearby_stops = stops;

  if(s_window) {
    window_set_user_data(s_window, buses);
    window_stack_push(s_window, true);
    
    // refresh the menu
    // layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
    // menu_layer_reload_data(s_menu_layer);        
  }
  ProgressWindowRemove();
}

void SettingsStopsInit() {
  s_menu_layer = NULL;
  s_window = window_create();
  if(s_window == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL WINDOW LAYER");
    return;
  }

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = WindowLoad,
    .unload = WindowUnload,
  });

  ProgressWindowPush();

  // Start process of getting the stops
  SendAppMessageGetNearbyStops();
}

void SettingsStopsDeinit() {
  window_destroy(s_window);
  s_window = NULL;
}
