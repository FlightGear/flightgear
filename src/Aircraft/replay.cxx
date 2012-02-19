// replay.cxx - a system to record and replay FlightGear flights
//
// Written by Curtis Olson, started July 2003.
// Updated by Thorsten Brehm, September 2011.
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

#include <float.h>

#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>

#include <Main/fg_props.hxx>

#include "replay.hxx"
#include "flightrecorder.hxx"

/**
 * Constructor
 */

FGReplay::FGReplay() :
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
}

/** 
 * Initialize the data structures
 */

void
FGReplay::init()
{
    disable_replay  = fgGetNode("/sim/replay/disable",      true);
    replay_master   = fgGetNode("/sim/freeze/replay-state", true);
    replay_time     = fgGetNode("/sim/replay/time",         true);
    replay_time_str = fgGetNode("/sim/replay/time-str",     true);
    replay_looped   = fgGetNode("/sim/replay/looped",       true);
    speed_up        = fgGetNode("/sim/speed-up",            true);
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

    // Flush queues
    clear();
    m_pRecorder->reinit();

    m_high_res_time   = fgGetDouble("/sim/replay/buffer/high-res-time",    60.0);
    m_medium_res_time = fgGetDouble("/sim/replay/buffer/medium-res-time", 600.0); // 10 mins
    m_low_res_time    = fgGetDouble("/sim/replay/buffer/low-res-time",   3600.0); // 1 h
    // short term sample rate is as every frame
    m_medium_sample_rate = fgGetDouble("/sim/replay/buffer/medium-res-sample-dt", 0.5); // medium term sample rate (sec)
    m_long_sample_rate   = fgGetDouble("/sim/replay/buffer/low-res-sample-dt",    5.0); // long term sample rate (sec)

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

/** Start replay session
 */
bool
FGReplay::start()
{
    // freeze the fdm, resume from sim pause
    double StartTime = get_start_time();
    double EndTime = get_end_time();
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
        fgSetString("/sim/messages/copilot", "Replay active. 'Esc' to stop.");
    fgSetBool  ("/sim/freeze/master",     0);
    fgSetBool  ("/sim/freeze/clock",      0);
    if (0 == replay_master->getIntValue())
    {
        replay_master->setIntValue(1);
        replay_time->setDoubleValue(-1);
        replay_time_str->setStringValue("");
    }
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
            fgSetBool("/sim/freeze/master",false);
            fgSetBool("/sim/freeze/clock",false);
            last_replay_state = 1;
        }
        else
        if ((replay_master->getIntValue() != 3)||
            (last_replay_state == 3))
        {
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
            fgSetString("/sim/messages/copilot", "Replay stopped. Your controls!");
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
                double duration = fgGetDouble( "/sim/replay/duration" );
                if( duration && (duration < (endTime - startTime)) ) {
                    current_time = endTime - duration;
                } else {
                    current_time = startTime;
                }
            }
            bool IsFinished = replay( replay_time->getDoubleValue() );
            if (IsFinished)
                current_time = (replay_looped->getBoolValue()) ? -1 : get_end_time()+0.01;
            else
                current_time += dt * speed_up->getDoubleValue();
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

    //cerr << "Recording replay" << endl;
    sim_time += dt * speed_up->getDoubleValue();

    // sanity check, don't collect data if FDM data isn't good
    if (!fgGetBool("/sim/fdm-initialized", false)) {
        return;
    }

    FGReplayData* r = record(sim_time);
    if (!r)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Out of memory!");
        return;
    }

    // update the short term list
    //stamp("point_06");
    short_term.push_back( r );
    //stamp("point_07");
    FGReplayData *st_front = short_term.front();
    
    if (!st_front)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Inconsistent data!");
    }

    if ( sim_time - st_front->sim_time > m_high_res_time ) {
        while ( sim_time - st_front->sim_time > m_high_res_time ) {
            st_front = short_term.front();
            recycler.push_back(st_front);
            short_term.pop_front();
        }
        //stamp("point_08");
        // update the medium term list
        if ( sim_time - last_mt_time > m_medium_sample_rate ) {
            last_mt_time = sim_time;
            st_front = short_term.front();
            medium_term.push_back( st_front );
            short_term.pop_front();

            FGReplayData *mt_front = medium_term.front();
            if ( sim_time - mt_front->sim_time > m_medium_res_time ) {
            //stamp("point_09");
                while ( sim_time - mt_front->sim_time > m_medium_res_time ) {
                    mt_front = medium_term.front();
                    recycler.push_back(mt_front);
                    medium_term.pop_front();
                }
                // update the long term list
                if ( sim_time - last_lt_time > m_long_sample_rate ) {
                    last_lt_time = sim_time;
                    mt_front = medium_term.front();
                    long_term.push_back( mt_front );
                    medium_term.pop_front();

                    FGReplayData *lt_front = long_term.front();
                    if ( sim_time - lt_front->sim_time > m_low_res_time ) {
                        //stamp("point_10");
                        while ( sim_time - lt_front->sim_time > m_low_res_time ) {
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

    if (recycler.size())
    {
        r = recycler.front();
        recycler.pop_front();
    }

    r = m_pRecorder->capture(time, r);

    return r;
}

/** 
 * interpolate a specific time from a specific list
 */
void
FGReplay::interpolate( double time, const replay_list_type &list)
{
    // sanity checking
    if ( list.size() == 0 )
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

    if ( short_term.size() > 0 ) {
        t1 = short_term.back()->sim_time;
        t2 = short_term.front()->sim_time;
        if ( time > t1 ) {
            // replay the most recent frame
            replay( time, short_term.back() );
            // replay is finished now
            return true;
            // cout << "first frame" << endl;
        } else if ( time <= t1 && time >= t2 ) {
            interpolate( time, short_term );
            // cout << "from short term" << endl;
        } else if ( medium_term.size() > 0 ) {
            t1 = short_term.front()->sim_time;
            t2 = medium_term.back()->sim_time;
            if ( time <= t1 && time >= t2 )
            {
                replay(time, medium_term.back(), short_term.front());
                // cout << "from short/medium term" << endl;
            } else {
                t1 = medium_term.back()->sim_time;
                t2 = medium_term.front()->sim_time;
                if ( time <= t1 && time >= t2 ) {
                    interpolate( time, medium_term );
                    // cout << "from medium term" << endl;
                } else if ( long_term.size() > 0 ) {
                    t1 = medium_term.front()->sim_time;
                    t2 = long_term.back()->sim_time;
                    if ( time <= t1 && time >= t2 )
                    {
                        replay(time, long_term.back(), medium_term.front());
                        // cout << "from medium/long term" << endl;
                    } else {
                        t1 = long_term.back()->sim_time;
                        t2 = long_term.front()->sim_time;
                        if ( time <= t1 && time >= t2 ) {
                            interpolate( time, long_term );
                            // cout << "from long term" << endl;
                        } else {
                            // replay the oldest long term frame
                            replay(time, long_term.front());
                            // cout << "oldest long term frame" << endl;
                        }
                    }
                } else {
                    // replay the oldest medium term frame
                    replay(time, medium_term.front());
                    // cout << "oldest medium term frame" << endl;
                }
            }
        } else {
            // replay the oldest short term frame
            replay(time, short_term.front());
            // cout << "oldest short term frame" << endl;
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
    if ( long_term.size() > 0 )
    {
        return long_term.front()->sim_time;
    } else if ( medium_term.size() > 0 )
    {
        return medium_term.front()->sim_time;
    } else if ( short_term.size() )
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
    if ( short_term.size() )
    {
        return short_term.back()->sim_time;
    } else
    {
        return 0.0;
    } 
}
