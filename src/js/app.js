/**
 * Requires an export from servers.js which provides a dictionary of the form:
 *  'SERVER_URL':{'key':API_KEY, 'lat':LATITUDE, 'lon':LONGITUDE}
*/

// import API keys
var servers = require('./servers').servers;
var OBA_SERVER = '';
var OBA_API_KEY = '';

// import test data
// TODO: don't make this a 'require' find a better way to do test 'hooks'
var test = require('./test');
var test_lat = test.lat;
var test_lon = test.lon;

var DIALOG_GPS_ERROR =
    "Location Error.\n\nCheck phone GPS settings & signal.";
var DIALOG_INTERNET_ERROR =
    "Connection failure.\n\nCheck phone internet connection.";
var GPS_TIMEOUT = 15000;
var GPS_MAX_AGE = 60000;
var APP_MESSAGE_MAX_ATTEMPTS = 7;
var APP_MESSAGE_TIMEOUT = 2000;
var HTTP_MAX_ATTEMPTS = 7;
var HTTP_RETRY_TIMEOUT = 2000;
var HTTP_REQUEST_TIMEOUT = 7500;
var MAX_STOPS = 50;

var arrivalsJsonCache = {};
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
 * Sets the global OBA_SERVER and OBA_API_KEY vars to that of the closest
 * server to (lat, lon)
 */
function setObaServerByLocation(lat, lon) {
  var distance = -1;
  for(var server in servers) {
    var server_distance = DistanceBetween(lat,
                                          lon,
                                          servers[server].lat,
                                          servers[server].lon);
    console.log(server + " distance: " + server_distance);
    if((distance == -1) || (server_distance < distance)) {
      distance = server_distance;
      OBA_SERVER = server;
      OBA_API_KEY = servers[server].key;
    }
  }
  console.log("Setting OBA server: " + OBA_SERVER);
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
    console.log('xhrRequest: starting ' + url);
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

    var scheduled_string =
      (new Date(scheduledArrivalTime)).toLocaleTimeString('en-US',
        { hour12: true, hour: "numeric", minute: "numeric"});
    var predicted_string = "n/a"; //scheduled_string;

    if(predictedArrivalTime !== undefined && predictedArrivalTime !== 0) {
      arrivalTime = predictedArrivalTime;
      var schedule_difference = predictedArrivalTime - scheduledArrivalTime;
      predicted_string =
        (new Date(predictedArrivalTime)).toLocaleTimeString('en-US',
          { hour12: true, hour: "numeric", minute: "numeric"});

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
 * send a message signifying the end of get routes or get stops transaction
 */
function sendEndOfStopsRoutes(transactionId, messageType) {
  var dictionary = {
    'AppMessage_routeId': 0,
    'AppMessage_routeName': '',
    'AppMessage_itemsRemaining': 0,
    'AppMessage_stopIdList': '',
    'AppMessage_description': '',
    'AppMessage_transactionId': transactionId,
    'AppMessage_messageType': messageType
  };

  sendAppMessage(dictionary,
    function() {
      console.log('sendEndOfStopsRoutes: transactionId ' + transactionId);
    }
  );
}

/**
 * Send the list of nearby routes as requested by the watch settings menu
 */
function sendRoutesToPebble(routes, stopStrings, transactionId, messageType) {
  console.log("#routes: " + routes.length);

  if((routes.length === 0) || (transactionId != currentTransaction)) {
    // completed sending routes to the pebble, or the transaction
    // was canceled
    // console.log("sendRoutesToPebble We're done here...");
    sendEndOfStopsRoutes(transactionId, messageType);
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
      'AppMessage_stopIdList': stopStrings[route.id],
      'AppMessage_description': description,
      'AppMessage_transactionId': transactionId,
      'AppMessage_messageType': messageType // nearby routes or routes for stop
    };

    console.log('sendRoutesToPebble: sending - ' + route.id + ',' + name + ',' +
                stopStrings[route.id] + ',' + description);

    sendAppMessage(dictionary,
      function() {
        sendRoutesToPebble(routes, stopStrings, transactionId, messageType);
      }
    );
  }
  else {
    console.log('sendRoutesToPebble: missing route id in JSON. transactionId: ' +
                transactionId);
    sendEndOfStopsRoutes(transactionId, messageType);
  }
}

/**
 * Send the list of nearby stops as requested by the watch settings menu
 */
function sendStopsToPebble(stops,
                           routes,
                           stopStrings,
                           routeStrings,
                           transactionId,
                           index,
                           index_end) {

  if((stops.length <= index) || 
     (index > index_end) || 
     (transactionId != currentTransaction)) {
    // completed sending stops to the pebble; now send corresponding routes.
    console.log("sendStopsToPebble: done.");
    //sendRoutesToPebble(routes, stopStrings, transactionId, 2 /*nearbyRoutes*/);
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
        sendStopsToPebble(stops, routes, stopStrings, routeStrings,
          transactionId, index+1, index_end);
      }
    );
  }
  else {
    console.log("sendStopsToPebble: done (with error).");
    //sendRoutesToPebble(routes, stopStrings, transactionId, 2 /*nearbyRoutes*/);
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
 * return an index of stops per route from OBA nearby stops json
 */
function buildRouteStopStrings(json) {
  var stringTable = {};

  if(json.data === null) {
    return stringTable;
  }

  if(json.data.hasOwnProperty('references')) {
    var stops = json.data.list;
    for(var i = 0; i < stops.length; i++) {
      for(var r = 0; r < stops[i].routeIds.length; r++) {
        var routeId = stops[i].routeIds[r];

        // console.log("RID: " + routeId);

        // parsing code expects all stops to be comma terminated
        if(stringTable[routeId]) {
          stringTable[routeId] = stringTable[routeId] + stops[i].id + ",";
        }
        else {
          stringTable[routeId] = stops[i].id + ",";
        }

        // console.log("ST:" + stringTable[routeId]);
      }
    }
  }
  else {
    // special case for New York (MTA)
    var stops = json.data.stops;
    for(var i = 0; i < stops.length; i++) {
      for(var r = 0; r < stops[i].routes.length; r++) {
        var routeId = stops[i].routes[r].id;

        // parsing code expects all stops to be comma terminated
        if(stringTable[routeId]) {
          stringTable[routeId] = stringTable[routeId] + stops[i].id + ",";
        }
        else {
          stringTable[routeId] = stops[i].id + ",";
        }
      }
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
// TODO: convert to the sendAppMessage function - this is redundant
function getLocationSuccess(attempts, pos) {
  if(attempts < APP_MESSAGE_MAX_ATTEMPTS) {
    var lat = pos.coords.latitude;
    var lon = pos.coords.longitude;

    // test code: set gps coords
    if(typeof test_lat !== 'undefined' && typeof test_lon !== 'undefined') {
      lat = test_lat;
      lon = test_lon;
    }

    var dictionary = {
      'AppMessage_lat': DecimalToDoubleByteArray(lat),
      'AppMessage_lon': DecimalToDoubleByteArray(lon),
      'AppMessage_messageType': 3 // location
    };

    console.log('getLocationSuccess: sending - ' + lat + ',' + lon);

    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log('getLocationSuccess: send success');
      },
      function(e) {
        console.log('getLocationSuccess: Error @ attempt: ' + attempts);

        // reset data for next attempt
        attempts += 1;
        setTimeout(function() {
          getLocationSuccess(attempts, pos);
          }, APP_MESSAGE_TIMEOUT*attempts);
      }
    );
  }
  else {
    console.log("getLocationSuccess: Failed sending AppMessage. Bailing.");
  }
}

/**
 * requests the bus stops near the current GPS coordinates and sends the
 * stops and routes to the watch
 */
function getNearbyStopsLocationSuccess(pos, transactionId, index, index_end) {
  var lat = pos.coords.latitude;
  var lon = pos.coords.longitude;

  // test code
  if(typeof test_lat !== 'undefined' && typeof test_lon !== 'undefined') {
    lat = test_lat;
    lon = test_lon;
  }

  // TODO: Make the search radius configurable; experiemented with 200-1000
  var url = OBA_SERVER + '/api/where/stops-for-location.json?key=' +
    OBA_API_KEY + '&lat=' + lat + '&lon=' + lon + '&radius=250';

  // Send request to OneBusAway
  xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object
      var json = JSON.parse(responseText);

      var routeStrings = buildStopRouteStrings(json);
      var stopStrings = buildRouteStopStrings(json);

      if(json.data !== null) {
        var stops;
        var routes;
        if(json.data.hasOwnProperty('references')) {
          stops = json.data.list;
          routes = json.data.references.routes;
        }
        else {
          // New York (MTA) special case
          stops = json.data.stops;
          routes = buildRoutesList(json);
        }

        // TODO: trim stops to max length to prevent out of memory errors on the
        // watch; or, better solution, paginate the results
        // if(stops.length > MAX_STOPS) {
        //   var diff = stops.length - MAX_STOPS;
        //   console.log("---WARNING--- cutting stop length from " + stops.length +
        //     " to " + MAX_STOPS);
        //   stops.splice((-1)*diff, diff);
        // }

        if(stops.length < index_end) {
          index_end = stops.length - 1;
        }

        // return the full list up to the max length requested by the watch
        sendStopsToPebble(stops, routes, stopStrings, routeStrings,
          transactionId, index, index_end);
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

        // buildRouteStopStrings faked
        var stopStrings = {};
        for(var r = 0; r < routes.length; r++) {
          var routeId = routes[r].id;
          stopStrings[routeId] = stopId + ",";
        }

        sendRoutesToPebble(routes, stopStrings, transactionId, 5 /*RoutesForStop*/);
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

        setObaServerByLocation(lat, lon);
        getLocationSuccess(0, pos);
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
function getNearbyStops(transactionId, index, index_end) {
  navigator.geolocation.getCurrentPosition(
      function(pos) {
        getNearbyStopsLocationSuccess(pos, transactionId, index, index_end);
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
        currentTransaction = e.payload.AppMessage_transactionId;
        var index = e.payload.AppMessage_index;
        var index_end = e.payload.AppMessage_count + index - 1;
        getNearbyStops(e.payload.AppMessage_transactionId, index, index_end);
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
