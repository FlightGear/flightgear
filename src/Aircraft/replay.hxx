// replay.hxx - a system to record and replay FlightGear flights
//
// Written by Curtis Olson, started July 2003.
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


#ifndef _FG_REPLAY_HXX
#define _FG_REPLAY_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <deque>

#include <simgear/math/sg_types.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

using std::deque;

class FGFlightRecorder;

typedef struct {
    double sim_time;
    char   raw_data;
    /* more data here, hidden to the outside world */
} FGReplayData;

typedef deque < FGReplayData *> replay_list_type;



/**
 * A recording/replay module for FlightGear flights
 * 
 */

class FGReplay : public SGSubsystem
{

public:

    FGReplay ();
    virtual ~FGReplay();

    virtual void init();
    virtual void reinit();
    virtual void bind();
    virtual void unbind();
    virtual void update( double dt );
    bool start();

private:
    void clear();
    FGReplayData* record(double time);
    void interpolate(double time, const replay_list_type &list);
    void replay(double time, FGReplayData* pCurrentFrame, FGReplayData* pOldFrame=NULL);

    bool replay( double time );
    double get_start_time();
    double get_end_time();

    double sim_time;
    double last_mt_time;
    double last_lt_time;
    int last_replay_state;

    replay_list_type short_term;
    replay_list_type medium_term;
    replay_list_type long_term;
    replay_list_type recycler;
    SGPropertyNode_ptr disable_replay;
    SGPropertyNode_ptr replay_master;
    SGPropertyNode_ptr replay_time;
    SGPropertyNode_ptr replay_time_str;
    SGPropertyNode_ptr replay_looped;
    SGPropertyNode_ptr speed_up;

    double m_high_res_time;    // default: 60 secs of high res data
    double m_medium_res_time;  // default: 10 mins of 1 fps data
    double m_low_res_time;     // default: 1 hr of 10 spf data
    // short term sample rate is as every frame
    double m_medium_sample_rate; // medium term sample rate (sec)
    double m_long_sample_rate;   // long term sample rate (sec)

    FGFlightRecorder* m_pRecorder;
};


#endif // _FG_REPLAY_HXX
