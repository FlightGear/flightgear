/*****************************************************************************

 Header:       FGWindItem.h	
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
wind item that gets stored in the micro weather class

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
21.12.1999 Christian Mayer	Added a fix for compatibility to gcc 2.8 which
				suggested by Oliver Delise
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGWindItem_H
#define FGWindItem_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <plib/sg.h>

#include "FGWeatherDefs.h"

/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
class FGWindItem;
FGWindItem operator-(const FGWindItem& arg);

#if ( __GNU_C__ == 2 && __GNU_MAJOR__ < 9 )
#  define const_sgVec3 const sgVec3
#else
#  define const_sgVec3 const sgVec3&
#endif

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGWindItem
{
private:
    sgVec3 value;

protected:
public:

    FGWindItem(const_sgVec3 v)	{ sgCopyVec3(value, v); }
    FGWindItem(const WeatherPrecision x, const WeatherPrecision y, const WeatherPrecision z)	
				{ sgSetVec3 (value, x, y, z); }
    FGWindItem()		{ sgZeroVec3(value);    }

    void          getValue(sgVec3 ret) const { sgCopyVec3(ret, value); };
    const sgVec3* getValue(void)       const { return &value;          };

    WeatherPrecision x(void) const { return value[0]; };
    WeatherPrecision y(void) const { return value[1]; };
    WeatherPrecision z(void) const { return value[2]; };

    FGWindItem& operator*= (const WeatherPrecision arg);
    FGWindItem& operator+= (const FGWindItem&      arg);
    FGWindItem& operator-= (const FGWindItem&      arg);

    friend FGWindItem operator-(const FGWindItem& arg);
};

inline FGWindItem& FGWindItem::operator*= (const WeatherPrecision arg)
{
  sgScaleVec3(value, arg);
  return *this;
}

inline FGWindItem& FGWindItem::operator+= (const FGWindItem& arg)
{
  sgAddVec3(value, *arg.getValue());
  return *this;
}

inline FGWindItem& FGWindItem::operator-= (const FGWindItem& arg)
{
  sgSubVec3(value, *arg.getValue());
  return *this;
}


inline FGWindItem operator-(const FGWindItem& arg)
{
    sgVec3 temp;

    sgNegateVec3(temp, *arg.getValue());

    return FGWindItem(temp);
}

/****************************************************************************/
#endif /*FGWindItem_H*/
