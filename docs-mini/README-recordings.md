# Flightgear recordings

Recording files generally have a `.fgtape` suffix.

As of 2020-12-22, there are three kinds of recordings:

* Normal
* Continuous
* Recovery

Normal recordings are compressed and contain frames at varying intervals with more recent frames being closer together in time. They are generated from the in-memory recording that Flightgear always maintains. They may contain multiplayer information.

Continuous recordings are uncompressed and written directly to a file while Flightgear runs, giving high resolution with near unlimited recording time. They may contain multiplayer information, and (as of 2020-12-23) may contain information about arbitrary properties.

Recovery recordings are essentially single-frame Continuous recordings. When enabled, Flightgear creates them periodically to allow recovery of a session if Flightgear crashes.

## Code 

The code that creates recordings is a not particularly clean or easy to work with.

It consists mainly of two source files:

* `src/Aircraft/flightrecorder.cxx`
* `src/Aircraft/replay.cxx`

Despite their names these files are both involved with record and replay. `src/Aircraft/flightrecorder.cxx` is lower-level; it takes care of setting up data for each frame when recording (see `FGFlightRecorder::capture()`) and reading data when replaying (see `FGFlightRecorder::replay()`).

`src/Aircraft/replay.cxx` is complicated and does various things. It maintains 3 in-memory buffers containing recording information at different temporal resolutions so that Flightgear can store any session in memory. For example only the most recent 60s is recorded at full frame rate.

## File format

Flightgear recording files have the following structure:

* Header:

    * A zero-terminated magic string: `FlightGear Flight Recorder Tape` (see the `FlightRecorderFileMagic`).

    * A Meta property tree containing a single `meta` node with various child nodes.

    * A Config property tree containing information about what signals will be contained in each frame.
    
        Each signal is a property; signals are used for the main recorded information such as position and orientation of the user's aircraft, and positions of flight surfaces, gear up/down etc. Aircraft may define their own customised set of signals.

    The Meta and Config property trees are each written as a binary length integer followed by a text representation of the properties.
    
    For Continuous recordings, the header is written by `FGReplay::continuousWriteHeader()`.

* A series of frames, each containg the data in a `FGReplayData` instance, looking like:

    * Frame time as a binary double.
    
    * Signals information as described in the header's Config property tree, in a fixed-size binary format.

    * If the header's Meta property tree has a child node `multiplayer` set to true, we append recorded information about multiplayer aircraft. This will be a binary integer containing the number of multiplayer messages to follow then, for each message, the message size followed by the message contents. The message contents are exactly the multiplayer packet as received from the network.

    * **[Work in progress as of 2020-12-23]** (Continuous recordings only) If the header's Meta property tree has a child node `record-properties` set to true, we append a variable list of property changes:

        * Length of following property-change data.

        * `<length><path><length><string_value>`

        * `...`
        
        Removal of a property is encoded as an item consisting of a zero integer followed by `<length><path>`.
        
        All numbers here are 16 bit binary.

In Normal recordings, all data after the header is in a single gzipped stream.

Continuous recordings do not use compression, in order to simplify random access when replaying.


## Replay of Continuous recordings

When a Continuous recording is loaded, `FGReplay::loadTape()` first steps through the entire file, building up a mapping in memory from frame times to offsets within the file, so that we can support the user jumping forwards and backwards in the recording.


## Multiplayer

### Recording while replaying:

If the user replays part of their history while we are saving to a Continuous recording, and the Continuous recording includes multiplayer information, then we carry on receiving multiplayer information from the network and writing it to the Continuous recording. The user's aircraft is recorded as being stationary while the user was replaying.

### Replaying

When replaying a recording that contains multiplayer information, only recorded multiplayer information is displayed to the user.

Chat messages are included in multiplayer recordings. When replaying, we attempt to hide recorded chat messages and only show live chat messages, but this is a little flakey, especially when jumping around in history.


### Details

The way that Multiplayer data is handled is a little complex. When recording, we build up a buffer of multiplayer packets that we have received since the last time that we wrote out a frame of data, then write them all out in the next frame we write.

When replaying, `FGFlightRecorder::replay()` sends replayed multiplayer messages into the low-level multiplayer packet-receiving code by calling `FGMultiplayMgr::pushMessageHistory()`.
