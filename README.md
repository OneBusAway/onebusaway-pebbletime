# OneBusAway for Pebble [![Build Status](https://travis-ci.org/OneBusAway/onebusaway-pebbletime.svg?branch=master)](https://travis-ci.org/OneBusAway/onebusaway-pebbletime) [![Join the OneBusAway chat](https://onebusaway.herokuapp.com/badge.svg)](https://onebusaway.herokuapp.com/)

<img src="/screenshots/basicdemo.gif" alt="Full demo" width="144">

OneBusAway for Pebble smart watches provides a OneBusAway experience tailored to the Pebble platform. This app is designed to help you figure out what bus to take as quickly and easily as possible to aid in your everyday commute.

[<img src="https://onebusaway.org/wp-content/uploads/pebble_2x.png" alt="Available on the Pebble App Store" height="64">](https://apps.rebble.io/en_US/application/58321a598c7fff9ce1000137?query=onebusaway&section=watchapps)

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
1. Run `pebble build` (or `pebble build -- --logging` to enable logging)

Important note about building - because this project uses a custom `wscript` file to compile correctly for ARM/math-sll, the project will not build on [CloudPebble](https://cloudpebble.net).

## Running / Installing
Install as normal for pebble apps (i.e. `pebble install --emulator=basalt`)
