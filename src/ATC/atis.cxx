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

#include <stdlib.h>	// atoi()
#include <string>
SG_USING_STD(string);

#include STL_IOSTREAM
#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
#endif

//#include <simgear/debug/logstream.hxx>
//#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>

//#ifndef FG_OLD_WEATHER
//#include <WeatherCM/FGLocalWeatherDatabase.h>
//#else
//#  include <Weather/weather.hxx>
//#endif

#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>

#include "atis.hxx"
#include "atislist.hxx"

string GetPhoneticIdent(int i) {
// TODO - Check i is between 1 and 26 and wrap if necessary
    switch(i) {
    case 1 : return("Alpha");
    case 2 : return("Bravo");
    case 3 : return("Charlie");
    case 4 : return("Delta");
    case 5 : return("Echo");
    case 6 : return("Foxtrot");
    case 7 : return("Golf");
    case 8 : return("Hotel");
    case 9 : return("Indigo");
    case 10 : return("Juliet");
    case 11 : return("Kilo");
    case 12 : return("Lima");
    case 13 : return("Mike");
    case 14 : return("November");
    case 15 : return("Oscar");
    case 16 : return("Papa");
    case 17 : return("Quebec");
    case 18 : return("Romeo");
    case 19 : return("Sierra");
    case 20 : return("Tango");
    case 21 : return("Uniform");
    case 22 : return("Victor");
    case 23 : return("Whiskey");
    case 24 : return("X-ray");
    case 25 : return("Yankee");
    case 26 : return("Zulu");
    }
    // We shouldn't get here
    return("Error");
}

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
    int phonetic_id;
    string phonetic_id_string;
    string time_str = fgGetString("sim/time/gmt-string");
    int hours;
    int minutes;

// Only update every so-many loops - FIXME - possibly register this with the event scheduler
// Ack this doesn't work since the static counter is shared between all instances of FGATIS
// OK, for now the radiostack is handling only calling this every-so-often but that is not really 
// a proper solution since the atis knows when the transmission is going to change not the radio.
    //static int i=0;
    //if(i == 0) {
	transmission = "";

	// Start with the transmitted station name.
	transmission += name;
	// Add "Information"
	transmission += " Information";

	//cout << "In atis.cxx, time_str = " << time_str << '\n';
	// Add the recording identifier
	// For now we will assume we only transmit every hour
	hours = atoi((time_str.substr(1,2)).c_str());	//Warning - this is fragile if the 
							//time string format changes
	//cout << "In atis.cxx, hours = " << hours << endl;
	phonetic_id = current_atislist->GetCallSign(ident, hours, 0);
	phonetic_id_string = GetPhoneticIdent(phonetic_id);
	transmission += " ";
	transmission += phonetic_id_string;

	// Output the recording time. - we'll just output the last whole hour for now.
	// FIXME - this only gets GMT time but that appears to be all the clock outputs for now
	//cout << "in atis.cxx, time = " << time_str << endl;
	transmission = transmission + "  Weather " + time_str.substr(0,3) + "00 hours Zulu";

	// Get the temperature
	// (Hardwire it for now since the global property returns the temperature at the current altitude
	//temperature = fgGetDouble("/environment/weather/temperature-K");
	temperature = 15 + 273.15;
	sprintf(buf, "%i", int(temperature - 273.15));
	transmission += "  Temperature ";
	transmission += buf;
	transmission += " degrees Celcius";

	// Get the pressure / altimeter

	// Get the visibility
	visibility = fgGetDouble("/environment/visibility-m");
	sprintf(buf, "%i", int(visibility/1600));
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
	    char buf2[72];
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
	transmission += "  Advise controller on initial contact you have ";
	transmission += phonetic_id_string;

    //}

//    i++;
//    if(i == 600) {
//	i=0;
//    }

    return(transmission);
}
