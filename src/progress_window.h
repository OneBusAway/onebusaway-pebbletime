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

#ifndef PROGRESS_WINDOW_H
#define PROGRESS_WINDOW_H

#include <pebble.h>
#include "appdata.h"

#define PROGRESS_LAYER_WINDOW_DELTA 33
#define PROGRESS_LAYER_WINDOW_WIDTH 80

void ProgressWindowPush(void (*exit_callback)());
void ProgressWindowRemove();

#endif /* end of include guard: PROGRESS_WINDOW_H */
