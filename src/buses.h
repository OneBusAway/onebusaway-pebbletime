#ifndef BUSES_H
#define BUSES_H

#include <pebble.h>
#include <pebble-math-sll/math-sll.h>

#define ID_LENGTH 20
#define DIRECTION_LENGTH 5
#define ROUTE_SHORT_LENGTH 10
#define STOP_LENGTH 50
#define DESCRIPTION_LENGTH 50

// WARNING: 
// CHANGING THIS STRUCT WILL CAUSE A PERSISTENT STORAGE VERSION CHANGE
// MODIFY WITH CARE
typedef struct {
  // static route properties
  char route_id[ID_LENGTH];
  char stop_id[ID_LENGTH];
  char route_name[ROUTE_SHORT_LENGTH];
  char stop_name[STOP_LENGTH];
  sll lat;
  sll lon;
  char direction[DIRECTION_LENGTH];
  char description[DESCRIPTION_LENGTH];
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
  char stop_id[ID_LENGTH];
  char stop_name[STOP_LENGTH];
  char detail_string[DESCRIPTION_LENGTH];
  sll lat;
  sll lon;
  char direction[DIRECTION_LENGTH];
} __attribute__((__packed__)) Stop;

typedef struct {
  Stop* data;
  uint32_t count;
} __attribute__((__packed__)) Stops;

typedef struct {
  char* route_id;
  char* route_name;
  char* description;
  char* stop_id_list;
} __attribute__((__packed__)) Route;

typedef struct {
  Route* data;
  uint32_t count;
} __attribute__((__packed__)) Routes;

void ListBuses(const Buses* buses);
void FilterBusesByLocation(const sll lat, const sll lon, Buses* buses);
bool AddBus(const Bus* bus, Buses* buses);
bool AddBusFromStopRoute(const Stop* stop, const Route* route, Buses* buses);
void RemoveBus(uint32_t index, Buses *buses);
int32_t GetBusIndex(const char* stop_id, 
                    const char* route_id, 
                    const Buses* buses);
void AddStop(const char* stop_id, 
             const char* stop_name,
             const char* detail_string,
             const sll lat,
             const sll lon, 
             const char * direction, 
             Stops* stops);
void AddRoute(const char *route_id, 
              const char *routeName, 
              const char *stop_id_list, 
              const char *description, 
              Routes* routes);
Stop StopConstructor(const char* stop_id, 
                     const char* stop_name, 
                     const char* detail_string, 
                     const sll lat, 
                     const sll lon, 
                     const char* direction);
void StopsInit(Stops* stops);
void StopsDestructor(Stops* stops);
Route RouteConstructor(const char* route_id, 
                       const char* route_name, 
                       const char* stop_id_list, 
                       const char* description);
void RouteDestructor(Route *t);
void RoutesInit(Routes *r);
void RoutesDestructor(Routes *r);

#endif /* end of include guard: BUSES_H */
