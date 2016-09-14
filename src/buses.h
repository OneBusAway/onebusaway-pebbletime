#ifndef BUSES_H
#define BUSES_H

#include <pebble.h>
#include <pebble-math-sll/math-sll.h>
#include "memlist.h"

// #define ID_LENGTH 20
// #define DIRECTION_LENGTH 5
// #define ROUTE_SHORT_LENGTH 10
// #define STOP_LENGTH 50
// #define DESCRIPTION_LENGTH 50

// WARNING:
// CHANGING THIS STRUCT WILL CAUSE A PERSISTENT STORAGE VERSION CHANGE
// MODIFY WITH CARE
typedef struct {
  // struct {
    sll lat;
    sll lon;
  // } Coordinates;
  char* route_id;
  char* stop_id;
  char* route_name;
  char* stop_name;
  char* direction;
  char* description;
} __attribute__((__packed__)) Bus;
// WARNING:
// CHANGING THIS STRUCT WILL CAUSE A PERSISTENT STORAGE VERSION CHANGE
// MODIFY WITH CARE

typedef struct {
  Bus* data;
  uint32_t count;

  // used to geographically filter nearby buses
  uint32_t* filter_index;
  uint32_t filter_count;
} __attribute__((__packed__)) Buses;

typedef struct {
  uint16_t index;
  char* stop_id;
  char* stop_name;
  char* detail_string;
  sll lat;
  sll lon;
  char* direction;
} __attribute__((__packed__)) Stop;

typedef struct {
  MemList* memlist;
  uint16_t total_size; // total number of stops
  uint16_t index_offset; // what index does the memlist start at
} __attribute__((__packed__)) Stops;

typedef struct {
  char* route_id;
  char* route_name;
  char* description;
  bool favorite;
} __attribute__((__packed__)) Route;

typedef struct {
  Route* data;
  uint32_t count;
} __attribute__((__packed__)) Routes;

void ListBuses(const Buses* buses);
void ListStops(const Stops* stops);
void FilterBusesByLocation(const sll lat, const sll lon, Buses* buses);
void BusesDestructor(Buses* buses);
bool AddBus(const Bus* bus, Buses* buses);
bool AddBusFromStopRoute(const Stop* stop, const Route* route, Buses* buses);
void RemoveBus(uint32_t index, Buses *buses);
int32_t GetBusIndex(const char* stop_id,
                    const char* route_id,
                    const Buses* buses);
void AddStop(const uint16_t index,
             const char* stop_id,
             const char* stop_name,
             const char* detail_string,
             const sll lat,
             const sll lon,
             const char * direction,
             Stops* stops);
void AddRoute(const char *route_id,
              const char *routeName,
              const char *description,
              Routes* routes);
Stop StopConstructor(const uint16_t index,
                     const char* stop_id,
                     const char* stop_name,
                     const char* detail_string,
                     const sll lat,
                     const sll lon,
                     const char* direction);
void StopDestructor(Stop* stop);
void StopsConstructor(Stops* stops);
void StopsDestructor(Stops* stops);
Route RouteConstructor(const char* route_id,
                       const char* route_name,
                       const char* description,
                       const bool favorite);
void RouteDestructor(Route *t);
void RoutesConstructor(Routes *r);
void RoutesDestructor(Routes *r);

#endif /* end of include guard: BUSES_H */
