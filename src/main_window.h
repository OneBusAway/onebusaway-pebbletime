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
 
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "appdata.h"
#include "arrivals.h"

#define NUM_MENU_SECTIONS 2
#define MENU_CELL_HEIGHT 44
#define MENU_CELL_HEIGHT_BUS 60

void MainWindowInit(AppData* appdata);
void MainWindowUpdateArrivals(AppData* appdata);
void MainWindowMarkForRefresh(AppData* appdata);

#endif //MAIN_WINDOW_H