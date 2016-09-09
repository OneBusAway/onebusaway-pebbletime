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
  // Upgrade persistence (as needed)
  PersistenceVersionControl();

  // Initialize app data
  appdata->initialized = false;
  appdata->show_settings = true;
  ArrivalsConstructor(&appdata->arrivals);
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
}

static void HandleDeinit(AppData* appdata) {
  BusesDestructor(&appdata->buses);
  ArrivalsDestructor(appdata->arrivals);
  FreeAndClearPointer((void**)&appdata->arrivals);
  CommunicationDeinit();
  MainWindowDeinit();
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
