/*****************************************************************************

 Header:       FGTurbulenceItem.h	
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
turbulence item that gets stored in the micro weather class

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGTurbulenceItem_H
#define FGTurbulenceItem_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <Math/point3d.hxx>
#include "FGWeatherDefs.h"
	
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
class FGTurbulenceItem;
FGTurbulenceItem operator-(const FGTurbulenceItem& arg);

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGTurbulenceItem
{
private:
    Point3D value;
    WeatherPrecition alt;

protected:
public:
    FGTurbulenceItem(const WeatherPrecition& a, const Point3D& v)   {alt = a; value = v;}
    FGTurbulenceItem(const Point3D& v)				    {alt = 0.0; value = v;}
    FGTurbulenceItem()						    {alt = 0.0; value = Point3D(0.0);}

    Point3D	     getValue() const { return value; };
    WeatherPrecition getAlt()   const { return alt; };

    FGTurbulenceItem& operator*= (const WeatherPrecition& arg);
    FGTurbulenceItem& operator+= (const FGTurbulenceItem& arg);
    FGTurbulenceItem& operator-= (const FGTurbulenceItem& arg);

    friend bool operator<(const FGTurbulenceItem& arg1, const FGTurbulenceItem& arg2 );
    friend FGTurbulenceItem operator-(const FGTurbulenceItem& arg);

};

inline FGTurbulenceItem& FGTurbulenceItem::operator*= (const WeatherPrecition& arg)
{
  value *= arg;
  return *this;
}

inline FGTurbulenceItem& FGTurbulenceItem::operator+= (const FGTurbulenceItem& arg)
{
  value += arg.value;
  return *this;
}

inline FGTurbulenceItem& FGTurbulenceItem::operator-= (const FGTurbulenceItem& arg)
{
  value -= arg.value;
  return *this;
}

inline FGTurbulenceItem operator-(const FGTurbulenceItem& arg)
{
    return FGTurbulenceItem(arg.alt, -arg.value);
}

/****************************************************************************/
#endif /*FGTurbulenceItem_H*/




