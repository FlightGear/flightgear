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

#include <simgear/math/sg_types.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <deque>
#include <vector>

class FGFlightRecorder;

typedef struct {
    double sim_time;
    char   raw_data;
    /* more data here, hidden to the outside world */
} FGReplayData;

typedef struct {
    double sim_time;
    std::string message;
    std::string speaker;
} FGReplayMessages;

typedef std::deque < FGReplayData *> replay_list_type;
typedef std::vector < FGReplayMessages > replay_messages_type;

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
    bool start(bool NewTape=false);

    bool saveTape(const SGPropertyNode* ConfigData);
    bool loadTape(const SGPropertyNode* ConfigData);

private:
    void clear();
    FGReplayData* record(double time);
    void interpolate(double time, const replay_list_type &list);
    void replay(double time, FGReplayData* pCurrentFrame, FGReplayData* pOldFrame=NULL);
    void guiMessage(const char* message);
    void loadMessages();
    void fillRecycler();

    bool replay( double time );
    void replayMessage( double time );

    double get_start_time();
    double get_end_time();

    bool listTapes(bool SameAircraftFilter, const SGPath& tapeDirectory);
    bool saveTape(const char* Filename, SGPropertyNode* MetaData);
    bool loadTape(const char* Filename, bool Preview, SGPropertyNode* UserData);

    double sim_time;
    double last_mt_time;
    double last_lt_time;
    double last_msg_time;
    replay_messages_type::iterator current_msg;
    int last_replay_state;
    bool was_finished_already;

    replay_list_type short_term;
    replay_list_type medium_term;
    replay_list_type long_term;
    replay_list_type recycler;
    replay_messages_type replay_messages;

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
