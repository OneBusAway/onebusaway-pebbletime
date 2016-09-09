#ifndef SETTINGS_STOPS_H
#define SETTINGS_STOPS_H

#include <pebble.h>
#include "main.h"
#include "buses.h"
#include "settings_routes.h"

void SettingsStopsUpdate(Stops stops, Buses* buses);
void SettingsStopsInit();
void SettingsStopsDeinit();

#endif /* end of include guard: SETTINGS_STOPS_H */
