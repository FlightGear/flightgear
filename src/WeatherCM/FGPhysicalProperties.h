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
#include <set>

FG_USING_STD(vector);
FG_USING_STD(set);
FG_USING_NAMESPACE(std);

#include <Math/point3d.hxx>
#include <Voronoi/point2d.h>
#include "FGWeatherDefs.h"

#include "FGWindItem.h"
#include "FGTurbulenceItem.h"
#include "FGTemperatureItem.h"
#include "FGAirPressureItem.h"
#include "FGVaporPressureItem.h"

#include "FGCloudItem.h"
#include "FGSnowRain.h"

class FGPhysicalProperties
{
public:
    set<FGWindItem> Wind;		    //all Wind vectors
    set<FGTurbulenceItem> Turbulence;	    //all Turbulence vectors
    set<FGTemperatureItem> Temperature;	    //in deg. Kelvin (I *only* accept SI!)
    FGAirPressureItem AirPressure;	    //in Pascal (I *only* accept SI!)
    set<FGVaporPressureItem> VaporPressure; //in Pascal (I *only* accept SI!)

    set<FGCloudItem> Clouds;		    //amount of covering and type
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
    out << "Position: " << p.p << "\nStored Wind: ";
    for (set<FGWindItem>::const_iterator WindIt = p.Wind.begin(); WindIt != p.Wind.end(); WindIt++)
	out << "(" << WindIt->getValue() << ") at " << WindIt->getAlt() << "m; ";

    out << "\nStored Turbulence: ";
    for (set<FGTurbulenceItem>::const_iterator TurbulenceIt = p.Turbulence.begin(); 
	                                       TurbulenceIt != p.Turbulence.end(); 
	                                       TurbulenceIt++)
	out << "(" << TurbulenceIt->getValue() << ") at " << TurbulenceIt->getAlt() << "m; ";

    out << "\nStored Temperature: ";
    for (set<FGTemperatureItem>::const_iterator TemperatureIt = p.Temperature.begin(); 
	                                        TemperatureIt != p.Temperature.end(); 
	                                        TemperatureIt++)
	out << TemperatureIt->getValue() << " at " << TemperatureIt->getAlt() << "m; ";

    out << "\nStored AirPressure: ";
    out << p.AirPressure.getValue(0) << " at " << 0 << "m; ";

    out << "\nStored VaporPressure: ";
    for (set<FGVaporPressureItem>::const_iterator VaporPressureIt = p.VaporPressure.begin(); 
	                                          VaporPressureIt != p.VaporPressure.end(); 
	                                          VaporPressureIt++)
	out << VaporPressureIt->getValue() << " at " << VaporPressureIt->getAlt() << "m; ";

    return out << "\n";
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

    for (set<FGWindItem>::iterator WindIt = Wind.begin(); 
	                           WindIt != Wind.end(); 
	                           WindIt++)
	*WindIt *= d;

    for (set<FGTurbulenceItem>::iterator TurbulenceIt = Turbulence.begin(); 
                                         TurbulenceIt != Turbulence.end(); 
                                         TurbulenceIt++)
	*TurbulenceIt  *= d;

    for (set<FGTemperatureItem>::iterator TemperatureIt = Temperature.begin(); 
                                          TemperatureIt != Temperature.end(); 
                                          TemperatureIt++)
	*TemperatureIt *= d;

    AirPressure *= d;

    for (set<FGVaporPressureItem>::iterator VaporPressureIt = VaporPressure.begin(); 
                                            VaporPressureIt != VaporPressure.end(); 
                                            VaporPressureIt++)
	*VaporPressureIt *= d;

    return *this;
}

inline FGPhysicalProperties& FGPhysicalProperties::operator += (const FGPhysicalProperties& p)
{
    for (set<FGWindItem>::const_iterator WindIt = p.Wind.begin(); 
					 WindIt != p.Wind.end(); 
					 WindIt++ )
	if (WindIt != Wind.upper_bound( FGWindItem(WindIt->getAlt(), Point3D(0)) ))
	    Wind.insert(*WindIt);
	else
	    *(Wind.upper_bound( FGWindItem(WindIt->getAlt(), Point3D(0))) ) += WindIt->getValue();
	    
    for (set<FGTurbulenceItem>::const_iterator TurbulenceIt = p.Turbulence.begin(); 
					       TurbulenceIt != p.Turbulence.end(); 
                                               TurbulenceIt++)
	if (TurbulenceIt != Turbulence.upper_bound( FGTurbulenceItem(TurbulenceIt->getAlt(), Point3D(0)) ))
	    Turbulence.insert(*TurbulenceIt);
	else
	    *(Turbulence.upper_bound( FGTurbulenceItem(TurbulenceIt->getAlt(), Point3D(0)) )) += TurbulenceIt->getValue();
	    
    for (set<FGTemperatureItem>::const_iterator TemperatureIt = p.Temperature.begin(); 
                                                TemperatureIt != p.Temperature.end(); 
                                                TemperatureIt++)
	if (TemperatureIt != Temperature.upper_bound( FGTemperatureItem(TemperatureIt->getAlt(), 0.0) ))
	    Temperature.insert(*TemperatureIt);
	else
	    *(Temperature.upper_bound( FGTemperatureItem(TemperatureIt->getAlt(), 0.0) )) += TemperatureIt->getValue();
	    
    AirPressure += p.AirPressure.getValue(0.0);
	    
    for (set<FGVaporPressureItem>::const_iterator VaporPressureIt = p.VaporPressure.begin(); 
                                                  VaporPressureIt != p.VaporPressure.end(); 
                                                  VaporPressureIt++)
	if (VaporPressureIt != VaporPressure.upper_bound( FGVaporPressureItem(VaporPressureIt->getAlt(), 0.0) ))
	    VaporPressure.insert(*VaporPressureIt);
	else
	    *(VaporPressure.upper_bound( FGVaporPressureItem(VaporPressureIt->getAlt(), 0.0) )) += VaporPressureIt->getValue();

    return *this;
}

inline FGPhysicalProperties& FGPhysicalProperties::operator -= (const FGPhysicalProperties& p)
{

    for (set<FGWindItem>::const_iterator WindIt = p.Wind.begin(); 
	                                 WindIt != p.Wind.end(); 
	                                 WindIt++)
	if (WindIt != Wind.upper_bound( FGWindItem(WindIt->getAlt(), Point3D(0)) ))
	    Wind.insert(-(*WindIt));
	else
	    *(Wind.upper_bound( FGWindItem(WindIt->getAlt(), Point3D(0)) ))  -= WindIt->getValue();
	    
    for (set<FGTurbulenceItem>::const_iterator TurbulenceIt = p.Turbulence.begin(); 
                                               TurbulenceIt != p.Turbulence.end(); 
                                               TurbulenceIt++)
	if (TurbulenceIt != Turbulence.upper_bound( FGTurbulenceItem(TurbulenceIt->getAlt(), Point3D(0)) ))
	    Turbulence.insert(-(*TurbulenceIt));
	else
	    *(Turbulence.upper_bound( FGTurbulenceItem(TurbulenceIt->getAlt(), Point3D(0)) )) -= TurbulenceIt->getValue();
	    
    for (set<FGTemperatureItem>::const_iterator TemperatureIt = p.Temperature.begin(); 
	                                        TemperatureIt != p.Temperature.end(); 
                                                TemperatureIt++)
	if (TemperatureIt != Temperature.upper_bound( FGTemperatureItem(TemperatureIt->getAlt(), 0.0) ))
	    Temperature.insert(-(*TemperatureIt));
	else
	    *(Temperature.upper_bound( FGTemperatureItem(TemperatureIt->getAlt(), 0.0) )) -= TemperatureIt->getValue();
	    
    AirPressure -= p.AirPressure.getValue(0.0);
	    
    for (set<FGVaporPressureItem>::const_iterator VaporPressureIt = p.VaporPressure.begin(); 
                                                  VaporPressureIt != p.VaporPressure.end(); 
                                                  VaporPressureIt++)
	if (VaporPressureIt != VaporPressure.upper_bound( FGVaporPressureItem(VaporPressureIt->getAlt(), 0.0) ))
	    VaporPressure.insert(-(*VaporPressureIt));
	else
	    *(VaporPressure.upper_bound( FGVaporPressureItem(VaporPressureIt->getAlt(), 0.0) )) -= VaporPressureIt->getValue();

    return *this;
}


inline Point3D FGPhysicalProperties::WindAt(const WeatherPrecition& a) const
{
    set<FGWindItem>::const_iterator it = Wind.lower_bound(FGWindItem(a, Point3D(0)));
    set<FGWindItem>::const_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->getValue() - it->getValue())/(it2->getAlt() - it->getAlt()) )*
             (a - it2->getAlt()) + 
              it2->getValue(); 
}

inline Point3D FGPhysicalProperties::TurbulenceAt(const WeatherPrecition& a) const
{
    set<FGTurbulenceItem>::const_iterator it = Turbulence.lower_bound(FGTurbulenceItem(a, Point3D(0)));
    set<FGTurbulenceItem>::const_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->getValue() - it->getValue() )/(it2->getAlt() - it->getAlt()) )*
           (a - it2->getAlt())+ it2->getValue(); 
}

inline WeatherPrecition FGPhysicalProperties::TemperatureAt(const WeatherPrecition& a) const
{
    set<FGTemperatureItem>::const_iterator it = Temperature.lower_bound(FGTemperatureItem(a, 0));
    set<FGTemperatureItem>::const_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->getValue() - it->getValue()) / (it2->getAlt() - it->getAlt()) )* 
           (a - it2->getAlt() )+ it2->getValue(); 
}

inline WeatherPrecition FGPhysicalProperties::AirPressureAt(const WeatherPrecition& a) const
{
    return AirPressure.getValue(a);
}

inline WeatherPrecition FGPhysicalProperties::VaporPressureAt(const WeatherPrecition& a) const
{
    set<FGVaporPressureItem>::const_iterator it = VaporPressure.lower_bound(FGVaporPressureItem(a, 0));
    set<FGVaporPressureItem>::const_iterator it2 = it;
    it--;

    //now I've got it->alt < a < it2->alt so I can interpolate
    return ( (it2->getValue() - it->getValue() ) / (it2->getAlt() - it->getAlt() ) ) *
           (a - it2->getAlt() )+ it2->getValue(); 
}


inline FGPhysicalProperties operator * (const FGPhysicalProperties& a, const WeatherPrecition& b)
{
    return FGPhysicalProperties(a) *= b;
}

inline FGPhysicalProperties operator * (const WeatherPrecition& b, const FGPhysicalProperties& a)
{
    return FGPhysicalProperties(a) *= b;
}

inline FGPhysicalProperties operator + (const FGPhysicalProperties& a, const FGPhysicalProperties& b)
{
    return FGPhysicalProperties(a) += (FGPhysicalProperties)b;
}

inline FGPhysicalProperties operator - (const FGPhysicalProperties& a, const FGPhysicalProperties& b)
{
    return FGPhysicalProperties(a) -= (FGPhysicalProperties)b;
}

/****************************************************************************/
#endif /*FGPhysicalProperties_H*/

