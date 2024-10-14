# Flightgear recordings

[TOC]

## Overview

Recording files generally have a `.fgtape` suffix.

As of 2020-12-22, there are three kinds of recordings:

* Normal
* Continuous
* Recovery

Normal recordings are compressed and contain frames at varying intervals with more recent frames being closer together in time. They are generated from the in-memory recording that Flightgear always maintains. They may contain multiplayer information.

Continuous recordings are written directly to a file while Flightgear runs, giving high resolution with near unlimited recording time. They may contain multiplayer information. As of 2020-12-23 they may contain information about extra properties, allowing replay of views and main window position/size. As of 2021-06-26 each frame's data can be compressed.

Recovery recordings are essentially single-frame Continuous recordings. When enabled, Flightgear creates them periodically to allow recovery of a session if Flightgear crashes.

## Properties that control recording and replay

* `/sim/replay/tape-directory` - where to save recordings.
* `/sim/replay/record-multiplayer` - if true, we include multiplayer information in Normal and Continuous recordings.
* Normal recordings:
    * `/sim/replay/buffer/high-res-time` - period for high resolution recording.
    * `/sim/replay/buffer/medium-res-time` - period for medium resolution.
    * `/sim/replay/buffer/low-res-time` - period for low resolution.
    * `/sim/replay/buffer/medium-res-sample-dt` - sample period for medium resolution.
    * `/sim/replay/buffer/low-res-sample-dt` - sample period for low resolution.
* Continuous recordings:
    * `/sim/replay/record-continuous` - if true, do continuous record to file.
    * `/sim/replay/record-signals` - if true (the default), include signals for user aircraft - these are the core values used to replay the user aircraft.
    * `/sim/replay/record-extra-properties` - if true, we include selected properties in recordings.
    * `/sim/replay/record-continuous-compression` - if 1, we compress each frame's data.
    * `/sim/replay/record-main-window` - if 1, we record main window position and size.
    * `/sim/replay/record-main-view` - if 1, we record main window view details.
    * `/sim/replay/replay-main-window-position` - if 1, we replay main window position.
    * `/sim/replay/replay-main-window-size` - if 1, we replay main window size.
    * `sim/replay/replay-main-view` - if 1, we replay main window view (view type, orientation, zoom etc).
* Recovery recordings:
    * `/sim/replay/record-recovery-period` - if non-zero, we update recovery recording in specified interval.

## Code 

The code that creates recordings is not particularly clean or easy to work with.

It consists mainly of two source files:

* `src/Aircraft/flightrecorder.cxx`
* `src/Aircraft/replay.cxx`

Despite their names these files are both involved with record and replay. `src/Aircraft/flightrecorder.cxx` is lower-level; it takes care of setting up data for each frame when recording (see `FGFlightRecorder::capture()`) and reading data when replaying (see `FGFlightRecorder::replay()`).

`src/Aircraft/replay.cxx` is complicated and does various things. It maintains 3 in-memory buffers containing recording information at different temporal resolutions so that Flightgear can store any session in memory. For example only the most recent 60s is recorded at full frame rate.

## File formats

### Normal recordings

Normal recordings are written as a compressed gzip stream using `simgear::gzContainerWriter`.

* Header:

    * A zero-terminated magic string: `FlightGear Flight Recorder Tape` (variable `FlightRecorderFileMagic`).

    * A Meta property tree containing a `meta` node with various child nodes.
    
    * A Config property tree containing information about what signals will be contained in each frame.
    
        Each signal is a property; signals are used for the main recorded information such as position and orientation of the user's aircraft, and positions of flight surfaces, gear up/down etc. Aircraft may define their own customized set of signals.

    The Meta and Config property trees are each written as `<length:64><text>` where `<text>` is a text representation of the properties. `<text>` is terminated with a zero which is included in the `<length:64>` field.

* A series of frames, each containing the data in a `FGReplayData` instance, looking like:

    * Frame time as a binary double.
    
    * Signals information as described in the header's `Config` property tree, represented as a 64-bit length followed by binary data.

### Continuous recordings

* Header:

    * A zero-terminated magic string: `FlightGear Flight Recorder Tape` (variable `FlightRecorderFileMagic`).

    * A property tree represented as a `uint32` length followed by zero-terminated text. This contains:

        * A `meta` node with various child nodes. If this contains `continuous-compression` with value `1`, then each frame's data is compressed..

        * `data[]` nodes describing the data items in each frame in the order in which they occur. Supported values are:

            * `signals` - core information about the user's aircraft.
            * `multiplayer` - information about multiplayer aircraft.
            * `extra-properties` - information about extra properties.

        * A `signals` node containing layout information for signals, in the same format as for Normal recordings.

    The header is written by `FGReplay::continuousWriteHeader()`.

* A series of frames, each containing the data in a `FGReplayData` instance, looking like:

    * Frame time as a binary double.
    
    * If compression is used:
    
        * uint8_t flags.
            * Bit 0: this frame has signals.
            * Bit 1: this frame has multiplayer information.
            * Bit 2: this frame has extra properties.
        * uint32_t compressed-size.

    * Frame data (can be compressed): a list of ordered `<length:32><data>` items as described by the `data[]` nodes in the header. This format allows Flightgear to skip data items that it doesn't understand if loading a recording that was created by a newer version.
        
        * For `signals`, `<data>` is binary data for the core aircraft properties.
    
        * For `multiplayer`, `<data>` is a list of `<length:16><packet>` items where `<packet>` is a multiplayer packet as received from the network.

        * For `extra-properties`, `<data>` is a list of property changes, each one being:

            * `<length:16><path><length:16><value>` - property `<path>` has changed to `<value>`.
        
            Removal of a property is encoded as `<0:16><length:16><path>`.


## Replay of Continuous recordings

When a Continuous recording is loaded, `FGReplay::loadTape()` first steps through the entire file, building up an index in memory that maps from frame times to a struct containing the offset of the frame in the file plus information on whether the frame has multiplayer and/or extra-properties information. This allows us to support the user jumping forwards and backwards in the recording.

If the recording uses compression, indexing uses the uint8_t flags and uint32_t compressed-size fields and does not need to decompress each frame's data.

If we are replaying from a URL, indexing takes place in the background (by requesting callbacks from the download's `simgear::HTTP::FileRequest`) and replay starts immediately. Thus we avoid having to wait until the entire recording has been downloaded before starting replay.


## Multiplayer

### Recording while replaying:

If the user replays part of their history while we are saving to a Continuous recording, and the Continuous recording includes multiplayer information, then we carry on receiving multiplayer information from the network and writing it to the Continuous recording. The user's aircraft is recorded as being stationary for the period when the user was replaying.

### Replaying

When replaying a recording that contains multiplayer information, the recorded multiplayer information is displayed to the user, and any live multiplayer information is ignored.

As of 2021-03-06 the code attempts to hide recorded chat messages and let through live chat messages when replaying. Unfortunately this doesn't work because it assumes that chat messages are sent as `CHAT_MSG_ID` packets, but actually they are sent as part of generic `POS_DATA_ID` packets (where they set the `/sim/multiplay/chat` property).


### Details

The way that Multiplayer data is handled is a little complex. When recording, we build up a buffer of multiplayer packets that we have received since the last time that we wrote out a frame of data, then write them all out in the next frame we write.

When replaying, `FGFlightRecorder::replay()` sends replayed multiplayer messages into the low-level multiplayer packet-receiving code by calling `FGMultiplayMgr::pushMessageHistory()`.
