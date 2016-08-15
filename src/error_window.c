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
static Layer *s_icon_layer;
static Layer *s_indicator_up_layer, *s_indicator_down_layer;
static ScrollLayer *s_scroll_layer;
static ContentIndicator *s_indicator;

static GBitmap *s_icon_bitmap;

static char* s_message = NULL;
bool s_critical_error = false;

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

static void WindowLoad(Window *window) {
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
      .background = GColorYellow
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
      .background = GColorYellow
    }
  };
  content_indicator_configure_direction(s_indicator, 
                                        ContentIndicatorDirectionDown, 
                                        &down_config);

  // icon layer
  uint32_t x_margin = DIALOG_MESSAGE_WINDOW_MARGIN;
  uint32_t y_margin = STATUS_BAR_LAYER_HEIGHT;
  s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WARNING);
  GRect bitmap_bounds = gbitmap_get_bounds(s_icon_bitmap);
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
  text_layer_set_text(s_text_layer, s_message);
  text_layer_set_background_color(s_text_layer, GColorClear);
  text_layer_set_text_alignment(s_text_layer,
      PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  text_layer_set_font(s_text_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));

  // add the scroll layer window
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));

  GSize max_size = text_layer_get_content_size(s_text_layer);
  //text_layer_set_size(s_text_layer, max_size);

  // TODO: there are boundary conditions for which this may not work. This was
  // tuned for some specific text strings and may require adjustment for others.
  scroll_layer_set_content_size(s_scroll_layer,
    PBL_IF_ROUND_ELSE(GSize(max_size.w, max_size.h + (4 * y_margin)),
                      GSize(max_size.w, max_size.h + (4 * y_margin))));

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

static void WindowUnload(Window *window) {
  layer_destroy(s_icon_layer);
  layer_destroy(s_indicator_up_layer);
  layer_destroy(s_indicator_down_layer);
  text_layer_destroy(s_text_layer);
  gbitmap_destroy(s_icon_bitmap);
  content_indicator_destroy(s_indicator);
  scroll_layer_destroy(s_scroll_layer);
  window_destroy(window);
  s_main_window = NULL;
  FreeAndClearPointer((void**)&s_message);
}

void ErrorWindowPush(const char* message, bool critical) {
  if(!s_main_window) {
    s_main_window = window_create();
    window_set_background_color(s_main_window, 
        PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = WindowLoad,
        .unload = WindowUnload,
    });
  }

  s_critical_error = critical;
  FreeAndClearPointer((void**)&s_message);
  int i = strlen(message);
  s_message = (char *)malloc(sizeof(char)*(i+1));
  StringCopy(s_message, message, i+1);

  window_stack_push(s_main_window, true);
}

void ErrorWindowRemove() {
  if(s_main_window) {
    window_stack_remove(s_main_window, true);
  }
}
