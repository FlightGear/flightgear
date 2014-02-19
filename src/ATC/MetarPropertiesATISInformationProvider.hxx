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
#if 0
    virtual std::string getStationId();
    virtual std::string getAtisId();
    virtual std::string getTime();
    virtual std::string getApproachType();
    virtual std::string getLandingRunway();
    virtual std::string getTakeoffRunway();
    virtual std::string getTransitionLevel();
    virtual std::string getWindDirection();
    virtual std::string getWindspeedKnots();
    virtual std::string getGustsKnots();
    virtual std::string getVisibilityMetric();
    virtual std::string getPhenomena();
    virtual std::string getClouds();
    virtual std::string getCavok();
    virtual std::string getTemperatureDeg();
    virtual std::string getDewpointDeg();
    virtual std::string getQnh();
    virtual std::string getTrend();
#endif
private:
    SGPropertyNode_ptr _metar;
};

#endif
