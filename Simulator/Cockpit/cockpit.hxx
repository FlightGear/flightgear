// cockpit.hxx -- cockpit defines and prototypes (initial draft)
//
// Written by Michele America, started September 1997.
//
// Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
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
 

#ifndef _COCKPIT_HXX
#define _COCKPIT_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include "hud.hxx"
#include "panel.hxx"

// Class fg_Cockpit          This class is a holder for the heads up display
//                           and is initialized with a
class fg_Cockpit  {
  private:
    int  Code;
    int  Status;

  public:
    fg_Cockpit   () : Code(1), Status(0) {};
    int   code  ( void ) { return Code; }
    int   status( void ) { return Status; }
};


typedef fg_Cockpit * pCockpit;

bool fgCockpitInit( fgAIRCRAFT *cur_aircraft );
void fgCockpitUpdate( void );


#endif // _COCKPIT_HXX


// $Log$
// Revision 1.2  1999/04/06 16:58:30  curt
// Clean ups and reorganizations:
// - Additional Thanks entry
// - more info on getting gfc library
// - converted some C style comments to C++ style.
//
// Revision 1.1.1.1  1999/04/05 21:32:48  curt
// Start of 0.6.x branch.
//
// Revision 1.4  1998/07/13 21:00:46  curt
// Integrated Charlies latest HUD updates.
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.3  1998/06/27 16:47:54  curt
// Incorporated Friedemann Reinhard's <mpt218@faupt212.physik.uni-erlangen.de>
// first pass at an isntrument panel.
//
// Revision 1.2  1998/05/11 18:13:10  curt
// Complete C++ rewrite of all cockpit code by Charlie Hotchkiss.
//
// Revision 1.1  1998/04/24 00:45:55  curt
// C++-ifing the code a bit.
//
// Revision 1.8  1998/04/22 13:26:19  curt
// C++ - ifing the code a bit.
//
// Revision 1.7  1998/04/21 17:02:34  curt
// Prepairing for C++ integration.
//
// Revision 1.6  1998/02/07 15:29:33  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.5  1998/01/22 02:59:29  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.4  1998/01/19 19:27:01  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.3  1998/01/19 18:40:19  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.2  1997/12/10 22:37:39  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.1  1997/08/29 18:03:21  curt
// Initial revision.
//

