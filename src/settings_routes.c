#include "settings_routes.h"
#include "utility.h"
#include "error_window.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Stop s_stop;
static Routes s_nearby_routes;

typedef struct {
  uint32_t index;
  bool favorite;
} RouteInfo;

typedef struct {
  RouteInfo* data;
  uint32_t count;
} RouteIndex;

static RouteIndex s_route_index;

static void BuildRouteIndex(const Buses* buses) {
  s_route_index.count = 0;
  FreeAndClearPointer((void**)&s_route_index.data);

  // APP_LOG(APP_LOG_LEVEL_INFO, 
  //         "route index: %u routes",
  //         (uint)s_nearby_routes.count);
  for(uint32_t i = 0; i < s_nearby_routes.count; i++) {
    // APP_LOG(APP_LOG_LEVEL_INFO, 
    //         "  route: %s - %s", 
    //         s_nearby_routes.data[i].route_id,  
    //         s_nearby_routes.data[i].route_name);

    // all stops in the stop_id_list are ',' terminated
    int stop_size = strlen(s_stop.stop_id);  
    char* stop_id = (char*)malloc(stop_size+1);
    snprintf(stop_id, stop_size+1, "%s,", s_stop.stop_id);

    if(strstr(s_nearby_routes.data[i].stop_id_list, stop_id) != NULL) {
      RouteInfo* temp = (RouteInfo*)malloc(sizeof(RouteInfo) * 
                                           (s_route_index.count+1));
      if(temp == NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "NULL ROUTE INDEX");
      }
      if(s_route_index.data != NULL) {
        memcpy(temp, s_route_index.data, sizeof(RouteInfo)*s_route_index.count);
        free(s_route_index.data);
      }
      s_route_index.data = temp;
      s_route_index.data[s_route_index.count].index = i;
      s_route_index.data[s_route_index.count].favorite = GetBusIndex(
          s_stop.stop_id, 
          s_nearby_routes.data[i].route_id, buses) < 0 ? false : true;
      s_route_index.count+=1;
    }
    free(stop_id);
  }
  // APP_LOG(APP_LOG_LEVEL_INFO, 
  //         "route index: %u routes indexed", 
  //         (uint)s_route_index.count);
  // for(uint32_t i = 0; i < s_route_index.count; i++) {
  //   APP_LOG(APP_LOG_LEVEL_INFO, 
  //           "  index: %u %s", 
  //           (uint)s_route_index.data[i].index, 
  //           s_nearby_routes.data[s_route_index.data[i].index].route_name);
  // }
}

//
// MENUS
//

static uint16_t MenuGetNumSectionsCallback(MenuLayer *menu_layer, 
                                           void *data) {
  return 1;
}

static uint16_t GetNumRowsCallback(MenuLayer *menu_layer, 
                                   uint16_t section_index, 
                                   void *context) {
  return s_route_index.count;
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
  if(cell_index->row <= s_route_index.count) {
    if(s_route_index.count == 0) {
      menu_cell_basic_draw(ctx, 
                           cell_layer, 
                           "Sorry!", 
                           "No routes at this stop.",
                           NULL);
    }
    else {
      uint32_t i = s_route_index.data[cell_index->row].index;
      if(s_nearby_routes.data == NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "NULL NEARBY ROUTES!");
      }
      if(i >= s_nearby_routes.count) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "INDEXING BEYOND END OF NEARBY ROUTES!");
      }

      // this used to be copy, but it crashed things no the watch hw
      // worked fine on the emulator - not sure why that didn't work?
      Route *r = &s_nearby_routes.data[i];

      const char* heart = "❤";
      // TODO - fix the size of name to a better value
      char name[30];
      if(s_route_index.data[cell_index->row].favorite) {
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
  if(cell_index->row <= s_route_index.count) {
    //TODO - is there a special case for 0 stops here? YES!
    Route *route = 
        &s_nearby_routes.data[s_route_index.data[cell_index->row].index];
    int32_t bus_index = GetBusIndex(s_stop.stop_id, 
                                    route->route_id, 
                                    (Buses*)context);
    if(bus_index >= 0) {
      RemoveBus(bus_index, (Buses*)context);
      s_route_index.data[cell_index->row].favorite = false;
    }
    else {
      bool result = AddBusFromStopRoute(&s_stop, route, (Buses*)context);
      s_route_index.data[cell_index->row].favorite = result;
      if(!result) {
        ErrorWindowPush(
            "Can't save favorite.\n\nMaximum number of favorite buses reached.", 
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

void SettingsRoutesStart(Stop stop, Routes routes, Buses* buses) {
  s_stop = stop;
  s_nearby_routes = routes;
  BuildRouteIndex(buses);
  window_set_user_data(s_window, buses);  
  window_stack_push(s_window, true);
}

void SettingsRoutesInit() {
  s_route_index.data = NULL;
  s_route_index.count = 0;

  s_window = window_create();
  
  if(s_window == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL WINDOW LAYER");
  }
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = WindowLoad,
    .unload = WindowUnload,
  });
}

void SettingsRoutesDeinit() {
  FreeAndClearPointer((void**)&s_route_index.data);
  window_destroy(s_window);
}
