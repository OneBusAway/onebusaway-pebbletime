#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "appdata.h"
#include "arrivals.h"

#define NUM_MENU_SECTIONS 2
#define MENU_CELL_HEIGHT 44
#define MENU_CELL_HEIGHT_BUS 60

void MainWindowInit(AppData* appdata);
void MainWindowDeinit();
void MainWindowUpdateArrivals(Arrivals* new_arrivals, AppData* appdata);
void MainWindowRefreshData(AppData* appdata);
void MainWindowCancelSettingsLoad(AppData* appdata);

#endif //MAIN_WINDOW_H