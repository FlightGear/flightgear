// replay.cxx - a system to record and replay FlightGear flights
//
// Written by Curtis Olson, started July 2003.
// Updated by Thorsten Brehm, September 2011 and November 2012.
//
// Copyright (C) 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdio>
#include <float.h>

#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/gzcontainerfile.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/strutils.hxx>

#include <Main/fg_props.hxx>

#include "replay.hxx"
#include "flightrecorder.hxx"

using std::deque;
using std::vector;
using simgear::gzContainerReader;
using simgear::gzContainerWriter;

#if 1
    #define MY_SG_DEBUG SG_DEBUG
#else
    #define MY_SG_DEBUG SG_ALERT
#endif

/** Magic string to verify valid FG flight recorder tapes. */
static const char* const FlightRecorderFileMagic = "FlightGear Flight Recorder Tape";

namespace ReplayContainer
{
    enum Type
    {
        Invalid    = -1,
        Header     = 0, /**< Used for initial file header (fixed identification string). */
        MetaData   = 1, /**< XML data / properties with arbitrary data, such as description, aircraft type, ... */
        Properties = 2, /**< XML data describing the recorded flight recorder properties.
                             Format is identical to flight recorder XML configuration. Also contains some
                             extra data to verify flight recorder consistency. */
        RawData    = 3  /**< Actual binary data blobs (the recorder's tape).
                             One "RawData" blob is used for each resolution. */
    };
}

/**
 * Constructor
 */

FGReplay::FGReplay() :
    sim_time(0),
    last_mt_time(0.0),
    last_lt_time(0.0),
    last_msg_time(0),
    last_replay_state(0),
    m_high_res_time(60.0),
    m_medium_res_time(600.0),
    m_low_res_time(3600.0),
    m_medium_sample_rate(0.5), // medium term sample rate (sec)
    m_long_sample_rate(5.0),   // long term sample rate (sec)
    m_pRecorder(new FGFlightRecorder("replay-config"))
{
}

/**
 * Destructor
 */

FGReplay::~FGReplay()
{
    clear();

    delete m_pRecorder;
    m_pRecorder = NULL;
}

/**
 * Clear all internal buffers.
 */
void
FGReplay::clear()
{
    while ( !short_term.empty() )
    {
        m_pRecorder->deleteRecord(short_term.front());
        short_term.pop_front();
    }
    while ( !medium_term.empty() )
    {
        m_pRecorder->deleteRecord(medium_term.front());
        medium_term.pop_front();
    }
    while ( !long_term.empty() )
    {
        m_pRecorder->deleteRecord(long_term.front());
        long_term.pop_front();
    }
    while ( !recycler.empty() )
    {
        m_pRecorder->deleteRecord(recycler.front());
        recycler.pop_front();
    }

    // clear messages belonging to old replay session
    fgGetNode("/sim/replay/messages", 0, true)->removeChildren("msg");
}

/** 
 * Initialize the data structures
 */

void
FGReplay::init()
{
    disable_replay  = fgGetNode("/sim/replay/disable",      true);
    replay_master   = fgGetNode("/sim/replay/replay-state", true);
    replay_time     = fgGetNode("/sim/replay/time",         true);
    replay_time_str = fgGetNode("/sim/replay/time-str",     true);
    replay_looped   = fgGetNode("/sim/replay/looped",       true);
    speed_up        = fgGetNode("/sim/speed-up",            true);

    // alias to keep backward compatibility
    fgGetNode("/sim/freeze/replay-state", true)->alias(replay_master);

    reinit();
}

/** 
 * Reset replay queues.
 */

void
FGReplay::reinit()
{
    sim_time = 0.0;
    last_mt_time = 0.0;
    last_lt_time = 0.0;
    last_msg_time = 0.0;

    // Flush queues
    clear();
    m_pRecorder->reinit();

    m_high_res_time   = fgGetDouble("/sim/replay/buffer/high-res-time",    60.0);
    m_medium_res_time = fgGetDouble("/sim/replay/buffer/medium-res-time", 600.0); // 10 mins
    m_low_res_time    = fgGetDouble("/sim/replay/buffer/low-res-time",   3600.0); // 1 h
    // short term sample rate is as every frame
    m_medium_sample_rate = fgGetDouble("/sim/replay/buffer/medium-res-sample-dt", 0.5); // medium term sample rate (sec)
    m_long_sample_rate   = fgGetDouble("/sim/replay/buffer/low-res-sample-dt",    5.0); // long term sample rate (sec)

    fillRecycler();
    loadMessages();

    replay_master->setIntValue(0);
    disable_replay->setBoolValue(0);
    replay_time->setDoubleValue(0);
    replay_time_str->setStringValue("");
}

/** 
 * Bind to the property tree
 */

void
FGReplay::bind()
{
}

/** 
 *  Unbind from the property tree
 */

void
FGReplay::unbind()
{
    // nothing to unbind
}

void
FGReplay::fillRecycler()
{
    // Create an estimated nr of required ReplayData objects
    // 120 is an estimated maximum frame rate.
    int estNrObjects = (int) ((m_high_res_time*120) + (m_medium_res_time*m_medium_sample_rate) +
                             (m_low_res_time*m_long_sample_rate));
    for (int i = 0; i < estNrObjects; i++)
    {
        FGReplayData* r = m_pRecorder->createEmptyRecord();
        if (r)
            recycler.push_back(r);
        else
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Out of memory!");
        }
    }
}

static void
printTimeStr(char* pStrBuffer,double _Time, bool ShowDecimal=true)
{
    if (_Time<0)
        _Time = 0;
    unsigned int Time = _Time*10;
    unsigned int h = Time/36000;
    unsigned int m = (Time % 36000)/600;
    unsigned int s = (Time % 600)/10;
    unsigned int d = Time % 10;

    int len;
    if (h>0)
        len = sprintf(pStrBuffer,"%u:%02u:%02u",h,m,s);
    else
        len = sprintf(pStrBuffer,"%u:%02u",m,s);

    if (len < 0)
    {
        *pStrBuffer = 0;
        return;
    }

    if (ShowDecimal)
        sprintf(&pStrBuffer[len],".%u",d);
}

void
FGReplay::guiMessage(const char* message)
{
    fgSetString("/sim/messages/copilot", message);
}

void
FGReplay::loadMessages()
{
    // load messages
    replay_messages.clear();
    simgear::PropertyList msgs = fgGetNode("/sim/replay/messages", true)->getChildren("msg");

    for (simgear::PropertyList::iterator it = msgs.begin();it != msgs.end();++it)
    {
        const char* msgText = (*it)->getStringValue("text", "");
        const double msgTime = (*it)->getDoubleValue("time", -1.0);
        const char* msgSpeaker = (*it)->getStringValue("speaker", "pilot");
        if ((msgText[0] != 0)&&(msgTime >= 0))
        {
            FGReplayMessages data;
            data.sim_time = msgTime;
            data.message = msgText;
            data.speaker = msgSpeaker;
            replay_messages.push_back(data);
        }
    }
    current_msg = replay_messages.begin();
}

void
FGReplay::replayMessage(double time)
{
    if (time < last_msg_time)
    {
        current_msg = replay_messages.begin();
    }

    // check if messages have to be triggered
    while ((current_msg != replay_messages.end())&&
           (time >= current_msg->sim_time))
    {
        // don't trigger messages when too long ago (fast-forward/skipped replay)
        if (time - current_msg->sim_time < 3.0)
        {
            fgGetNode("/sim/messages", true)->getNode(current_msg->speaker, true)->setStringValue(current_msg->message);
        }
        ++current_msg;
    }
    last_msg_time = time;
}

/** Start replay session
 */
bool
FGReplay::start(bool NewTape)
{
    // freeze the fdm, resume from sim pause
    double StartTime = get_start_time();
    double EndTime = get_end_time();
    was_finished_already = false;
    fgSetDouble("/sim/replay/start-time", StartTime);
    fgSetDouble("/sim/replay/end-time",   EndTime);
    char StrBuffer[30];
    printTimeStr(StrBuffer,StartTime,false);
    fgSetString("/sim/replay/start-time-str", StrBuffer);
    printTimeStr(StrBuffer,EndTime,false);
    fgSetString("/sim/replay/end-time-str",   StrBuffer);

    unsigned long buffer_elements =  short_term.size()+medium_term.size()+long_term.size();
    fgSetDouble("/sim/replay/buffer-size-mbyte",
                buffer_elements*m_pRecorder->getRecordSize() / (1024*1024.0));
    if ((fgGetBool("/sim/freeze/master"))||
        (0 == replay_master->getIntValue()))
        guiMessage("Replay active. 'Esc' to stop.");
    fgSetBool  ("/sim/freeze/master",     0);
    fgSetBool  ("/sim/freeze/clock",      0);
    if (0 == replay_master->getIntValue())
    {
        replay_master->setIntValue(1);
        if (NewTape)
        {
            // start replay at initial time, when loading a new tape
            replay_time->setDoubleValue(StartTime);
        }
        else
        {
            // start replay at "loop interval" when starting instant replay
            replay_time->setDoubleValue(-1);
        }
        replay_time_str->setStringValue("");
    }
    loadMessages();

    return true;
}

/** 
 *  Update the saved data
 */

void
FGReplay::update( double dt )
{
    int current_replay_state = last_replay_state;
    timingInfo.clear();
    stamp("begin");

    if ( disable_replay->getBoolValue() )
    {
        if (fgGetBool("/sim/freeze/master",false)||
            fgGetBool("/sim/freeze/clock",false))
        {
            // unpause - disable the replay system in next loop
            fgSetBool("/sim/freeze/master",false);
            fgSetBool("/sim/freeze/clock",false);
            last_replay_state = 1;
        }
        else
        if ((replay_master->getIntValue() != 3)||
            (last_replay_state == 3))
        {
            // disable the replay system
            current_replay_state = replay_master->getIntValue();
            replay_master->setIntValue(0);
            replay_time->setDoubleValue(0);
            replay_time_str->setStringValue("");
            disable_replay->setBoolValue(0);
            speed_up->setDoubleValue(1.0);
            speed_up->setDoubleValue(1.0);
            if (fgGetBool("/sim/replay/mute",false))
            {
                fgSetBool("/sim/sound/enabled",true);
                fgSetBool("/sim/replay/mute",false);
            }
            guiMessage("Replay stopped. Your controls!");
        }
    }

    int replay_state = replay_master->getIntValue();
    if ((replay_state == 0)&&
        (last_replay_state > 0))
    {
        if (current_replay_state == 3)
        {
            // take control at current replay position ("My controls!").
            // May need to uncrash the aircraft here :)
            fgSetBool("/sim/crashed", false);
        }
        else
        {
            // normal replay exit, restore most recent frame
            replay(DBL_MAX);
        }

        // replay is finished
        last_replay_state = replay_state;
        return;
    }

    // remember recent state
    last_replay_state = replay_state;

    switch(replay_state)
    {
        case 0:
            // replay inactive, keep recording
            break;
        case 1: // normal replay
        case 3: // prepare to resume normal flight at current replay position 
        {
            // replay active
            double current_time = replay_time->getDoubleValue();
            bool ResetTime = (current_time<=0.0);
            if (ResetTime)
            {
                // initialize start time
                double startTime = get_start_time();
                double endTime = get_end_time();
                fgSetDouble( "/sim/replay/start-time", startTime );
                fgSetDouble( "/sim/replay/end-time", endTime );
                double duration = 0;
                if (replay_looped->getBoolValue())
                    fgGetDouble("/sim/replay/duration");
                if( duration && (duration < (endTime - startTime)) ) {
                    current_time = endTime - duration;
                } else {
                    current_time = startTime;
                }
            }
            bool IsFinished = replay( replay_time->getDoubleValue() );
            if (IsFinished)
            {
                if (!was_finished_already)
                {
                    guiMessage("End of tape. 'Esc' to return.");
                    was_finished_already = true;
                }
                current_time = (replay_looped->getBoolValue()) ? -1 : get_end_time()+0.01;
            }
            else
            {
                current_time += dt * speed_up->getDoubleValue();
                was_finished_already = false;
            }
            replay_time->setDoubleValue(current_time);
            char StrBuffer[30];
            printTimeStr(StrBuffer,current_time);
            replay_time_str->setStringValue((const char*)StrBuffer);

            // when time skipped (looped replay), trigger listeners to reset views etc
            if (ResetTime)
                replay_master->setIntValue(replay_state);

            return; // don't record the replay session 
        }
        case 2: // normal replay operation
            return; // don't record the replay session
        default:
            throw sg_range_exception("unknown FGReplay state");
    }

    // flight recording

    // sanity check, don't collect data if FDM data isn't good
    if ((!fgGetBool("/sim/fdm-initialized", false))||(dt==0.0))
        return;

    {
        double new_sim_time = sim_time + dt * speed_up->getDoubleValue();
        // don't record multiple records with the same timestamp (or go backwards in time)
        if (new_sim_time <= sim_time)
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Time warp detected!");
            return;
        }
        sim_time = new_sim_time;
    }

    FGReplayData* r = record(sim_time);
    if (!r)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Out of memory!");
        return;
    }

    // update the short term list
    short_term.push_back( r );
    FGReplayData *st_front = short_term.front();
    
    if (!st_front)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Inconsistent data!");
    }

    if ( sim_time - st_front->sim_time > m_high_res_time )
    {
        while ( sim_time - st_front->sim_time > m_high_res_time )
        {
            st_front = short_term.front();
            recycler.push_back(st_front);
            short_term.pop_front();
        }

        // update the medium term list
        if ( sim_time - last_mt_time > m_medium_sample_rate )
        {
            last_mt_time = sim_time;
            st_front = short_term.front();
            medium_term.push_back( st_front );
            short_term.pop_front();

            FGReplayData *mt_front = medium_term.front();
            if ( sim_time - mt_front->sim_time > m_medium_res_time )
            {
                while ( sim_time - mt_front->sim_time > m_medium_res_time )
                {
                    mt_front = medium_term.front();
                    recycler.push_back(mt_front);
                    medium_term.pop_front();
                }
                // update the long term list
                if ( sim_time - last_lt_time > m_long_sample_rate )
                {
                    last_lt_time = sim_time;
                    mt_front = medium_term.front();
                    long_term.push_back( mt_front );
                    medium_term.pop_front();

                    FGReplayData *lt_front = long_term.front();
                    if ( sim_time - lt_front->sim_time > m_low_res_time )
                    {
                        while ( sim_time - lt_front->sim_time > m_low_res_time )
                        {
                            lt_front = long_term.front();
                            recycler.push_back(lt_front);
                            long_term.pop_front();
                        }
                    }
                }
            }
        }
    }

#if 0
    cout << "short term size = " << short_term.size()
         << "  time = " << sim_time - short_term.front().sim_time
         << endl;
    cout << "medium term size = " << medium_term.size()
         << "  time = " << sim_time - medium_term.front().sim_time
         << endl;
    cout << "long term size = " << long_term.size()
         << "  time = " << sim_time - long_term.front().sim_time
         << endl;
#endif
   //stamp("point_finished");
}

FGReplayData*
FGReplay::record(double time)
{
    FGReplayData* r = NULL;

    if (! recycler.empty())
    {
        r = recycler.front();
        recycler.pop_front();
    }

    return m_pRecorder->capture(time, r);
}

/** 
 * interpolate a specific time from a specific list
 */
void
FGReplay::interpolate( double time, const replay_list_type &list)
{
    // sanity checking
    if ( list.empty() )
    {
        // handle empty list
        return;
    } else if ( list.size() == 1 )
    {
        // handle list size == 1
        replay(time, list[0]);
        return;
    }

    unsigned int last = list.size() - 1;
    unsigned int first = 0;
    unsigned int mid = ( last + first ) / 2;

    bool done = false;
    while ( !done )
    {
        // cout << "  " << first << " <=> " << last << endl;
        if ( last == first ) {
            done = true;
        } else if ( list[mid]->sim_time < time && list[mid+1]->sim_time < time ) {
            // too low
            first = mid;
            mid = ( last + first ) / 2;
        } else if ( list[mid]->sim_time > time && list[mid+1]->sim_time > time ) {
            // too high
            last = mid;
            mid = ( last + first ) / 2;
        } else {
            done = true;
        }
    }

    replay(time, list[mid+1], list[mid]);
}

/** 
 *  Replay a saved frame based on time, interpolate from the two
 *  nearest saved frames.
 *  Returns true when replay sequence has finished, false otherwise.
 */

bool
FGReplay::replay( double time ) {
    // cout << "replay: " << time << " ";
    // find the two frames to interpolate between
    double t1, t2;

    replayMessage(time);

    if ( ! short_term.empty() ) {
        t1 = short_term.back()->sim_time;
        t2 = short_term.front()->sim_time;
        if ( time > t1 ) {
            // replay the most recent frame
            replay( time, short_term.back() );
            // replay is finished now
            return true;
        } else if ( time <= t1 && time >= t2 ) {
            interpolate( time, short_term );
        } else if ( ! medium_term.empty() ) {
            t1 = short_term.front()->sim_time;
            t2 = medium_term.back()->sim_time;
            if ( time <= t1 && time >= t2 )
            {
                replay(time, medium_term.back(), short_term.front());
            } else {
                t1 = medium_term.back()->sim_time;
                t2 = medium_term.front()->sim_time;
                if ( time <= t1 && time >= t2 ) {
                    interpolate( time, medium_term );
                } else if ( ! long_term.empty() ) {
                    t1 = medium_term.front()->sim_time;
                    t2 = long_term.back()->sim_time;
                    if ( time <= t1 && time >= t2 )
                    {
                        replay(time, long_term.back(), medium_term.front());
                    } else {
                        t1 = long_term.back()->sim_time;
                        t2 = long_term.front()->sim_time;
                        if ( time <= t1 && time >= t2 ) {
                            interpolate( time, long_term );
                        } else {
                            // replay the oldest long term frame
                            replay(time, long_term.front());
                        }
                    }
                } else {
                    // replay the oldest medium term frame
                    replay(time, medium_term.front());
                }
            }
        } else {
            // replay the oldest short term frame
            replay(time, short_term.front());
        }
    } else {
        // nothing to replay
        return true;
    }
    return false;
}

/** 
 * given two FGReplayData elements and a time, interpolate between them
 */
void
FGReplay::replay(double time, FGReplayData* pCurrentFrame, FGReplayData* pOldFrame)
{
    m_pRecorder->replay(time,pCurrentFrame,pOldFrame);
}

double
FGReplay::get_start_time()
{
    if ( ! long_term.empty() )
    {
        return long_term.front()->sim_time;
    } else if ( ! medium_term.empty() )
    {
        return medium_term.front()->sim_time;
    } else if ( ! short_term.empty() )
    {
        return short_term.front()->sim_time;
    } else
    {
        return 0.0;
    }
}

double
FGReplay::get_end_time()
{
    if ( ! short_term.empty() )
    {
        return short_term.back()->sim_time;
    } else
    {
        return 0.0;
    } 
}

/** Save raw replay data in a separate container */
static bool
saveRawReplayData(gzContainerWriter& output, const replay_list_type& ReplayData, size_t RecordSize)
{
    // get number of records in this stream
    size_t Count = ReplayData.size();

    // write container header for raw data
    if (!output.writeContainerHeader(ReplayContainer::RawData, Count * RecordSize))
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to save replay data. Cannot write data container. Disk full?");
        return false;
    }

    // write the raw data (all records in the given list)
    replay_list_type::const_iterator it = ReplayData.begin();
    size_t CheckCount = 0;
    while ((it != ReplayData.end())&&
           !output.fail())
    {
        const FGReplayData* pRecord = *it++;
        output.write((char*)pRecord, RecordSize);
        CheckCount++;
    }

    // Did we really write as much as we intended?
    if (CheckCount != Count)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to save replay data. Expected to write " << Count << " records, but wrote " << CheckCount);
        return false;
    }

    SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Saved " << CheckCount << " records of size " << RecordSize);
    return !output.fail();
}

/** Load raw replay data from a separate container */
static bool
loadRawReplayData(gzContainerReader& input, FGFlightRecorder* pRecorder, replay_list_type& ReplayData, size_t RecordSize)
{
    size_t Size = 0;
    simgear::ContainerType Type = ReplayContainer::Invalid;

    // write container header for raw data
    if (!input.readContainerHeader(&Type, &Size))
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to load replay data. Missing data container.");
        return false;
    }
    else
    if (Type != ReplayContainer::RawData)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to load replay data. Expected data container, got " << Type);
        return false;
    }

    // read the raw data
    size_t Count = Size / RecordSize;
    SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Loading replay data. Container size is " << Size << ", record size " << RecordSize <<
           ", expected record count " << Count << ".");

    size_t CheckCount = 0;
    for (CheckCount=0; (CheckCount<Count)&&(!input.eof()); ++CheckCount)
    {
        FGReplayData* pBuffer = pRecorder->createEmptyRecord();
        input.read((char*) pBuffer, RecordSize);
        ReplayData.push_back(pBuffer);
    }

    // did we get all we have hoped for?
    if (CheckCount != Count)
    {
        if (input.eof())
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "Unexpected end of file.");
        }
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to load replay data. Expected " << Count << " records, but got " << CheckCount);
        return false;
    }

    SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Loaded " << CheckCount << " records of size " << RecordSize);
    return true;
}

/** Write flight recorder tape with given filename and meta properties to disk */
bool
FGReplay::saveTape(const char* Filename, SGPropertyNode* MetaDataProps)
{
    bool ok = true;

    /* open output stream *******************************************/
    gzContainerWriter output(Filename, FlightRecorderFileMagic);
    if (!output.good())
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Cannot open file" << Filename);
        return false;
    }

    /* write meta data **********************************************/
    ok &= output.writeContainer(ReplayContainer::MetaData, MetaDataProps);

    /* write flight recorder configuration **************************/
    SGPropertyNode_ptr Config;
    if (ok)
    {
        Config = new SGPropertyNode();
        m_pRecorder->getConfig(Config.get());
        ok &= output.writeContainer(ReplayContainer::Properties, Config.get());
    }

    /* write raw data ***********************************************/
    if (Config)
    {
        size_t RecordSize = Config->getIntValue("recorder/record-size", 0);
        SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Total signal count: " <<  Config->getIntValue("recorder/signal-count", 0)
               << ", record size: " << RecordSize);
        if (ok)
            ok &= saveRawReplayData(output, short_term,  RecordSize);
        if (ok)
            ok &= saveRawReplayData(output, medium_term, RecordSize);
        if (ok)
            ok &= saveRawReplayData(output, long_term,   RecordSize);
        Config = 0;
    }

    /* done *********************************************************/
    output.close();

    return ok;
}

/** Write flight recorder tape to disk. User/script command. */
bool
FGReplay::saveTape(const SGPropertyNode* ConfigData)
{
    const char* tapeDirectory = fgGetString("/sim/replay/tape-directory", "");
    const char* aircraftType  = fgGetString("/sim/aircraft", "unknown");

    SGPropertyNode_ptr myMetaData = new SGPropertyNode();
    SGPropertyNode* meta = myMetaData->getNode("meta", 0, true);

    // add some data to the file - so we know for which aircraft/version it was recorded
    meta->setStringValue("aircraft-type",           aircraftType);
    meta->setStringValue("aircraft-description",    fgGetString("/sim/description", ""));
    meta->setStringValue("aircraft-fdm",            fgGetString("/sim/flight-model", ""));
    meta->setStringValue("closest-airport-id",      fgGetString("/sim/airport/closest-airport-id", ""));
    const char* aircraft_version = fgGetString("/sim/aircraft-version", "");
    if (aircraft_version[0]==0)
        aircraft_version = "(undefined)";
    meta->setStringValue("aircraft-version", aircraft_version);

    // add information on the tape's recording duration
    double Duration = get_end_time()-get_start_time();
    meta->setDoubleValue("tape-duration", Duration);
    char StrBuffer[30];
    printTimeStr(StrBuffer, Duration, false);
    meta->setStringValue("tape-duration-str", StrBuffer);

    // add simulator version
    copyProperties(fgGetNode("/sim/version", 0, true), meta->getNode("version", 0, true));
    if (ConfigData->getNode("user-data"))
    {
        copyProperties(ConfigData->getNode("user-data"), meta->getNode("user-data", 0, true));
    }

    // store replay messages
    copyProperties(fgGetNode("/sim/replay/messages", 0, true), myMetaData->getNode("messages", 0, true));

    // generate file name (directory + aircraft type + date + time + suffix)
    SGPath p(tapeDirectory);
    p.append(aircraftType);
    p.concat("-");
    time_t calendar_time = time(NULL);
    struct tm *local_tm;
    local_tm = localtime( &calendar_time );
    char time_str[256];
    strftime( time_str, 256, "%Y%m%d-%H%M%S", local_tm);
    p.concat(time_str);
    p.concat(".fgtape");

    bool ok = true;
    // make sure we're not overwriting something
    if (p.exists())
    {
        // same timestamp!?
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Error, flight recorder tape file with same name already exists.");
        ok = false;
    }

    if (ok)
        ok &= saveTape(p.c_str(), myMetaData.get());

    if (ok)
        guiMessage("Flight recorder tape saved successfully!");
    else
        guiMessage("Failed to save tape! See log output.");

    return ok;
}

/** Read a flight recorder tape with given filename from disk and return meta properties.
 * Actual data and signal configuration is not read when in "Preview" mode.
 */
bool
FGReplay::loadTape(const char* Filename, bool Preview, SGPropertyNode* UserData)
{
    bool ok = true;

    /* open input stream ********************************************/
    gzContainerReader input(Filename, FlightRecorderFileMagic);
    if (input.eof() || !input.good())
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Cannot open file " << Filename);
        ok = false;
    }

    SGPropertyNode_ptr MetaDataProps = new SGPropertyNode();

    /* read meta data ***********************************************/
    if (ok)
    {
        char* MetaData = NULL;
        size_t Size = 0;
        simgear::ContainerType Type = ReplayContainer::Invalid;
        if (!input.readContainer(&Type, &MetaData, &Size) || Size<1)
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "File not recognized. This is not a valid FlightGear flight recorder tape: " << Filename
                    << ". Invalid meta data.");
            ok = false;
        }
        else
        if (Type != ReplayContainer::MetaData)
        {
            SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Invalid header. Container type " << Type);
            SG_LOG(SG_SYSTEMS, SG_ALERT, "File not recognized. This is not a valid FlightGear flight recorder tape: " << Filename);
            ok = false;
        }
        else
        {
            try
            {
                readProperties(MetaData, Size-1, MetaDataProps);
                copyProperties(MetaDataProps->getNode("meta", 0, true), UserData);
            } catch (const sg_exception &e)
            {
              SG_LOG(SG_SYSTEMS, SG_ALERT, "Error reading flight recorder tape: " << Filename
                     << ", XML parser message:" << e.getFormattedMessage());
              ok = false;
            }
        }

        if (MetaData)
        {
            //printf("%s\n", MetaData);
            free(MetaData);
            MetaData = NULL;
        }
    }

    /* read flight recorder configuration **************************/
    if ((ok)&&(!Preview))
    {
        SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Loading flight recorder data...");
        char* ConfigXML = NULL;
        size_t Size = 0;
        simgear::ContainerType Type = ReplayContainer::Invalid;
        if (!input.readContainer(&Type, &ConfigXML, &Size) || Size<1)
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "File not recognized. This is not a valid FlightGear flight recorder tape: " << Filename
                   << ". Invalid configuration container.");
            ok = false;
        }
        else
        if ((!ConfigXML)||(Type != ReplayContainer::Properties))
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "File not recognized. This is not a valid FlightGear flight recorder tape: " << Filename
                   << ". Unexpected container type, expected \"properties\".");
            ok = false;
        }

        SGPropertyNode_ptr Config = new SGPropertyNode();
        if (ok)
        {
            try
            {
                readProperties(ConfigXML, Size-1, Config);
            } catch (const sg_exception &e)
            {
              SG_LOG(SG_SYSTEMS, SG_ALERT, "Error reading flight recorder tape: " << Filename
                     << ", XML parser message:" << e.getFormattedMessage());
              ok = false;
            }
            if (ok)
            {
                // reconfigure the recorder - and wipe old data (no longer matches the current recorder)
                m_pRecorder->reinit(Config);
                clear();
                fillRecycler();
            }
        }

        if (ConfigXML)
        {
            free(ConfigXML);
            ConfigXML = NULL;
        }

        /* read raw data ***********************************************/
        if (ok)
        {
            size_t RecordSize = m_pRecorder->getRecordSize();
            size_t OriginalSize = Config->getIntValue("recorder/record-size", 0);
            // check consistency - ugly things happen when data vs signals mismatch
            if ((OriginalSize != RecordSize)&&
                (OriginalSize != 0))
            {
                ok = false;
                SG_LOG(SG_SYSTEMS, SG_ALERT, "Error: Data inconsistency. Flight recorder tape has record size " << RecordSize
                       << ", expected size was " << OriginalSize << ".");
            }

            if (ok)
                ok &= loadRawReplayData(input, m_pRecorder, short_term,  RecordSize);
            if (ok)
                ok &= loadRawReplayData(input, m_pRecorder, medium_term, RecordSize);
            if (ok)
                ok &= loadRawReplayData(input, m_pRecorder, long_term,   RecordSize);

            // restore replay messages
            if (ok)
            {
                copyProperties(MetaDataProps->getNode("messages", 0, true),
                               fgGetNode("/sim/replay/messages", 0, true));
            }
            sim_time = get_end_time();
            // TODO we could (re)store these too
            last_mt_time = last_lt_time = sim_time;
        }
        /* done *********************************************************/
    }

    input.close();

    if (!Preview)
    {
        if (ok)
        {
            guiMessage("Flight recorder tape loaded successfully!");
            start(true);
        }
        else
            guiMessage("Failed to load tape. See log output.");
    }

    return ok;
}

/** List available tapes in current directory.
 * Limits to tapes matching current aircraft when SameAircraftFilter is enabled.
 */
bool
FGReplay::listTapes(bool SameAircraftFilter, const SGPath& tapeDirectory)
{
    const std::string& aircraftType = simgear::strutils::uppercase(fgGetString("/sim/aircraft", "unknown"));

    // process directory listing of ".fgtape" files
    simgear::Dir dir(tapeDirectory);
    simgear::PathList list =  dir.children(simgear::Dir::TYPE_FILE, ".fgtape");

    SGPropertyNode* TapeList = fgGetNode("/sim/replay/tape-list", true);
    TapeList->removeChildren("tape");
    int Index = 0;
    size_t l = aircraftType.size();
    for (simgear::PathList::iterator it = list.begin(); it!=list.end(); ++it)
    {
        SGPath file(it->file());
        std::string name(file.base());
        if ((!SameAircraftFilter)||
            (0==simgear::strutils::uppercase(name).compare(0,l, aircraftType)))
        {
            TapeList->getNode("tape", Index++, true)->setStringValue(name);
        }
    }

    return true;
}

/** Load a flight recorder tape from disk. User/script command. */
bool
FGReplay::loadTape(const SGPropertyNode* ConfigData)
{
    SGPath tapeDirectory(fgGetString("/sim/replay/tape-directory", ""));

    // see if shall really load the file - or just obtain the meta data for preview
    bool Preview = ConfigData->getBoolValue("preview", 0);

    // file/tape to be loaded
    std::string tape = ConfigData->getStringValue("tape", "");

    if (tape.empty())
    {
        if (!Preview)
            return listTapes(ConfigData->getBoolValue("same-aircraft", 0), tapeDirectory);
        return true;
    }
    else
    {
        SGPropertyNode* UserData = fgGetNode("/sim/gui/dialogs/flightrecorder/preview", true);
        tapeDirectory.append(tape);
        tapeDirectory.concat(".fgtape");
        SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Checking flight recorder file " << tapeDirectory << ", preview: " << Preview);
        return loadTape(tapeDirectory.c_str(), Preview, UserData);
    }
}
