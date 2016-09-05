#ifndef APPDATA_H
#define APPDATA_H

#include "buses.h"
#include "arrivals.h"

typedef struct {
  Buses buses;
  Arrivals* arrivals;
  bool show_settings;
  bool initialized;
} AppData;

#endif //APPDATA_H