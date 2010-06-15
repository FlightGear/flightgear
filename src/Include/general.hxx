// general.hxx -- a general house keeping data structure definition for 
//                various info that might need to be accessible from all 
//                parts of the sim.
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _GENERAL_HXX
#define _GENERAL_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/structure/OSGVersion.hxx>
#define FG_OSG_VERSION SG_OSG_VERSION

// #define FANCY_FRAME_COUNTER
#ifdef FANCY_FRAME_COUNTER
#define FG_FRAME_RATE_HISTORY 10
#endif


// the general house keeping structure definition
class FGGeneral {
    // Info about OpenGL
    char *glVendor;
    char *glRenderer;
    char *glVersion;
    int glMaxTexSize;
    int glDepthBits;

    // Last frame rate measurement
    int frame_rate;
#ifdef FANCY_FRAME_COUNTER
    double frames[FG_FRAME_RATE_HISTORY];
#endif

public:
    inline char* get_glVendor() { return glVendor; }
    inline void set_glVendor( char *str ) { glVendor = str; }
    inline char* get_glRenderer() const { return glRenderer; }
    inline void set_glRenderer( char *str ) { glRenderer = str; }
    inline char* get_glVersion() { return glVersion; }
    inline void set_glVersion( char *str ) { glVersion = str; }
    inline void set_glMaxTexSize( int i ) { glMaxTexSize = i; }
    inline int get_glMaxTexSize() const { return glMaxTexSize; }
    inline void set_glDepthBits( int d ) { glDepthBits = d; }
    inline int get_glDepthBits() const { return glDepthBits; }
    inline double get_frame_rate() const { return frame_rate; }
#ifdef FANCY_FRAME_COUNTER
    inline double get_frame(int idx) const { return frames[idx]; }
    inline void set_frame( int idx, double value ) { frames[idx] = value; }
    inline void set_frame_rate( double rate ) { frame_rate = rate; }
#else
    inline void set_frame_rate( int rate ) { frame_rate = rate; }
#endif
};

// general contains all the general house keeping parameters.
extern FGGeneral general;


#endif // _GENERAL_HXX


