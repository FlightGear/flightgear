// atis.cxx - routines to generate the ATIS info string
//
// Written by David Luff, started October 2001.
//
// Copyright (C) 2001  David C Luff - david.luff@nottingham.ac.uk
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <string>
SG_USING_STD(string);

//#include STL_IOSTREAM
//#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
//SG_USING_STD(cout);
//#endif

//#include <simgear/debug/logstream.hxx>
//#include <simgear/misc/sgstream.hxx>
//#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>

//#ifndef FG_OLD_WEATHER
//#include <WeatherCM/FGLocalWeatherDatabase.h>
//#else
//#  include <Weather/weather.hxx>
//#endif

#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>

#include "atis.hxx"

// Constructor
FGATIS::FGATIS() {
}

// Destructor
FGATIS::~FGATIS() {
}

string FGATIS::get_transmission() {
//void FGATIS::get_transmission() {

    string transmission = "";
    double visibility;
    double temperature;
    char buf[10];

// Only update every so-many loops - FIXME - possibly register this with the event scheduler
// Ack this doesn't work since the static counter is shared between all instances of FGATIS
// OK, for now the radiostack is handling only calling this every-so-often but that is not really 
// a proper solution since the atis knows when the transmission is going to change not the radio.
    //static int i=0;
    //if(i == 0) {
	transmission = "";

	// Start with the transmitted station name.
	transmission += name;

	// Add the recording identifier
	// TODO - this is hardwired for now - ultimately we need to start with a random one and then increment it with each recording
	transmission += " Charlie";

	// Output the recording time. - we'll just output the last whole hour for now.
	string time_str = fgGetString("sim/time/gmt-string");
	// FIXME - this only gets GMT time but that appears to be all the clock outputs for now
	//cout << "in atis.cxx, time = " << time_str << endl;
	transmission = transmission + "  Weather " + time_str.substr(0,3) + "00 hours Zulu";

	// Get the temperature
	// Hardwire the temperature for now - is the local weather database running yet?
	transmission += "  Temperature 25 degrees Celcius";

	// Get the pressure / altimeter

	// Get the visibility
	visibility = fgGetDouble("/environment/visibility-m");
	sprintf(buf, "%d", int(visibility/1600));
	transmission += "  Visibility ";
	transmission += buf;
	transmission += " miles";

	// Get the cloudbase
	if(fgGetBool("/environment/clouds/status")) {
	    double cloudbase = fgGetDouble("/environment/clouds/altitude-ft");
	    // For some reason the altitude returned doesn't seem to correspond to the actual cloud altitude.
	    char buf3[10];
	    cout << "cloudbase = " << cloudbase << endl;
	    sprintf(buf3, "%i", int(cloudbase));
	    transmission = transmission + "  Cloudbase " + buf3 + " feet";
	}

	// Based on the airport-id and wind get the active runway
	//FGRunway *r;
	SGPath path( globals->get_fg_root() );
	path.append( "Airports" );
	path.append( "runways.mk4" );
	FGRunways runways( path.c_str() );

	//Set the heading to into the wind
	double hdg = fgGetDouble("/environment/wind-from-heading-deg");
	double speed = fgGetDouble("/environment/wind-speed-knots");

	//cout << "in atis.cxx, hdg = " << hdg << " speed = " << speed << endl;

	//If no wind use 270degrees
	if(speed == 0) {
	    hdg = 270;
	    transmission += "  Winds light and variable";
	} else {
	    //add a description of the wind to the transmission
	    char buf2[48];
	    sprintf(buf2, "%s %i %s %i %s", "  Winds ", int(speed), " knots from ", int(hdg), " degrees");
	    transmission += buf2;
	}

	string rwy_no = runways.search(ident, hdg);
	if(rwy_no != "NN") {
	    transmission += "  Landing and departing runway ";
	    transmission += rwy_no;
	    //cout << "in atis.cxx, r.rwy_no = " << rwy_no << " r.id = " << r->id << " r.heading = " << r->heading << endl;
	}

	// Anything else?

	// TODO - unhardwire the identifier
	transmission += "  Advise controller on initial contact you have Charlie";

    //}

//    i++;
//    if(i == 600) {
//	i=0;
//    }

    return(transmission);
}