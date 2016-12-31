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
 
#ifndef ERROR_WINDOW_H
#define ERROR_WINDOW_H

#include <pebble.h>

#define DIALOG_MESSAGE_WINDOW_MARGIN 5
#define MAX_ERROR_STRING_LENGTH 256

void ErrorWindowInit();
void ErrorWindowDeinit();
void ErrorWindowPush(const char *, bool);
void ErrorWindowRemove();

#endif //ERROR_WINDOW_H