// auto_gui.hxx -- autopilot gui interface
//
// Written by Norman Vine <nhv@cape.com>
// Arranged by Curt Olson <curt@flightgear.org>
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

#include <string.h>

#include <Aircraft/aircraft.hxx>
#include <FDM/flight.hxx>
#include <Controls/controls.hxx>
                       
FG_USING_STD(string);

                  
// Defines
#define AP_CURRENT_HEADING -1


// prototypes
// void fgAPToggleWayPoint( void );
// void fgAPToggleHeading( void );
// void fgAPToggleAltitude( void );
// void fgAPToggleTerrainFollow( void );
// void fgAPToggleAutoThrottle( void );

// bool fgAPTerrainFollowEnabled( void );
// bool fgAPAltitudeEnabled( void );
// bool fgAPHeadingEnabled( void );
// bool fgAPWayPointEnabled( void );
// bool fgAPAutoThrottleEnabled( void );

// void fgAPAltitudeAdjust( double inc );
// void fgAPHeadingAdjust( double inc );
// void fgAPAutoThrottleAdjust( double inc );

// void fgAPHeadingSet( double value );

// double fgAPget_TargetLatitude( void );
// double fgAPget_TargetLongitude( void );
// // double fgAPget_TargetHeading( void );
// double fgAPget_TargetDistance( void );
// double fgAPget_TargetAltitude( void );

// char *fgAPget_TargetLatitudeStr( void );
// char *fgAPget_TargetLongitudeStr( void );
// char *fgAPget_TargetDistanceStr( void );
// char *fgAPget_TargetHeadingStr( void );
// char *fgAPget_TargetAltitudeStr( void );
// char *fgAPget_TargetLatLonStr( void );

//void fgAPset_tgt_airport_id( const string );
//string fgAPget_tgt_airport_id( void );

// void fgAPReset(void);

class puObject;
void fgAPAdjust( puObject * );
void NewHeading(puObject *cb);
void NewAltitude(puObject *cb);
void NewTgtAirport(puObject *cb);

void NewTgtAirportInit();
void fgAPAdjustInit() ;
void NewHeadingInit();
void NewAltitudeInit();


#endif // _AUTO_GUI_HXX
