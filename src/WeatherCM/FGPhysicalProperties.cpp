/*****************************************************************************

 Module:       FGPhysicalProperties.cpp
 Author:       Christian Mayer
 Date started: 28.05.99
 Called by:    main program

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
Initialice the FGPhysicalProperties struct to something sensible(?)

HISTORY
------------------------------------------------------------------------------
29.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include "FGPhysicalProperties.h"
#include "FGWeatherDefs.h"

#include "FGWeatherUtils.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/
FGPhysicalProperties::FGPhysicalProperties()
{
    sgVec3 zero;
    sgZeroVec3( zero );
    /************************************************************************/
    /* This standart constructor fills the class with a standard weather    */
    /************************************************************************/

    Wind[-1000.0] = FGWindItem(zero);		    //no Wind by default
    Wind[10000.0] = FGWindItem(zero);		    //no Wind by default

    Turbulence[-1000.0] = FGTurbulenceItem(zero);   //no Turbulence by default
    Turbulence[10000.0] = FGTurbulenceItem(zero);   //no Turbulence by default

    //Initialice with the CINA atmosphere
    Temperature[    0.0] = +15.0 + 273.16;  
    Temperature[11000.0] = -56.5 + 273.16;			    
    Temperature[20000.0] = -56.5 + 273.16;			    

    AirPressure = FGAirPressureItem(101325.0);	

    VaporPressure[    0.0] = FG_WEATHER_DEFAULT_VAPORPRESSURE;   //in Pa (I *only* accept SI!)
    VaporPressure[10000.0] = FG_WEATHER_DEFAULT_VAPORPRESSURE;   //in Pa (I *only* accept SI!)

    //Clouds.insert(FGCloudItem())    => none
    SnowRainIntensity = 0.0;     
    snowRainType = Rain;	    
    LightningProbability = 0.0;
}

unsigned int FGPhysicalProperties::getNumberOfCloudLayers(void) const
{
    return Clouds.size();
}

FGCloudItem FGPhysicalProperties::getCloudLayer(unsigned int nr) const
{
    map<WeatherPrecision,FGCloudItem>::const_iterator CloudsIt = Clouds.begin();

    //set the iterator to the 'nr'th entry
    for (; nr > 0; nr--)
	CloudsIt++;

    return CloudsIt->second;
}

ostream& operator<< ( ostream& out, const FGPhysicalProperties2D& p )
{
    typedef map<FGPhysicalProperties::Altitude, FGWindItem      >::const_iterator wind_iterator;
    typedef map<FGPhysicalProperties::Altitude, FGTurbulenceItem>::const_iterator turbulence_iterator;
    typedef map<FGPhysicalProperties::Altitude, WeatherPrecision>::const_iterator scalar_iterator;

    out << "Position: (" << p.p[0] << ", " << p.p[1] << ", " << p.p[2] << ")\n";
    
    out << "Stored Wind: ";
    for (wind_iterator       WindIt = p.Wind.begin(); 
			     WindIt != p.Wind.end(); 
			     WindIt++)
	out << "(" << WindIt->second.x() << ", " << WindIt->second.y() << ", " << WindIt->second.z() << ") m/s at (" << WindIt->first << ") m; ";
    out << "\n";

    out << "Stored Turbulence: ";
    for (turbulence_iterator TurbulenceIt = p.Turbulence.begin(); 
			     TurbulenceIt != p.Turbulence.end(); 
			     TurbulenceIt++)
	out << "(" << TurbulenceIt->second.x() << ", " << TurbulenceIt->second.y() << ", " << TurbulenceIt->second.z() << ") m/s at (" << TurbulenceIt->first << ") m; ";
    out << "\n";

    out << "Stored Temperature: ";
    for (scalar_iterator     TemperatureIt = p.Temperature.begin(); 
			     TemperatureIt != p.Temperature.end(); 
			     TemperatureIt++)
	out << Kelvin2Celsius(TemperatureIt->second) << " degC at " << TemperatureIt->first << "m; ";
    out << "\n";

    out << "Stored AirPressure: ";
    out << p.AirPressure.getValue(0)/100.0 << " hPa at " << 0.0 << "m; ";
    out << "\n";

    out << "Stored VaporPressure: ";
    for (scalar_iterator     VaporPressureIt = p.VaporPressure.begin(); 
			     VaporPressureIt != p.VaporPressure.end(); 
			     VaporPressureIt++)
	out << VaporPressureIt->second/100.0 << " hPa at " << VaporPressureIt->first << "m; ";
    out << "\n";

    return out << "\n";
}



