/*****************************************************************************

 Header:       FGVaporPressureItem.h	
 Author:       Christian Mayer
 Date started: 28.05.99

 ---------- Copyright (C) 1999  Christian Mayer (vader@t-online.de) ----------

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
Vapor pressure item that is stored in the micro weather class

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
10.10.1999 Christian Mayer	added mutable for gcc 2.95 portability
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGVaporPressureItem_H
#define FGVaporPressureItem_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include "FGWeatherDefs.h"

//for the case that mutable isn't supported:
#include <simgear/compiler.h>
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
class FGVaporPressureItem;
FGVaporPressureItem operator-(const FGVaporPressureItem& arg);

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGVaporPressureItem
{
private:
    mutable WeatherPrecision value;
    WeatherPrecision alt;

protected:
public:

    FGVaporPressureItem(const WeatherPrecision& a, const WeatherPrecision& v)	{alt = a; value = v;}
    FGVaporPressureItem(const WeatherPrecision& v)				{alt = 0.0; value = v;}
    FGVaporPressureItem()							{alt = 0.0; value = FG_WEATHER_DEFAULT_VAPORPRESSURE;}

    WeatherPrecision getValue() const { return value; };
    WeatherPrecision getAlt()   const { return alt;   };

    FGVaporPressureItem& operator*= (const WeatherPrecision& arg);
    FGVaporPressureItem& operator+= (const FGVaporPressureItem& arg);
    FGVaporPressureItem& operator-= (const FGVaporPressureItem& arg);

    friend bool operator<(const FGVaporPressureItem& arg1, const FGVaporPressureItem& arg2 );
    friend FGVaporPressureItem operator-(const FGVaporPressureItem& arg);

};

inline FGVaporPressureItem& FGVaporPressureItem::operator*= (const WeatherPrecision& arg)
{
  value *= arg;
  return *this;
}
inline FGVaporPressureItem& FGVaporPressureItem::operator+= (const FGVaporPressureItem& arg)
{
  value += arg.value;
  return *this;
}

inline FGVaporPressureItem& FGVaporPressureItem::operator-= (const FGVaporPressureItem& arg)
{
  value -= arg.value;
  return *this;
}

inline FGVaporPressureItem operator-(const FGVaporPressureItem& arg)
{
    return FGVaporPressureItem(arg.alt, -arg.value);
}

/****************************************************************************/
#endif /*FGVaporPressureItem_H*/
