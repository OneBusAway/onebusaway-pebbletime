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
  bool is_arrival;
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
                const bool is_arrival,
                const Buses* buses,
                Arrivals* arrival);
Arrival ArrivalConstructor(const char* trip_id, 
                           const char* scheduled_arrival, 
                           const char* predicted_arrival, 
                           const char* delta_string, 
                           const int32_t delta, 
                           const uint8_t bus_index, 
                           const char arrival_code,
                           const bool is_arrival);
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
