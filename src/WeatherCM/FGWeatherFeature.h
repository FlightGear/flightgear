/*****************************************************************************

 Header:       FGWeatherFeature.h	
 Author:       Christian Mayer
 Date started: 28.05.99

 -------- Copyright (C) 1999 Christian Mayer (fgfs@christianmayer.de) --------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
------------------------------------------------------------------------------
Abstract class that every weather feature, such as wind layers, are derivated
from

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
30.06.1999 Christian Mayer	STL portability
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGWeatherFeature_H
#define FGWeatherFeature_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <simgear/compiler.h>

#include <plib/sg.h>

#include "FGWeatherDefs.h"
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
enum LayerType {
    fgWind,
    fgTurbulence,
    fgTemperature,
    fgAirDensity,
    fgCloud,
    fgRain
};

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGWeatherFeature
{
private:
protected:
    sgVec3 position;		    //middle of the feature in lat/lon/alt
    WeatherPrecision minSize;	    //smalest size of the feature
				    //=> a disk is specifies

    LayerType FeatureType; 

public:
    LayerType getFeature(void)         const { return FeatureType; }
    bool isFeature(const LayerType& f) const { return (f == FeatureType); }
};

/****************************************************************************/
#endif /*FGWeatherFeature_H*/
