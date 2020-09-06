
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


#ifndef _FG_TEST_SUITE_LOGGING_HXX
#define _FG_TEST_SUITE_LOGGING_HXX


#include <sstream>

#include <simgear/debug/LogCallback.hxx>


// A logstream callback based on the simgear FileLogCallback class.
class StreamLogCallback : public simgear::LogCallback
{
public:
    StreamLogCallback(std::ostringstream& stream, sgDebugClass c, sgDebugPriority p, bool priorityMatch):
        simgear::LogCallback(c, p),
        m_priority(p),
        m_stream(stream),
        m_priorityMatch(priorityMatch)
    {
    }

    virtual void operator()(sgDebugClass c, sgDebugPriority p, const char* file, int line, const std::string& message)
    {
        if (m_priorityMatch && p != m_priority) return;
        if (!shouldLog(c, p)) return;
        if (m_priorityMatch)
            m_stream << simgear::LogCallback::debugClassToString(c) << ":" << file << ":" << line << ": " << message << std::endl;
        else
            m_stream << simgear::LogCallback::debugClassToString(c) << ":" << (int) p << ":" << file << ":" << line << ": " << message << std::endl;
    }

private:
    sgDebugPriority m_priority;
    std::ostringstream &m_stream;
    bool m_priorityMatch;
};


// All of the captured IO streams.
class capturedIO
{
    public:
        // Constructor and destructor.
        capturedIO(sgDebugClass, sgDebugPriority);
        ~capturedIO();

        // The IO streams.
        std::ostringstream sg_interleaved;
        std::ostringstream sg_bulk_only;
        std::ostringstream sg_debug_only;
        std::ostringstream sg_info_only;
        std::ostringstream sg_warn_only;
        std::ostringstream sg_alert_only;

        // The callback objects.
        StreamLogCallback *callback_interleaved;
        StreamLogCallback *callback_bulk_only;
        StreamLogCallback *callback_debug_only;
        StreamLogCallback *callback_info_only;
        StreamLogCallback *callback_warn_only;
        StreamLogCallback *callback_alert_only;

        // The logging class and priority text.
        std::string log_class;
        std::string log_priority;
};


// Return the global stream capture data structure, creating it if needed.
capturedIO & getIOstreams(sgDebugClass c=SG_ALL, sgDebugPriority p=SG_BULK);

// Set up to capture all the simgear logging priorities as separate streams.
void setupLogging(sgDebugClass, sgDebugPriority, bool);

// Deactivate all the simgear logging priority IO captures.
void stopLogging();


#endif  // _FG_TEST_SUITE_LOGGING_HXX
