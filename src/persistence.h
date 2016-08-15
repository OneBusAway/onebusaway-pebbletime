#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <pebble.h>
#include "buses.h"

#define PERSISTENCE_VERSION 1
#define PERSIST_KEY_VERSION 1
#define PERSIST_KEY_BUSES_COUNT 2

// note that this is incremented to store each bus (0@1000, 1@1000, etc.)
#define PERSIST_KEY_BUSES 1000
#define PERSIST_KEY_ROUTE_ID 2000
#define PERSIST_KEY_STOP_ID 3000
#define PERSIST_KEY_ROUTE_NAME 4000
#define PERSIST_KEY_STOP_NAME 5000
#define PERSIST_KEY_DIRECTION 6000
#define PERSIST_KEY_DESCRIPTION 7000

void PersistenceVersionControl();
void LoadBusesFromPersistence(Buses* buses);
bool SaveBusToPersistence(const Bus* bus, const uint i);
bool SaveBusCountToPersistence(uint32_t count);
void DeleteBusFromPersistence(const Buses* buses, uint32_t index);

#endif //#PERSISTENCE_H