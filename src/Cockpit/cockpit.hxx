/**************************************************************************
 * cockpit.hxx -- cockpit defines and prototypes (initial draft)
 *
 * Written by Michele America, started September 1997.
 *
 * Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 **************************************************************************/
 

#ifndef _COCKPIT_HXX
#define _COCKPIT_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <Aircraft/aircraft.hxx>
#include "panel.hxx"

// Class fg_Cockpit          This class is a holder for the heads up display
//                          and is initialized with a
class fg_Cockpit  {
  private:
    int  Code;
    int  cockpitStatus;
    SGPropertyNode * hud_status;

  public:
    fg_Cockpit   () : Code(1), cockpitStatus(0) {};
    int   code  ( void ) { return Code; }
    int   status( void ) { return cockpitStatus; }
};


typedef fg_Cockpit * pCockpit;

bool fgCockpitInit( fgAIRCRAFT *cur_aircraft );
void fgCockpitUpdate( void );


#endif /* _COCKPIT_HXX */
