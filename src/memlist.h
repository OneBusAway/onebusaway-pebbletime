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

#endif