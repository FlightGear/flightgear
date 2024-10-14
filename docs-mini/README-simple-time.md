# Simple-time

[TOC]

## Overview

Simple-time allows Flightgear multiplayer sessions to see a consistent and smoothly varying view of each other, regardless of framerates or network delays.

For example in multiplayer refueling, each pilot will see the same relative position of the two aircraft.


## Using simple-time

Simple-time is enabled in the "Time Mode" dialogue (from menu "File/Time Mode"), or from the command line with: `--prop:bool:/sim/time/simple-time/enabled=true`.


There are no configuration settings.


## How it works

When simple-time is being used, Flightgear always sends its computer's UTC time in multiplayer packets. Received packets' time stamps will always be slightly in the past due to network delays, so multiplayer aircraft are positioned using extrapolation - by predicting where they would be now based on where they were a small time ago, using the velocity information in the MP packet.

So with multiple instances of Flightgear running, as long as their clocks are synchronized to within a few tenths of a second (typically via NTP, Network Time Protocol, the standard way for computers to set their clocks to the global standard) and network delays are similarly not more than a few tenths of a second, each pilot will see a consistent view of all aircraft.


## Coping with MP packets from Flightgear instances that are not using simple-time.

If we are running with simple-time but a particular MP aircraft's MP packet times are not within a second or two of our UTC time, we assume that they are not using simple-time, and we use a smoothed version of the time difference as compensation (basically assuming zero network delay). Thus the MP aircraft will appear to move smoothly to us, but the two pilots will probably not see the same relative positions of the aircraft.


## Recordings

If simple-time is being used, recordings will also contain the UTC time, for both the user aircraft and any multiplayer aircraft if they are included in the recording (see "File/Flight Recorder Control"). Thus replaying a recording could replicate the original position of the sun etc.


## Testing

The script `flightgear/scripts/python/recordreplay.py` includes checks of simple-time behaviour. 
