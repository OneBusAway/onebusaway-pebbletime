#include <pebble.h>
#include "main_window.h"
#include "bus_details.h"
#include "progress_window.h"
#include "communication.h"
#include "utility.h"

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static bool s_loading;
static char* s_last_selected_trip_id;

// TODO: rename appdata.show_settings as more like 'needs refresh'
// for the bus and/or arival data - alternatively, create a callback to
// to the window to trigger the actual refresh on data change events.
 
void MainWindowCancelSettingsLoad(AppData* appdata) {
  appdata->show_settings = false;
}

void MainWindowRefreshData(AppData* appdata) {
  appdata->show_settings = true;
}

static void DoneLoading() {
  if(s_loading) {
    vibes_double_pulse();
  }
  s_loading = false;
}

static void UpdateLoadingFlag(AppData* appdata) {
  if(!appdata->initialized || (s_loading && (appdata->buses.count > 1 && 
     appdata->buses.filter_count > 0))) {
    s_loading = true;
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "update loading flag (true): %u %u %u", 
             (uint)appdata->initialized, 
             (uint)appdata->buses.count, 
             (uint)appdata->buses.filter_count);
  }
  else {
    s_loading = false;
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "update loading flag (false): %u %u %u", 
            (uint)appdata->initialized, 
            (uint)appdata->buses.count, 
            (uint)appdata->buses.filter_count);
  }
  
  // refresh the menu
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  menu_layer_reload_data(s_menu_layer);
}

void MainWindowUpdateArrivals(Arrivals* new_arrivals, AppData* appdata) {
  // try to keep the same arrival selected in the menu across refreshes
  MenuIndex m = menu_layer_get_selected_index(s_menu_layer);
  if(m.section == 0) {
    if(m.row < appdata->arrivals->count) {
      Arrival* arrival = MemListGet(appdata->arrivals, m.row);
      char* trip_id = arrival->trip_id;

      MenuIndex set = MenuIndex(0,0);
      for(uint i = 0; i < new_arrivals->count; i++) {
        Arrival* new_arrival = MemListGet(new_arrivals, i);
        if(strcmp(new_arrival->trip_id, trip_id) == 0) {
          set.row = i;
          break;
        }
      }
      menu_layer_set_selected_index(s_menu_layer, set, 
          MenuRowAlignCenter, false);
    }
  }

  // move the temp arrivals to the display arrivals
  ArrivalsDestructor(appdata->arrivals);
  appdata->arrivals = ArrivalsCopy(new_arrivals);

  // update the the bus detals window, if it's being shown
  BusDetailsWindowUpdate(appdata);
    
  // show the data, all arrivals are in
  DoneLoading();
    
  // refresh the menu ux
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  menu_layer_reload_data(s_menu_layer);
}

static uint16_t MenuGetNumSectionsCallback(MenuLayer *menu_layer,
                                           void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t GetNumRowsCallback(MenuLayer *menu_layer,
                                   uint16_t section_index,
                                   void *context) {

  AppData* appdata = context;
  switch (section_index) {
    // bus list
    case 0:
      return (!s_loading && appdata->arrivals->count > 0) ? 
              appdata->arrivals->count : 1;
      break;
    // settings
    case 1:
      return 1;
      break;
    default:
      return 0;
      break;
  }
}

static int16_t MenuGetHeaderHeightCallback(MenuLayer *menu_layer,
                                           uint16_t section_index,
                                           void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void MenuDrawHeaderCallback(GContext* ctx,
                                   const Layer *cell_layer,
                                   uint16_t section_index,
                                   void *data) {

  // Determine which section we're working with
  switch (section_index) {
    case 0:
      // Draw title text in the section header
      //menu_cell_basic_header_draw(ctx, cell_layer, "Routes nearby");
      MenuCellDrawHeader(ctx, cell_layer, "Favorites nearby");
      break;
    case 1:
      //menu_cell_basic_header_draw(ctx, cell_layer, "Settings");
      MenuCellDrawHeader(ctx, cell_layer, "Settings");
      break;
  }
}

static void DrawMenuCell(GContext* ctx, 
                         const Layer *cell_layer, 
                         const char* routeName,
                         const char* timeDelta,
                         const char* time,
                         const GColor color,
                         const char* stopInfo) {

  GRect bounds = layer_get_bounds(cell_layer);

  GFont medium_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  GFont super_tiny_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  
  uint y_offset = -4;
  uint padding = 5;
  
  // title_layer
  int time_width = 46;
  int edge_padding = PBL_IF_ROUND_ELSE(
      menu_cell_layer_is_highlighted(cell_layer) ? 12: 24, 2);
  int title_width = bounds.size.w - time_width - edge_padding*2;
  int title_height = PBL_IF_ROUND_ELSE(
      menu_cell_layer_is_highlighted(cell_layer) ? 
      bounds.size.h/2 : bounds.size.h, bounds.size.h/2);
  
  GRect title_bounds = GRect(edge_padding, y_offset, title_width, title_height);

  graphics_draw_text(ctx, 
                     routeName, 
                     medium_font, 
                     title_bounds, 
                     GTextOverflowModeTrailingEllipsis, 
                     GTextAlignmentLeft, 
                     NULL);

  // details
  int details_width = title_width - padding;
  int details_height = bounds.size.h - title_height;
  GRect details_bounds = GRect(edge_padding, 
                               title_height + y_offset, 
                               details_width, 
                               details_height);
  
  graphics_draw_text(ctx, 
                     stopInfo,
                     super_tiny_font,
                     details_bounds,
                     GTextOverflowModeTrailingEllipsis, 
                     GTextAlignmentLeft, 
                     NULL);
  
  // time
  int time_height = details_height;
  GRect time_bounds = GRect(bounds.size.w - time_width - edge_padding,
                            title_height + y_offset,
                            time_width, 
                            time_height);

  graphics_draw_text(ctx, 
                     time,
                     super_tiny_font,
                     time_bounds,
                     GTextOverflowModeTrailingEllipsis, 
                     GTextAlignmentCenter, 
                     NULL);
  
  // time delta
  int delta_height = title_height;
  GRect delta_bounds = GRect(bounds.size.w - time_width - edge_padding, 
                             y_offset, 
                             time_width, 
                             delta_height);
  GColor delta_background = color;
  GColor delta_foreground = GColorWhite;
  if(gcolor_equal(delta_background, GColorLightGray)) {
    delta_foreground = GColorBlack;
  }
  
  GRect highlight_bounds = delta_bounds;
  highlight_bounds.origin.y = 2;
  highlight_bounds.size.h -= 4;
  graphics_context_set_fill_color(ctx, delta_background);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, highlight_bounds, 3, GCornersAll);
  graphics_draw_round_rect(ctx, highlight_bounds, 3);

  graphics_context_set_text_color(ctx, delta_foreground);
  graphics_draw_text(ctx, 
                     timeDelta, 
                     medium_font, 
                     delta_bounds,
                     GTextOverflowModeTrailingEllipsis, 
                     GTextAlignmentCenter, 
                     NULL);
}

static void DrawRowCallback(GContext *ctx,
                            const Layer *cell_layer,
                            MenuIndex *cell_index,
                            void *context) {
        
  AppData* appdata = context;

  switch (cell_index->section) {
    // nearby buses menu
    case 0:
      // loading buses at launch
      if(s_loading) {
        menu_cell_basic_draw(ctx,
                             cell_layer, 
                             "Loading...", 
                             "Getting nearby buses.", 
                             NULL);
      }
      // not currently loading buses
      else {
        //if(cell_index->row <= appdata->buses.filter_count) {
        if(cell_index->row <= appdata->arrivals->count) {
          // no nearby routes
          if(appdata->arrivals->count == 0) {
            if(appdata->buses.count != 0 && appdata->buses.filter_count == 0) {
              menu_cell_basic_draw(ctx, 
                                   cell_layer, 
                                   "Add Favorites", 
                                    "No favorites nearby", 
                                    NULL);
            }
            else if(appdata->buses.count == 0) {
              menu_cell_basic_draw(ctx, 
                                   cell_layer, 
                                   "Add Favorites", 
                                   "No favorites saved", 
                                   NULL);
            }
            else {
              menu_cell_basic_draw(ctx, 
                                   cell_layer,
                                   "Add Favorites", 
                                   "No upcoming arrivals", 
                                   NULL);
            }
          }
          // display nearby route details
          else {
            Arrival* a = MemListGet(appdata->arrivals, cell_index->row);
            
            // TODO: does this need size checking?
            uint i = a->bus_index;

            // TODO: data out of the appdata->arrivals, because directly
            // referencing it was causing memory referencing issues -
            // could be a bug in appdata->arrivals handling/freeing
            char delta[10];
            snprintf(delta, sizeof(delta), "%s", a->delta_string);
              
            // truncate in place
            char* insert = strstr(delta, ":");
            if(insert != NULL) {
              *insert = '\0'; 
            }

            char time[10];
            if(a->arrival_code == 's') {
              snprintf(time, sizeof(time), "%s", a->scheduled_arrival);
            }
            else {
              snprintf(time, sizeof(time), "%s", a->predicted_arrival);
            }
            
            // TODO: arbitrary constant - consider removing 
            char stopInfo[55];
            if(strlen(appdata->buses.data[i].direction) > 0) {
              snprintf(stopInfo, 
                       sizeof(stopInfo), 
                       "(%s) %s",
                       appdata->buses.data[i].direction,
                       appdata->buses.data[i].stop_name);
            }
            else {
              snprintf(stopInfo, 
                       sizeof(stopInfo), 
                       "%s",
                       appdata->buses.data[i].stop_name);
            }

            DrawMenuCell(ctx, 
                         cell_layer,
                         appdata->buses.data[i].route_name,
                         delta,
                         time, 
                         ArrivalColor(*a), 
                         stopInfo);
          }
        }
        else {
          APP_LOG(APP_LOG_LEVEL_ERROR, "Request for more buses than exist!");
        }
      }
      break;

    // settings menu
    case 1:
      switch (cell_index->row) {
        case 0:
          menu_cell_basic_draw(ctx, cell_layer, "Add Favorites", NULL, NULL);
          break;
        default :
          APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown Settings Menu Option");
          break;
      }
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown section option");
      break;
  }
}

static int16_t GetCellHeightCallback(struct MenuLayer *menu_layer,
                                     MenuIndex *cell_index,
                                     void *context) {
        
  AppData* appdata = context;

  if(cell_index->section == 1 || appdata->arrivals->count == 0) {
    return MENU_CELL_HEIGHT;}
  else {
  #if defined(PBL_ROUND)
    return menu_layer_is_index_selected(menu_layer, cell_index) ? 
        MENU_CELL_HEIGHT_BUS : MENU_CELL_HEIGHT_BUS / 2; 
  #else 
    return MENU_CELL_HEIGHT_BUS;
  #endif
  }
}

static void SelectCallback(
    struct MenuLayer *menu_layer,
    MenuIndex *cell_index,
    void *context) {
        
  AppData* appdata = context;

  switch(cell_index->section) {
    // nearby buses menu
    case 0:
      // While loading at first launch, don't allow interaction on the routes
      if(!s_loading) {
        if(cell_index->row <= appdata->arrivals->count) {
          // special case: no nearby buses to show,
          // show Add Route shortcut instead.
          if(appdata->arrivals->count == 0) {
            // Start progress window
            appdata->show_settings = true;
            ProgressWindowPush(appdata);
            // Start process of getting the stops
            SendAppMessageGetNearbyStops();
          }
          else {
            // record the trip_id of the bus selected, to put the menu
            // cursor back in the right place when returning to this window
            Arrival* arrival = (Arrival*)MemListGet(appdata->arrivals, 
                cell_index->row);
            uint i = arrival->bus_index;
            FreeAndClearPointer((void**)&s_last_selected_trip_id);
            char* trip_id = arrival->trip_id;
            s_last_selected_trip_id = (char*)malloc(strlen(trip_id)+1);
            StringCopy(s_last_selected_trip_id, trip_id, strlen(trip_id)+1);

            // show the detail window for the bus selected
            BusDetailsWindowPush(appdata->buses.data[i], 
                                 arrival, 
                                 appdata);
          }
        }
        else {
          APP_LOG(APP_LOG_LEVEL_ERROR, "SelectCallback: too many buses");
        }
      }
      break;
    case 1:
      // settings menu
      switch (cell_index->row) {
        case 0:
          // Start progress window
          appdata->show_settings = true;
          ProgressWindowPush(appdata);
          // Start process of getting the stops
          SendAppMessageGetNearbyStops();
          break;
        default :
          APP_LOG(APP_LOG_LEVEL_ERROR, 
                  "SelectCallback: Unknown Settings Menu Option");
          break;
      }

      layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
      menu_layer_reload_data(s_menu_layer);

      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, 
              "SelectCallback: Unknown Settings Menu Option");
      break;
  }
}

static void WindowLoad(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  APP_LOG(APP_LOG_LEVEL_ERROR, 
              "bounds %u %u %u %u", bounds.origin.x, bounds.origin.y, bounds.size.h, bounds.size.w);
  s_menu_layer = menu_layer_create(bounds);
  if(s_menu_layer == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL MENU LAYER");
  }

  #if defined(PBL_COLOR)
    menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_menu_layer, GColorDarkGray, GColorWhite);
  #endif

  AppData* appdata = window_get_user_data(window);

  menu_layer_set_callbacks(s_menu_layer, appdata, (MenuLayerCallbacks) {
    .get_num_sections = MenuGetNumSectionsCallback,
    .get_num_rows = GetNumRowsCallback,
    .get_header_height = MenuGetHeaderHeightCallback,
    .draw_header = MenuDrawHeaderCallback,
    .draw_row = DrawRowCallback,
    .get_cell_height = GetCellHeightCallback,
    .select_click = SelectCallback,
  });
  

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void WindowUnload(Window *window) {
  menu_layer_destroy(s_menu_layer);
}

static void WindowAppear(Window *window) {
  AppData* appdata = window_get_user_data(window);

  if(appdata->show_settings) {
    // returning from settings where settings have changed, update
    // the contents of the menu
    
    appdata->show_settings = false;

    if(appdata->buses.count == 0) {
      // if there are no buses saved, don't act like you're loading buses
      s_loading = false;
    }
    else {
      s_loading = true;
    }

    // select the first item in the list when the window appears
    MenuIndex m = MenuIndex(0,0);
    menu_layer_set_selected_index(s_menu_layer, m, MenuRowAlignCenter, false);

    // get updated arrival times
    UpdateArrivals(appdata);

    // refresh the menu
    UpdateLoadingFlag(appdata);
  }
  else {
    // try to keep the same arrival selected when returning from
    // another window
    MenuIndex m = MenuIndex(0,0);
    if(s_last_selected_trip_id != NULL) {
      for(uint i = 0; i < appdata->arrivals->count; i++) {
        Arrival* arrival = (Arrival*)MemListGet(appdata->arrivals, i);
        if(strcmp(arrival->trip_id, s_last_selected_trip_id) == 0) {
          m.row = i;
          break;
        }
      }
    }
    menu_layer_set_selected_index(s_menu_layer, m, MenuRowAlignCenter, false);
  } 
  
  // set timer to update arrivals
  StartArrivalsUpdateTimer(appdata);
}

static void WindowDisappear(Window *window) {
  AppData* appdata = window_get_user_data(window);  
    
  // stop the timer if we're going to go change the routes
  if(appdata->show_settings) {
    StopArrivalsUpdateTimer();
  }
}

void MainWindowInit(AppData* appdata) {
  s_main_window = window_create();

  if(s_main_window == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL MAIN WINDOW");
  }

  if(appdata->buses.count == 0) {
    // if there are no buses saved, don't act like you're loading buses
    // at launch
    s_loading = false;
  }
  else {
    s_loading = true;
  }

  s_last_selected_trip_id = NULL;

  window_set_user_data(s_main_window, appdata);

  window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = WindowLoad,
      .unload = WindowUnload,
      .appear = WindowAppear,
      .disappear = WindowDisappear,
  });
  window_stack_push(s_main_window, true);
}

void MainWindowDeinit() {
  FreeAndClearPointer((void**)&s_last_selected_trip_id);
  window_destroy(s_main_window);
}