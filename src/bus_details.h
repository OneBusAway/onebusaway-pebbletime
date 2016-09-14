#ifndef BUS_DETAILS_H
#define BUS_DETAILS_H

#include "buses.h"
#include "arrivals.h"
#include "appdata.h"

#ifndef PBL_PLATFORM_APLITE

#define NUM_LAYERS 2

typedef enum {
  ScrollDirectionDown,
  ScrollDirectionUp,
} ScrollDirection;

void BusDetailsWindowUpdate(AppData* appdata);
#endif

void BusDetailsWindowPush(const Bus, const Arrival*, AppData* appdata);
void BusDetailsWindowRemove();


#endif //BUS_DETAILS_H