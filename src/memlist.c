#include "memlist.h"

MemList* MemListCreate(uint16_t size) {
  MemList* mem_list = malloc(sizeof(MemList));
  mem_list->data = NULL;
  mem_list->object_size = size;
  mem_list->count = 0;
  return mem_list;
}

void MemListClear(MemList* list) {
  free(list->data);
  list->data = NULL;
  list->count = 0;
}

uint16_t MemListCount(const MemList* list) {
  if (list == NULL) {
    return 0;
  }
  return list->count;
}

void* MemListGet(const MemList* list, uint16_t pos) {
  if(pos >= list->count) {
    return NULL;
  }
  return list->data+((list->object_size)*pos);
}

bool MemListAppend(MemList* list, void* object) {
  void* temp_data = malloc(list->object_size*(list->count+1));
  if(temp_data == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "null - size:%u count:%u", 
        (uint)list->object_size, (uint)list->count);
    return false;
  }
  
  // copy beginning of list
  memcpy(temp_data, list->data, (list->object_size)*(list->count));
 
  // insert
  memcpy(temp_data+(list->object_size*list->count), object, list->object_size);
  
  free(list->data);
  list->data = temp_data;
  list->count += 1;
  
  return true;
}

bool MemListInsertAfter(MemList* list, void* object, uint16_t pos) {
  if(pos >= list->count) {
    return false;
  }
  
  void* temp_data = malloc(list->object_size*(list->count+1));
  if(temp_data == NULL) {
    return false;
  }
  
  // copy beginning of list
  memcpy(temp_data, list->data, (list->object_size)*pos);
 
  // insert
  memcpy(temp_data+(list->object_size)*pos, object, list->object_size);
  
  // copy end of list
  memcpy(temp_data+(list->object_size)*(pos+1), 
         list->data+(list->object_size)*(pos), 
         list->object_size*(list->count-pos));

  free(list->data);
  list->data = temp_data;
  list->count += 1;
  
  return true;
}

MemList* MemListCopy(const MemList* list) {
  MemList* ret_list = malloc(sizeof(MemList));
  memcpy(ret_list, list, sizeof(MemList));
  uint16_t size = list->object_size*(list->count);
  void* temp_data = malloc(size);
  memcpy(temp_data, list->data, size);
  ret_list->data = temp_data;
  return ret_list;
}

bool MemListRemove(MemList* list, uint16_t pos) {
  if(pos >= list->count) {
    return false;
  }
  
  void* temp_data = malloc(list->object_size*(list->count-1));
  if(temp_data == NULL) {
    return false;
  }
  
  // copy beginning of list
  memcpy(temp_data, list->data, (list->object_size)*pos);
 
  // copy end of list
  memcpy(temp_data+(list->object_size)*(pos), 
         list->data+(list->object_size)*(pos+1), 
         list->object_size*(list->count-pos-1));

  free(list->data);
  list->data = temp_data;
  list->count -= 1;
  
  return true;
}