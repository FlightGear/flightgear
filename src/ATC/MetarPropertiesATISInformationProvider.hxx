/*
Provide Data for the ATIS Encoder from metarproperties
Copyright (C) 2014 Torsten Dreyer

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __METARPROPERTIES_ATIS_ENCODER_HXX
#define __METARPROPERTIES_ATIS_ENCODER_HXX

/* ATIS encoder from metarproperties */

#include <simgear/props/props.hxx>

#include <string>
#include "ATISEncoder.hxx"

class MetarPropertiesATISInformationProvider : public ATISInformationProvider
{
public:
    MetarPropertiesATISInformationProvider( SGPropertyNode_ptr metar );
    virtual ~MetarPropertiesATISInformationProvider();

protected:
    virtual bool isValid();
    virtual std::string airportId();
    virtual long getTime();
    virtual int getWindDeg();
    virtual int getWindMinDeg();
    virtual int getWindMaxDeg();
    virtual int getWindSpeedKt();
    virtual int getGustsKt();
    virtual int getQnh();
    virtual bool isCavok();
    virtual int getVisibilityMeters();
    virtual std::string getPhenomena();
    virtual CloudEntries getClouds();
    virtual int getTemperatureDeg();
    virtual int getDewpointDeg();
    virtual std::string getTrend();
private:
    SGPropertyNode_ptr _metar;
};

#endif
