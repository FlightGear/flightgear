// auto_gui.hxx -- autopilot gui interface
//
// Written by Norman Vine <nhv@cape.com>
// Arranged by Curt Olson <http://www.flightgear.org/~curt>
//
// Copyright (C) 1998 - 2000
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
                       
                       
#ifndef _AUTO_GUI_HXX
#define _AUTO_GUI_HXX

#include <simgear/compiler.h>

#include STL_STRING

SG_USING_STD( string );

// Defines
#define AP_CURRENT_HEADING -1

// prototypes

class puObject;
// void fgAPAdjust( puObject * );
void NewHeading(puObject *cb);
void NewAltitude(puObject *cb);
void AddWayPoint(puObject *cb);
void PopWayPoint(puObject *cb);
void ClearRoute(puObject *cb);

void NewTgtAirportInit();
void fgAPAdjustInit() ;
void NewHeadingInit();
void NewAltitudeInit();

int NewWaypoint( string Tgt_Alt );


#endif // _AUTO_GUI_HXX
