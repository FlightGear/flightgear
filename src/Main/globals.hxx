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

public:

    FGGlobals();
    ~FGGlobals();

    inline bool get_frozen() const { return freeze; }
    inline void toggle_frozen() { freeze = !freeze; }

    inline long int get_warp() const { return warp; }
    inline void set_warp( long int w ) { warp = w; }
    inline void inc_warp( long int w ) { warp += w; }

    inline long int get_warp_delta() const { return warp_delta; }
    inline void set_warp_delta( long int d ) { warp_delta = d; }
    inline void inc_warp_delta( long int d ) { warp_delta += d; }
};


extern FGGlobals *globals;


#endif // _GLOBALS_HXX


