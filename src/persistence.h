#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <pebble.h>
#include "buses.h"

#define PERSISTENCE_VERSION 1
#define PERSIST_KEY_VERSION 1
#define PERSIST_KEY_BUSES_COUNT 2

// note that this is incremented to store each bus (0@100, 1@101, etc.)
#define PERSIST_KEY_BUSES 100 

void PersistenceVersionControl();
void LoadBusesFromPersistence(Buses* buses);
bool SaveBusToPersistence(const Bus* bus, const uint i);
bool SaveBusCountToPersistence(uint32_t count);
void DeleteBusFromPersistence(const Buses* buses, uint32_t index);

#endif //#PERSISTENCE_H