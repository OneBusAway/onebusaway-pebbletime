// import API keys
var OBA_SERVER = '';
var OBA_API_KEY = '48d59e79-ed33-4be0-9db3-912f8f521fec';

// import test data
// TODO: don't make this a 'require' find a better way to do test 'hooks'
var test = require('./test');
var test_lat = test.lat;
var test_lon = test.lon;

var DIALOG_GPS_ERROR =
    "Location Error\n\nCheck phone GPS settings & signal";
var DIALOG_INTERNET_ERROR =
    "Connection failure\n\nCheck phone internet connection";
var GPS_TIMEOUT = 15000;
var GPS_MAX_AGE = 60000;
var APP_MESSAGE_MAX_ATTEMPTS = 7;
var APP_MESSAGE_TIMEOUT = 2000;
var HTTP_MAX_ATTEMPTS = 7;
var HTTP_RETRY_TIMEOUT = 2000;
var HTTP_REQUEST_TIMEOUT = 7500;

var arrivalsJsonCache = {};
var stopsJsonCache = {};
var currentTransaction = -1;

/** Extend Number object with method to convert numeric degrees to radians */
if (Number.prototype.toRadians === undefined) {
    Number.prototype.toRadians = function() { return this * Math.PI / 180; };
}

/**
 * Calculate the distance between two gps coordinates.
 * http://www.movable-type.co.uk/scripts/latlong.html
 */
function DistanceBetween(lat1, lon1, lat2, lon2) {
  var R = 6371; // kilometres
  var φ1 = lat1.toRadians();
  var φ2 = lat2.toRadians();
  var Δφ = (lat2-lat1).toRadians();
  var Δλ = (lon2-lon1).toRadians();

  var a = Math.sin(Δφ/2) * Math.sin(Δφ/2) +
          Math.cos(φ1) * Math.cos(φ2) *
          Math.sin(Δλ/2) * Math.sin(Δλ/2);
  var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));

  return R * c;
}

/**
 * Get the OBA regions via the regions API
 */
function setObaServerAndSendLocation(lat, lon) {
  var url = 'http://regions.onebusaway.org/regions-v3.json';
  xhrRequest(url, 'GET',
    function(responseText) {
      var json = JSON.parse(responseText);
      if(json.data !== null && json.data.hasOwnProperty("list")) {
        var regions = json.data.list;
        setRegion(regions, lat, lon);
      }
    }
  )
}

/**
 * Get the distance from a location and the closest bound in a region
 */
function getRegionDistance(region, lat, lon) {
  var distance = -1;
  for(b in region.bounds) {
    var bound = region.bounds[b];
    var bound_distance = DistanceBetween(lat, lon, bound.lat, bound.lon);
    if((distance == -1) || (bound_distance < distance)) {
      distance = bound_distance;
    } 
  }
  return distance;
}

/**
 * Sets the global OBA_SERVER and OBA_API_KEY vars to that of the closest
 * region server to (lat, lon)
 */
function setRegion(regions, lat, lon) {
  var distance = -1;
  for(var r in regions) {
    var region_distance = getRegionDistance(regions[r], lat, lon);
    if((distance == -1) || (region_distance < distance)) {
      distance = region_distance;
      OBA_SERVER = regions[r].obaBaseUrl;
    }
  }

  // TODO: consider storing the last known OBA_SERVER and restoring it
  // here in case the regions API isn't working...
  console.log("Setting OBA server: " + OBA_SERVER);
  
  getLocationSuccess(lat, lon);
}

/** Convert a decimal value to a C-compatible 'double' byte array */
function DecimalToDoubleByteArray(value) {
  var buffer = new ArrayBuffer(8);
  var floatArray = new Float64Array(buffer);
  floatArray[0] = value;

  var ret_bytes = [];

  var byteArray = new Uint8Array(buffer);
  for(var i=0; i < 8; ++i) {
    ret_bytes.push(byteArray[i]);
  }

  return ret_bytes;
}


function randomIntFromInterval(min,max) {
  return Math.floor(Math.random()*(max-min+1)+min);
}

/** Send an AppMessage dictionary to the watch */
function sendAppMessage(dictionary, successFunction) {
  var attempts = 0;

  function send() {
    if(attempts < APP_MESSAGE_MAX_ATTEMPTS) {
      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          successFunction();
        },
        function(e) {
          console.log('sendAppMessage: Error @ attempt: ' + attempts);
          attempts += 1;
          setTimeout(function() { send(); },
            randomIntFromInterval(0, APP_MESSAGE_TIMEOUT*attempts));
        }
      );
    }
    else {
      console.log("sendAppMessage: Failed sending AppMessage. Bailing." +
                  "Content:" + JSON.stringify(dictionary));
    }
  }

  send();
}

/** Trigger an error message on the watch */
function sendError(message) {
  // data to send back to watch
  var dictionary = {
    'AppMessage_messageType': 4 // Error
  };

  if(message) {
    dictionary.AppMessage_description = message;
  }

  console.log("sendError:\n  " + JSON.stringify(dictionary));

  // Send to Pebble
  sendAppMessage(dictionary, function() { });
}

/** Web request with backoff and retry */
function xhrRequest(url, type, callback) {
  var attempts = 0;

  function xhrRequestRetry() {
    attempts += 1;
    if(attempts < HTTP_MAX_ATTEMPTS) {
      setTimeout(function() {
        xhrRequestDo(); },
        randomIntFromInterval(0, HTTP_RETRY_TIMEOUT*attempts));
    }
    else {
      console.log('xhrRequest: Failed after ' + attempts +
                  ' attempts. Bailing.');
      sendError(DIALOG_INTERNET_ERROR  + "\n\n0x0000");
    }
  }

  function xhrRequestDo() {
    // console.log('xhrRequest: starting ' + url);
    var xhr = new XMLHttpRequest();
    xhr.timeout = HTTP_REQUEST_TIMEOUT*(attempts+1);
    xhr.onload = function () {
      if(this.status == 200) {
        //console.log('xhrRequest: success 200');
        callback(this.responseText);
      }
      else {
        console.log('xhrRequest: error - not 200');
        xhrRequestRetry();
      }
    };
    xhr.onerror = function (e) {
      console.log('xhrRequest: error - unknown');
      xhrRequestRetry();
    };
    xhr.ontimeout = function (f) {
      console.log('xhrRequest: error - timeout');
      xhrRequestRetry();
    };
    xhr.open(type, url);
    xhr.send();
  }
  xhrRequestDo();
}

function sign(x) { return x > 0 ? '' : x < 0 ? '-' : ''; }

function millisToMinutesAndSeconds(millis) {
  var m = Math.abs(millis);
  var minutes = Math.floor(m / 60000);
  var seconds = ((m % 60000) / 1000).toFixed(0);
  return sign(millis) + minutes + ":" + (seconds < 10 ? '0' : '') + seconds;
}

/**
 * Convert milliseconds into human readable time, subsituting 'Now' for
 * times below |1 min|
 */
function millisToMinutesAndSecondsOba(millis) {
  var m = Math.abs(millis);
  var minutes = Math.floor(m / 60000);
  var seconds = ((m % 60000) / 1000).toFixed(0);
  if (minutes > 0) {
    return sign(millis) + minutes + ":" + (seconds < 10 ? '0' : '') + seconds;
  }
  else {
    return "Now";
  }
}

function formatTimeHHMMA(date) {
  function minFormat(n){return (n<10?'0':'')+n}
  var h = date.getHours();
  return (h%12 || 12) + ':' + 
      minFormat(date.getMinutes()) + (h<12? 'AM' :'PM');
}

/**
 * Sends the completion signal for an arrivals request on specific bus, starts
 * the request for the next bus in the busArray
 */
function completeArrivalsRequest(busArray, transactionId) {
  // signal completion of this request
  var dictionary = {
    'AppMessage_stopId': 0,
    'AppMessage_routeId': 0,
    'AppMessage_tripId': 0,
    'AppMessage_arrivalDelta': 0,
    'AppMessage_arrivalDeltaString': 0,
    'AppMessage_itemsRemaining': 0,
    'AppMessage_transactionId': transactionId,
    'AppMessage_arrivalCode': 's',
    'AppMessage_scheduled': "",
    'AppMessage_predicted': "",
    'AppMessage_messageType': 0 // arrival time
  };

  // Send to Pebble
  sendAppMessage(dictionary,
    function(e) {
      // get the arrivals for the next bus in the request
      getArrivals(busArray, transactionId);
    }
  );
}

/**
 * Sends bus arrival information from 'arrivals' for the 'bus' to the watch,
 * one arrival at a time. When the current arrivals have been exhausted,
 * starts the process of getting the arrivals fort he next bus in 'busArray'
 */
function getNextArrival(bus, busArray, arrivals, currentTime, transactionId) {
  var stopId = bus.stopId;
  var routeId = bus.routeId;

  // find the next arrival for this bus
  var arrival = arrivals.shift();
  while(arrival && arrival.routeId && arrival.routeId != routeId) {
    arrival = arrivals.shift();
  }

  // if we found an arrival for the bus, send it along,
  // or, the transaction has not been canceled
  if(arrival && arrival.routeId && (arrival.routeId == routeId) &&
     (transactionId == currentTransaction)) {
    var arrivalDelta = 0;
    var arrivalTime = 0;
    var scheduledArrivalTime = arrival.scheduledArrivalTime;
    var predictedArrivalTime = arrival.predictedArrivalTime;

    var arrivalCode = 's';

    var scheduled_string = formatTimeHHMMA(new Date(scheduledArrivalTime));
    var predicted_string = "n/a";

    if(predictedArrivalTime !== undefined && predictedArrivalTime !== 0) {
      arrivalTime = predictedArrivalTime;
      var schedule_difference = predictedArrivalTime - scheduledArrivalTime;
      predicted_string = formatTimeHHMMA(new Date(predictedArrivalTime));

      // set arrival status
      if(schedule_difference > 60000) {
        arrivalCode = 'l'; // late by over 1 min
      }
      else if(schedule_difference < -60000) {
        arrivalCode = 'e'; // early by more than 1 min
      }
      else {
        arrivalCode = 'o';
      }
    }
    else {
      // arrival time is unknown
      arrivalTime = scheduledArrivalTime;
    }

    arrivalDelta = arrivalTime - currentTime;

    // data to send back to watch
    var dictionary = {
      'AppMessage_stopId': stopId,
      'AppMessage_routeId': routeId,
      'AppMessage_tripId': arrival.tripId,
      'AppMessage_arrivalDelta': arrivalDelta,
      'AppMessage_arrivalDeltaString':
          millisToMinutesAndSecondsOba(arrivalDelta),
      'AppMessage_itemsRemaining': 1,
      'AppMessage_transactionId': transactionId,
      'AppMessage_arrivalCode': arrivalCode,
      'AppMessage_scheduled': scheduled_string,
      'AppMessage_predicted': predicted_string,
      'AppMessage_messageType': 0 // arrival time
    };

    // console.log("getNextArrival:\n  " + JSON.stringify(dictionary));

    // Send to Pebble
    sendAppMessage(dictionary,
      function(e) {
        getNextArrival(bus, busArray, arrivals, currentTime, transactionId);
      }
    );
  }
  else {
    completeArrivalsRequest(busArray, transactionId);
  }
}

/**
 * Parse json 'responseText' from the arrivals OBA call for 'bus' and
 * trigger the sending of the arrivals info to the watch
 */
function processArrivalsResponse(bus, busArray, transactionId, responseText) {
  // responseText contains a JSON object
  var json = JSON.parse(responseText);

  if(json.data !== null && json.currentTime !== null) {
    var arrivalsAndDepartures;
    if(json.data.hasOwnProperty("entry") &&
      json.data.entry.hasOwnProperty("arrivalsAndDepartures")) {
      arrivalsAndDepartures = json.data.entry.arrivalsAndDepartures;
    }
    else if (json.data.hasOwnProperty("arrivalsAndDepartures")) {
      // special case for New York (MTA)
      arrivalsAndDepartures = json.data.arrivalsAndDepartures;
    }
    var currentTime = json.currentTime;

    // arrivalsAndDepartures can be zero length; it does not represent
    // an unrecoverable error
    getNextArrival(bus,
                   busArray,
                   arrivalsAndDepartures,
                   currentTime,
                   transactionId);
  }
  else {
  //   sendError(DIALOG_INTERNET_ERROR + "\n\n0x0001");
    completeArrivalsRequest(busArray, transactionId);
  }
}

/**
 * for each bus in the 'busArray' get the bus' arrivals at it's stop and send
 * them back to the watch - since this can result in multiple calls to get
 * arrivals and departures data for the same stop from OBA, the results
 * are cached for each transaction with the watch
 */
function getArrivals(busArray, transactionId) {
  var bus = busArray.shift();

  if(bus) {
    var stopId = bus.stopId;

    if(arrivalsJsonCache[stopId]) {
      processArrivalsResponse(bus, busArray, transactionId, arrivalsJsonCache[stopId]);
    }
    else {
      var url = OBA_SERVER + '/api/where/arrivals-and-departures-for-stop/' +
        stopId + '.json?key=' + OBA_API_KEY;

      // Send request to OneBusAway
      xhrRequest(url, 'GET',
        function(responseText) {
          arrivalsJsonCache[stopId] = responseText;
          processArrivalsResponse(bus, busArray, transactionId, responseText);
        }
      );
    }
  }
}


/**
 * build an array of route & stop pairs representing a bus from the
 * delimited list sent from the watch
 */
function parseBusList(busList) {
  // parse out the bus list (| separate "stopId,routeId" pairs)
  var busPairs = busList.split("|");
  var busArray = [];
  for(var i = 0; i < busPairs.length; i++) {
    var pair = busPairs[i].split(",");
    var bus = {
      "stopId":pair[0],
      "routeId":pair[1]
    };
    busArray.push(bus);
  }
  return busArray;
}

/**
 * send a message signifying the end of get routes transaction
 */
function sendEndOfRoutes(transactionId, messageType) {
  var dictionary = {
    'AppMessage_routeId': 0,
    'AppMessage_routeName': '',
    'AppMessage_itemsRemaining': 0,
    'AppMessage_description': '',
    'AppMessage_transactionId': transactionId,
    'AppMessage_messageType': messageType
  };

  sendAppMessage(dictionary,
    function() {
      console.log('sendEndOfRoutes: transactionId ' + transactionId);
    }
  );
}

/**
 * send a message signifying the end of get stops transaction
 */
function sendEndOfStops(transactionId) {
  // data to send back to watch
  var dictionary = {
    'AppMessage_stopId': 0,
    'AppMessage_stopName': "",
    'AppMessage_lat': 0,
    'AppMessage_lon': 0,
    'AppMessage_itemsRemaining': 0,
    'AppMessage_routeListString': "",
    'AppMessage_direction': "",
    'AppMessage_transactionId': transactionId,
    'AppMessage_messageType': 1, // nearby stops
    'AppMessage_index': 0,
    'AppMessage_count': 0
  };

  // Send to Pebble
  sendAppMessage(dictionary,
    function() {
      console.log('sendEndOfStops: transactionId ' + transactionId);
    }
  );
}

/**
 * Send the list of nearby routes as requested by the watch settings menu
 */
function sendRoutesToPebble(routes, transactionId, messageType) {
  console.log("#routes: " + routes.length);

  if((routes.length === 0) || (transactionId != currentTransaction)) {
    // completed sending routes to the pebble, or the transaction
    // was canceled
    // console.log("sendRoutesToPebble We're done here...");
    sendEndOfRoutes(transactionId, messageType);
    return;
  }

  // pull the next route
  var route = routes.shift();

  if(route.id) {
    var name = route.shortName ? route.shortName : route.longName;
    name = name ? name.toUpperCase() : 'Unknown';

    var description = route.description ? route.description : route.longName;

    // data to send back to watch
    var dictionary = {
      'AppMessage_routeId': route.id,
      'AppMessage_routeName': name,
      'AppMessage_itemsRemaining': 1, // positive int == not done
      'AppMessage_description': description,
      'AppMessage_transactionId': transactionId,
      'AppMessage_messageType': messageType // nearby routes or routes for stop
    };

    console.log('sendRoutesToPebble: sending - ' + route.id + ',' + name + ',' +
                description);

    sendAppMessage(dictionary,
      function() {
        sendRoutesToPebble(routes, transactionId, messageType);
      }
    );
  }
  else {
    console.log('sendRoutesToPebble: missing route id in JSON. transactionId: ' +
                transactionId);
    sendEndOfRoutes(transactionId, messageType);
  }
}

/**
 * Send the list of nearby stops as requested by the watch settings menu
 */
function sendStopsToPebble(stops,
                           routeStrings,
                           transactionId,
                           index,
                           index_end) {

  if(stops.length == 0) {
    // special case for no stops to send
    sendEndOfStops(transactionId);
    return;
  }

  if((stops.length <= index) || 
     (index > index_end) || 
     (transactionId != currentTransaction)) {
    // completed sending stops to the pebble
    console.log("sendStopsToPebble: done.");
    return;
  }

  // pull the next stop
  var stop = stops[index];

  if(stop.id && stop.name) {
    var direction = stop.direction;

    var routeList = routeStrings[stop.id];

    // data to send back to watch
    var dictionary = {
      'AppMessage_stopId': stop.id,
      'AppMessage_stopName': stop.name,
      'AppMessage_lat': DecimalToDoubleByteArray(stop.lat),
      'AppMessage_lon': DecimalToDoubleByteArray(stop.lon),
      'AppMessage_itemsRemaining': index_end - index,
      'AppMessage_routeListString': routeList,
      'AppMessage_direction': direction,
      'AppMessage_transactionId': transactionId,
      'AppMessage_messageType': 1, // nearby stops
      'AppMessage_index': index,
      'AppMessage_count': stops.length
    };

    console.log('sendStopsToPebble: sending - ' + index + ") " + stop.id + ',' +
      stop.name + ',' + routeList);

    // Send to Pebble
    sendAppMessage(dictionary,
      function() {
        sendStopsToPebble(stops, routeStrings, transactionId, index+1, index_end);
      }
    );
  }
  else {
    console.log("sendStopsToPebble: done (with error).");
  }
}

/**
 * return an index of routes per stop from OBA nearby stops json
 */
function buildStopRouteStrings(json) {
  var stringTable = {};

  if(json.data === null) {
    return stringTable;
  }

  if(json.data.hasOwnProperty('references')) {
    var routes = json.data.references.routes;
    var routeTable = {};
    for(var j = 0; j < routes.length; j++) {
      // console.log('routes: ' + routes[j].id + ' ' + routes[j].shortName);
      var name = routes[j].shortName ? routes[j].shortName : routes[j].longName;
      routeTable[routes[j].id] = name.toUpperCase();
    }

    var stops = json.data.list;
    for(var i = 0; i < stops.length; i++) {
      var routeString = "";

      // console.log('bulidStopRouteID: ' + JSON.stringify(stops[i].routeIds));

      for(var r = 0; r < stops[i].routeIds.length; r++) {
        if(r === 0) {
          routeString = routeTable[stops[i].routeIds[r]];
        }
        else {
          routeString = routeString + "," + routeTable[stops[i].routeIds[r]];
        }
      }
      // console.log("stop: " + stops[i].id + " routestring: " + routeString);
      stringTable[stops[i].id] = routeString;
    }
  }
  else {
    // special case for New York (MTA) which does not provide <references>,
    // among other annoyances
    var stops = json.data.stops;
    for(var i = 0; i < stops.length; i++) {
      var routeString = "";
      var stop = stops[i];
      for(var r = 0; r < stop.routes.length; r++) {
        var route = stop.routes[r];
        var name = route.shortName ? route.shortName : route.longName;
        name = name.toUpperCase();
        if(r === 0) {
          routeString = name;
        }
        else {
          routeString = routeString + "," + name;
        }
      }
      console.log("stop: " + stops[i].id + " routestring: " + routeString);
      stringTable[stops[i].id] = routeString;
    }
  }

  return stringTable;
}

/**
 * constructs the list of routes
 */
function buildRoutesList(json) {
  var routes = [];

  if(json.data === null) {
    return routes;
  }

  if(json.data.hasOwnProperty('references')) {
    routes = json.data.references.routes;
  }
  else {
    var routehash = {};
    var stops = json.data.stops;
    for(var i = 0; i < stops.length; i++) {
      for(var r = 0; r < stops[i].routes.length; r++) {
        var route = stops[i].routes[r];
        routehash[route.id] = route;
      }
    }
    for(r in routehash) {
      routes.push(routehash[r]);
    }
  }
  console.log(routes);
  return routes;
}

/**
 * sends the current GPS coordinates to the watch
 */
function getLocationSuccess(lat, lon) {
  var dictionary = {
    'AppMessage_lat': DecimalToDoubleByteArray(lat),
    'AppMessage_lon': DecimalToDoubleByteArray(lon),
    'AppMessage_messageType': 3 // location
  };

  sendAppMessage(dictionary, 
    function(e) { console.log('getLocationSuccess: send success'); });
}

/**
 * parse json from nearby stops request and send stops to the watch
 */
function sendStopsToPebbleJson(json, transactionId, index, index_end) {
  var routeStrings = buildStopRouteStrings(json);

  var stops;
  if(json.data.hasOwnProperty('references')) {
    stops = json.data.list;
  }
  else {
    // New York (MTA) special case
    stops = json.data.stops;
  }

  // set the end index to match the size of the stops array
  if(stops.length <= index_end) {
    index_end = stops.length - 1;
  }

  sendStopsToPebble(stops, routeStrings, transactionId, index, index_end);
}

/**
 * requests the bus stops near the current GPS coordinates and sends the
 * stops and routes to the watch
 */
function getNearbyStopsLocationSuccess(pos, transactionId, index, index_end, radius) {
  var lat = pos.coords.latitude;
  var lon = pos.coords.longitude;

  // test code
  if(typeof test_lat !== 'undefined' && typeof test_lon !== 'undefined') {
    lat = test_lat;
    lon = test_lon;
  }

  // TODO: Make the search radius configurable; experiemented with 200-1000
  var url = OBA_SERVER + '/api/where/stops-for-location.json?key=' +
    OBA_API_KEY + '&lat=' + lat + '&lon=' + lon + '&radius=' + radius;

  // Send request to OneBusAway
  xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object
      var json = JSON.parse(responseText);
      if(json.data !== null) {
        stopsJsonCache = json;
        sendStopsToPebbleJson(stopsJsonCache, transactionId, index, index_end);
      }
      else {
        sendEndOfStops(transactionId);
      }
    }
  );
}

/**
 * requests the routes for a particular stop and sends the routes to the watch
 */
function getRoutesForStop(stopId, transactionId) {
  var url = OBA_SERVER + '/api/where/stop/' + stopId +
    '.json?key=' + OBA_API_KEY;

  // Send request to OneBusAway
  xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object
      var json = JSON.parse(responseText);
      var routes;
      if(json.data !== null) {
        if(json.data.hasOwnProperty('references')) {
          routes = json.data.references.routes;
        }
        else {
          // New York (MTA) special case
          routes = buildRoutesList(json);
        }

        sendRoutesToPebble(routes, transactionId, 5 /*RoutesForStop*/);
      }
      else {
        sendEndOfRoutes(transactionId, 5 /*RoutesForStop*/);
      }
    }
  );
}

/**
 * gets the current GPS location, sends lat/long to the watch
 */
function getLocation() {
  navigator.geolocation.getCurrentPosition(
      function(pos) {
        var lat = pos.coords.latitude;
        var lon = pos.coords.longitude;

        // test code
        if(typeof test_lat !== 'undefined' && typeof test_lon !== 'undefined') {
          lat = test_lat;
          lon = test_lon;
        }

        setObaServerAndSendLocation(lat, lon);
      },
      function(e) {
        console.log("Error requesting location!");
        sendError(DIALOG_GPS_ERROR + "\n\n0x0002");
      },
      {timeout: GPS_TIMEOUT, maximumAge: GPS_MAX_AGE}
  );
}

/**
 * gets the nearby OBA stops based on the current location and sends the
 * results to the watch
 */
function getNearbyStops(transactionId, index, index_end, radius) {
  navigator.geolocation.getCurrentPosition(
      function(pos) {
        getNearbyStopsLocationSuccess(pos, transactionId, index, index_end, radius);
      },
      function(e) {
        console.log("Error requesting location!");
        sendError(DIALOG_GPS_ERROR + "\n\n0x0003");
      },
      {timeout: GPS_TIMEOUT, maximumAge: GPS_MAX_AGE}
  );
}

/**
 * Listen for when the watch app is open and ready, triggers a location update
 * when data can be sent to the watch
 */
Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKit JS ready!');

    // send the current location to the watch
    getLocation();
  }
);

/**
 * Listen for when an AppMessage is received and start the corresponding
 * transaction
 */
Pebble.addEventListener('appmessage',
  function(e) {
    // console.log('AppMessage received: ' + JSON.stringify(e.payload));

    switch(e.payload.AppMessage_messageType) {
      case 0: // get arrival times
        currentTransaction = e.payload.AppMessage_transactionId;
        var busList = parseBusList(e.payload.AppMessage_busList);
        // var halfLength = Math.ceil(busList.length / 2);
        // var leftSide = busList.splice(0,halfLength);
        arrivalsJsonCache = {};
        getArrivals(busList, e.payload.AppMessage_transactionId);
        // getArrivals(leftSide, e.payload.AppMessage_transactionId);
        break;
      case 1: // get nearyby stops
        var transactionId = e.payload.AppMessage_transactionId;
        var index = e.payload.AppMessage_index;
        var index_end = e.payload.AppMessage_count + index - 1;
        var radius = e.payload.AppMessage_radius;
        if(currentTransaction != transactionId) {
          stopsJsonCache = {};
          currentTransaction = transactionId;        
          getNearbyStops(transactionId, index, index_end, radius);
        }
        else {
          sendStopsToPebbleJson(stopsJsonCache, transactionId, index, index_end);
        }
        break;
      case 3: // get location
        getLocation();
        break;
      case 5: // get routes for a stop
        var stopId = e.payload.AppMessage_stopId;
        currentTransaction = e.payload.AppMessage_transactionId;
        getRoutesForStop(stopId, e.payload.AppMessage_transactionId);
        break;
      default:
        console.log('unknown messageType:' + e.payload.AppMessage_messageType);
        break;
    }
  }
);
