// globals.hxx -- Global state that needs to be shared among the sim modules
//
// Written by Curtis Olson, started July 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifndef _GLOBALS_HXX
#define _GLOBALS_HXX

#include <simgear/compiler.h>

#include <vector>
#include STL_STRING


#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/route/route.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/misc/props.hxx>

#include <Sound/soundmgr.hxx>
#include "viewmgr.hxx"

FG_USING_STD( vector );
FG_USING_STD( string );

typedef vector<string> string_list;


class FGGlobals {

private:

    // Root of FlightGear data tree
    string fg_root;

    // Root of FlightGear scenery tree
    string fg_scenery;

    // Freeze sim
    bool freeze;

    // An offset in seconds from the true time.  Allows us to adjust
    // the effective time of day.
    long int warp;

    // How much to change the value of warp each iteration.  Allows us
    // to make time progress faster than normal (or even run in reverse.)
    long int warp_delta;

    // Time structure
    SGTime *time_params;

    // Sky structures
    SGEphemeris *ephem;

    // Magnetic Variation
    SGMagVar *mag;

    // Global autopilot "route"
    SGRoute *route;

    // sound manager
    FGSoundMgr *soundmgr;

    // viewer manager
    FGViewMgr *viewmgr;
    FGViewer *current_view;

    // properties
    SGPropertyNode *props;
    SGPropertyNode *initial_state;

    // list of serial port-like configurations
    string_list *channel_options_list;

public:

    FGGlobals();
    ~FGGlobals();

    inline const string &get_fg_root () const { return fg_root; }
    inline void set_fg_root (const string &root) { fg_root = root; }

    inline const string &get_fg_scenery () const { return fg_scenery; }
    inline void set_fg_scenery (const string &scenery) {
      fg_scenery = scenery;
    }

    inline bool get_freeze() const { return freeze; }
    inline void set_freeze( bool f ) { freeze = f; }

    inline long int get_warp() const { return warp; }
    inline void set_warp( long int w ) { warp = w; }
    inline void inc_warp( long int w ) { warp += w; }

    inline long int get_warp_delta() const { return warp_delta; }
    inline void set_warp_delta( long int d ) { warp_delta = d; }
    inline void inc_warp_delta( long int d ) { warp_delta += d; }

    inline SGTime *get_time_params() const { return time_params; }
    inline void set_time_params( SGTime *t ) { time_params = t; }

    inline SGEphemeris *get_ephem() const { return ephem; }
    inline void set_ephem( SGEphemeris *e ) { ephem = e; }

    inline SGMagVar *get_mag() const { return mag; }
    inline void set_mag( SGMagVar *m ) { mag = m; }

    inline SGRoute *get_route() const { return route; }
    inline void set_route( SGRoute *r ) { route = r; }

    inline FGSoundMgr *get_soundmgr() const { return soundmgr; }
    inline void set_soundmgr( FGSoundMgr *sm ) { soundmgr = sm; }

    inline FGViewMgr *get_viewmgr() const { return viewmgr; }
    inline void set_viewmgr( FGViewMgr *vm ) { viewmgr = vm; }
    inline FGViewer *get_current_view() const { return current_view; }
    inline void set_current_view( FGViewer *v ) { current_view = v; }

    inline SGPropertyNode *get_props () { return props; }
    inline void set_props( SGPropertyNode *n ) { props = n; }

    inline string_list *get_channel_options_list () {
	return channel_options_list;
    }
    inline void set_channel_options_list( string_list *l ) {
	channel_options_list = l;
    }


    /**
     * Save the current state as the initial state.
     */
    void saveInitialState ();


    /**
     * Restore the saved initial state, if any.
     */
    void restoreInitialState ();

};


extern FGGlobals *globals;


#endif // _GLOBALS_HXX


