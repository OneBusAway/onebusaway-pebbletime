#include "communication.h"
#include "arrivals.h"
#include "settings_stops.h"
#include "main_window.h"
#include "progress_window.h"
#include "utility.h"
#include "error_window.h"

static AppTimer *s_timer;
static Stops s_nearby_stops;
static Routes s_nearby_routes;
static Arrivals s_temp_arrivals;
static sll s_cached_lat;
static sll s_cached_lon;
static uint32_t s_outstanding_requests;
static uint32_t s_transaction_id;
static uint32_t s_skipped_arrival_updates;
static uint32_t s_last_outstanding_request_at_skipped;

// AppMessage error translators
const char *TranslateError(const AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: 
      return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: 
      return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: 
      return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: 
      return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: 
      return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: 
      return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: 
      return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: 
      return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: 
      return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED:
      return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED:
      return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY:
      return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED:
      return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR:
      return "APP_MSG_INTERNAL_ERROR";
    case APP_MSG_INVALID_STATE:
      return "APP_MSG_INVALID_STATE";
    default:
      return "UNKNOWN ERROR";
  }
}

static void CancelOutstandingRequests() {
  s_outstanding_requests = 0;
  s_skipped_arrival_updates = 0;
  s_last_outstanding_request_at_skipped = 0;
  s_transaction_id += 1;
}

static void NextTimer(AppData* appdata);

static void UpdateArrivalsCallback(void *context) {
  // only start the next arrivals transaction if the previous one has finished
  if(s_outstanding_requests == 0) {
    s_skipped_arrival_updates = 0;
    s_last_outstanding_request_at_skipped = 0;
    UpdateArrivals((AppData*)context);
  }
  else {
    if(s_outstanding_requests == s_last_outstanding_request_at_skipped) {
      // only increment if progress is stalled
      s_skipped_arrival_updates += 1;
    }
    s_last_outstanding_request_at_skipped = s_outstanding_requests;
    if(s_skipped_arrival_updates > 2) {
      APP_LOG(APP_LOG_LEVEL_ERROR, 
              "timer: too many skipped updates, forcing update_arrivals()");
      
      // reset 
      CancelOutstandingRequests();
      UpdateArrivals((AppData*)context);    
    }
  }
  
  NextTimer((AppData*)context);
}

static void NextTimer(AppData* appdata) {
  s_timer = app_timer_register(30000 /*ms*/, UpdateArrivalsCallback, appdata);
}

void StartArrivalsUpdateTimer(AppData* appdata) {
  if(s_timer == NULL) {
    NextTimer(appdata);
  }
}

void StopArrivalsUpdateTimer() {
  if(s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

// static void SendAppMessageGetRoutesForStopCallback(void* context) {
//   Stop* stop = context;
//   SendAppMessageGetRoutesForStop(stop);
// }

void SendAppMessageGetRoutesForStop(Stop* stop) {
  // if(s_outstanding_requests > 0) {
  //   // wait until the app message queue is clear
  //   APP_LOG(APP_LOG_LEVEL_INFO, 
  //           "SendAppMessageGetRoutesForStop: ...waiting...");
  //   app_timer_register(1500, SendAppMessageGetRoutesForStopCallback, stop);
  //   return;
  // }
  
  CancelOutstandingRequests();
  
  APP_LOG(APP_LOG_LEVEL_ERROR, "SendAppMessageGetRoutesForStop: Sending...!");

  // clear out existing nearby stops and add the single stop
  StopsDestructor(&s_nearby_stops);
  AddStop(stop->stop_id, 
          stop->stop_name, 
          stop->detail_string,
          stop->lat, 
          stop->lon, 
          stop->direction, 
          &s_nearby_stops);

  // clear the nearby routes
  RoutesDestructor(&s_nearby_routes);

  // Prepare dictionary
  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  // s_transaction_id += 1;
  APP_LOG(APP_LOG_LEVEL_INFO, 
          "----Initiated transaction id: %u",
          (uint)s_transaction_id);

  s_outstanding_requests = 1;

  // Write data
  dict_write_uint32(iterator, kAppMessageMessageType, kAppMessageRoutesForStop);
  dict_write_cstring(iterator, kAppMessageStopId, stop->stop_id);
  dict_write_uint32(iterator, kAppMessageTransactionId, s_transaction_id);

  // Send data
  app_message_outbox_send();  
}

void SendAppMessageGetNearbyStops() {
  // if(s_outstanding_requests > 0) {
  //   // wait until the app message queue is clear
  //   APP_LOG(APP_LOG_LEVEL_INFO, "SendAppMessageGetNearbyStops: ...waiting...");
  //   app_timer_register(1500, SendAppMessageGetNearbyStops, NULL);
  //   return;
  // }
  CancelOutstandingRequests();

  APP_LOG(APP_LOG_LEVEL_INFO, "SendAppMessageGetNearbyStops: Sending...!");

  // clear out existing nearby stops, routes
  StopsDestructor(&s_nearby_stops);
  RoutesDestructor(&s_nearby_routes);

  // s_transaction_id += 1;
  APP_LOG(APP_LOG_LEVEL_INFO, 
          "----Initiated transaction id: %u",
          (uint)s_transaction_id);

  s_outstanding_requests = 1;

  // Prepare dictionary
  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  // Write data
  dict_write_uint32(iterator, kAppMessageMessageType, kAppMessageNearbyStops);
  dict_write_uint32(iterator, kAppMessageTransactionId, s_transaction_id);

  // Send data
  app_message_outbox_send();
}

// TODO: ADD WAIT/RETRY LOGIC + OUTSTANDING REQUESTS BLOCK/LOCK
static void SendAppMessageGetLocation() {
  CancelOutstandingRequests();
  
  APP_LOG(APP_LOG_LEVEL_INFO, "SendAppMessageGetLocation: Sending...!");

  // Prepare dictionary
  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);

  // Write data
  dict_write_uint32(iterator, kAppMessageMessageType, kAppMessageLocation);

  // Send data
  app_message_outbox_send();
}

static void FilterBusesByCachedLocation(Buses* buses) {
  // TODO: consider just relying upon the phone to cache the location
  // (i.e. get rid of this function) or at least allowing the locaiotn
  // to update periodically (update the location) - all caching here does is
  // avoid the app message backandforth
  if(s_cached_lat == CONST_0 || s_cached_lon == CONST_0) {
    APP_LOG(APP_LOG_LEVEL_INFO, 
            "FilterBusesByCachedLocation: trigger location request");
    SendAppMessageGetLocation();
  }
  else {
    FilterBusesByLocation(s_cached_lat, s_cached_lon, buses);
  }
}

// static void SendAppMessageUpdateArrivals(Buses* buses);

// static void SendAppMessageUpdateArrivalsCallback(void* context) {
//   SendAppMessageUpdateArrivals(context);
// }

static void SendAppMessageUpdateArrivals(Buses* buses) {
  // if(s_outstanding_requests > 0) {
  //   // wait until the app message queue is clear
  //   APP_LOG(APP_LOG_LEVEL_INFO, "SendAppMessageUpdateArrivals: ...waiting...");
  //   app_timer_register(1500, SendAppMessageUpdateArrivalsCallback, buses);
  //   return;
  // }

  CancelOutstandingRequests();

  // make sure any new/removed buses are (in)visible as they should be
  FilterBusesByCachedLocation(buses);

  // clear temp arrivals before getting new ones
  ArrivalsDestructor(&s_temp_arrivals);

  // build the strings of stop/route pairs
  char* busList = NULL;
  for(uint i = 0; i < buses->filter_count; i++) {
    uint32_t b = buses->filter_index[i];
    if(b >= buses->count) {
      APP_LOG(APP_LOG_LEVEL_ERROR, 
              "Critical error! Filtered bus index out of range.");
      return;
    }
    char* stop = buses->data[b].stop_id;
    char* route = buses->data[b].route_id;
    char* bus = NULL;
    if(busList == NULL) {
      uint size = strlen(stop)+strlen(route)+2;
      bus = malloc(size);
      snprintf(bus, size, "%s,%s", stop, route);
    }
    else {
      uint size = strlen(busList)+strlen(stop)+strlen(route)+3;
      bus = malloc(size);
      snprintf(bus, size, "%s|%s,%s", busList, stop, route);
    }
    
    free(busList);
    busList = bus;
  }

  if(busList != NULL) {
    // s_transaction_id += 1;
    APP_LOG(APP_LOG_LEVEL_INFO,
            "----Initiated transaction id: %u",
            (uint)s_transaction_id);

    s_outstanding_requests = buses->filter_count;
    
    // Prepare dictionary
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);

    // Write data
    dict_write_uint32(iterator, kAppMessageMessageType, kAppMessageArrivalTime);
    dict_write_cstring(iterator, kAppMessagebusList, busList);
    dict_write_uint32(iterator, kAppMessageTransactionId, s_transaction_id);

    // Send data
    app_message_outbox_send();
    
    free(busList);
  }
}

void UpdateArrivals(AppData* appdata) {
  if((appdata->buses.count == 0) || appdata->show_settings) {
    // reset/clear arrivals if there are no buses at all, or a reset of the
    // view has been triggered
    ArrivalsDestructor(&appdata->arrivals);
  }
  else {
    SendAppMessageUpdateArrivals(&appdata->buses);
  }
}

static void HandleAppMessageArrivalTime(DictionaryIterator *iterator,
                                        void *context) {

  Tuple *stop_id_tuple = dict_find(iterator, kAppMessageStopId);
  Tuple *route_id_tuple = dict_find(iterator, kAppMessageRouteId);
  Tuple *arrival_delta_tuple = dict_find(iterator, kAppMessageArrivalDelta);
  Tuple *scheduled_tuple = dict_find(iterator, kAppMessageScheduled);
  Tuple *predicted_tuple = dict_find(iterator, kAppMessagePredicted);
  Tuple *arrivalCode_tuple = dict_find(iterator, kAppMessageArrivalCode);
  Tuple *arrival_delta_string_tuple = dict_find(iterator, 
      kAppMessageArrivalDeltaString);
  Tuple *transaction_id_tuple = dict_find(iterator, kAppMessageTransactionId);
  Tuple *items_remaining_tuple = dict_find(iterator, kAppMessageItemsRemaining);
  Tuple *trip_id_tuple = dict_find(iterator, kAppMessageTripId);

  // TODO; stop passing bus index around.
  if(stop_id_tuple && route_id_tuple && arrival_delta_tuple &&
     scheduled_tuple && predicted_tuple && arrivalCode_tuple &&
     arrival_delta_string_tuple && transaction_id_tuple && 
     items_remaining_tuple && trip_id_tuple) {

    AppData* appdata = context;

    // active transaction?
    if(transaction_id_tuple->value->uint32 == s_transaction_id) {
      // completed request check
      if(items_remaining_tuple->value->uint32 == 0) {
        if(s_outstanding_requests > 0) {
          s_outstanding_requests -= 1;
        }
        // APP_LOG(APP_LOG_LEVEL_INFO, "Requests outstanding: %u",
        //   (uint)s_outstanding_requests);
      }
      else {
        // add the arrival if completion is not signaled
        AddArrival(stop_id_tuple->value->cstring,
                   route_id_tuple->value->cstring,
                   trip_id_tuple->value->cstring,
                   scheduled_tuple->value->cstring,
                   predicted_tuple->value->cstring,
                   arrival_delta_string_tuple->value->cstring,
                   arrival_delta_tuple->value->int32,
                   *(arrivalCode_tuple->value->cstring),
                   &appdata->buses,
                   &s_temp_arrivals);
      }

      if(s_outstanding_requests == 0) {
        APP_LOG(APP_LOG_LEVEL_INFO, 
                "----Completed transaction id: %u",
                (uint)s_transaction_id);

        MainWindowUpdateArrivals(&s_temp_arrivals, appdata);
         
        // prevent a double free later
        s_temp_arrivals.data = NULL;
        s_temp_arrivals.count = 0;
      }
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Missing data - kAppMessageArrivalTime!");
  }
}

static void HandleAppMessageNearbyStops(DictionaryIterator *iterator,
                                        void *context) {
      
  Tuple *stop_id_tuple = dict_find(iterator, kAppMessageStopId);
  Tuple *items_remaining_tuple = dict_find(iterator, kAppMessageItemsRemaining);
  Tuple *stop_name_tuple = dict_find(iterator, kAppMessageStopName);
  Tuple *route_list_string_tuple = dict_find(iterator, 
      kAppMessageRouteListString);
  Tuple *lat_tuple = dict_find(iterator, kAppMessageLat);
  Tuple *lon_tuple = dict_find(iterator, kAppMessageLon);
  Tuple *direction_tuple = dict_find(iterator, kAppMessageDirection);
  Tuple *transaction_id_tuple = dict_find(iterator, kAppMessageTransactionId);
      
  if(stop_id_tuple && items_remaining_tuple && stop_name_tuple &&
     route_list_string_tuple && lat_tuple && lon_tuple
     && direction_tuple && transaction_id_tuple) {

    AppData* appdata = context;
    // active transaction? user canceled settings menu?
    if((transaction_id_tuple->value->uint32 == s_transaction_id)
       && appdata->show_settings) {

      // TODO: A better way to resolve this would be to up the transaction
      // id upon canceled transactions instead of checking to see if 
      // showing settings - scenario: user initates the settings load,
      // but then cancels out... don't want the settings showing up after
      // they've been canceled
      
      double lat, lon;
      memcpy(&lat, lat_tuple->value->data, sizeof(double));
      memcpy(&lon, lon_tuple->value->data, sizeof(double));
      sll sll_lat = dbl2sll(lat);
      sll sll_lon = dbl2sll(lon);
      
      // TODO: check for cohearancy - don't just create a stop
      // every time, but check that it's happening in order.
      AddStop(stop_id_tuple->value->cstring,
              stop_name_tuple->value->cstring, 
              route_list_string_tuple->value->cstring,
              sll_lat, 
              sll_lon, 
              direction_tuple->value->cstring, 
              &s_nearby_stops);

      // TODO: COULD start showing settings at this point,
      // if we weren't passing in the nearby routes by copy to the
      // settigns menu (they're not loaded yet) -
      // need a different way to pass s_nearby_routes if we're
      // gonna go to settings before they're ready. V2
    }
    else {
      // another transaction has been initiated or the user has canceled
      // the showing of settings.

      // TODO: ideally, we'd send a message to the JS to tell it to stop
      // the process of sending routes & stops because the user has canceled
      // and it's wasted work. Probably wouldn't trigger that from here, though.
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Missing data - kAppMessageNearbyStops!");
  }
}

static void HandleAppMessageNearbyRoutes(
    DictionaryIterator *iterator,
    void *context,
    const uint32_t message_type) {
      
  Tuple *items_remaining_tuple = dict_find(iterator, kAppMessageItemsRemaining);
  Tuple *route_id_tuple = dict_find(iterator, kAppMessageRouteId);
  Tuple *route_name_tuple = dict_find(iterator, kAppMessageRouteName);
  Tuple *stop_id_list_tuple = dict_find(iterator, kAppMessageStopIdList);
  Tuple *description_tuple = dict_find(iterator, kAppMessageDescription);
  Tuple *transaction_id_tuple = dict_find(iterator, kAppMessageTransactionId);

  if(items_remaining_tuple && route_id_tuple && route_name_tuple
     && stop_id_list_tuple && description_tuple && transaction_id_tuple) {

    AppData* appdata = context;
    // active transaction? user canceled settings menu?
    if(transaction_id_tuple->value->uint32 == s_transaction_id) {
       
      // TODO: A better way to resolve this would be to up the transaction
      // id upon canceled transactions instead of checking to see if 
      // showing settings - scenario: user initates the settings load,
      // but then cancels out... don't want the settings showing up after
      // they've been canceled

      uint32_t items = items_remaining_tuple->value->uint32;

      APP_LOG(APP_LOG_LEVEL_INFO, 
               "HandelAppMessageNearbyRoutes - items %u", 
               (uint)items);


      if(items == 0) {
        APP_LOG(APP_LOG_LEVEL_INFO, 
                "kAppMessageNearbyRoutes - last route returned.");

        s_outstanding_requests = 0;

        // get rid of the progress window and show the settings window
        if(appdata->show_settings) {
          AppData* appdata = context;
          if(message_type == kAppMessageNearbyRoutes) {
            SettingsStopsStart(s_nearby_stops, 
                              s_nearby_routes, 
                              &appdata->buses);
          }
          else {
            // kAppMessageRoutesForStop
            SettingsRoutesStart(s_nearby_stops.data[0], 
                                s_nearby_routes, 
                                &appdata->buses);
          }
          
          ProgressWindowRemove();
        }
      }
      else {
        AddRoute(route_id_tuple->value->cstring, 
                 route_name_tuple->value->cstring, 
                 stop_id_list_tuple->value->cstring, 
                 description_tuple->value->cstring,
                 &s_nearby_routes);
      }
    }
    else {
      // another transaction has been initiated or the user has canceled
      // the showing of settings.
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Missing data - kAppMessageNearbyRoutes!");
  }
}

static void HandleAppMessageLocation(DictionaryIterator *iterator,
                                     void *context) {

  Tuple *lat_tuple = dict_find(iterator, kAppMessageLat);
  Tuple *lon_tuple = dict_find(iterator, kAppMessageLon);

  if(lat_tuple && lon_tuple) {
    double lat, lon;
    memcpy(&lat, lat_tuple->value->data, sizeof(double));
    memcpy(&lon, lon_tuple->value->data, sizeof(double));
    s_cached_lat = dbl2sll(lat);
    s_cached_lon = dbl2sll(lon);

    AppData* appdata = context;
    appdata->initialized = true;

    s_outstanding_requests = 0;

    // update bus arrival time
    UpdateArrivals(appdata);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Missing data - kAppMessageLocation!");
  }
}

static void HandleAppMessageError(DictionaryIterator *iterator,
                                  void *context) {

  Tuple *description_tuple = dict_find(iterator, kAppMessageDescription);

  if(description_tuple) {
    ErrorWindowPush(description_tuple->value->cstring, true);
  }
  else {
    ErrorWindowPush(DIALOG_MESSAGE_GENERAL_ERROR, true);
  }
}

static void InboxReceivedCallback(DictionaryIterator *iterator,
                                  void *context) {

  Tuple *message_type_tuple = dict_find(iterator, kAppMessageMessageType);
  
  if(message_type_tuple) {
    uint32_t message_type = message_type_tuple->value->uint32;

    switch(message_type) {
      case kAppMessageArrivalTime:
        HandleAppMessageArrivalTime(iterator, context);
        break;
      case kAppMessageNearbyStops:
        HandleAppMessageNearbyStops(iterator, context);
        break;
      case kAppMessageNearbyRoutes:
      case kAppMessageRoutesForStop:
        HandleAppMessageNearbyRoutes(iterator, context, message_type);
        break;
      case kAppMessageLocation:
        HandleAppMessageLocation(iterator, context);
        break;
      case kAppMessageError:
        HandleAppMessageError(iterator, context);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Invalid message type received!");
        break;
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No message type received!");
  }
}

static void InboxDroppedCallback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Incoming message dropped!");
  APP_LOG(APP_LOG_LEVEL_INFO, 
          "In dropped: %i - %s",
          reason, 
          TranslateError(reason));
}

static void OutboxFailedCallback(DictionaryIterator *iterator,
                                 AppMessageResult reason,
                                 void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
  APP_LOG(APP_LOG_LEVEL_INFO, 
          "In failed: %i - %s", 
          reason, 
          TranslateError(reason));
      
  // Retry forever
  // TODO: consider limiting this so it doesn't retry forever.
  app_message_outbox_begin(&iterator);
  app_message_outbox_send();
}

static void OutboxSentCallback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void CommunicationInit(AppData* appdata) {
  s_timer = NULL;
  StopsInit(&s_nearby_stops);
  RoutesInit(&s_nearby_routes);
  ArrivalsInit(&s_temp_arrivals);
  s_cached_lat = CONST_0;
  s_cached_lon = CONST_0;
  s_outstanding_requests = 0;
  s_transaction_id = 0;
  s_skipped_arrival_updates = 0;
  s_last_outstanding_request_at_skipped = 0;
  
    // Register callbacks
  app_message_set_context(appdata);
  app_message_register_inbox_received(InboxReceivedCallback);
  app_message_register_inbox_dropped(InboxDroppedCallback);
  app_message_register_outbox_failed(OutboxFailedCallback);
  app_message_register_outbox_sent(OutboxSentCallback);

  // Open app message
  app_message_open(1024, 1024);

}

void CommunicationDeinit() {
  StopArrivalsUpdateTimer();
  StopsDestructor(&s_nearby_stops);
  RoutesDestructor(&s_nearby_routes);
  ArrivalsDestructor(&s_temp_arrivals);
  app_message_deregister_callbacks();
}