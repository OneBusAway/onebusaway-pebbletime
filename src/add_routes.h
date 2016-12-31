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

#ifndef ADD_ROUTES_H
#define ADD_ROUTES_H

#include <pebble.h>
#include "buses.h"

#define MENU_CELL_HEIGHT_BUS 60

void AddRoutesUpdate(Routes, Buses*);
void AddRoutesInit(Stop, Buses*);

#endif /* end of include guard: ADD_ROUTES_H */
