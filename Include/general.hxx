// general.hxx -- a general house keeping data structure definition for 
//                various info that might need to be accessible from all 
//                parts of the sim.
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
// (Log is kept at end of this file)


#ifndef _GENERAL_HXX
#define _GENERAL_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


// #define FG_FRAME_RATE_HISTORY 10


// the general house keeping structure definition
class FGGeneral {
    // Info about OpenGL
    char *glVendor;
    char *glRenderer;
    char *glVersion;

    // Last frame rate measurement
    int frame_rate;
    // double frames[FG_FRAME_RATE_HISTORY];

public:

    inline void set_glVendor( char *str ) { glVendor = str; }
    inline char* get_glRenderer() const { return glRenderer; }
    inline void set_glRenderer( char *str ) { glRenderer = str; }
    inline void set_glVersion( char *str ) { glVersion = str; }
    inline double get_frame_rate() const { return frame_rate; }
    inline void set_frame_rate( int rate ) { frame_rate = rate; }
};

// general contains all the general house keeping parameters.
extern FGGeneral general;


#endif // _GENERAL_HXX


// $Log$
// Revision 1.1  1999/01/06 21:47:39  curt
// renamed general.h to general.hxx
// More portability enhancements to compiler.h
//
// Revision 1.9  1998/12/18 23:34:42  curt
// Converted to a simpler frame rate calculation method.
//
// Revision 1.8  1998/08/20 15:09:46  curt
// Added a status flat for instrument panel use.
//
// Revision 1.7  1998/07/03 14:36:11  curt
// Added conversion constants to fg_constants.h to assist with converting
//   between various world units and coordinate systems.
// Added gl vendor/renderer/version info to general structure.  Initialized
//   in fg_init.cxx
//
// Revision 1.6  1998/05/13 18:23:46  curt
// fg_typedefs.h: updated version by Charlie Hotchkiss
// general.h: moved fg_root info to fgOPTIONS structure.
//
// Revision 1.5  1998/05/07 23:03:17  curt
// Lowered size of frame rate history buffer.
//
// Revision 1.4  1998/05/06 03:14:30  curt
// Added a shared frame rate counter.
//
// Revision 1.3  1998/03/14 00:27:58  curt
// Promoted fgGENERAL to a "type" of struct.
//
// Revision 1.2  1998/01/22 02:59:35  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.1  1997/12/15 21:02:16  curt
// Moved to .../FlightGear/Src/Include/
//
// Revision 1.3  1997/12/10 22:37:34  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.2  1997/08/27 03:29:38  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.1  1997/08/23 11:37:12  curt
// Initial revision.
//

