#ifndef APPDATA_H
#define APPDATA_H

#include "buses.h"
#include "arrivals.h"

typedef struct {
  Buses buses;
  Arrivals* arrivals;
  Arrivals* next_arrivals;
  bool refresh_arrivals;
  bool initialized;
} AppData;

#endif //APPDATA_H