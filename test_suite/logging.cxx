/*
 * Copyright (C) 2016 Edward d'Auvergne
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <iomanip>

#include "logging.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/debug/OsgIoCapture.hxx>


// The global stream capture data structure.
static capturedIO *_iostreams = NULL;


// capturedIO constructor.
capturedIO::capturedIO(sgDebugClass c, sgDebugPriority p)
{
    callback_interleaved = new StreamLogCallback(sg_interleaved, c, p, false);
    callback_bulk_only = new StreamLogCallback(sg_bulk_only, c, SG_BULK, true);
    callback_debug_only = new StreamLogCallback(sg_debug_only, c, SG_DEBUG, true);
    callback_info_only = new StreamLogCallback(sg_info_only, c, SG_INFO, true);
    callback_warn_only = new StreamLogCallback(sg_warn_only, c, SG_WARN, true);
    callback_alert_only = new StreamLogCallback(sg_alert_only, c, SG_ALERT, true);

    // Store the class as a string.
    if (c == SG_ALL)
        log_class = "SG_ALL";
    else {
        std::stringstream stream;
        stream << "0x" << std::right << std::setfill('0') << std::setw(8) << std::hex << c;
        log_class = stream.str();
    }
    
    // Store the priority as a string.
    if (p == SG_BULK)
        log_priority = "SG_BULK";
    else if (p == SG_DEBUG)
        log_priority = "SG_DEBUG";
    else if (p == SG_INFO)
        log_priority = "SG_INFO";
    else if (p == SG_WARN)
        log_priority = "SG_WARN";
    else if (p == SG_ALERT)
        log_priority = "SG_ALERT";
    else if (p == SG_POPUP)
        log_priority = "SG_POPUP";
    else if (p == SG_DEV_WARN)
        log_priority = "SG_DEV_WARN";
    else if (p == SG_DEV_ALERT)
        log_priority = "SG_DEV_ALERT";
}

// capturedIO destructor.
capturedIO::~capturedIO()
{
    // Destroy the callback objects.
    delete callback_interleaved;
    delete callback_bulk_only;
    delete callback_debug_only;
    delete callback_info_only;
    delete callback_warn_only;
    delete callback_alert_only;
}


// Return the global stream capture data structure, creating it if needed.
capturedIO & getIOstreams(sgDebugClass c, sgDebugPriority p)
{
    // Initialise the global stream capture data structure, if needed.
    if (!_iostreams)
        _iostreams = new capturedIO(c, p);

    // Return a pointer to the global object.
    return *_iostreams;
}


// Set up to capture all the simgear logging priorities as separate streams.
void setupLogging(sgDebugClass c, sgDebugPriority p, bool split)
{
    // Get the single logstream instance.
    logstream &log = sglog();

    // Set up the logstream testing mode.
    log.setTestingMode(true);

    // OSG IO capture.
    osg::setNotifyHandler(new NotifyLogger);

    // IO capture.
    capturedIO &obj = getIOstreams(c, p);
    if (!split)
        log.addCallback(obj.callback_interleaved);
    else {
        if (p <= SG_BULK)
            log.addCallback(obj.callback_bulk_only);
        if (p <= SG_DEBUG)
            log.addCallback(obj.callback_debug_only);
        if (p <= SG_INFO)
            log.addCallback(obj.callback_info_only);
        if (p <= SG_WARN)
            log.addCallback(obj.callback_warn_only);
        if (p <= SG_ALERT)
            log.addCallback(obj.callback_alert_only);
    }
}


// Deactivate all the simgear logging priority IO captures.
void stopLogging()
{
    // Get the single logstream instance.
    logstream &log = sglog();

    // IO decapture.
    capturedIO &obj = getIOstreams();
    log.removeCallback(obj.callback_interleaved);
    log.removeCallback(obj.callback_bulk_only);
    log.removeCallback(obj.callback_debug_only);
    log.removeCallback(obj.callback_info_only);
    log.removeCallback(obj.callback_warn_only);
    log.removeCallback(obj.callback_alert_only);

    // Clean up the IO stream object.
    delete _iostreams;
    _iostreams = NULL;

    // Stop the simgear logstream.
    simgear::shutdownLogging();
}
