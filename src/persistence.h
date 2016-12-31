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
 
#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <pebble.h>
#include "buses.h"

#define PERSISTENCE_VERSION 1

// persistence keys
#define PERSIST_KEY_VERSION 1
#define PERSIST_KEY_BUSES_COUNT 2
#define PERSIST_KEY_ARRIVAL_RADIUS 3
#define PERSIST_KEY_SEARCH_RADIUS 4

// note that this is incremented to store each bus (0@1000, 1@1000, etc.)
#define PERSIST_KEY_BUSES 1000
#define PERSIST_KEY_ROUTE_ID 2000
#define PERSIST_KEY_STOP_ID 3000
#define PERSIST_KEY_ROUTE_NAME 4000
#define PERSIST_KEY_STOP_NAME 5000
#define PERSIST_KEY_DIRECTION 6000
#define PERSIST_KEY_DESCRIPTION 7000

#define DEFAULT_ARRIVAL_RADIUS 1000
#define DEFAULT_SEARCH_RADIUS 300

void PersistenceInit();
void LoadBusesFromPersistence(Buses* buses);
bool SaveBusToPersistence(const Bus* bus, const uint i);
bool SaveBusCountToPersistence(uint32_t count);
void DeleteBusFromPersistence(const Buses* buses, uint32_t index);
uint PersistReadArrivalRadius();
bool PersistWriteArrivalRadius(const uint32_t radius);
uint PersistReadSearchRadius();
bool PersistWriteSearchRadius(const uint32_t radius);

#endif //#PERSISTENCE_H