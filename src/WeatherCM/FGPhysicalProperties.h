/*****************************************************************************

 Header:       FGPhysicalProperties.h	
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
#ifndef FGPhysicalProperties_H
#define FGPhysicalProperties_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <map>

#include <plib/sg.h>

#include "FGWeatherDefs.h"

#include "FGAirPressureItem.h"
#include "FGWindItem.h"
#include "FGTurbulenceItem.h"

#include "FGCloudItem.h"
#include "FGSnowRain.h"

FG_USING_STD(vector);
FG_USING_STD(map);
FG_USING_NAMESPACE(std);

/****************************************************************************/
/* FOREWARD DEFINITIONS							    */
/****************************************************************************/
class FGPhysicalProperties2D;
ostream& operator<< ( ostream& out, const FGPhysicalProperties2D& p );

class FGPhysicalProperties
{
public:
    typedef WeatherPrecision Altitude;  

    map<Altitude,FGWindItem>       Wind;	    //all Wind vectors
    map<Altitude,FGTurbulenceItem> Turbulence;	    //all Turbulence vectors
    map<Altitude,WeatherPrecision> Temperature;	    //in deg. Kelvin (I *only* accept SI!)
    FGAirPressureItem              AirPressure;	    //in Pascal (I *only* accept SI!)
    map<Altitude,WeatherPrecision> VaporPressure;   //in Pascal (I *only* accept SI!)

    map<Altitude,FGCloudItem>      Clouds;	    //amount of covering and type

    WeatherPrecision		   SnowRainIntensity;	    //this also stands for hail, snow,...
    SnowRainType     		   snowRainType;	    
    WeatherPrecision 		   LightningProbability;    //in lightnings per second   

    FGPhysicalProperties();   //consructor to fill it with FG standart weather

    //return values at specified altitudes
    void	     WindAt         (sgVec3 ret, const WeatherPrecision a) const;
    void	     TurbulenceAt   (sgVec3 ret, const WeatherPrecision a) const;
    WeatherPrecision TemperatureAt  (const WeatherPrecision a) const;
    WeatherPrecision AirPressureAt  (const WeatherPrecision x) const;	//x is used here instead of a on purpose
    WeatherPrecision VaporPressureAt(const WeatherPrecision a) const;

    //for easier access to the cloud stuff:
    unsigned int getNumberOfCloudLayers(void) const;
    FGCloudItem  getCloudLayer(unsigned int nr) const;
    
    FGPhysicalProperties& operator =  ( const FGPhysicalProperties& p ); 
    FGPhysicalProperties& operator *= ( const WeatherPrecision      d ); 
    FGPhysicalProperties& operator += ( const FGPhysicalProperties& p ); 
    FGPhysicalProperties& operator -= ( const FGPhysicalProperties& p ); 
};

class FGPhysicalProperties2D : public FGPhysicalProperties
{
public:
    sgVec2 p;	    //position of the property (lat/lon)
    friend ostream& operator<< ( ostream& out, const FGPhysicalProperties2D& p );

    FGPhysicalProperties2D() {}

    FGPhysicalProperties2D(const FGPhysicalProperties& prop, const sgVec2& pos) 
    {
	Wind = prop.Wind; 
	Turbulence = prop.Turbulence;
	Temperature = prop.Temperature;
	AirPressure = prop.AirPressure; 
	VaporPressure = prop.VaporPressure; 
	sgCopyVec2(p, pos);
    }
};

typedef vector<FGPhysicalProperties>                 FGPhysicalPropertiesVector;
typedef FGPhysicalPropertiesVector::iterator         FGPhysicalPropertiesVectorIt;
typedef FGPhysicalPropertiesVector::const_iterator   FGPhysicalPropertiesVectorConstIt;

typedef vector<FGPhysicalProperties2D>               FGPhysicalProperties2DVector;
typedef FGPhysicalProperties2DVector::iterator       FGPhysicalProperties2DVectorIt;
typedef FGPhysicalProperties2DVector::const_iterator FGPhysicalProperties2DVectorConstIt;



inline FGPhysicalProperties& FGPhysicalProperties::operator = ( const FGPhysicalProperties& p )
{
    Wind          = p.Wind; 
    Turbulence    = p.Turbulence; 
    Temperature   = p.Temperature;
    AirPressure   = p.AirPressure; 
    VaporPressure = p.VaporPressure; 
    return *this;
}

inline FGPhysicalProperties& FGPhysicalProperties::operator *= ( const WeatherPrecision d )
{
    typedef map<FGPhysicalProperties::Altitude, FGWindItem      >::iterator wind_iterator;
    typedef map<FGPhysicalProperties::Altitude, FGTurbulenceItem>::iterator turbulence_iterator;
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecision>::iterator scalar_iterator;

    for (wind_iterator       WindIt = Wind.begin(); 
			     WindIt != Wind.end(); 
			     WindIt++)
	WindIt->second*= d;

    for (turbulence_iterator TurbulenceIt = Turbulence.begin(); 
			     TurbulenceIt != Turbulence.end(); 
			     TurbulenceIt++)
	TurbulenceIt->second *= d;

    for (scalar_iterator     TemperatureIt = Temperature.begin(); 
                             TemperatureIt != Temperature.end(); 
                             TemperatureIt++)
	TemperatureIt->second *= d;

    AirPressure *= d;

    for (scalar_iterator     VaporPressureIt = VaporPressure.begin(); 
                             VaporPressureIt != VaporPressure.end(); 
                             VaporPressureIt++)
	VaporPressureIt->second *= d;

    return *this;
}

inline FGPhysicalProperties& FGPhysicalProperties::operator += (const FGPhysicalProperties& p)
{
    typedef map<FGPhysicalProperties::Altitude, FGWindItem      >::const_iterator wind_iterator;
    typedef map<FGPhysicalProperties::Altitude, FGTurbulenceItem>::const_iterator turbulence_iterator;
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecision>::const_iterator scalar_iterator;

    for (wind_iterator       WindIt = p.Wind.begin(); 
			     WindIt != p.Wind.end(); 
			     WindIt++)
	if (!Wind.insert(*WindIt).second)	    //when it's not inserted => it's already existing
	    Wind[WindIt->first] += WindIt->second;  //=> add the value
	    
    for (turbulence_iterator TurbulenceIt = p.Turbulence.begin(); 
			     TurbulenceIt != p.Turbulence.end(); 
			     TurbulenceIt++)
	if (!Turbulence.insert(*TurbulenceIt).second)	
	    Turbulence[TurbulenceIt->first] += TurbulenceIt->second;

    for (scalar_iterator     TemperatureIt = p.Temperature.begin(); 
                             TemperatureIt != p.Temperature.end(); 
                             TemperatureIt++)
	if (!Temperature.insert(*TemperatureIt).second)	
	    Temperature[TemperatureIt->first] += TemperatureIt->second;
	    
    AirPressure += p.AirPressure.getValue();
	    
    for (scalar_iterator     VaporPressureIt = p.VaporPressure.begin(); 
                             VaporPressureIt != p.VaporPressure.end(); 
                             VaporPressureIt++)
	if (!VaporPressure.insert(*VaporPressureIt).second)	
	    VaporPressure[VaporPressureIt->first] += VaporPressureIt->second;

    return *this;
}

inline FGPhysicalProperties& FGPhysicalProperties::operator -= (const FGPhysicalProperties& p)
{
    typedef map<FGPhysicalProperties::Altitude, FGWindItem      >::const_iterator wind_iterator;
    typedef map<FGPhysicalProperties::Altitude, FGTurbulenceItem>::const_iterator turbulence_iterator;
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecision>::const_iterator scalar_iterator;

    for (wind_iterator       WindIt = p.Wind.begin(); 
			     WindIt != p.Wind.end(); 
			     WindIt++)
	if (!Wind.insert( make_pair(WindIt->first, -WindIt->second) ).second)	//when it's not inserted => it's already existing
	    Wind[WindIt->first] -= WindIt->second;				//=> substract the value
	    
    for (turbulence_iterator TurbulenceIt = p.Turbulence.begin(); 
			     TurbulenceIt != p.Turbulence.end(); 
			     TurbulenceIt++)
	if (!Turbulence.insert( make_pair(TurbulenceIt->first, -TurbulenceIt->second) ).second)	
	    Turbulence[TurbulenceIt->first] -= TurbulenceIt->second;
	    
    for (scalar_iterator     TemperatureIt = p.Temperature.begin(); 
                             TemperatureIt != p.Temperature.end(); 
                             TemperatureIt++)
	if (!Temperature.insert( make_pair(TemperatureIt->first, -TemperatureIt->second) ).second)	
	    Temperature[TemperatureIt->first] -= TemperatureIt->second;
	    
    AirPressure -= p.AirPressure.getValue();
	    
    for (scalar_iterator     VaporPressureIt = p.VaporPressure.begin(); 
                             VaporPressureIt != p.VaporPressure.end(); 
                             VaporPressureIt++)
	if (!VaporPressure.insert( make_pair(VaporPressureIt->first, -VaporPressureIt->second) ).second)	
	    VaporPressure[VaporPressureIt->first] -= VaporPressureIt->second;


    return *this;
}

inline void FGPhysicalProperties::WindAt(sgVec3 ret, const WeatherPrecision a) const
{
    typedef map<FGPhysicalProperties::Altitude, FGWindItem>::const_iterator vector_iterator;

    vector_iterator it = Wind.lower_bound(a);
    vector_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    sgSubVec3(ret, *it2->second.getValue(), *it->second.getValue());
    sgScaleVec3(ret, (a - it2->first) / (it2->first - it->first));
    sgAddVec3(ret, *it2->second.getValue());
}

inline void FGPhysicalProperties::TurbulenceAt(sgVec3 ret, const WeatherPrecision a) const
{
    typedef map<FGPhysicalProperties::Altitude, FGTurbulenceItem>::const_iterator vector_iterator;

    vector_iterator it = Turbulence.lower_bound(a);
    vector_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    sgSubVec3(ret, *it2->second.getValue(), *it->second.getValue());
    sgScaleVec3(ret, (a - it2->first) / (it2->first - it->first));
    sgAddVec3(ret, *it2->second.getValue());
}

inline WeatherPrecision FGPhysicalProperties::TemperatureAt(const WeatherPrecision a) const
{
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecision>::const_iterator scalar_iterator;

    scalar_iterator it = Temperature.lower_bound(a);
    scalar_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->second - it->second)/(it2->first - it->first) ) * (a - it2->first) + it2->second; 
}

//inline WeatherPrecision FGPhysicalProperties::AirPressureAt(const WeatherPrecision x) const
//moved to FGPhysicalProperties.cpp as it got too complex to inline

inline WeatherPrecision FGPhysicalProperties::VaporPressureAt(const WeatherPrecision a) const
{
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecision>::const_iterator scalar_iterator;

    scalar_iterator it = VaporPressure.lower_bound(a);
    scalar_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->second - it->second)/(it2->first - it->first) ) * (a - it2->first) + it2->second; 
}


inline FGPhysicalProperties operator * (FGPhysicalProperties a, const WeatherPrecision b)
{
    return a *= b;
}

inline FGPhysicalProperties operator * (const WeatherPrecision b, FGPhysicalProperties a)
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

