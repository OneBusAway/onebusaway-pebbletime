#ifndef ARRIVALS_H
#define ARRIVALS_H

#include <pebble.h>
#include "buses.h"
#include "memlist.h"

typedef struct ArrivalsColor {
  GColor foreground;
  GColor background;
  GColor boarder;
} ArrivalColors;

typedef struct Arrival {
  char* trip_id;
  char* delta_string;
  char* predicted_arrival;
  char* scheduled_arrival;
  int32_t delta;
  uint8_t bus_index;
  char arrival_code;
} __attribute__((__packed__)) Arrival;

typedef MemList Arrivals;

// typedef struct Arrivals {
//   uint32_t count;
//   Arrival* data;
// } __attribute__((__packed__)) Arrivals;

void ListArrivals(const Arrivals* arrivals);
void AddArrival(const char* stop_id, 
                const char* route_id,
                const char* trip_id, 
                const char* scheduled_string,
                const char* predicted_string, 
                const char* arrival_string,
                const int32_t arrival_delta, 
                const char arrival_code, 
                const Buses* buses,
                Arrivals* arrival);
Arrival ArrivalConstructor(const char* trip_id, 
                           const char* scheduled_arrival, 
                           const char* predicted_arrival, 
                           const char* delta_string, 
                           const int32_t delta, 
                           const uint8_t bus_index, 
                           const char arrival_code);
Arrival ArrivalCopy(const Arrival*);
void ArrivalDestructor(Arrival*);
Arrivals* ArrivalsCopy(const Arrivals*);
void ArrivalsConstructor(Arrivals**);
void ArrivalsDestructor(Arrivals*);
ArrivalColors ArrivalColor(const Arrival);
const char* ArrivalText(const Arrival);
const char* ArrivalDepartedText(const Arrival);
const char* ArrivalPredicted(const Arrival);
const char* ArrivalScheduled(const Arrival);

#endif // ARRIVALS_H
