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
 
#ifndef MEMLIST_H
#define MEMLIST_H

#include <pebble.h>

typedef struct MemList {
  void* data;
  uint16_t object_size;
  uint16_t count;
} MemList;

MemList* MemListCreate(uint16_t size);
void MemListClear(MemList* list);
uint16_t MemListCount(const MemList* list);
void* MemListGet(const MemList* list, uint16_t pos);
bool MemListAppend(MemList* list, void* object);
bool MemListInsertAfter(MemList* list, void* object, uint16_t pos);
MemList* MemListCopy(const MemList* list);
bool MemListRemove(MemList* list, uint16_t pos);

#endif