/*****************************************************************************

 Module:       FGPhysicalProperties_bind.cpp
 Author:       Christian Mayer
 Date started: 15.03.02
 Called by:    main program

 -------- Copyright (C) 2002 Christian Mayer (fgfs@christianmayer.de) --------

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
15.03.2002 Christian Mayer	Created
*****************************************************************************/

#include "FGPhysicalProperties.h"


WeatherPrecision FGPhysicalProperties::getWind_x( int number ) const
{
    if (number >= Wind.size())
    {
      cerr << "Error: There are " << Wind.size() << " entries for wind, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,FGWindItem>::const_iterator it = Wind.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->second.x();
}

void FGPhysicalProperties::setWind_x( int number, WeatherPrecision x)
{
    if (number >= Wind.size())
    {
      cerr << "Error: There are " << Wind.size() << " entries for wind, but number " << number << " got requested!\n";
    }

    map<Altitude,FGWindItem>::iterator it = Wind.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->second.x( x );
}

WeatherPrecision FGPhysicalProperties::getWind_y( int number ) const
{
    if (number >= Wind.size())
    {
      cerr << "Error: There are " << Wind.size() << " entries for wind, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,FGWindItem>::const_iterator it = Wind.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->second.y();
}

void FGPhysicalProperties::setWind_y( int number, WeatherPrecision y)
{
    if (number >= Wind.size())
    {
      cerr << "Error: There are " << Wind.size() << " entries for wind, but number " << number << " got requested!\n";
    }

    map<Altitude,FGWindItem>::iterator it = Wind.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->second.y( y );
}

WeatherPrecision FGPhysicalProperties::getWind_z( int number ) const
{
    if (number >= Wind.size())
    {
      cerr << "Error: There are " << Wind.size() << " entries for wind, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,FGWindItem>::const_iterator it = Wind.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->second.z();
}

void FGPhysicalProperties::setWind_z( int number, WeatherPrecision z)
{
    if (number >= Wind.size())
    {
      cerr << "Error: There are " << Wind.size() << " entries for wind, but number " << number << " got requested!\n";
    }

    map<Altitude,FGWindItem>::iterator it = Wind.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->second.z( z );
}

WeatherPrecision FGPhysicalProperties::getWind_a( int number ) const
{
    if (number >= Wind.size())
    {
      cerr << "Error: There are " << Wind.size() << " entries for wind, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,FGWindItem>::const_iterator it = Wind.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->first;
}

void FGPhysicalProperties::setWind_a( int number, WeatherPrecision a)
{
    if (number >= Wind.size())
    {
      cerr << "Error: There are " << Wind.size() << " entries for wind, but number " << number << " got requested!\n";
    }

    map<Altitude,FGWindItem>::iterator it = Wind.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->first;
}

WeatherPrecision FGPhysicalProperties::getTurbulence_x( int number ) const
{
    if (number >= Turbulence.size())
    {
      cerr << "Error: There are " << Turbulence.size() << " entries for Turbulence, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,FGTurbulenceItem>::const_iterator it = Turbulence.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->second.x();
}

void FGPhysicalProperties::setTurbulence_x( int number, WeatherPrecision x)
{
    if (number >= Turbulence.size())
    {
      cerr << "Error: There are " << Turbulence.size() << " entries for Turbulence, but number " << number << " got requested!\n";
    }

    map<Altitude,FGTurbulenceItem>::iterator it = Turbulence.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->second.x( x );
}

WeatherPrecision FGPhysicalProperties::getTurbulence_y( int number ) const
{
    if (number >= Turbulence.size())
    {
      cerr << "Error: There are " << Turbulence.size() << " entries for Turbulence, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,FGTurbulenceItem>::const_iterator it = Turbulence.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->second.y();
}

void FGPhysicalProperties::setTurbulence_y( int number, WeatherPrecision y)
{
    if (number >= Turbulence.size())
    {
      cerr << "Error: There are " << Turbulence.size() << " entries for Turbulence, but number " << number << " got requested!\n";
    }

    map<Altitude,FGTurbulenceItem>::iterator it = Turbulence.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->second.y( y );
}

WeatherPrecision FGPhysicalProperties::getTurbulence_z( int number ) const
{
    if (number >= Turbulence.size())
    {
      cerr << "Error: There are " << Turbulence.size() << " entries for Turbulence, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,FGTurbulenceItem>::const_iterator it = Turbulence.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->second.z();
}

void FGPhysicalProperties::setTurbulence_z( int number, WeatherPrecision z)
{
    if (number >= Turbulence.size())
    {
      cerr << "Error: There are " << Turbulence.size() << " entries for Turbulence, but number " << number << " got requested!\n";
    }

    map<Altitude,FGTurbulenceItem>::iterator it = Turbulence.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->second.z( z );
}

WeatherPrecision FGPhysicalProperties::getTurbulence_a( int number ) const
{
    if (number >= Turbulence.size())
    {
      cerr << "Error: There are " << Turbulence.size() << " entries for Turbulence, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,FGTurbulenceItem>::const_iterator it = Turbulence.begin(); 

    while( number > 0)
    { 
      it++;
      number--;
    }

    return it->first;
}

void FGPhysicalProperties::setTurbulence_a( int number, WeatherPrecision a)
{
    if (number >= Turbulence.size())
    {
      cerr << "Error: There are " << Turbulence.size() << " entries for Turbulence, but number " << number << " got requested!\n";
    }

    map<Altitude,FGTurbulenceItem>::iterator it = Turbulence.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->first;
}

WeatherPrecision FGPhysicalProperties::getTemperature_x( int number ) const
{
    if (number >= Temperature.size())
    {
      cerr << "Error: There are " << Temperature.size() << " entries for Temperature, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,WeatherPrecision>::const_iterator it = Temperature.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->second;
}

void FGPhysicalProperties::setTemperature_x( int number, WeatherPrecision x)
{
    if (number >= Temperature.size())
    {
      cerr << "Error: There are " << Temperature.size() << " entries for Temperature, but number " << number << " got requested!\n";
    }

    map<Altitude,WeatherPrecision>::iterator it = Temperature.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->second = x;
}

WeatherPrecision FGPhysicalProperties::getTemperature_a( int number ) const
{
    if (number >= Temperature.size())
    {
      cerr << "Error: There are " << Temperature.size() << " entries for Temperature, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,WeatherPrecision>::const_iterator it = Temperature.begin(); 

    while( number > 0)
    { 
      it++;
      number--;
    }

    return it->first;
}

void FGPhysicalProperties::setTemperature_a( int number, WeatherPrecision a)
{
    if (number >= Temperature.size())
    {
      cerr << "Error: There are " << Temperature.size() << " entries for Temperature, but number " << number << " got requested!\n";
    }

    map<Altitude,WeatherPrecision>::iterator it = Temperature.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->first;
}
WeatherPrecision FGPhysicalProperties::getVaporPressure_x( int number ) const
{
    if (number >= VaporPressure.size())
    {
      cerr << "Error: There are " << VaporPressure.size() << " entries for VaporPressure, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,WeatherPrecision>::const_iterator it = VaporPressure.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    return it->second;
}

void FGPhysicalProperties::setVaporPressure_x( int number, WeatherPrecision x)
{
    if (number >= VaporPressure.size())
    {
      cerr << "Error: There are " << Temperature.size() << " entries for VaporPressure, but number " << number << " got requested!\n";
    }

    map<Altitude,WeatherPrecision>::iterator it = VaporPressure.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->second = x;
}

WeatherPrecision FGPhysicalProperties::getVaporPressure_a( int number ) const
{
    if (number >= VaporPressure.size())
    {
      cerr << "Error: There are " << VaporPressure.size() << " entries for VaporPressure, but number " << number << " got requested!\n";
      return 0.0;
    }

    map<Altitude,WeatherPrecision>::const_iterator it = VaporPressure.begin(); 

    while( number > 0)
    { 
      it++;
      number--;
    }

    return it->first;
}

void FGPhysicalProperties::setVaporPressure_a( int number, WeatherPrecision a)
{
    if (number >= VaporPressure.size())
    {
      cerr << "Error: There are " << VaporPressure.size() << " entries for VaporPressure, but number " << number << " got requested!\n";
    }

    map<Altitude,WeatherPrecision>::iterator it = VaporPressure.begin(); 

    while( number > 0)
    {
      it++;
      number--;
    }

    it->first;
}
