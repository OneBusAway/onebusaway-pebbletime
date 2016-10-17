#include "settings_routes.h"
#include "utility.h"
#include "error_window.h"
#include "progress_window.h"
#include "communication.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Stop s_stop;
static Routes s_nearby_routes;

void BuildFavorites(Buses* buses) {
  for(uint i = 0; i < s_nearby_routes.count; i++) {
    bool favorite = GetBusIndex(
        s_stop.stop_id, 
        s_nearby_routes.data[i].route_id, buses) < 0 ? false : true;
    s_nearby_routes.data[i].favorite = favorite;
  }
}

static uint16_t MenuGetNumSectionsCallback(MenuLayer *menu_layer, 
                                           void *data) {
  return 1;
}

static uint16_t GetNumRowsCallback(MenuLayer *menu_layer, 
                                   uint16_t section_index, 
                                   void *context) {
  return s_nearby_routes.count;
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
  MenuCellDrawHeader(ctx, cell_layer, "Select favorites");
}

static void DrawRowCallback(GContext *ctx, 
                            const Layer *cell_layer, 
                            MenuIndex *cell_index, 
                            void *context) {
  if(cell_index->row <= s_nearby_routes.count) {
    if(s_nearby_routes.count == 0) {
      menu_cell_basic_draw(ctx, 
                           cell_layer, 
                           "Sorry", 
                           "No routes at this stop",
                           NULL);
    }
    else {
      uint32_t i = cell_index->row;

      // this used to be copy, but it crashed things no the watch hw
      // worked fine on the emulator - not sure why that didn't work?
      Route *r = &s_nearby_routes.data[i];

      const char* heart = "❤";
      // TODO - fix the size of name to a better value
      char name[30];
      if(r->favorite) {
        snprintf(name, 30, "%s %s", heart, r->route_name);
      }
      else {
        snprintf(name, 30, "%s", r->route_name);
      }
      MenuCellDraw(ctx ,cell_layer, name, r->description);
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Request for more routes than exist!");
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
  if(cell_index->row <= s_nearby_routes.count) {
    //TODO - is there a special case for 0 stops here? YES!
    Route *route = &s_nearby_routes.data[cell_index->row];
    int32_t bus_index = GetBusIndex(s_stop.stop_id, 
                                    route->route_id, 
                                    (Buses*)context);
    if(bus_index >= 0) {
      RemoveBus(bus_index, (Buses*)context);
      s_nearby_routes.data[cell_index->row].favorite = false;
    }
    else {
      bool result = AddBusFromStopRoute(&s_stop, route, (Buses*)context);
      s_nearby_routes.data[cell_index->row].favorite = result;
      if(!result) {
        ErrorWindowPush(
            "Can't save favorite\n\nMaximum number of favorite buses reached", 
            false);
      }
    }
    menu_layer_reload_data(s_menu_layer);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "SelectCallback: too many routes");
  }
}

static void WindowLoad(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  Buses* buses = window_get_user_data(window);

  s_menu_layer = menu_layer_create(bounds);
  if(s_menu_layer == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL MENU LAYER");
    ErrorWindowPush(
      "Critical error\n\nOut of memory\n\n0x100011", 
      true);
  }

#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_menu_layer, GColorBlue, GColorWhite);
#endif

  menu_layer_set_callbacks(s_menu_layer, buses, (MenuLayerCallbacks) {
    .get_num_sections = MenuGetNumSectionsCallback,
    .get_num_rows = GetNumRowsCallback,
    .get_header_height = GetHeaderHeightCallback,
    .draw_header = DrawHeaderCallback,
    .draw_row = DrawRowCallback,
    .get_cell_height = GetCellHeightCallback,
    .select_click = SelectCallback
  });

  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void WindowUnload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

void SettingsRoutesUpdate(Routes routes, Buses* buses) {
  s_nearby_routes = routes;
  BuildFavorites(buses);

  if(!s_window) {
    s_window = window_create();
  
    if(s_window == NULL) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "NULL WINDOW LAYER");
      ErrorWindowPush(
        "Critical error\n\nOut of memory\n\n0x100011", 
        true);
      return;
    }
    
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = WindowLoad,
      .unload = WindowUnload,
    });
    window_set_user_data(s_window, buses);  
    window_stack_push(s_window, true);  
  }
  else if(s_menu_layer) {
    window_set_user_data(s_window, buses);
    
    // refresh the menu
    layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
    menu_layer_reload_data(s_menu_layer);    
  }
  ProgressWindowRemove();
}

void SettingsRoutesInit(Stop stop, Buses* buses) {
  s_stop = stop;
  s_window = NULL;
  s_menu_layer = NULL;
  
  ProgressWindowPush();
  SendAppMessageGetRoutesForStop(&s_stop);
}

void SettingsRoutesDeinit() {
  window_destroy(s_window);
}
