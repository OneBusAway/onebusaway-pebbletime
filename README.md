# OneBusAway for Pebble

<img src="/screenshots/basicdemo.gif" alt="Full demo" width="144">

OneBusAway for Pebble smart watches provides a OneBusAway experience tailored to the Pebble platform. This app is designed to help you figure out what bus to take as quickly and easily as possible to aid in your everyday commute.

## Supported Pebble Hardware
1. Pebble Time (basalt)
1. Pebble Time Round (chalk)
1. Pebble 2 (diorite)

## Features
1. At a glance, real-time arrival information for public transportation
1. Shows to only 'favorite' stops nearby your current location
1. Favorite buses from multiple nearby stops shown at the same time (to help you decide which bus to go take, even if they're not at the same stop)

# Usage
## At first launch, you will need to add some favorite stops:

<img src="/screenshots/addingfavorites.gif" alt="How to add favorites" width="144">

## Data updates in real time:

<img src="/screenshots/updates-fastplay.gif" alt="Watch as buses get updated" width="144">

## Bus details show you all you need to know about an upcoming arrival:

<img src="/screenshots/busdetails.gif" alt="Full details about each arrival" width="144"> <img src="/screenshots/busdetails-waiting-fastplay.gif" alt="Bus details update in realtime" width="144">

## Settings to help customize your experience:

<img src="/screenshots/settings.gif" alt="Change the geographic search radius" width="144">
- "Favorites nearby" search radius controls what arrivals are shown based on how close their stops are to your current location
- "Adding favorites" search radius controls how far to look for stops when adding new favorites

## Building
1. Clone this repository
1. Install the [Pebble SDK](https://developer.pebble.com/sdk).
1. Update `servers.js` with your OneBusAway API keys
1. Run `pebble build` (or `pebble build -- --logging` to enable logging)

## Running / Installing
Install as normal for pebble apps (i.e. `pebble install --emulator=basalt`)