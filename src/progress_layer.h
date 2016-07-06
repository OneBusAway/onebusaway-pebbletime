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