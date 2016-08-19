#include "persistence.h"
#include "utility.h"
// write out the persistence version to enable later version control
void PersistenceVersionControl() {
  persist_write_int(PERSIST_KEY_VERSION, PERSISTENCE_VERSION);
}

#ifndef RELEASE
// Persistant storage error translator
const char *TranslateStorageError(const status_t result) {
  switch (result) {
    case S_SUCCESS: return "S_SUCCESS";
    case E_ERROR: return "E_ERROR";
    case E_UNKNOWN: return "E_UNKNOWN";
    case E_INTERNAL: return "E_INTERNAL";
    case E_INVALID_ARGUMENT: return "E_INVALID_ARGUMENT";
    case E_OUT_OF_MEMORY: return "E_OUT_OF_MEMORY";
    case E_OUT_OF_STORAGE: return "E_OUT_OF_STORAGE";
    case E_OUT_OF_RESOURCES: return "E_OUT_OF_RESOURCES";
    case E_RANGE: return "E_RANGE";
    case E_DOES_NOT_EXIST: return "E_DOES_NOT_EXIST";
    case E_INVALID_OPERATION: return "E_INVALID_OPERATION";
    case E_BUSY: return "E_BUSY";
    case E_AGAIN: return "E_AGAIN";
    case S_TRUE: return "S_TRUE";
    //case S_FALSE: return "S_FALSE";
    case S_NO_MORE_ITEMS: return "S_NO_MORE_ITEMS";
    case S_NO_ACTION_REQUIRED: return "S_NO_ACTION_REQUIRED";
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown error - %i", (int)result);
      return "UNKNOWN ERROR";
  }
}
#endif

// check to see if a write call should retry upon failure
static bool RetryPersist(status_t result) {
  switch (result) {
    case E_BUSY:
    case E_AGAIN:
      return true;
      break;
    default:
      return false;
      break;
  }
}

int PersistAllocateAndReadString(uint32_t key, char** dest) {
  char* buffer = (char*)malloc(PERSIST_STRING_MAX_LENGTH);
  if(buffer == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL STRING POINTER");
  }  
  int ret = persist_read_string(key, 
                                buffer, 
                                PERSIST_STRING_MAX_LENGTH);
  if(ret == E_DOES_NOT_EXIST) {
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "Warning - persist_read_string error @ key %u",
            (uint)key);
  }
  else {
    *dest = (char*)malloc(strlen(buffer)+1);
    if(*dest == NULL) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "NULL STRING POINTER");
    }
    strcpy(*dest, buffer);
  }

  free(buffer);
  return ret;
}

void LoadBusesFromPersistence(Buses* buses) {
  // check storage for buses
  buses->count = 0;
  buses->data = NULL;
  buses->filter_count = 0;
  buses->filter_index = NULL;

  if(PERSIST_DATA_MAX_LENGTH < sizeof(Bus)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "Warning - bus overloaded! %ul < %ul", 
             PERSIST_DATA_MAX_LENGTH, 
             sizeof(Bus));
  }

  if(persist_exists(PERSIST_KEY_BUSES) && 
     persist_exists(PERSIST_KEY_BUSES_COUNT)) {
    buses->count = persist_read_int(PERSIST_KEY_BUSES_COUNT);
    buses->data = (Bus *)malloc(sizeof(Bus)*(buses->count));

    if(buses->data != NULL) {
      for(uint32_t i = 0; i < buses->count; i++) {
        int ret = persist_read_data(PERSIST_KEY_BUSES+i, 
                                    &(buses->data[i]), 
                                    sizeof(Bus));
        if(ret == E_DOES_NOT_EXIST) {
          APP_LOG(APP_LOG_LEVEL_ERROR, 
                  "Warning - persist_read_data error @ %u",
                  (uint)i);
        }
        PersistAllocateAndReadString(PERSIST_KEY_ROUTE_ID+i,
                                     &(buses->data[i].route_id));
        PersistAllocateAndReadString(PERSIST_KEY_STOP_ID+i,
                                     &(buses->data[i].stop_id));
        PersistAllocateAndReadString(PERSIST_KEY_ROUTE_NAME+i,
                                     &(buses->data[i].route_name));
        PersistAllocateAndReadString(PERSIST_KEY_STOP_NAME+i,
                                     &(buses->data[i].stop_name));
        PersistAllocateAndReadString(PERSIST_KEY_DIRECTION+i,
                                     &(buses->data[i].direction));
        PersistAllocateAndReadString(PERSIST_KEY_DESCRIPTION+i,
                                     &(buses->data[i].description));
      }
    }
    else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "NULL BUS POINTER");
    }
  }
}

// void SaveBusesToPersistence(const Buses* buses) {
//   if((buses->count != 0) && (buses->data != NULL)) {
//     for(uint32_t i = 0; i < buses->count; i++) {
//       status_t e = E_AGAIN;
//       while(RetryPersist(e)) {
//         e = persist_write_data(PERSIST_KEY_BUSES+i, 
//             &(buses->data[i]), sizeof(Bus));
//       }
//     }
//   }

//   // always write out the current number of buses, even if it's zero.
//   persist_write_int(PERSIST_KEY_BUSES_COUNT, buses->count);
// }

// Delete persistent data for a given index
void DeletePersistence(const uint i){
  persist_delete(PERSIST_KEY_BUSES+i);
  persist_delete(PERSIST_KEY_ROUTE_ID+i);
  persist_delete(PERSIST_KEY_STOP_ID+i);
  persist_delete(PERSIST_KEY_ROUTE_NAME+i);
  persist_delete(PERSIST_KEY_STOP_NAME+i);
  persist_delete(PERSIST_KEY_DIRECTION+i);
  persist_delete(PERSIST_KEY_DESCRIPTION+i);
}

// Write a string to persistence, return success or failure
bool PersistWriteString(const uint32_t key, const char* string) {
  return (0 < persist_write_string(key, string));
}

// Save a single bus to persistence storage at the index passed into the
// function. Returns success or failure of writing out to persistence.
bool SaveBusToPersistence(const Bus* bus, const uint i) {
  bool retval = false;
  if(0 < persist_write_data(PERSIST_KEY_BUSES+i, bus, sizeof(Bus))) {
    // success
    retval = true;    
  }

  retval &= PersistWriteString(PERSIST_KEY_ROUTE_ID+i, bus->route_id);
  retval &= PersistWriteString(PERSIST_KEY_STOP_ID+i, bus->stop_id);
  retval &= PersistWriteString(PERSIST_KEY_ROUTE_NAME+i, bus->route_name);
  retval &= PersistWriteString(PERSIST_KEY_STOP_NAME+i, bus->stop_name);
  retval &= PersistWriteString(PERSIST_KEY_DIRECTION+i, bus->direction);
  retval &= PersistWriteString(PERSIST_KEY_DESCRIPTION+i, bus->description);
 
  if(!retval) {
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "SaveBusToPersistence - saving bus %u failed", 
            (uint)i);
    // clean up anything that happened to have been saved
    DeletePersistence(i);
  }

  return retval;
}

// Save a single bus to persistence storage. Returns success or failure of 
// writing out to persistence.
bool SaveBusCountToPersistence(uint32_t count) {
  status_t e = E_AGAIN;
  while(RetryPersist(e)) {
    e = persist_write_int(PERSIST_KEY_BUSES_COUNT, count);
  }

  bool retval = false;
  if(0 < e) {
    retval = true;
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "SaveBusCountToPersistence - saving bus count %u failed",
            (uint)count);
  }

  return retval;
}

// Remove a single bus from persistent storage using the buses' current
// index. Assumes that the Buses passed in and the persistence are identical.
// Only modifies the persistence; does not modify buses.
// TODO: this uses the copy of buses to avoid reading and re-writing the 
// persistance - probably should just do that to avoid consistency problems
void DeleteBusFromPersistence(const Buses* buses, uint32_t index) {
  if((buses->data != NULL) && (buses->count > 0) && (index < buses->count)) {
    // delete the bus at the end of the storage 
    uint32_t count = buses->count-1;
    DeletePersistence(count);
    
    // shift the buses
    for(uint32_t i = index; i < count; i++) {
      // reset data before saving
      APP_LOG(APP_LOG_LEVEL_INFO, "(re)writing bus %u", (uint)i);
      SaveBusToPersistence(&(buses->data[i+1]), i);
    }

    // update the number of buses
    SaveBusCountToPersistence(count);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "Cannot delete bus index %u", 
            (uint)index);
  }
}