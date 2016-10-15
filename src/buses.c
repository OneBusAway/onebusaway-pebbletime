#include "buses.h"
#include "utility.h"
#include "location.h"
#include "persistence.h"
#include "error_window.h"

void ListBuses(const Buses* buses) {
#ifdef LOGGING_ENABLED
  APP_LOG(APP_LOG_LEVEL_INFO, "Number of buses:%u", (uint)buses->count);
  for(uint32_t i = 0; i < buses->count; i++)  {
    Bus b = buses->data[i];
    APP_LOG(APP_LOG_LEVEL_INFO,
            "%u - route:%s\troute_id:%s\tstop_id:%s",
            (uint)i,
            b.route_name,
            b.route_id,
            b.stop_id);
  }
#endif
}

void ListStops(const Stops* stops) {
#ifdef LOGGING_ENABLED
  APP_LOG(APP_LOG_LEVEL_INFO, "Number of stops:%u", (uint)stops->total_size);
  APP_LOG(APP_LOG_LEVEL_INFO, 
          "Number of memlist entries:%u @ offset: %u", 
          (uint)MemListCount(stops->memlist),
          (uint)stops->index_offset);
          
  for(uint32_t i = 0; i < MemListCount(stops->memlist); i++)  {
    Stop* s = MemListGet(stops->memlist, i);
    APP_LOG(APP_LOG_LEVEL_INFO,
            "%u - index:%u\tstop_id:%s\tname:%s\tdetails:%s\tdir:%s",
            (uint)i,
            (uint)s->index,
            s->stop_id,
            s->stop_name,
            s->detail_string,
            s->direction);
  }
#endif
}

void FilterBusesByLocation(const sll lat, const sll lon, Buses* buses) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Filtering buses by location:");
  buses->filter_count = 0;
  FreeAndClearPointer((void**)&buses->filter_index);
  for(uint32_t i = 0; i < buses->count; i++)  {
    Bus b = buses->data[i];
    // distance in KM
    sll d = DistanceBetweenSLL(b.lat, b.lon, lat, lon);

    //TODO / IDEA: make this return at least one stop,
    //  or search outward from the radius to find some...
    //TODO: make DEFINE or var set in settings, coordinate with JS OBA calls
    //  w/ radius set. One pattern could be that this distance is 2x, 4x the
    //  stop search dist?
    if(d < dbl2sll(1.000)) {
      uint32_t* temp = (uint32_t*)malloc(sizeof(uint32_t) *
                                         (buses->filter_count+1));
      if(temp == NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "NULL FILTER POINTER");
        ErrorWindowPush(
            "Critical error. Out of memory. 0x100022.", 
            true);
      }
      if(buses->filter_index != NULL) {
        memcpy(temp, buses->filter_index, sizeof(uint32_t)*buses->filter_count);
        free(buses->filter_index);
      }
      buses->filter_index = temp;
      buses->filter_index[buses->filter_count] = i;
      buses->filter_count += 1;
    }
  }
}

void BusDestructor(Bus* bus) {
  FreeAndClearPointer((void**)&bus->route_id);
  FreeAndClearPointer((void**)&bus->stop_id);
  FreeAndClearPointer((void**)&bus->route_name);
  FreeAndClearPointer((void**)&bus->stop_name);
  FreeAndClearPointer((void**)&bus->description);
  FreeAndClearPointer((void**)&bus->direction);
}

void BusesDestructor(Buses* buses) {
  for(uint i = 0; i < buses->count; i++) {
    BusDestructor(&buses->data[i]);
  }
  FreeAndClearPointer((void**)&buses->data);
  FreeAndClearPointer((void**)&buses->filter_index);
}

static bool CreateBus(const char* route_id,
                      const char* route_name,
                      const char* description,
                      const char* stop_id,
                      const char* stop_name,
                      const sll lat,
                      const sll lon,
                      const char* direction,
                      Buses* buses) {

  APP_LOG(APP_LOG_LEVEL_INFO,
          "Creating bus - routename:%s, route_id:%s, buses: %p, count: %i",
          route_name,
          route_id,
          buses,
          (int)buses->count);

  // create new bus
  bool success = true;
  Bus temp_bus;
  success &= StringAllocateAndCopy(&temp_bus.route_id, route_id);
  success &= StringAllocateAndCopy(&temp_bus.stop_id, stop_id);
  success &= StringAllocateAndCopy(&temp_bus.route_name, route_name);
  success &= StringAllocateAndCopy(&temp_bus.stop_name, stop_name);
  success &= StringAllocateAndCopy(&temp_bus.description, description);
  success &= StringAllocateAndCopy(&temp_bus.direction, direction);
  temp_bus.lat = lat;
  temp_bus.lon = lon;

  if(success && SaveBusToPersistence(&temp_bus, buses->count)) {
    // add bus to the end of buses
    Bus* temp_buses = (Bus *)malloc(sizeof(Bus)*((buses->count)+1));
    if(temp_buses == NULL) {
      // APP_LOG(APP_LOG_LEVEL_ERROR, "NULL TEMP BUSES");
      BusDestructor(&temp_bus);
      return false;
    }
    if(buses->data != NULL) {
      memcpy(temp_buses, buses->data, sizeof(Bus)*(buses->count));
      free(buses->data);
    }

    buses->data = temp_buses;
    buses->data[buses->count] = temp_bus;
    buses->count+=1;
    success = SaveBusCountToPersistence(buses->count);
  }
  else {
    BusDestructor(&temp_bus);
    return false;
  }
  return success;
}

bool AddBus(const Bus* bus, Buses* buses) {
  return CreateBus(bus->route_id,
                   bus->route_name,
                   bus->description,
                   bus->stop_id,
                   bus->stop_name,
                   bus->lat,
                   bus->lon,
                   bus->direction,
                   buses);
}

bool AddBusFromStopRoute(const Stop* stop, const Route* route, Buses* buses) {
  return CreateBus(route->route_id,
                   route->route_name,
                   route->description,
                   stop->stop_id,
                   stop->stop_name,
                   stop->lat,
                   stop->lon,
                   stop->direction,
                   buses);
}

int32_t GetBusIndex(
    const char* stop_id,
    const char* route_id,
    const Buses* buses) {

  for(uint32_t i  = 0; i < buses->count; i++) {
    Bus bus = buses->data[i];
    if((strcmp(bus.stop_id, stop_id) == 0) &&
        (strcmp(bus.route_id, route_id) == 0)) {
      return i;
    }
  }
  return -1;
}

void RemoveBus(uint32_t index, Buses *buses) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Removing bus - index:%u", (uint)index);
  if(buses->count <= 0) return;
  if(buses->count <= index) return;

  // delete persistence
  DeleteBusFromPersistence(buses, index);

  // destroy bus
  BusDestructor(&buses->data[index]);

  if(buses->count == 1) {
    FreeAndClearPointer((void**)&buses->data);
    buses->count = 0;
    return;
  }

  Bus* temp_buses = (Bus *)malloc(sizeof(Bus)*((buses->count)-1));
  if(temp_buses == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL BUS POINTER");
    ErrorWindowPush(
        "Critical error. Out of memory. 0x100024.", 
        true);
  }

  // first part copy
  if(index > 0) {
    memcpy(temp_buses, buses->data, sizeof(Bus)*(index));
  }
  // second part copy
  if((index+1) < buses->count) {
    memcpy(&temp_buses[index],
           &buses->data[index+1],
           sizeof(Bus)*(buses->count-index-1));
  }

  free(buses->data);
  buses->data = temp_buses;
  buses->count-=1;
}

void AddStop(const uint16_t index,
             const char* stop_id,
             const char* stop_name,
             const char* detail_string,
             const sll lat,
             const sll lon,
             const char * direction,
             Stops* stops) {

  APP_LOG(APP_LOG_LEVEL_INFO,
          "Creating stop - index: %u, stop:%s, name:%s, details:%s",
          (uint)index,
          stop_id,
          stop_name,
          detail_string);

  int16_t pos = -1;
  for(int16_t i = 0; i < MemListCount(stops->memlist); i++) {
    Stop* stop = (Stop*)MemListGet(stops->memlist, i);
    if(stop->index > index) {
      pos = i;
      break;
    }
    else if (stop->index == index) {
      return;
    }
  }

  Stop temp = StopConstructor(index,
                              stop_id,
                              stop_name,
                              detail_string,
                              lat,
                              lon,
                              direction);
  
  bool success = true;
  
  if(pos == -1) {
    success &= MemListAppend(stops->memlist, &temp);
  }
  else {
    success &= MemListInsertAfter(stops->memlist, &temp, pos);
  }
  
  uint16_t count = MemListCount(stops->memlist);
  if(count > 15) {
    if(pos == -1 || pos > 5) {
      // trim the start of the list
      Stop* stop = (Stop*)MemListGet(stops->memlist, 0);
      StopDestructor(stop);
      success &= MemListRemove(stops->memlist, 0);
    }
    else {
      // trim the end of the list
      Stop* stop = (Stop*)MemListGet(stops->memlist, count-1);
      StopDestructor(stop);
      success &= MemListRemove(stops->memlist, count-1); 
    }
  } 
  Stop* stop = (Stop*)MemListGet(stops->memlist, 0);

  APP_LOG(APP_LOG_LEVEL_DEBUG,
          " - stop 0: %s, new_index_offset:%u",
          stop->stop_name,
          (uint)stop->index);
  stops->index_offset = stop->index; 
  
  ListStops(stops);

  if(!success) {
    ErrorWindowPush(
        "Critical error. Out of memory. 0x100028.", 
        true);
  }
}

void AddRoute(const char *route_id,
              const char *routeName,
              const char *description,
              Routes* routes) {

  APP_LOG(APP_LOG_LEVEL_INFO,
          "Creating route - route:%s, name:%s",
          route_id,
          routeName);
  Route* temp_routes = (Route*)malloc(sizeof(Route)*((routes->count)+1));
  if(temp_routes == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL ROUTES POINTER");
    ErrorWindowPush(
        "Critical error. Out of memory. 0x100020.", 
        true);
    return;
  }
  if(routes->data != NULL) {
    memcpy(temp_routes, routes->data, sizeof(Route)*(routes->count));
    free(routes->data);
  }
  routes->data = temp_routes;
  Route temp_route = RouteConstructor(route_id,
                                      routeName,
                                      description,
                                      false);
  routes->data[routes->count] = temp_route;
  routes->count+=1;
}

Stop StopConstructor(const uint16_t index,
                     const char* stop_id,
                     const char* stop_name,
                     const char* detail_string,
                     const sll lat,
                     const sll lon,
                     const char* direction) {

  Stop stop;
  stop.index = index;
  StringAllocateAndCopy(&stop.stop_id, stop_id);
  StringAllocateAndCopy(&stop.stop_name, stop_name);
  StringAllocateAndCopy(&stop.detail_string, detail_string);
  stop.lat = lat;
  stop.lon = lon;
  StringAllocateAndCopy(&stop.direction, direction);

  return stop;
}

void StopDestructor(Stop* stop) {
  FreeAndClearPointer((void**)&stop->stop_id);
  FreeAndClearPointer((void**)&stop->stop_name);
  FreeAndClearPointer((void**)&stop->detail_string);
  FreeAndClearPointer((void**)&stop->direction);
}

void StopsConstructor(Stops *stops) {
  stops->index_offset = 0;
  stops->total_size = 0;
  stops->memlist = MemListCreate(sizeof(Stop));
}

void StopsDestructor(Stops *stops) {
  for(uint32_t i = 0; i < MemListCount(stops->memlist); i++) {
    StopDestructor((Stop*)MemListGet(stops->memlist, i));
  }
  stops->index_offset = 0;
  stops->total_size = 0;
  MemListClear(stops->memlist);
  FreeAndClearPointer((void**)&stops->memlist);
}

Route RouteConstructor(const char* route_id,
                       const char* route_name,
                       const char* direction,
                       const bool favorite) {

  Route t;
  StringAllocateAndCopy(&t.route_id, route_id);
  StringAllocateAndCopy(&t.route_name, route_name);
  StringAllocateAndCopy(&t.description, direction);
  t.favorite = favorite;
  return t;
}

void RouteDestructor(Route *t) {
  FreeAndClearPointer((void**)&t->route_id);
  FreeAndClearPointer((void**)&t->route_name);
  FreeAndClearPointer((void**)&t->description);
}

void RoutesDestructor(Routes *r) {
  for(uint32_t i = 0; i < r->count; i++) {
    RouteDestructor(&r->data[i]);
  }
  FreeAndClearPointer((void**)&r->data);
  r->count = 0;
}

void RoutesConstructor(Routes *r) {
  r->data = NULL;
  r->count = 0;
}
