#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <pebble.h>
#include "buses.h"
#include "appdata.h"

#define DIALOG_MESSAGE_BLUETOOTH_ERROR "Bluetooth Disconnected.\n\nReconnect phone to continue."
#define DIALOG_MESSAGE_GENERAL_ERROR "Something went wrong.\n\nWe're sorry. Please try again."

// AppMessage dictionary keys
enum AppMessageKeys {
  kAppMessageMessageType = 0,
  kAppMessageStopId,
  kAppMessageRouteId,
  kAppMessageArrivalDelta,
  kAppMessagebusIndex,
  kAppMessageItemsRemaining,
  kAppMessageRouteListString,
  kAppMessageStopName,
  kAppMessageStopIdList,
  kAppMessageRouteName,
  kAppMessageDescription,
  kAppMessageLat,
  kAppMessageLon,
  kAppMessageDirection,
  kAppMessageTransactionId,
  kAppMessageArrivalCode,
  kAppMessageScheduled,
  kAppMessagePredicted,
  kAppMessageTripId,
  kAppMessagebusList,
  kAppMessageArrivalDeltaString,
  kAppMessageIndex,
  kAppMessageCount
};

// Enumerations for kAppMessageMessageType
// Defines the types of app messages that can be sent/recieved between
// the watch and the phone
enum AppMessageType {
  kAppMessageArrivalTime = 0,
  kAppMessageNearbyStops,
  kAppMessageNearbyRoutes,
  kAppMessageLocation,
  kAppMessageError,
  kAppMessageRoutesForStop
};

void CommunicationInit(AppData* appdata);
void CommunicationDeinit();
void StartArrivalsUpdateTimer(AppData* appdata);
void StopArrivalsUpdateTimer();
void UpdateArrivals(AppData* appdata);
void SendAppMessageGetNearbyStops(uint16_t index, uint16_t count);
void SendAppMessageInitiateGetNearbyStops(Stops* stops);
void SendAppMessageGetRoutesForStop(Stop* stop);

#endif // COMMUNICATION_H