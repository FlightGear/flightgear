/*****************************************************************************

 Header:       FGTemperatureItem.h	
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
Temperature item that is stored in the micro weather class

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGTemperatureItem_H
#define FGTemperatureItem_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include "FGWeatherDefs.h"
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
class FGTemperatureItem;
FGTemperatureItem operator-(const FGTemperatureItem& arg);

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGTemperatureItem
{
private:
    WeatherPrecition value;
    WeatherPrecition alt;

protected:
public:

    FGTemperatureItem(const WeatherPrecition& a, const WeatherPrecition& v) {alt = a; value = v;}
    FGTemperatureItem(const WeatherPrecition& v)			    {alt = 0.0; value = v;}
    FGTemperatureItem()							    {alt = 0.0; value = (WeatherPrecition)FG_WEATHER_DEFAULT_TEMPERATURE;}

    WeatherPrecition getValue() const { return value; };
    WeatherPrecition getAlt()   const { return alt;   };  

    FGTemperatureItem& operator*= (const WeatherPrecition& arg);
    FGTemperatureItem& operator+= (const FGTemperatureItem& arg);
    FGTemperatureItem& operator-= (const FGTemperatureItem& arg);

    friend bool operator<(const FGTemperatureItem& arg1, const FGTemperatureItem& arg2 );
    friend FGTemperatureItem operator-(const FGTemperatureItem& arg);
    
};

inline FGTemperatureItem& FGTemperatureItem::operator*= (const WeatherPrecition& arg)
{
  value *= arg;
  return *this;
}

inline FGTemperatureItem& FGTemperatureItem::operator+= (const FGTemperatureItem& arg)
{
  value += arg.value;
  return *this;
}
inline FGTemperatureItem& FGTemperatureItem::operator-= (const FGTemperatureItem& arg)
{
  value -= arg.value;
  return *this;
}


inline FGTemperatureItem operator-(const FGTemperatureItem& arg)
{
    return FGTemperatureItem(arg.alt, -arg.value);
}




/****************************************************************************/
#endif /*FGTemperatureItem_H*/
