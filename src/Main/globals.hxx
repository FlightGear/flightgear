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


#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/route/route.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/misc/props.hxx>

#include "options.hxx"
#include "viewmgr.hxx"


class FGGlobals {

private:

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

    // options
    FGOptions *options;

    // viewer maneger
    FGViewMgr *viewmgr;
    FGViewer *current_view;

    // properties
    SGPropertyNode *props;

public:

    FGGlobals();
    ~FGGlobals();

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

    inline FGOptions *get_options() const { return options; }
    inline void set_options( FGOptions *o ) { options = o; }

    inline FGViewMgr *get_viewmgr() const { return viewmgr; }
    inline void set_viewmgr( FGViewMgr *vm ) { viewmgr = vm; }
    inline FGViewer *get_current_view() const { return current_view; }
    inline void set_current_view( FGViewer *v ) { current_view = v; }

    inline SGPropertyNode *get_props () { return props; }
    inline void set_props( SGPropertyNode *n ) { props = n; }
    // inline const SGPropertyNode &get_props () const { return props; }
    
};


extern FGGlobals *globals;


#endif // _GLOBALS_HXX


