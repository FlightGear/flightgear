/*******-*- Mode: C++ -*-************************************************************

 Header:       FGPhysicalProperties.h	
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
Define the simulated physical properties of the weather

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	Changed struct to class 
20.06.1999 Christian Mayer	added lots of consts
30.06.1999 Christian Mayer	STL portability
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGPhysicalProperties_H
#define FGPhysicalProperties_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <Include/compiler.h>
#include <vector>
#include <map>

FG_USING_STD(vector);
FG_USING_STD(map);
FG_USING_NAMESPACE(std);

#include <Math/point3d.hxx>
#include <Voronoi/point2d.h>
#include "FGWeatherDefs.h"

#include "FGAirPressureItem.h"

#include "FGCloudItem.h"
#include "FGSnowRain.h"

class FGPhysicalProperties
{
public:
    typedef WeatherPrecition Altitude;  

    map<Altitude,Point3D> Wind;			    //all Wind vectors
    map<Altitude,Point3D> Turbulence;		    //all Turbulence vectors
    map<Altitude,WeatherPrecition> Temperature;	    //in deg. Kelvin (I *only* accept SI!)
    FGAirPressureItem AirPressure;		    //in Pascal (I *only* accept SI!)
    map<Altitude,WeatherPrecition> VaporPressure;   //in Pascal (I *only* accept SI!)

    map<Altitude,FGCloudItem> Clouds;		    //amount of covering and type

    WeatherPrecition SnowRainIntensity;     //this also stands for hail, snow,...
    SnowRainType     snowRainType;	    
    WeatherPrecition LightningProbability;

    FGPhysicalProperties();   //consructor to fill it with FG standart weather

    //return values at specified altitudes
    Point3D WindAt(const WeatherPrecition& a) const;
    Point3D TurbulenceAt(const WeatherPrecition& a) const;
    WeatherPrecition TemperatureAt(const WeatherPrecition& a) const;
    WeatherPrecition AirPressureAt(const WeatherPrecition& a) const;
    WeatherPrecition VaporPressureAt(const WeatherPrecition& a) const;

    //for easier access to the cloud stuff:
    unsigned int getNumberOfCloudLayers(void) const;
    FGCloudItem getCloudLayer(unsigned int nr) const;
    
    FGPhysicalProperties& operator = ( const FGPhysicalProperties& p ); 
    FGPhysicalProperties& operator *= ( const WeatherPrecition& d ); 
    FGPhysicalProperties& operator += ( const FGPhysicalProperties& p); 
    FGPhysicalProperties& operator -= ( const FGPhysicalProperties& p); 
};

typedef vector<FGPhysicalProperties> FGPhysicalPropertiesVector;
typedef FGPhysicalPropertiesVector::iterator FGPhysicalPropertiesVectorIt;
typedef FGPhysicalPropertiesVector::const_iterator FGPhysicalPropertiesVectorConstIt;

class FGPhysicalProperties2D;
ostream& operator<< ( ostream& out, const FGPhysicalProperties2D& p );

class FGPhysicalProperties2D : public FGPhysicalProperties
{
public:
    Point2D p;	    //position of the property (lat/lon)
    friend ostream& operator<< ( ostream& out, const FGPhysicalProperties2D& p );

    FGPhysicalProperties2D() {}

    FGPhysicalProperties2D(const FGPhysicalProperties& prop, const Point2D& pos) 
    {
	Wind = prop.Wind; Turbulence = prop.Turbulence; Temperature = prop.Temperature;
	AirPressure = prop.AirPressure; VaporPressure = prop.VaporPressure; p = pos;
    }
};

typedef vector<FGPhysicalProperties2D> FGPhysicalProperties2DVector;
typedef FGPhysicalProperties2DVector::iterator FGPhysicalProperties2DVectorIt;
typedef FGPhysicalProperties2DVector::const_iterator FGPhysicalProperties2DVectorConstIt;

inline ostream& operator<< ( ostream& out, const FGPhysicalProperties2D& p )
{
    typedef map<FGPhysicalProperties::Altitude, Point3D         >::const_iterator vector_iterator;
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecition>::const_iterator scalar_iterator;

    out << "Position: " << p.p << endl;
    
    out << "Stored Wind: ";
    for (vector_iterator WindIt = p.Wind.begin(); 
			 WindIt != p.Wind.end(); 
			 WindIt++)
	out << "(" << WindIt->first << ") at " << WindIt->second << "m; ";
    out << endl;

    out << "Stored Turbulence: ";
    for (vector_iterator TurbulenceIt = p.Turbulence.begin(); 
			 TurbulenceIt != p.Turbulence.end(); 
			 TurbulenceIt++)
	out << "(" << TurbulenceIt->first << ") at " << TurbulenceIt->second << "m; ";
    out << endl;

    out << "Stored Temperature: ";
    for (scalar_iterator TemperatureIt = p.Temperature.begin(); 
			 TemperatureIt != p.Temperature.end(); 
			 TemperatureIt++)
	out << TemperatureIt->first << " at " << TemperatureIt->second << "m; ";
    out << endl;

    out << "Stored AirPressure: ";
    out << p.AirPressure.getValue(0) << " at " << 0.0 << "m; ";
    out << endl;

    out << "Stored VaporPressure: ";
    for (scalar_iterator VaporPressureIt = p.VaporPressure.begin(); 
			 VaporPressureIt != p.VaporPressure.end(); 
			 VaporPressureIt++)
	out << VaporPressureIt->first << " at " << VaporPressureIt->second << "m; ";
    out << endl;

    return out << endl;
}


inline FGPhysicalProperties& FGPhysicalProperties::operator = ( const FGPhysicalProperties& p )
{
    Wind          = p.Wind; 
    Turbulence    = p.Turbulence; 
    Temperature   = p.Temperature;
    AirPressure   = p.AirPressure; 
    VaporPressure = p.VaporPressure; 
    return *this;
}

inline FGPhysicalProperties& FGPhysicalProperties::operator *= ( const WeatherPrecition& d )
{
    typedef map<FGPhysicalProperties::Altitude, Point3D         >::iterator vector_iterator;
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecition>::iterator scalar_iterator;

    for (vector_iterator WindIt = Wind.begin(); 
	                 WindIt != Wind.end(); 
	                 WindIt++)
	WindIt->second *= d;

    for (vector_iterator TurbulenceIt = Turbulence.begin(); 
                         TurbulenceIt != Turbulence.end(); 
                         TurbulenceIt++)
	TurbulenceIt->second  *= d;

    for (scalar_iterator TemperatureIt = Temperature.begin(); 
                         TemperatureIt != Temperature.end(); 
                         TemperatureIt++)
	TemperatureIt->second *= d;

    AirPressure *= d;

    for (scalar_iterator VaporPressureIt = VaporPressure.begin(); 
                         VaporPressureIt != VaporPressure.end(); 
                         VaporPressureIt++)
	VaporPressureIt->second *= d;

    return *this;
}

inline FGPhysicalProperties& FGPhysicalProperties::operator += (const FGPhysicalProperties& p)
{
    typedef map<FGPhysicalProperties::Altitude, Point3D         >::const_iterator vector_iterator;
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecition>::const_iterator scalar_iterator;

    for (vector_iterator WindIt = p.Wind.begin(); 
			 WindIt != p.Wind.end(); 
			 WindIt++ )
	if (!Wind.insert(*WindIt).second)	    //when it's not inserted => it's already existing
	    Wind[WindIt->first] += WindIt->second;  //=> add the value
	    
    for (vector_iterator TurbulenceIt = p.Turbulence.begin(); 
			 TurbulenceIt != p.Turbulence.end(); 
                         TurbulenceIt++)
	if (!Turbulence.insert(*TurbulenceIt).second)	
	    Turbulence[TurbulenceIt->first] += TurbulenceIt->second;
	    
    for (scalar_iterator TemperatureIt = p.Temperature.begin(); 
                         TemperatureIt != p.Temperature.end(); 
                         TemperatureIt++)
	if (!Temperature.insert(*TemperatureIt).second)	
	    Temperature[TemperatureIt->first] += TemperatureIt->second;
	    
    AirPressure += p.AirPressure.getValue(0.0);
	    
    for (scalar_iterator VaporPressureIt = p.VaporPressure.begin(); 
                         VaporPressureIt != p.VaporPressure.end(); 
                         VaporPressureIt++)
	if (!VaporPressure.insert(*VaporPressureIt).second)	
	    VaporPressure[VaporPressureIt->first] += VaporPressureIt->second;

    return *this;
}

inline FGPhysicalProperties& FGPhysicalProperties::operator -= (const FGPhysicalProperties& p)
{
    typedef map<FGPhysicalProperties::Altitude, Point3D         >::const_iterator vector_iterator;
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecition>::const_iterator scalar_iterator;

    for (vector_iterator WindIt = p.Wind.begin(); 
			 WindIt != p.Wind.end(); 
			 WindIt++ )
	if (!Wind.insert( make_pair(WindIt->first, -WindIt->second) ).second)	//when it's not inserted => it's already existing
	    Wind[WindIt->first] -= WindIt->second;				//=> substract the value
	    
    for (vector_iterator TurbulenceIt = p.Turbulence.begin(); 
			 TurbulenceIt != p.Turbulence.end(); 
                         TurbulenceIt++)
	if (!Turbulence.insert( make_pair(TurbulenceIt->first, -TurbulenceIt->second) ).second)	
	    Turbulence[TurbulenceIt->first] -= TurbulenceIt->second;
	    
    for (scalar_iterator TemperatureIt = p.Temperature.begin(); 
                         TemperatureIt != p.Temperature.end(); 
                         TemperatureIt++)
	if (!Temperature.insert( make_pair(TemperatureIt->first, -TemperatureIt->second) ).second)	
	    Temperature[TemperatureIt->first] -= TemperatureIt->second;
	    
    AirPressure -= p.AirPressure.getValue(0.0);
	    
    for (scalar_iterator VaporPressureIt = p.VaporPressure.begin(); 
                         VaporPressureIt != p.VaporPressure.end(); 
                         VaporPressureIt++)
	if (!VaporPressure.insert( make_pair(VaporPressureIt->first, -VaporPressureIt->second) ).second)	
	    VaporPressure[VaporPressureIt->first] -= VaporPressureIt->second;


    return *this;
}


inline Point3D FGPhysicalProperties::WindAt(const WeatherPrecition& a) const
{
    typedef map<FGPhysicalProperties::Altitude, Point3D>::const_iterator vector_iterator;

    vector_iterator it = Wind.lower_bound(a);
    vector_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->second - it->second)/(it2->first - it->first) ) * (a - it2->first) + it2->second; 
}

inline Point3D FGPhysicalProperties::TurbulenceAt(const WeatherPrecition& a) const
{
    typedef map<FGPhysicalProperties::Altitude, Point3D>::const_iterator vector_iterator;

    vector_iterator it = Turbulence.lower_bound(a);
    vector_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->second - it->second)/(it2->first - it->first) ) * (a - it2->first) + it2->second; 
}

inline WeatherPrecition FGPhysicalProperties::TemperatureAt(const WeatherPrecition& a) const
{
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecition>::const_iterator scalar_iterator;

    scalar_iterator it = Temperature.lower_bound(a);
    scalar_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->second - it->second)/(it2->first - it->first) ) * (a - it2->first) + it2->second; 
}

inline WeatherPrecition FGPhysicalProperties::AirPressureAt(const WeatherPrecition& a) const
{
    return AirPressure.getValue(a);
}

inline WeatherPrecition FGPhysicalProperties::VaporPressureAt(const WeatherPrecition& a) const
{
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecition>::const_iterator scalar_iterator;

    scalar_iterator it = VaporPressure.lower_bound(a);
    scalar_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->second - it->second)/(it2->first - it->first) ) * (a - it2->first) + it2->second; 
}


inline FGPhysicalProperties operator * (FGPhysicalProperties a, const WeatherPrecition& b)
{
    return a *= b;
}

inline FGPhysicalProperties operator * (const WeatherPrecition& b, FGPhysicalProperties a)
{
    return a *= b;
}

inline FGPhysicalProperties operator + (FGPhysicalProperties a, const FGPhysicalProperties& b)
{
    return a += b;
}

inline FGPhysicalProperties operator - (FGPhysicalProperties a, const FGPhysicalProperties& b)
{
    return a -= b;
}

/****************************************************************************/
#endif /*FGPhysicalProperties_H*/

