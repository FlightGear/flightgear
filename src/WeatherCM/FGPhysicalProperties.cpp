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

    VaporPressure[-1000.0] = FG_WEATHER_DEFAULT_VAPORPRESSURE;   //in Pa (I *only* accept SI!)
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
    out << p.AirPressure.getValue()/100.0 << " hPa at " << 0.0 << "m; ";
    out << "\n";

    out << "Stored VaporPressure: ";
    for (scalar_iterator     VaporPressureIt = p.VaporPressure.begin(); 
			     VaporPressureIt != p.VaporPressure.end(); 
			     VaporPressureIt++)
	out << VaporPressureIt->second/100.0 << " hPa at " << VaporPressureIt->first << "m; ";
    out << "\n";

    return out << "\n";
}


inline double F(const WeatherPrecision factor, const WeatherPrecision a, const WeatherPrecision b, const WeatherPrecision r, const WeatherPrecision x)
{
    const double c = 1.0 / (-b + a * r);
    return factor * c * ( 1.0 / (r + x) + a * c * log(fabs((r + x) * (b + a * x))) );
}

WeatherPrecision FGPhysicalProperties::AirPressureAt(const WeatherPrecision x) const
{
    const double rho0 = (AirPressure.getValue()*FG_WEATHER_DEFAULT_AIRDENSITY*FG_WEATHER_DEFAULT_TEMPERATURE)/(TemperatureAt(0)*FG_WEATHER_DEFAULT_AIRPRESSURE);
    const double G = 6.673e-11;    //Gravity; in m^3 kg^-1 s^-2
    const double m = 5.977e24;	    //mass of the earth in kg
    const double r = 6368e3;	    //radius of the earth in metres
    const double factor = -(rho0 * TemperatureAt(0) * G * m) / AirPressure.getValue();

    double a, b, FF = 0.0;

    //ok, integrate from 0 to a now.
    if (Temperature.size() < 2)
    {	//take care of the case that there aren't enough points
	//actually this should be impossible...

	if (Temperature.size() == 0)
	{
	    cerr << "ERROR in FGPhysicalProperties: Air pressure at " << x << " metres altiude requested,\n";
	    cerr << "      but there isn't enough data stored! No temperature is aviable!\n";
	    return FG_WEATHER_DEFAULT_AIRPRESSURE;
	}

	//ok, I've got only one point. So I'm assuming that that temperature is
	//the same for all altitudes. 
	a = 1;
	b = TemperatureAt(0);
	FF += F(factor, a, b, r, x  );
	FF -= F(factor, a, b, r, 0.0);
    }
    else
    {	//I've got at least two entries now
	
	//integrate 'backwards' by integrating the strip ]n,x] first, then ]n-1,n] ... to [0,n-m]
	
	if (x>=0.0)
	{
	    map<WeatherPrecision, WeatherPrecision>::const_iterator temp2 = Temperature.upper_bound(x);
	    map<WeatherPrecision, WeatherPrecision>::const_iterator temp1 = temp2; temp1--;
	    
	    if (temp1->first == x)
	    {	//ignore that interval
		temp1--; temp2--;
	    }

	    bool first_pass = true;
	    while(true)
	    {
		if (temp2 == Temperature.end())
		{
		    //temp2 doesn't exist. So cheat by assuming that the slope is the
		    //same as between the two earlier temperatures
		    temp1--; temp2--;
		    a = (temp2->second - temp1->second)/(temp2->first - temp1->first);
		    b = temp1->second - a * temp1->first;
		    temp1++; temp2++;
		}
		else
		{
		    a = (temp2->second - temp1->second)/(temp2->first - temp1->first);
		    b = temp1->second - a * temp1->first;
		}
		
		if (first_pass)
		{
		    
		    FF += F(factor, a, b, r, x);
		    first_pass = false;
		}
		else
		{
		    FF += F(factor, a, b, r, temp2->first);
		}

		if (temp1->first>0.0)
		{
		    FF -= F(factor, a, b, r, temp1->first);
		    temp1--; temp2--;
		}
		else
		{
		    FF -= F(factor, a, b, r, 0.0);
		    return AirPressure.getValue() * exp(FF);
		}
	    }
	}
	else
	{   //ok x is smaller than 0.0, so do everything in reverse
	    map<WeatherPrecision, WeatherPrecision>::const_iterator temp2 = Temperature.upper_bound(x);
	    map<WeatherPrecision, WeatherPrecision>::const_iterator temp1 = temp2; temp1--;
	    
	    bool first_pass = true;
	    while(true)
	    {
		if (temp2 == Temperature.begin())
		{
		    //temp1 doesn't exist. So cheat by assuming that the slope is the
		    //same as between the two earlier temperatures
		    temp1 = Temperature.begin(); temp2++;
		    a = (temp2->second - temp1->second)/(temp2->first - temp1->first);
		    b = temp1->second - a * temp1->first;
		    temp2--;
		}
		else
		{
		    a = (temp2->second - temp1->second)/(temp2->first - temp1->first);
		    b = temp1->second - a * temp1->first;
		}
		
		if (first_pass)
		{
		    
		    FF += F(factor, a, b, r, x);
		    first_pass = false;
		}
		else
		{
		    FF += F(factor, a, b, r, temp2->first);
		}
		
		if (temp2->first<0.0)
		{
		    FF -= F(factor, a, b, r, temp1->first);
		    
		    if (temp2 == Temperature.begin())
		    {
			temp1 = Temperature.begin(); temp2++;
		    }
		    else
		    {
			temp1++; temp2++;
		    }
		}
		else
		{
		    FF -= F(factor, a, b, r, 0.0);
		    return AirPressure.getValue() * exp(FF);
		}
	    }
	}
    }
    
    return AirPressure.getValue() * exp(FF);
}


