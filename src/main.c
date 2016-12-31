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
 
#include <pebble.h>
#include "main.h"
#include "appdata.h"
#include "persistence.h"
#include "communication.h"
#include "main_window.h"
#include "error_window.h"
#include "utility.h"

static void BluetoothCallback(bool connected) {
  if(connected) {
    ErrorWindowRemove();
  }
  else {
    ErrorWindowPush(DIALOG_MESSAGE_BLUETOOTH_ERROR, true);
  }
}

static void HandleInit(AppData* appdata) {
  PersistenceInit();
  ErrorWindowInit();

  // Initialize app data
  appdata->initialized = false;
  appdata->refresh_arrivals = false;
  ArrivalsConstructor(&appdata->arrivals);
  ArrivalsConstructor(&appdata->next_arrivals);
  LoadBusesFromPersistence(&appdata->buses);

  // Initialize app message communication
  CommunicationInit(appdata);

  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = BluetoothCallback
  });
  BluetoothCallback(connection_service_peek_pebble_app_connection());

  // Initialize windows
  MainWindowInit(appdata);

  CheckHeapMemory();

#ifdef LOGGING_ENABLED
  light_enable(true);
#endif
}

static void HandleDeinit(AppData* appdata) {
  BusesDestructor(&appdata->buses);
  ArrivalsDestructor(appdata->arrivals);
  FreeAndClearPointer((void**)&appdata->arrivals);
  ArrivalsDestructor(appdata->next_arrivals);
  FreeAndClearPointer((void**)&appdata->next_arrivals);
  CommunicationDeinit();
  ErrorWindowDeinit();
}

void AppExit() {
  // pop all the windows, which triggers the app exit
  window_stack_pop_all(true);
}

int main(void) {
  AppData appdata;
  HandleInit(&appdata);
  app_event_loop();
  HandleDeinit(&appdata);
}
