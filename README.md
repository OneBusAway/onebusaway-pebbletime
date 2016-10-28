# OneBusAway for Pebble

![Screen Shot](/screenshots/basicdemo.gif =144x)

<img src="/screenshots/basicdemo.gif" alt="Screen shot" width="144">

![Screen Shot](/screenshots/pebble_screenshot_2016-07-06_17-56-08.png?raw=true) ![Screen Shot](/screenshots/pebble_screenshot_2016-07-06_17-57-15.png?raw=true) ![Screen Shot](/screenshots/pebble_screenshot_2016-07-06_21-16-17.png?raw=true) ![Screen Shot](/screenshots/pebble_screenshot_2016-07-06_21-17-04.png?raw=true)

OneBusAway for Pebble smart watches provides a OneBusAway experience tailored to the Pebble platform. This app is designed to help you figure out what bus to take as quickly and easily as possible to aid in your everyday commute.

## Supported Pebble Hardware
1. Pebble Time (basalt)
1. Pebble Time Round (chalk)
1. Pebble 2 (diorite)

## Features
1. At a glance, real-time arrival information for public transportation
1. Shows to only 'favorite' stops nearby your current location
1. Favorite buses from multiple nearby stops shown to the user at the same time (to help you decide which bus to go take, even if they're not at the same stop)

# Usage
## At first launch, you will need to add some favorite stops:

![Screen Shot](/screenshots/addingfavorites.gif?raw=true =144x)

## Data updates in real time:

![Screen Shot](/screenshots/updates-fastplay.gif?raw=true)

## Bus details show you all you need to know about an upcoming arrival:

![Screen Shot](/screenshots/busdetails.gif?raw=true) ![Screen Shot](/screenshots/busdetails-waiting-fastplay.gif?raw=true)

## Settings to help customize your experience:

![Screen Shot](/screenshots/settings.gif?raw=true)
- "Favorites nearby" search radius controls what arrivals are shown based on how close their stops are to your current location
- "Adding favorites" search radius controls how far to look for stops when adding new favorites

## Building
1. Clone this repository
1. Install the [Pebble SDK](https://developer.pebble.com/sdk).
1. Update `servers.js` with your OneBusAway API keys
1. Run `pebble build` (or `pebble build -- --logging' to enable logging)

## Running / Installing
Install as normal for pebble apps (i.e. `pebble install --emulator=basalt`)