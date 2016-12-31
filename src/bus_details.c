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
#include <pebble-math-sll/math-sll.h>
#include "arrivals.h"
#include "bus_details.h"
#include "add_routes.h"
#include "main_window.h"
#include "error_window.h"
#include "progress_window.h"
#include "utility.h"

typedef struct {
  Bus bus;
  Arrival arrival;
  struct {
    TextLayer *header;
    Layer *status_box;
    TextLayer *status;
    TextLayer *arrival_label;
    TextLayer *arrival;
    TextLayer *stop_details;
    TextLayer *predicted_label;
    TextLayer *predicted;
  } card_one;
  struct {
    TextLayer *scheduled_label;
    TextLayer *scheduled;
    TextLayer *direction_label; 
    TextLayer *direction;
    TextLayer *bus_details_label;
    TextLayer *bus_details;
  } card_two;
} BusDetailsContent;

BusDetailsContent s_content;

static Window *s_window;
static Layer* s_layers[NUM_LAYERS];
static Layer *s_indicator_up_layer, *s_indicator_down_layer;
static Layer* s_menu_circle_layer;
static ContentIndicator *s_indicator;
static int s_layer_index;
static GFont s_res_header_font;
static GFont s_res_gothic_18;
static GFont s_res_gothic_18_bold;

static ActionMenuLevel *s_action_menu_root;

static void ActionMenuCallback(ActionMenu* action_menu, 
                               const ActionMenuItem* action, 
                               void* context) {

  AppData* appdata = (AppData*)context;

  int item = (int)action_menu_item_get_action_data(action);

  if(item == 0) {
    int32_t bus_index = GetBusIndex(s_content.bus.stop_id, 
                                    s_content.bus.route_id, 
                                    &appdata->buses);
    RemoveBus(bus_index, &appdata->buses);
    MainWindowMarkForRefresh(appdata);
    BusDetailsWindowRemove();
  }
  else if(item == 1) {
    bool result = AddBus(&s_content.bus, &appdata->buses);
    if(!result) {
      ErrorWindowPush(
          "Can't save favorite\n\nMaximum number of favorite buses reached", 
          false);
    }
    MainWindowMarkForRefresh(appdata);
    BusDetailsWindowRemove();
  }
  else if(item == 2) {
    MainWindowMarkForRefresh(appdata);
    // kick off request for routes for the stops
    
    // TOOD: memory leak - use action menu .did_close to cleanup? 
    Stop stop = StopConstructor(0,
                                s_content.bus.stop_id, 
                                s_content.bus.stop_name, 
                                "", 
                                s_content.bus.lat,
                                s_content.bus.lon,
                                s_content.bus.direction);
    AddRoutesInit(stop, &appdata->buses);
    // TODO: where does Deinit get called?
    BusDetailsWindowRemove();
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "bus_details: unknown action selected.");
  }
}

static void ShowActionMenu(AppData* appdata) {
  s_action_menu_root = action_menu_level_create(2);
  
  uint32_t add_remove_action = 0;
  const char* action_string = "Remove from favorites";
  // determine if this bus is currently a favorite or not
  int32_t bus_index = GetBusIndex(s_content.bus.stop_id,
                                  s_content.bus.route_id, 
                                  &appdata->buses);
  if(bus_index < 0) {
    // bus doesn't exist, use add action
    add_remove_action = 1;
    action_string = "Add favorite";
  }
  
  action_menu_level_add_action(s_action_menu_root,
                               "Add favorites from this stop", 
                               ActionMenuCallback, 
                               (void*)2);
  action_menu_level_add_action(s_action_menu_root,
                               action_string, 
                               ActionMenuCallback, 
                               (void*)add_remove_action);
  
  ActionMenuConfig action_config = (ActionMenuConfig) {
    .root_level = s_action_menu_root,
    .context = appdata
  };
 
  action_menu_open(&action_config);
}

static void MenuCircleUpdateProc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  GRect bounds = layer_get_bounds(layer);
  GPoint p = GPoint(bounds.size.w+7, bounds.size.h/2);
  graphics_fill_circle(ctx, p, 12);
}

static void SelectSingleClickHandler(
    ClickRecognizerRef recognizer, 
    void *context) {
  ShowActionMenu((AppData*)context);
}

static void StatusBoxUpdateProc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  ArrivalColors colors = ArrivalColor(s_content.arrival);
  graphics_context_set_fill_color(ctx, colors.background);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, colors.boarder);
  graphics_fill_rect(ctx, bounds, 3, GCornersAll);
  graphics_draw_round_rect(ctx, bounds, 3);
}

#ifdef PBL_ROUND
static TextLayer* TextLayerCreateRound(const int r, const int y, const int h) {
  const int padding = 4;

  // calculate the sagitta of the arc
  int sagitta;
  if(y > r) {
    // bottom of the circle
    sagitta = (2*r-y)-h;
  }
  else {
    // top of the circle
    sagitta = y+h/4;
  }

  // calculate the width of the arc
  int width = sll2int(sllsqrt(int2sll(8*sagitta*r - 4*sagitta*sagitta))) - 
      padding*2;
  return text_layer_create(GRect(r-(width/2), y, width, h));
}
#endif

static void InitTextLayers() {
  // set text layer sizes & locations

  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
#ifdef PBL_RECT
  uint padding = 6;
  uint w = bounds.size.w - padding*2;
  uint label_width = 68;
  uint content_width = w - label_width;
  uint label_height = 22;
  s_content.card_one.header = text_layer_create(
      GRect(padding, 0, w, 45));
  s_content.card_one.stop_details = text_layer_create(
      GRect(padding, 45, w, 35));
  s_content.card_one.status_box =
      layer_create(GRect(padding, 81, w, 26));
  s_content.card_one.status = text_layer_create(
      GRect(padding, 81, w, 26));
  s_content.card_one.arrival_label = text_layer_create(
      GRect(padding, 110, label_width, label_height));
  s_content.card_one.arrival = text_layer_create(
      GRect(label_width, 110, content_width, label_height));
  s_content.card_one.predicted_label = text_layer_create(
      GRect(padding, 132, label_width, label_height));
  s_content.card_one.predicted = text_layer_create(
      GRect(label_width, 132, content_width, label_height));

  int offset = STATUS_BAR_LAYER_HEIGHT/2;
  s_content.card_two.scheduled_label = text_layer_create(
      GRect(padding, offset, label_width, label_height));
  s_content.card_two.scheduled = text_layer_create(
      GRect(label_width, offset, content_width, label_height));
  s_content.card_two.direction_label = text_layer_create(
      GRect(padding, label_height+offset, label_width, label_height));
  s_content.card_two.direction = text_layer_create(
      GRect(label_width, label_height+offset, content_width, label_height));
  s_content.card_two.bus_details_label = text_layer_create(
      GRect(padding, (label_height*2)+offset, w, label_height));
  s_content.card_two.bus_details = text_layer_create(
      GRect(padding, (label_height*3)+offset, w, 90)); 

#else
  int r = bounds.size.h/2;
  s_content.card_one.header = TextLayerCreateRound(r, 2, 45); 
  s_content.card_one.stop_details = TextLayerCreateRound(r, 45, 35);
  s_content.card_one.status_box =
      layer_create(GRect(6, 81, bounds.size.w - 12, 26));  
  s_content.card_one.status = text_layer_create(
      GRect(6, 81, bounds.size.w - 12, 26));
  s_content.card_one.arrival_label = TextLayerCreateRound(r, 110, 22);
  s_content.card_one.arrival = TextLayerCreateRound(r, 110, 22);
  text_layer_set_background_color(s_content.card_one.arrival, GColorClear);
  s_content.card_one.predicted_label = TextLayerCreateRound(r, 132, 22);
  s_content.card_one.predicted = TextLayerCreateRound(r, 132, 22);
  text_layer_set_background_color(s_content.card_one.predicted, GColorClear);

  int offset = STATUS_BAR_LAYER_HEIGHT/2+2;
  s_content.card_two.direction_label = TextLayerCreateRound(r, offset, 22);
  s_content.card_two.direction = TextLayerCreateRound(r, offset, 22);
  s_content.card_two.scheduled_label = TextLayerCreateRound(r, 22+offset, 22);
  s_content.card_two.scheduled = TextLayerCreateRound(r, 22+offset, 22);
  text_layer_set_background_color(s_content.card_two.scheduled, GColorClear);
  text_layer_set_background_color(s_content.card_two.direction, GColorClear);
  s_content.card_two.bus_details_label = TextLayerCreateRound(r, 44+offset, 22);
  s_content.card_two.bus_details = TextLayerCreateRound(r, 66+offset, 90); 
#endif

  // set text layer default fonts
  s_res_header_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  s_res_gothic_18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  text_layer_set_font(s_content.card_one.header, s_res_header_font);
  // TODO: replace the default font.
  //text_layer_set_font(s_content.card_one.stop_details, s_res_gothic_18_bold);
  text_layer_set_font(s_content.card_one.status, s_res_gothic_18_bold);
  text_layer_set_font(s_content.card_one.arrival_label, s_res_gothic_18);
  text_layer_set_font(s_content.card_one.arrival, s_res_gothic_18_bold);
  text_layer_set_font(s_content.card_one.predicted_label, s_res_gothic_18);
  text_layer_set_font(s_content.card_one.predicted, s_res_gothic_18_bold);
  text_layer_set_font(s_content.card_two.scheduled_label, s_res_gothic_18);
  text_layer_set_font(s_content.card_two.scheduled, s_res_gothic_18_bold);
  text_layer_set_font(s_content.card_two.direction_label, s_res_gothic_18);
  text_layer_set_font(s_content.card_two.direction, s_res_gothic_18_bold);
  text_layer_set_font(s_content.card_two.bus_details_label, s_res_gothic_18);
  text_layer_set_font(s_content.card_two.bus_details, s_res_gothic_18_bold);

  // set text alignment
#ifdef PBL_ROUND
  text_layer_set_text_alignment(s_content.card_one.header, 
                                GTextAlignmentCenter);
  text_layer_set_text_alignment(s_content.card_one.stop_details, 
                                GTextAlignmentCenter);
  text_layer_set_text_alignment(s_content.card_two.bus_details_label, 
                                GTextAlignmentCenter);
  text_layer_set_text_alignment(s_content.card_two.bus_details, 
                                GTextAlignmentCenter);
#endif
  text_layer_set_text_alignment(s_content.card_one.status, 
                                GTextAlignmentCenter);
  text_layer_set_text_alignment(s_content.card_one.arrival, 
                                GTextAlignmentRight);
  text_layer_set_text_alignment(s_content.card_one.predicted, 
                                GTextAlignmentRight);
  text_layer_set_text_alignment(s_content.card_two.scheduled, 
                                GTextAlignmentRight);
  text_layer_set_text_alignment(s_content.card_two.direction, 
                                GTextAlignmentRight);

  // set text overflow mode
  text_layer_set_overflow_mode(s_content.card_one.header, 
                               GTextOverflowModeTrailingEllipsis);  
}

static void SetTextStrings() {
  const char* header_text = s_content.bus.route_name;
  text_layer_set_text(s_content.card_one.header, header_text);
  text_layer_set_text(s_content.card_one.stop_details, 
                      s_content.bus.stop_name);
  text_layer_set_text(s_content.card_one.status, 
                      ArrivalText(s_content.arrival));
  text_layer_set_text(s_content.card_one.arrival_label, 
                      ArrivalDepartedText(s_content.arrival));
  text_layer_set_text(s_content.card_one.arrival, 
                      s_content.arrival.delta_string);
  text_layer_set_text(s_content.card_one.predicted_label, "Predicted:");
  text_layer_set_text(s_content.card_one.predicted, 
                      ArrivalPredicted(s_content.arrival));
  text_layer_set_text(s_content.card_two.scheduled_label, "Scheduled:");
  text_layer_set_text(s_content.card_two.scheduled, 
                      ArrivalScheduled(s_content.arrival));
  text_layer_set_text(s_content.card_two.direction_label, "Direction:");
  text_layer_set_text(s_content.card_two.direction, s_content.bus.direction);
  text_layer_set_text(s_content.card_two.bus_details_label, "Description:");
  text_layer_set_text(s_content.card_two.bus_details, 
                      s_content.bus.description);
 
  // see how big the text would be if it were "unbounded"
  GRect header_unlimited_bounds = layer_get_bounds(
      text_layer_get_layer(s_content.card_one.header));
  header_unlimited_bounds.size.h = 1000;
  GSize header_content_unbounded_size = graphics_text_layout_get_content_size(
      header_text,
      s_res_header_font, 
      header_unlimited_bounds,
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);

  // if the content is too large, pick a smaller font
  GRect header_frame = layer_get_frame(
      text_layer_get_layer(s_content.card_one.header));
  // GSize header_content_size = text_layer_get_content_size(
  //     s_content.card_one.header);
  if(header_frame.size.h < header_content_unbounded_size.h) {
    s_res_header_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
    text_layer_set_font(s_content.card_one.header, s_res_header_font);
    // header_content_size = text_layer_get_content_size(
    //    s_content.card_one.header);
  }
  else {
    text_layer_set_font(s_content.card_one.header, s_res_header_font);  
  }

  // center text within the header layer
  // uint y_offset = (header_frame.size.h - header_content_size.h - 16) / 2;
  // GRect header_size = GRect(header_frame.origin.x, 
  //     header_frame.origin.y + y_offset,
  //     header_frame.size.w, 
  //     header_frame.size.h - y_offset);
  // layer_set_frame(text_layer_get_layer(s_content.card_one.header), 
  //    header_size);
  
   // center text within the stop detalis layer
  // GRect stop_frame = layer_get_frame(
  //     text_layer_get_layer(s_content.card_one.stop_details));
  // GSize stop_content_size = text_layer_get_content_size(
  //    s_content.card_one.stop_details);
  // y_offset = (stop_frame.size.h - stop_content_size.h) / 2;
  // GRect stop_size = GRect(stop_frame.origin.x, 
  //    stop_frame.origin.y + y_offset,
  //    stop_frame.size.w, stop_frame.size.h - y_offset);
  // layer_set_frame(text_layer_get_layer(s_content.card_one.stop_details), 
  //    stop_size);
  
  // set color of the s_content.card_one.status
  ArrivalColors colors = ArrivalColor(s_content.arrival);
  text_layer_set_background_color(s_content.card_one.status, GColorClear);
  text_layer_set_text_color(s_content.card_one.status, colors.foreground);
}

static Layer* GetFirstCardLayer(const GRect bounds) {
  Layer* ret_layer = layer_create(bounds);
  
  // indicator layer
  s_indicator_down_layer = layer_create(GRect(
      0, 
      bounds.size.h - STATUS_BAR_LAYER_HEIGHT/2,
      bounds.size.w, 
      STATUS_BAR_LAYER_HEIGHT/2));
  const ContentIndicatorConfig down_config = (ContentIndicatorConfig) {
    .layer = s_indicator_down_layer,
    .times_out = false,
    .alignment = GAlignCenter,
    .colors = {
      .foreground = GColorBlack,
      .background = GColorWhite
    }
  };
  
  // content indicator
  content_indicator_configure_direction(s_indicator, 
                                        ContentIndicatorDirectionDown,
                                        &down_config);
  
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_one.header));    
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_one.stop_details));
  layer_set_update_proc(s_content.card_one.status_box, StatusBoxUpdateProc);
  layer_add_child(ret_layer,
                  s_content.card_one.status_box);
  layer_add_child(ret_layer,
                  text_layer_get_layer(s_content.card_one.status));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_one.arrival_label));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_one.arrival));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_one.predicted_label));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_one.predicted));
  layer_add_child(ret_layer, s_indicator_down_layer);
  
  return ret_layer;
}

static Layer* GetSecondCardLayer(const GRect bounds) {
  Layer* ret_layer = layer_create(bounds);

  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_two.scheduled_label));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_two.scheduled));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_two.direction_label));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_two.direction));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_two.bus_details_label));
  layer_add_child(ret_layer, 
                  text_layer_get_layer(s_content.card_two.bus_details));

  // indicator layer
  s_indicator_up_layer = layer_create(GRect(bounds.origin.x, 
                                            bounds.origin.y,
                                            bounds.size.w,
                                            STATUS_BAR_LAYER_HEIGHT/2));
  const ContentIndicatorConfig up_config = (ContentIndicatorConfig) {
    .layer = s_indicator_up_layer,
    .times_out = false,
    .alignment = GAlignCenter,
    .colors = {
      .foreground = GColorBlack,
      .background = GColorWhite
    }
  };
  content_indicator_configure_direction(s_indicator, 
                                        ContentIndicatorDirectionUp,
                                        &up_config);
  layer_add_child(ret_layer, s_indicator_up_layer);
  
  return ret_layer;
}

static void WindowLoad(Window *window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
   
  InitTextLayers();
  SetTextStrings();
  
  s_layer_index = 0;
  s_indicator = content_indicator_create();
  s_layers[0] = GetFirstCardLayer(bounds);
  s_layers[1] = GetSecondCardLayer(bounds);
  layer_set_hidden(s_layers[1], true);
  
  content_indicator_set_content_available(s_indicator, 
                                          ContentIndicatorDirectionUp,
                                          true);
  content_indicator_set_content_available(s_indicator, 
                                          ContentIndicatorDirectionDown, 
                                          true);
  layer_add_child(window_layer, s_layers[0]);
  layer_add_child(window_layer, s_layers[1]);
  
  s_menu_circle_layer = layer_create(bounds);
  layer_set_update_proc(s_menu_circle_layer, MenuCircleUpdateProc);
  layer_add_child(window_layer, s_menu_circle_layer);
}

static void WindowUnload(Window *window) {
  ArrivalDestructor(&s_content.arrival);
//   FreeAndClearPointer((void**)&s_content.arrival);
  
  text_layer_destroy(s_content.card_one.header);
  layer_destroy(s_content.card_one.status_box);
  text_layer_destroy(s_content.card_one.status);
  text_layer_destroy(s_content.card_one.predicted_label);
  text_layer_destroy(s_content.card_one.predicted);
  text_layer_destroy(s_content.card_one.arrival_label);
  text_layer_destroy(s_content.card_one.arrival);
  text_layer_destroy(s_content.card_one.stop_details);
  text_layer_destroy(s_content.card_two.scheduled_label);
  text_layer_destroy(s_content.card_two.scheduled);
  text_layer_destroy(s_content.card_two.direction_label);
  text_layer_destroy(s_content.card_two.direction);
  text_layer_destroy(s_content.card_two.bus_details_label);
  text_layer_destroy(s_content.card_two.bus_details);
  layer_destroy(s_indicator_up_layer);
  layer_destroy(s_indicator_down_layer);
  layer_destroy(s_layers[0]);
  layer_destroy(s_layers[1]);
  layer_destroy(s_menu_circle_layer);
  content_indicator_destroy(s_indicator);
  
  window_destroy(s_window);
  s_window = NULL;
  
  action_menu_hierarchy_destroy(s_action_menu_root, NULL, NULL);
  s_action_menu_root = NULL;
}

static void SwitchLayer(const int current, const int next) {
  layer_set_hidden(s_layers[current], true);
  layer_set_hidden(s_layers[next], false);
  s_layer_index = next;
}

static void SetLayerFromDelta(const int delta) {
  int next_index = s_layer_index + delta;
  if((0 <= next_index) && (next_index < NUM_LAYERS)) {
    SwitchLayer(s_layer_index, next_index);
  }
}

static void AskForScroll(ScrollDirection direction) {
  int delta = direction == ScrollDirectionUp ? -1 : +1;
  SetLayerFromDelta(delta);
}

static void UpClickHandler(ClickRecognizerRef recognizer, void* context) {
  AskForScroll(ScrollDirectionUp);
}

static void DownClickHandler(ClickRecognizerRef recognizer, void* context) {
  AskForScroll(ScrollDirectionDown);
}

static void WindowClickConfigProvider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, UpClickHandler);
  window_single_click_subscribe(BUTTON_ID_DOWN, DownClickHandler);
  window_single_click_subscribe(BUTTON_ID_SELECT, SelectSingleClickHandler);
}

void BusDetailsWindowPush(
    const Bus bus, 
    const Arrival* arrival,
    AppData* appdata) {

    s_action_menu_root = NULL;
    s_content.bus = bus;
    s_content.arrival = ArrivalCopy(arrival);

  if(!s_window) {
    s_window = window_create();
    window_set_background_color(s_window, GColorWhite);
    window_set_click_config_provider_with_context(s_window, 
                                                  WindowClickConfigProvider, 
                                                  appdata);
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = WindowLoad,
        .unload = WindowUnload,
    });
  }
  window_stack_push(s_window, true);
}

static const char* BusDetailsWindowGetTripId() {
  if(s_window) {
    return s_content.arrival.trip_id;
  }
  else {
    return NULL;
  }
}

static void BusDetailsWindowUpdateContent(const Bus bus,
                                          const Arrival* arrival) {
  if(s_window && (s_content.arrival.trip_id != NULL) && 
     (strcmp(s_content.arrival.trip_id, arrival->trip_id) == 0)) {
    s_content.bus = bus;
    ArrivalDestructor(&s_content.arrival);
    // FreeAndClearPointer((void**)&s_content.arrival);
    // s_content.arrival = malloc(sizeof(Arrival));
    s_content.arrival = ArrivalCopy(arrival);
    
    SetTextStrings();

    Layer* window_layer = window_get_root_layer(s_window);
    layer_mark_dirty(window_layer);
  }
}

void BusDetailsWindowUpdate(AppData* appdata) {
  const char* trip_id = BusDetailsWindowGetTripId();
  
  if(trip_id != NULL) {
    for(uint i = 0; i < appdata->arrivals->count; i++) {
      Arrival* arrival = (Arrival*)MemListGet(appdata->arrivals, i);
      if(strcmp(arrival->trip_id, trip_id) == 0) {
        uint32_t bus_index = arrival->bus_index;
        BusDetailsWindowUpdateContent(appdata->buses.data[bus_index], 
                                      arrival);
        break;
      }
    }
  }
}

void BusDetailsWindowRemove(void) {
  if(s_window) {
    window_stack_remove(s_window, true);
  }
  ArrivalDestructor(&s_content.arrival);
}

