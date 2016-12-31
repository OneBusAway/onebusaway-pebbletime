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
 
// Original Source: https://github.com/pebble-examples/ui-patterns

#include "progress_layer.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct {
  int16_t progress_percent;
  int16_t corner_radius;
  GColor foreground_color;
  GColor background_color;
} ProgressLayerData;

static int16_t ScaleProgressBarWidthPx(unsigned int progress_percent, 
                                       int16_t rect_width_px) {
  return ((progress_percent * (rect_width_px)) / 100);
}

static void ProgressLayerUpdateProc(ProgressLayer* progress_layer, 
                                    GContext* ctx) {
  ProgressLayerData *data = (ProgressLayerData *)layer_get_data(progress_layer);
  GRect bounds = layer_get_bounds(progress_layer);

  int16_t progress_bar_width_px = ScaleProgressBarWidthPx(
      data->progress_percent, bounds.size.w);
  GRect progress_bar = GRect(bounds.origin.x, 
                             bounds.origin.y, 
                             progress_bar_width_px, 
                             bounds.size.h);

  graphics_context_set_fill_color(ctx, 
                                  data->background_color);
  graphics_fill_rect(ctx, 
                     bounds, 
                     data->corner_radius, 
                     GCornersAll);

  graphics_context_set_fill_color(ctx, 
                                  data->foreground_color);
  graphics_fill_rect(ctx, 
                     progress_bar, 
                     data->corner_radius, 
                     GCornersAll);

#ifndef PBL_COLOR
  graphics_context_set_stroke_color(ctx, data->background_color);
  graphics_draw_rect(ctx, progress_bar);
#endif
}

ProgressLayer* ProgressLayerCreate(GRect frame) {
  ProgressLayer *progress_layer = layer_create_with_data(
      frame, sizeof(ProgressLayerData));
  layer_set_update_proc(progress_layer, ProgressLayerUpdateProc);
  layer_mark_dirty(progress_layer);

  ProgressLayerData *data = (ProgressLayerData *)layer_get_data(progress_layer);
  data->progress_percent = 0;
  data->corner_radius = 1;
  data->foreground_color = GColorBlack;
  data->background_color = GColorWhite; 

  return progress_layer;
}

void ProgressLayerDestroy(ProgressLayer* progress_layer) {
  if (progress_layer) {
    layer_destroy(progress_layer);
  }
}

void ProgressLayerIncrementProgress(ProgressLayer* progress_layer, 
                                    int16_t progress) {
  ProgressLayerData *data = (ProgressLayerData *)layer_get_data(progress_layer);
  data->progress_percent = MIN(100, data->progress_percent + progress);
  layer_mark_dirty(progress_layer);
}

void ProgressLayerSetProgress(ProgressLayer* progress_layer, 
                              int16_t progress_percent) {
  ProgressLayerData *data = (ProgressLayerData *)layer_get_data(progress_layer);
  data->progress_percent = MIN(100, progress_percent);
  layer_mark_dirty(progress_layer);
}

void ProgressLayerSetCornerRadius(ProgressLayer* progress_layer, 
                                  uint16_t corner_radius) {
  ProgressLayerData *data = (ProgressLayerData *)layer_get_data(progress_layer);
  data->corner_radius = corner_radius;
  layer_mark_dirty(progress_layer);
}

void ProgressLayerSetForegroundColor(ProgressLayer* progress_layer, 
                                     GColor color) {
  ProgressLayerData *data = (ProgressLayerData *)layer_get_data(progress_layer);
  data->foreground_color = color;
  layer_mark_dirty(progress_layer);
}

void ProgressLayerSetBackgroundColor(ProgressLayer* progress_layer, 
                                     GColor color) {
  ProgressLayerData *data = (ProgressLayerData *)layer_get_data(progress_layer);
  data->background_color = color;
  layer_mark_dirty(progress_layer);
}
