/*****************************************************************************

 Header:       FGAirPressureItem.h	
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
Air pressure item that is stored in the micro weather class

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer      Created
08.06.1999 Christian Mayer      Added international air preasure formula
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
15.12.1999 Christian Mayer	changed the air pressure calculation to a much
				more realistic formula. But as I need for that
				the temperature I moved the code to 
				FGPhysicalProperties
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGAirPressureItem_H
#define FGAirPressureItem_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <math.h>

#include "FGWeatherDefs.h"
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
class FGAirPressureItem;
FGAirPressureItem operator-(const FGAirPressureItem& arg);

/****************************************************************************/
/* CLASS DECLARATION							    */
/* NOTE: The value stored in 'value' is the air preasure that we'd have at  */
/*       an altitude of 0.0 The correct airpreasure at the stored altitude  */
/*       gets  calulated in  FGPhyiscalProperties  as  I need  to know  the */
/*       temperatures at the different altitudes for that.		    */
/****************************************************************************/
class FGAirPressureItem
{
private:
    WeatherPrecision value; //that's the airpressure at 0 metres

protected:
public:

    FGAirPressureItem(const WeatherPrecision v)	{value = v;                             }
    FGAirPressureItem()				{value = FG_WEATHER_DEFAULT_AIRPRESSURE;}

    WeatherPrecision getValue() const
    { 
	return value;
    };

    void setValue(WeatherPrecision p) 
    { 
	value = p;
    };

    FGAirPressureItem& operator*=(const WeatherPrecision   arg);
    FGAirPressureItem& operator+=(const FGAirPressureItem& arg);
    FGAirPressureItem& operator-=(const FGAirPressureItem& arg);

    friend FGAirPressureItem operator-(const FGAirPressureItem& arg);
};

inline FGAirPressureItem& FGAirPressureItem::operator*= (const WeatherPrecision arg)
{
  value *= arg;
  return *this;
}

inline FGAirPressureItem& FGAirPressureItem::operator+= (const FGAirPressureItem& arg)
{
  value += arg.value;
  return *this;
}

inline FGAirPressureItem& FGAirPressureItem::operator-= (const FGAirPressureItem& arg)
{
  value -= arg.value;
  return *this;
}

inline FGAirPressureItem operator-(const FGAirPressureItem& arg)
{
    return FGAirPressureItem(-arg.value);
}

/****************************************************************************/
#endif /*FGAirPressureItem_H*/






