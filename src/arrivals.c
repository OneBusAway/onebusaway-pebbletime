#include <pebble.h>
#include "arrivals.h"
#include "utility.h"

void ListArrivals(const Arrivals* arrivals) {
#ifdef LOGGING_ENABLED
  APP_LOG(APP_LOG_LEVEL_INFO, "Number of arrivals:%u", (uint)arrivals->count);
  for(uint32_t i = 0; i < arrivals->count; i++)  {
    Arrival* a = (Arrival*)MemListGet(arrivals, i);
    
    APP_LOG(APP_LOG_LEVEL_INFO, 
            "%u - index:%u\tdelta:%s\tds:%i",
            (uint)i, 
            (uint)a->bus_index, 
            a->delta_string, 
            (int)a->delta);
  }
#endif
}

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
                Arrivals* arrivals) {

  int32_t index = GetBusIndex(stop_id, route_id, buses);

  if(index < 0) {
    return;
  }

  Arrival temp = ArrivalConstructor(trip_id, 
                                    scheduled_string, 
                                    predicted_string, 
                                    arrival_string, 
                                    arrival_delta, 
                                    index, 
                                    arrival_code,
                                    is_arrival);

  int16_t pos = -1;
  for(int16_t i = 0; i < MemListCount(arrivals); i++) {
    Arrival* arrival = (Arrival*)MemListGet(arrivals, i);
    if(arrival->delta > arrival_delta) {
      pos = i;
      break;
    }
  }
  
  if(pos == -1) {
    MemListAppend(arrivals, &temp);
  }
  else {
    MemListInsertAfter(arrivals, &temp, pos);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, 
          "AddArrival: @%i index:%u delta:%s",
          (int)pos, 
          (uint)index, 
          arrival_string);
}

Arrival ArrivalConstructor(const char* trip_id, 
                           const char* scheduled_arrival, 
                           const char* predicted_arrival, 
                           const char* delta_string, 
                           const int32_t delta, 
                           const uint8_t bus_index,
                           const char arrival_code,
                           const bool is_arrival) {
                              
  Arrival arrival;
  int l = strlen(trip_id);
  arrival.trip_id = (char *)malloc(sizeof(char)*(l+1));
  StringCopy(arrival.trip_id, trip_id, l+1);
  l = strlen(scheduled_arrival);
  arrival.scheduled_arrival = (char *)malloc(sizeof(char)*(l+1));
  StringCopy(arrival.scheduled_arrival, scheduled_arrival, l+1);
  l = strlen(predicted_arrival);
  arrival.predicted_arrival = (char *)malloc(sizeof(char)*(l+1));
  StringCopy(arrival.predicted_arrival, predicted_arrival, l+1);
  l = strlen(delta_string);
  arrival.delta_string = (char *)malloc(sizeof(char)*(l+1));
  StringCopy(arrival.delta_string, delta_string, l+1);
  arrival.delta = delta;
  arrival.bus_index = bus_index;
  arrival.arrival_code = arrival_code;
  arrival.is_arrival = is_arrival;
  return arrival;
}

Arrival ArrivalCopy(const Arrival* arrival) {
  return ArrivalConstructor(arrival->trip_id, 
                            arrival->scheduled_arrival, 
                            arrival->predicted_arrival, 
                            arrival->delta_string, 
                            arrival->delta, 
                            arrival->bus_index, 
                            arrival->arrival_code,
                            arrival->is_arrival);
}

void ArrivalDestructor(Arrival* arrival) {
  arrival->delta = arrival->bus_index = 0;
  arrival->arrival_code = 'x';
  arrival->is_arrival = false;
  FreeAndClearPointer((void**)&arrival->trip_id);
  FreeAndClearPointer((void**)&arrival->scheduled_arrival);
  FreeAndClearPointer((void**)&arrival->predicted_arrival);
  FreeAndClearPointer((void**)&arrival->delta_string);
}

Arrivals* ArrivalsCopy(const Arrivals* arrivals) {
  Arrivals* arrivals_copy = MemListCopy(arrivals);
  for(uint16_t i = 0; i < MemListCount(arrivals); i++) {
    Arrival* dest = MemListGet(arrivals_copy, i);
    *dest = ArrivalCopy((Arrival*)MemListGet(arrivals, i));
  }
  return arrivals_copy;
}

void ArrivalsConstructor(Arrivals** arrivals) {
  *arrivals = MemListCreate(sizeof(Arrival));
}

void ArrivalsDestructor(Arrivals* arrivals) {
  for(uint16_t i = 0; i < MemListCount(arrivals); i++) {
    ArrivalDestructor((Arrival*)MemListGet(arrivals, i));
  }
  MemListClear(arrivals);
}

ArrivalColors ArrivalColor(const Arrival arrival) {
  ArrivalColors retval;
  switch(arrival.arrival_code) {
    case 'e':
      // predicted early
      retval.foreground = GColorWhite;
      retval.background = PBL_IF_COLOR_ELSE(GColorRed, GColorBlack);
      retval.boarder = PBL_IF_COLOR_ELSE(GColorBlack, GColorWhite);
      break;
    case 'o':
      // predicted on-time
      retval.foreground = PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack);
      retval.background = PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorLightGray);
      retval.boarder = GColorBlack;
      break;
    case 'l':
      // predicted late
      retval.foreground = PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack);
      retval.background = PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite);
      retval.boarder = GColorBlack;
      break;
    case 's':
    default:
      // scheduled time
      retval.foreground = GColorBlack;
      retval.background = GColorLightGray;
      retval.boarder = GColorBlack;
      break;
  }
  return retval;
}

const char* ArrivalText(const Arrival a) {
  switch(a.arrival_code) {
    case 'e':
      return "Early";
      break;
    case 'o':
      return "On time";
      break;
    case 'l':
      return "Delayed";
      break;
    case 's':
    default:
      return "Scheduled time";
  }
}

const char* ArrivalDepartedText(const Arrival arrival) {
  if(arrival.is_arrival) {
    if(arrival.delta >= 0) {
      return "Arrives in:";
    }
    else {
      return "Arrived:";
    }
  }
  else {
    if(arrival.delta >= 0) {
      return "Departs in:";
    }
    else {
      return "Departed:";
    }
  }
}

const char* ArrivalPredicted(const Arrival arrival) {
  int i = strlen(arrival.predicted_arrival);
  if(i > 0) {
    return arrival.predicted_arrival;
  }
  else {
    return "n/a";
  }
}

const char* ArrivalScheduled(const Arrival arrival) {
  int i = strlen(arrival.scheduled_arrival);
  if(i > 0) {
    return arrival.scheduled_arrival;
  }
  else {
    return "n/a";
  }
 }
