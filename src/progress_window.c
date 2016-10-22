// Original Source: https://github.com/pebble-examples/ui-patterns

#include "progress_window.h"
#include "main_window.h"
#include "progress_layer.h"

static Window *s_window;
static ProgressLayer *s_progress_layer;

static AppTimer *s_timer;
static int s_progress;

typedef struct ProgressContext {
  void (*exit_callback)();
} ProgressContext;

static void ProgressCallback(void *context);

static void NextTimer() {
  s_timer = app_timer_register(PROGRESS_LAYER_WINDOW_DELTA, 
                               ProgressCallback, 
                               NULL);
}

static void ProgressCallback(void *context) {
  s_progress += (s_progress < 100) ? 1 : -100;
  ProgressLayerSetProgress(s_progress_layer, s_progress);
  NextTimer();
}

static void WindowLoad(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  uint height = 6;
  GRect progress_bounds = GRect((bounds.size.w - PROGRESS_LAYER_WINDOW_WIDTH)/2,
                                bounds.size.h/2-height, 
                                PROGRESS_LAYER_WINDOW_WIDTH, 
                                height);
  s_progress_layer = ProgressLayerCreate(progress_bounds);
  ProgressLayerSetProgress(s_progress_layer, 0);
  ProgressLayerSetCornerRadius(s_progress_layer, 30);
  ProgressLayerSetForegroundColor(s_progress_layer, GColorWhite);
  ProgressLayerSetBackgroundColor(s_progress_layer, GColorBlack);
  layer_add_child(window_layer, s_progress_layer);

}

static void WindowUnload(Window *window) {
  ProgressLayerDestroy(s_progress_layer);

  window_destroy(window);
  s_window = NULL;
}

static void WindowAppear(Window *window) {
  s_progress = 0;
  NextTimer();
}

static void WindowDisappear(Window *window) {
  if(s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

static void BackSingleClickHandler(ClickRecognizerRef recognizer, void *context) {
  ProgressContext* progress_context = (ProgressContext*)context;
  if(progress_context != NULL) {
    progress_context->exit_callback();
  }
  ProgressWindowRemove();
}

static void ClickConfigHandler(Window *window) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_BACK, BackSingleClickHandler);
}

void ProgressWindowPush(void (*exit_callback)()) {
  if(!s_window) {
    s_window = window_create();
    window_set_background_color(s_window, GColorLightGray);
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = WindowLoad,
      .appear = WindowAppear,
      .disappear = WindowDisappear,
      .unload = WindowUnload
    });
    ProgressContext* progress_context = malloc(sizeof(ProgressContext));
    progress_context->exit_callback = exit_callback;
    window_set_user_data(s_window, progress_context);
    window_set_click_config_provider_with_context(
        s_window, 
        (ClickConfigProvider) ClickConfigHandler,
        progress_context);
  }
  window_stack_push(s_window, true);
}

void ProgressWindowRemove() {
  if(s_window) {
    ProgressContext* progress_context = window_get_user_data(s_window);
    free(progress_context);
    window_stack_remove(s_window, true);
  }
} 
