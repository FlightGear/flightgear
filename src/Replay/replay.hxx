// replay.hxx - a system to record and replay FlightGear flights
//
// Written by Curtis Olson, started Juley 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#include <Network/net_ctrls.hxx>
#include <Network/net_fdm.hxx>
#include <Main/fgfs.hxx>

SG_USING_STD(deque);


class FGReplayData {

public:

    double sim_time;
    FGNetFDM fdm;
    FGNetCtrls ctrls;
};

typedef deque < FGReplayData > replay_list_type;



/**
 * A recording/replay module for FlightGear flights
 * 
 */

class FGReplay : public FGSubsystem
{

public:

    FGReplay ();
    virtual ~FGReplay();

    virtual void init();
    virtual void bind();
    virtual void unbind();
    virtual void update( double dt );

    void replay( double time );
    double get_start_time();
    double get_end_time();
    
private:

    static const double st_list_time = 10.0;   // 60 secs of high res data
    static const double mt_list_time = 30.0;  // 10 mins of 1 fps data
    static const double lt_list_time = 60.0; // 1 hr of 10 spf data

    double sim_time;
    double last_mt_time;
    double last_lt_time;

    replay_list_type short_term;
    replay_list_type medium_term;
    replay_list_type long_term;
};


#endif // _FG_REPLAY_HXX
