/*****************************************************************************

 Header:       FGPhysicalProperty.h	
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
Define the simulated physical property of the weather in one point

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer      Changed struct to class
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
#ifndef FGPhysicalProperty_H
#define FGPhysicalProperty_H

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

#include <vector>

#include <plib/sg.h>

#include "FGWeatherDefs.h"
#include "FGPhysicalProperties.h"

SG_USING_STD(vector);
SG_USING_NAMESPACE(std);

/****************************************************************************/
/* used for output:							    */
/****************************************************************************/
class FGPhysicalProperty;
bool operator == (const FGPhysicalProperty& a, const FGPhysicalProperty& b);  // p1 == p2?

class FGPhysicalProperty
{
private:
protected:
public:
    sgVec3           Wind;		//Wind vector
    sgVec3           Turbulence;	//Turbulence vector
    WeatherPrecision Temperature;	//in deg. Kelvin (I *only* accept SI!)
    WeatherPrecision AirPressure;	//in Pascal (I *only* accept SI!)
    WeatherPrecision VaporPressure;	//in Pascal (I *only* accept SI!)

    FGPhysicalProperty();   //consructor to fill it with FG standart weather
    FGPhysicalProperty(const FGPhysicalProperties& p, const WeatherPrecision altitude);

    //allow calculations for easier handling such as interpolating
    FGPhysicalProperty& operator =  ( const FGPhysicalProperty& p );	// assignment of a FGPhysicalProperty
    FGPhysicalProperty& operator += ( const FGPhysicalProperty& p );	// incrementation by a FGPhysicalProperty
    FGPhysicalProperty& operator -= ( const FGPhysicalProperty& p );	// decrementation by a FGPhysicalProperty
    FGPhysicalProperty& operator *= ( const double d );			// multiplication by a constant
    FGPhysicalProperty& operator /= ( const double d );			// division by a constant

    friend FGPhysicalProperty operator - (const FGPhysicalProperty& p);			// -p1
    friend bool operator == (const FGPhysicalProperty& a, const FGPhysicalProperty& b);	// p1 == p2?
};

class FGPhysicalProperty3D : public FGPhysicalProperty
{
private:
protected:
public:
    sgVec3 p;	    //position of the property (lat/lon/alt)
};

typedef vector<FGPhysicalProperty>               FGPhysicalPropertyVector;
typedef FGPhysicalPropertyVector::iterator       FGPhysicalPropertyVectorIt;
typedef FGPhysicalPropertyVector::const_iterator FGPhysicalPropertyVectorConstIt;

typedef vector<FGPhysicalProperty3D>               FGPhysicalProperty3DVector;
typedef FGPhysicalProperty3DVector::iterator       FGPhysicalProperty3DVectorIt;
typedef FGPhysicalProperty3DVector::const_iterator FGPhysicalProperty3DVectorConstIt;

inline FGPhysicalProperty& FGPhysicalProperty::operator = ( const FGPhysicalProperty& p )
{
    sgCopyVec3(Wind, p.Wind); 
    sgCopyVec3(Turbulence, p.Turbulence); 
    Temperature = p.Temperature; 
    AirPressure = p.AirPressure; 
    VaporPressure = p.VaporPressure; 
    return *this;
}

inline FGPhysicalProperty& FGPhysicalProperty::operator += ( const FGPhysicalProperty& p )
{
    sgAddVec3(Wind, p.Wind); 
    sgAddVec3(Turbulence, p.Turbulence); 
    Temperature += p.Temperature; 
    AirPressure += p.AirPressure; 
    VaporPressure += p.VaporPressure; 
    return *this;
}

inline FGPhysicalProperty& FGPhysicalProperty::operator -= ( const FGPhysicalProperty& p )
{
    sgSubVec3(Wind, p.Wind); 
    sgSubVec3(Turbulence, p.Turbulence); 
    Temperature -= p.Temperature; 
    AirPressure -= p.AirPressure; 
    VaporPressure -= p.VaporPressure; 
    return *this;
}

inline FGPhysicalProperty& FGPhysicalProperty::operator *= ( const double d )
{
    sgScaleVec3(Wind, d); 
    sgScaleVec3(Turbulence, d); 
    Temperature *= d; 
    AirPressure *= d; 
    VaporPressure *= d; 
    return *this;
}

inline FGPhysicalProperty& FGPhysicalProperty::operator /= ( const double d )
{
    sgScaleVec3(Wind, 1.0 / d); 
    sgScaleVec3(Turbulence, 1.0 / d); 
    Temperature /= d; 
    AirPressure /= d; 
    VaporPressure /= d; 
    return *this;
}

inline  FGPhysicalProperty operator - (const FGPhysicalProperty& p)
{
    FGPhysicalProperty x;
    sgNegateVec3(x.Wind, p.Wind); 
    sgNegateVec3(x.Turbulence, p.Turbulence); 
    x.Temperature = -p.Temperature; 
    x.AirPressure = -p.AirPressure; 
    x.VaporPressure = -p.VaporPressure; 
    return x;
}

inline bool operator == (const FGPhysicalProperty& a, const FGPhysicalProperty& b)
{
    return (
    sgEqualVec3(a.Wind, b.Wind) &&
    sgEqualVec3(a.Turbulence, b.Turbulence) && 
    (a.Temperature == b.Temperature) && 
    (a.AirPressure == b.AirPressure) && 
    (a.VaporPressure == b.VaporPressure)); 
}

inline bool operator != (const FGPhysicalProperty& a, const FGPhysicalProperty& b)
{
    return !(a == b);
}

inline FGPhysicalProperty operator + (const FGPhysicalProperty& a, const FGPhysicalProperty& b)
{
    return FGPhysicalProperty(a) += b;
}

inline FGPhysicalProperty operator - (const FGPhysicalProperty& a, const FGPhysicalProperty& b)
{
    return FGPhysicalProperty(a) -= b;
}

inline FGPhysicalProperty operator * (const FGPhysicalProperty& a, const WeatherPrecision b)
{
    return FGPhysicalProperty(a) *= b;
}

inline FGPhysicalProperty operator * (const WeatherPrecision b, const FGPhysicalProperty& a)
{
    return FGPhysicalProperty(a) *= b;
}

inline FGPhysicalProperty operator / (const FGPhysicalProperty& a, const WeatherPrecision b)
{
    return FGPhysicalProperty(a) *= (1.0/b);
}

/****************************************************************************/
#endif /*FGPhysicalProperty_H*/
