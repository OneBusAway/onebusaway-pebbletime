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
 
//
// Adapted from example implementation of the dialog message UI pattern
// from the Pebble UI-Patterns 
// (https://github.com/pebble-examples/ui-patterns)
//

#include "error_window.h"
#include "main.h"
#include "utility.h"

static Window *s_main_window;
static TextLayer *s_text_layer;
static Layer *s_indicator_up_layer, *s_indicator_down_layer;
static ScrollLayer *s_scroll_layer;
static ContentIndicator *s_indicator;


static char* s_message = NULL;
bool s_critical_error = false;
static Layer *s_icon_layer;
static GBitmap *s_icon_bitmap;

static void IconUpdateProc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect bitmap_bounds = gbitmap_get_bounds(s_icon_bitmap);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, 
                               s_icon_bitmap, 
                               (GRect) {
                                 .origin = bounds.origin, 
                                 .size = bitmap_bounds.size
                               });
}

static void BackSingleClickHandler(ClickRecognizerRef recognizer, 
                                   void *context) {
  if(s_critical_error) {
    AppExit();
  }
  else {
    ErrorWindowRemove();
  }
}

static void ClickConfigHandler(Window *window) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_BACK, BackSingleClickHandler);
}

static void WindowInit(Window *window) {
  window_set_background_color(window, 
    PBL_IF_COLOR_ELSE(GColorYellow, GColorLightGray));

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // scrollable layer
  s_scroll_layer = scroll_layer_create(bounds);

  // content indicators
  s_indicator = scroll_layer_get_content_indicator(s_scroll_layer);

  // Create two Layers to draw the arrows
  s_indicator_up_layer = layer_create(GRect(bounds.origin.x, 
                                            bounds.origin.y, 
                                            bounds.size.w, 
                                            STATUS_BAR_LAYER_HEIGHT/2));
  s_indicator_down_layer = layer_create(GRect(
      0, 
      bounds.size.h - STATUS_BAR_LAYER_HEIGHT/2,
      bounds.size.w, 
      STATUS_BAR_LAYER_HEIGHT/2));
  layer_add_child(window_layer, s_indicator_up_layer);
  layer_add_child(window_layer, s_indicator_down_layer);

  // Configure the properties of each indicator
  const ContentIndicatorConfig up_config = (ContentIndicatorConfig) {
    .layer = s_indicator_up_layer,
    .times_out = false,
    .alignment = GAlignCenter,
    .colors = {
      .foreground = GColorBlack,
      .background = PBL_IF_COLOR_ELSE(GColorYellow, GColorLightGray)
    }
  };
  content_indicator_configure_direction(s_indicator, 
                                        ContentIndicatorDirectionUp, 
                                        &up_config);

  const ContentIndicatorConfig down_config = (ContentIndicatorConfig) {
    .layer = s_indicator_down_layer,
    .times_out = false,
    .alignment = GAlignCenter,
    .colors = {
      .foreground = GColorBlack,
      .background = PBL_IF_COLOR_ELSE(GColorYellow, GColorLightGray)
    }
  };
  content_indicator_configure_direction(s_indicator, 
                                        ContentIndicatorDirectionDown, 
                                        &down_config);

  uint32_t x_margin = DIALOG_MESSAGE_WINDOW_MARGIN;
  uint32_t y_margin = STATUS_BAR_LAYER_HEIGHT;
  GRect bitmap_bounds = GRect(0,0,0,0);

  // icon layer
  s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WARNING);
  bitmap_bounds = gbitmap_get_bounds(s_icon_bitmap);
  s_icon_layer = layer_create(PBL_IF_ROUND_ELSE(
      GRect((bounds.size.w - bitmap_bounds.size.w) / 2,
            y_margin, 
            bitmap_bounds.size.w, 
            bitmap_bounds.size.h),
      GRect(x_margin, 
            y_margin,
            bitmap_bounds.size.w, 
            bitmap_bounds.size.h)
  ));
  layer_set_update_proc(s_icon_layer, IconUpdateProc);
  scroll_layer_add_child(s_scroll_layer, s_icon_layer);

  // text layer
  GRect max_text_bounds = PBL_IF_ROUND_ELSE(
      GRect(bounds.origin.x, 
            y_margin + bitmap_bounds.size.h,
            bounds.size.w, 
            2000),
      GRect(x_margin, 
            y_margin + bitmap_bounds.size.h,
            bounds.size.w - (2 * x_margin), 
            2000)
  );
  s_text_layer = text_layer_create(max_text_bounds);
  text_layer_set_background_color(s_text_layer, GColorClear);
  text_layer_set_text_alignment(s_text_layer,
      PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  text_layer_set_font(s_text_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));

  // add the scroll layer window
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));

  // set the scroll layer's paging, properties
  text_layer_enable_screen_text_flow_and_paging(s_text_layer, 6);
  scroll_layer_set_paging(s_scroll_layer, true);
  scroll_layer_set_shadow_hidden(s_scroll_layer, true);

  // set click config on the window
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  ScrollLayerCallbacks callbacks = {
    .click_config_provider = (ClickConfigProvider) ClickConfigHandler,
    .content_offset_changed_handler = NULL };
  scroll_layer_set_callbacks(s_scroll_layer, callbacks);

}

static void WindowLoad(Window *window) {
  text_layer_set_text(s_text_layer, s_message);
  layer_mark_dirty(text_layer_get_layer(s_text_layer));

  GSize max_size = text_layer_get_content_size(s_text_layer);
  //text_layer_set_size(s_text_layer, max_size);

  // TODO: there are boundary conditions for which this may not work. This was
  // tuned for some specific text strings and may require adjustment for others.
  uint32_t y_margin = STATUS_BAR_LAYER_HEIGHT;
  scroll_layer_set_content_size(s_scroll_layer,
    PBL_IF_ROUND_ELSE(GSize(max_size.w, max_size.h + (4 * y_margin)),
                      GSize(max_size.w, max_size.h + (4 * y_margin))));

  scroll_layer_set_content_offset(s_scroll_layer, GPointZero, false);
  layer_mark_dirty(scroll_layer_get_layer(s_scroll_layer));
}

static void WindowUnload(Window *window) {
  layer_destroy(s_icon_layer);
  gbitmap_destroy(s_icon_bitmap);
  layer_destroy(s_indicator_up_layer);
  layer_destroy(s_indicator_down_layer);
  text_layer_destroy(s_text_layer);
  content_indicator_destroy(s_indicator);
  scroll_layer_destroy(s_scroll_layer);
  window_destroy(window);
  s_main_window = NULL;
  FreeAndClearPointer((void**)&s_message);
}

void ErrorWindowInit() {
  // reserve memory to be able to display the error window
  // even in low memory conditions
  if(!s_main_window) {
    s_main_window = window_create();
    if(s_main_window == NULL) {
      APP_LOG(APP_LOG_LEVEL_ERROR, 
          "CAN'T ALLOCATE ERROR WINDOW MEMORY. GIVE UP. ALL HOPE IS LOST.");
    }
    WindowInit(s_main_window);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = WindowLoad,
    //     .unload = WindowUnload,
    });
  }
  s_message = malloc(sizeof(char)*MAX_ERROR_STRING_LENGTH);
  if(s_message == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, 
        "CAN'T ALLOCATE ERROR WINDOW MEMORY. GIVE UP. ALL HOPE IS LOST.");
  }
}

void ErrorWindowPush(const char* message, bool critical) {
  s_critical_error = critical;
  int i = MIN(strlen(message)+1, MAX_ERROR_STRING_LENGTH);
  StringCopy(s_message, message, i);
  window_stack_push(s_main_window, true);
}

void ErrorWindowRemove() {
  if(s_main_window) {
    window_stack_remove(s_main_window, true);
  }
}

void ErrorWindowDeinit() {
  if(s_main_window) {
    WindowUnload(s_main_window);
  }
}
