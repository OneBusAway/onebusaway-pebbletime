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
 
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <pebble.h>
#include "buses.h"
#include "appdata.h"

#define DIALOG_MESSAGE_BLUETOOTH_ERROR "Bluetooth Disconnected\n\nReconnect phone to continue"
#define DIALOG_MESSAGE_GENERAL_ERROR "Something went wrong\n\nSorry - Please try again"

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
  kAppMessagePLACEHOLDER, // unused
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
  kAppMessageCount,
  kAppMessageRadius,
  kAppMessageIsArrival
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