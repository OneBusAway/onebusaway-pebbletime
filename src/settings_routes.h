#ifndef SETTINGS_ROUTES_H
#define SETTINGS_ROUTES_H

#include <pebble.h>
#include "buses.h"

#define MENU_CELL_HEIGHT_BUS 60

void SettingsRoutesUpdate(Routes, Buses*);
void SettingsRoutesInit(Stop, Buses*);
void SettingsRoutesDeinit();

#endif /* end of include guard: SETTINGS_ROUTES_H */
