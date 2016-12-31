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
#include "about_window.h"
#include "utility.h"
#include "pebble_process_info.h"
extern const PebbleProcessInfo __pbl_app_info;

static Window *s_window;
static ScrollLayer *s_scroll_layer;
static TextLayer *s_text_layer;

// Lorum ipsum to have something to scroll
static char s_scroll_text[] = "OneBusAway for Pebble\n\nPlease send feedback to:\npebble@onebusaway.org\n\nVersion XX.XX";

static void WindowLoad(Window* window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);
  s_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  s_text_layer = text_layer_create(max_text_bounds);

  char *match = NULL;
  match = strstr (s_scroll_text, "XX.XX");
  if(match != NULL) {
    APP_LOG(APP_LOG_LEVEL_INFO,
            "app version %d.%d", 
            __pbl_app_info.process_version.major, 
            __pbl_app_info.process_version.minor );
    char version[5];
    snprintf(version,
             5,
             "%d.%d",
            __pbl_app_info.process_version.major, 
            __pbl_app_info.process_version.minor);
    strncpy(match, version, 5);
  }
  text_layer_set_text(s_text_layer, s_scroll_text);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));

  // Trim text layer and scroll content to fit text box
  GSize max_size = text_layer_get_content_size(s_text_layer);
  text_layer_set_size(s_text_layer, max_size);
  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, max_size.h + 4));

  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
  scroll_layer_set_paging(s_scroll_layer, true);
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
}

static void WindowUnload(Window *window) {
  text_layer_destroy(s_text_layer);
  scroll_layer_destroy(s_scroll_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void AboutWindowInit() {
  s_window = window_create();
  if(s_window == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL WINDOW LAYER");
    return;
  }

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = WindowLoad,
    .unload = WindowUnload
  });

  window_stack_push(s_window, true);
}