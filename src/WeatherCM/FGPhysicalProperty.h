/*****************************************************************************

 Header:       FGPhysicalProperty.h	
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
Define the simulated physical property of the weather in one point

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer      Changed struct to class
20.06.1999 Christian Mayer	added lots of consts
30.06.1999 Christian Mayer	STL portability
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGPhysicalProperty_H
#define FGPhysicalProperty_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <Include/compiler.h>
#include <vector>
FG_USING_STD(vector);
FG_USING_NAMESPACE(std);

#include <Math/point3d.hxx>
#include <Voronoi/point2d.h>
#include "FGWeatherDefs.h"
#include "FGPhysicalProperties.h"

/****************************************************************************/
/* used for output:							    */
/****************************************************************************/
class FGPhysicalProperty
{
private:
protected:
public:
    Point3D Wind;			//Wind vector
    Point3D Turbulence;			//Turbulence vector
    WeatherPrecition Temperature;	//in deg. Kelvin (I *only* accept SI!)
    WeatherPrecition AirPressure;	//in Pascal (I *only* accept SI!)
    WeatherPrecition VaporPressure;	//in Pascal (I *only* accept SI!)

    FGPhysicalProperty();   //consructor to fill it with FG standart weather
    FGPhysicalProperty(const FGPhysicalProperties& p, const WeatherPrecition& altitude);

    //allow calculations for easier handling such as interpolating
    FGPhysicalProperty& operator = ( const FGPhysicalProperty& p );	 // assignment of a Point3D
    FGPhysicalProperty& operator += ( const FGPhysicalProperty& p );	 // incrementation by a Point3D
    FGPhysicalProperty& operator -= ( const FGPhysicalProperty& p );	 // decrementation by a Point3D
    FGPhysicalProperty& operator *= ( const double& d );		 // multiplication by a constant
    FGPhysicalProperty& operator /= ( const double& d );		 // division by a constant

    friend FGPhysicalProperty operator - (const FGPhysicalProperty& p);	            // -p1
    friend bool operator == (const FGPhysicalProperty& a, const FGPhysicalProperty& b);  // p1 == p2?
};

typedef vector<FGPhysicalProperty> FGPhysicalPropertyVector;
typedef FGPhysicalPropertyVector::iterator FGPhysicalPropertyVectorIt;
typedef FGPhysicalPropertyVector::const_iterator FGPhysicalPropertyVectorConstIt;

class FGPhysicalProperty3D : public FGPhysicalProperty
{
private:
protected:
public:
    Point3D p;	    //position of the property (lat/lon/alt)
};

typedef vector<FGPhysicalProperty3D> FGPhysicalProperty3DVector;
typedef FGPhysicalProperty3DVector::iterator FGPhysicalProperty3DVectorIt;
typedef FGPhysicalProperty3DVector::const_iterator FGPhysicalProperty3DVectorConstIt;

inline FGPhysicalProperty& FGPhysicalProperty::operator = ( const FGPhysicalProperty& p )
{
    Wind = p.Wind; 
    Turbulence = p.Turbulence; 
    Temperature = p.Temperature; 
    AirPressure = p.AirPressure; 
    VaporPressure = p.VaporPressure; 
    return *this;
}

inline FGPhysicalProperty& FGPhysicalProperty::operator += ( const FGPhysicalProperty& p )
{
    Wind += p.Wind; 
    Turbulence += p.Turbulence; 
    Temperature += p.Temperature; 
    AirPressure += p.AirPressure; 
    VaporPressure += p.VaporPressure; 
    return *this;
}

inline FGPhysicalProperty& FGPhysicalProperty::operator -= ( const FGPhysicalProperty& p )
{
    Wind -= p.Wind; 
    Turbulence -= p.Turbulence; 
    Temperature -= p.Temperature; 
    AirPressure -= p.AirPressure; 
    VaporPressure -= p.VaporPressure; 
    return *this;
}

inline FGPhysicalProperty& FGPhysicalProperty::operator *= ( const double& d )
{
    Wind *= d; 
    Turbulence *= d; 
    Temperature *= d; 
    AirPressure *= d; 
    VaporPressure *= d; 
    return *this;
}

inline FGPhysicalProperty& FGPhysicalProperty::operator /= ( const double& d )
{
    Wind /= d; 
    Turbulence /= d; 
    Temperature /= d; 
    AirPressure /= d; 
    VaporPressure /= d; 
    return *this;
}

inline  FGPhysicalProperty operator - (const FGPhysicalProperty& p)
{
    FGPhysicalProperty x;
    x.Wind = -p.Wind; 
    x.Turbulence = -p.Turbulence; 
    x.Temperature = -p.Temperature; 
    x.AirPressure = -p.AirPressure; 
    x.VaporPressure = -p.VaporPressure; 
    return x;
}

inline bool operator == (const FGPhysicalProperty& a, const FGPhysicalProperty& b)
{
    return (
    (a.Wind == b.Wind) &&
    (a.Turbulence == b.Turbulence) && 
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

inline FGPhysicalProperty operator * (const FGPhysicalProperty& a, const WeatherPrecition& b)
{
    return FGPhysicalProperty(a) *= b;
}

inline FGPhysicalProperty operator * (const WeatherPrecition& b, const FGPhysicalProperty& a)
{
    return FGPhysicalProperty(a) *= b;
}

inline FGPhysicalProperty operator / (const FGPhysicalProperty& a, const WeatherPrecition& b)
{
    return FGPhysicalProperty(a) *= (1.0/b);
}


/****************************************************************************/
#endif /*FGPhysicalProperty_H*/
