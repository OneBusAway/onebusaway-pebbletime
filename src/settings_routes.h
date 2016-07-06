#ifndef SETTINGS_ROUTES_H
#define SETTINGS_ROUTES_H

#include <pebble.h>
#include "buses.h"

#define MENU_CELL_HEIGHT_BUS 60

void SettingsRoutesStart(Stop, Routes, Buses* buses);
void SettingsRoutesInit();
void SettingsRoutesDeinit();

#endif /* end of include guard: SETTINGS_ROUTES_H */
