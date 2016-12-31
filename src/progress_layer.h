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

#ifndef PROGRESS_LAYER_H
#define PROGRESS_LAYER_H

#include <pebble.h>

typedef Layer ProgressLayer;

ProgressLayer* ProgressLayerCreate(GRect frame);
void ProgressLayerDestroy(ProgressLayer* progress_layer);
void ProgressLayerIncrementProgress(ProgressLayer* progress_layer, int16_t progress);
void ProgressLayerSetProgress(ProgressLayer* progress_layer, int16_t progressPercent);
void ProgressLayerSetCornerRadius(ProgressLayer* progress_layer, uint16_t corner_radius);
void ProgressLayerSetForegroundColor(ProgressLayer* progress_layer, GColor color);
void ProgressLayerSetBackgroundColor(ProgressLayer* progress_layer, GColor color);

#endif //PROGRESS_LAYER_H