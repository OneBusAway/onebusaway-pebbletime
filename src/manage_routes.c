#include "manage_routes.h"
#include "utility.h"
#include "error_window.h"
#include "add_routes.h"
//#include "progress_window.h"
//#include "communication.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Stop s_stop;
static Routes s_routes;
static ActionMenuLevel *s_action_menu_root;

typedef struct {
  Route* route;
  AppData* appdata;
} ActionContext;

static void ActionMenuCallback(ActionMenu* action_menu, 
                               const ActionMenuItem* action, 
                               void* context) {

  ActionContext* action_context = (ActionContext*)context;

  uint action_selection = (uint)action_menu_item_get_action_data(action);

  if(action_selection == 0) {
    // Remove Bus
    int32_t bus_index = GetBusIndex(s_stop.stop_id, 
                                    action_context->route->route_id, 
                                    &action_context->appdata->buses);
    RemoveBus(bus_index, &action_context->appdata->buses);
  }
  else if(action_selection == 1) {
    SettingsRoutesInit(s_stop, &action_context->appdata->buses);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "bus_details: unknown action selected.");
  }
}

void ActionMenuDidClose(ActionMenu* action_menu, 
                        const ActionMenuItem* action, 
                        void* context) {
  free(context);
}

static void ShowActionMenu(AppData* appdata, Route* route) {
  s_action_menu_root = action_menu_level_create(2);
  
  const char* action_string = "Remove from favorites";
  
  action_menu_level_add_action(s_action_menu_root,
                               action_string, 
                               ActionMenuCallback, 
                               (void*)0);

  action_menu_level_add_action(s_action_menu_root,
                               "Add favorites from this stop", 
                               ActionMenuCallback, 
                               (void*)1);

  ActionContext* action_context = malloc(sizeof(ActionContext));
  action_context->route = route;
  action_context->appdata = appdata;
  
  ActionMenuConfig action_config = (ActionMenuConfig) {
    .root_level = s_action_menu_root,
    .context = action_context,
    .did_close = ActionMenuDidClose
  };
 
  action_menu_open(&action_config);
}

static uint16_t MenuGetNumSectionsCallback(MenuLayer *menu_layer, 
                                           void *data) {
  return 1;
}

static uint16_t GetNumRowsCallback(MenuLayer *menu_layer, 
                                   uint16_t section_index, 
                                   void *context) {
  return (s_routes.count > 0) ? s_routes.count : 1;
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
  MenuCellDrawHeader(ctx, cell_layer, "Favorites Routes");
}

static void DrawRowCallback(GContext *ctx, 
                            const Layer *cell_layer, 
                            MenuIndex *cell_index, 
                            void *context) {
  if(cell_index->row <= s_routes.count) {
    if(s_routes.count == 0) {
      menu_cell_basic_draw(ctx, 
                           cell_layer, 
                           "Sorry", 
                           "No routes at this stop",
                           NULL);
    }
    else {
      Route *r = &s_routes.data[cell_index->row];
      MenuCellDraw(ctx, cell_layer, r->route_name, r->description);
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
  if(s_routes.count == 0) {
    // Do nothing
    VibeMicroPulse();
  }
  else if(cell_index->row <= s_routes.count) {
    Route *route = &s_routes.data[cell_index->row];
    // menu_layer_reload_data(s_menu_layer);
    ShowActionMenu((AppData*)context, route);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "SelectCallback: too many routes");
  }
}

static void WindowAppear(Window *window) {
  AppData* appdata = window_get_user_data(window);

  RoutesDestructor(&s_routes);
  RoutesConstructor(&s_routes);
  CreateRoutesFromBuses(&appdata->buses, 
                        &s_stop, 
                        &s_routes);
  
  // refresh the menu
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  menu_layer_reload_data(s_menu_layer);    
}

static void WindowLoad(Window *window) {
  Buses* buses = window_get_user_data(window);

  RoutesConstructor(&s_routes);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

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
  RoutesDestructor(&s_routes);
  window_destroy(s_window);
  s_window = NULL;
  action_menu_hierarchy_destroy(s_action_menu_root, NULL, NULL);
  s_action_menu_root = NULL;
}

void ManageRoutesInit(Stop stop, AppData* appdata) {
  s_stop = stop;
  s_menu_layer = NULL;
  s_action_menu_root = NULL;
  
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
    .appear = WindowAppear
  });

  window_set_user_data(s_window, &appdata->buses);
  window_stack_push(s_window, true);
}

