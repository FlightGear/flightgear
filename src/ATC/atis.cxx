// atis.cxx - routines to generate the ATIS info string
// This is the implementation of the FGATIS class
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
#include <stdio.h>	// sprintf
#include <string>
SG_USING_STD(string);

#include STL_IOSTREAM
#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
#endif

//#include <simgear/debug/logstream.hxx>
//#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>

#ifdef FG_WEATHERCM
# include <WeatherCM/FGLocalWeatherDatabase.h>
#else
# include <Environment/environment_mgr.hxx>
# include <Environment/environment.hxx>
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Airports/runways.hxx>

#include "atis.hxx"
#include "atislist.hxx"
#include "ATCdisplay.hxx"

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
FGATIS::FGATIS()
  : type(0),
    lon(0.0), lat(0.0),
    elev(0.0),
    x(0.0), y(0.0), z(0.0),
    freq(0),
    range(0),
    display(false),
    displaying(false),
    ident(""),
    name(""),
    transmission(""),
    trans_ident(""),
    atis_failed(false)
{
}

// Destructor
FGATIS::~FGATIS() {
}

// Main update function - checks whether we are displaying or not the correct message.
void FGATIS::Update() {
    if(display) {
	if(displaying) {
	    // Check if we need to update the message
	    // - basically every hour and if the weather changes significantly at the station
	    //globals->get_ATC_display()->ChangeRepeatingMessage(transmission);
	} else {
	    // We need to get and display the message
	    UpdateTransmission();
	    globals->get_ATC_display()->RegisterRepeatingMessage(transmission);
	    displaying = true;
	}
    } else {
	// We shouldn't be displaying
	if(displaying) {
	    globals->get_ATC_display()->CancelRepeatingMessage();
	    displaying = false;
	}
    }
}

// Sets the actual broadcast ATIS transmission.
void FGATIS::UpdateTransmission() {
    double visibility;
    char buf[10];
    int phonetic_id;
    string phonetic_id_string;
    string time_str = fgGetString("sim/time/gmt-string");
    int hours;
    // int minutes;

#ifdef FG_WEATHERCM
    sgVec3 position = { lat, lon, elev };
    FGPhysicalProperty stationweather = WeatherDatabase->get(position);
#else
    FGEnvironment stationweather =
      globals->get_environment_mgr()->getEnvironment(lat, lon, elev);
#endif

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
#ifdef FG_WEATHERCM
    sprintf(buf, "%i", int(stationweather.Temperature - 273.15));
#else
    sprintf(buf, "%i", int(stationweather.get_temperature_degc()));
#endif
    transmission += "  Temperature ";
    transmission += buf;
    transmission += " degrees Celsius";

	// Get the pressure / altimeter
        // pressure is: stationweather.AirPressure in Pascal

	// Get the visibility
#ifdef FG_WEATHERCM
	visibility = fgGetDouble("/environment/visibility-m");
#else
        visibility = stationweather.get_visibility_m();
#endif
	sprintf(buf, "%i", int(visibility/1600));
	transmission += "  Visibility ";
	transmission += buf;
	transmission += " miles";

	// Get the cloudbase
	// FIXME: kludge for now
	if (!strcmp(fgGetString("/environment/clouds/layer[0]/type"),
		    "clear")) {
	    double cloudbase =
	      fgGetDouble("/environment/clouds/layer[0]/elevation-ft");
	    // For some reason the altitude returned doesn't seem to correspond to the actual cloud altitude.
	    char buf3[10];
	    // cout << "cloudbase = " << cloudbase << endl;
	    sprintf(buf3, "%i", int(cloudbase));
	    transmission = transmission + "  Cloudbase " + buf3 + " feet";
	}

	// Based on the airport-id and wind get the active runway
	//FGRunway *r;
	SGPath path( globals->get_fg_root() );
	path.append( "Airports" );
	path.append( "runways.mk4" );
	FGRunways runways( path.c_str() );

#ifdef FG_WEATHERCM
	//Set the heading to into the wind
        double wind_x = stationweather.Wind[0];
        double wind_y = stationweather.Wind[1];

        double speed = sqrt( wind_x*wind_x + wind_y*wind_y ) * SG_METER_TO_NM / (60.0*60.0);
        double hdg;

	//If no wind use 270degrees
	if(speed == 0) {
	    hdg = 270;
	    transmission += "  Winds light and variable";
	} else {
            // //normalize the wind to get the direction
            //wind_x /= speed; wind_y /= speed;

            hdg = - atan2 ( wind_x, wind_y ) * SG_RADIANS_TO_DEGREES ;
            if (hdg < 0.0)
              hdg += 360.0;

	    //add a description of the wind to the transmission
	    char buf2[72];
	    sprintf(buf2, "%s %i %s %i %s", "  Winds ", int(speed), " knots from ", int(hdg), " degrees");
	    transmission += buf2;
	}
#else
	double speed = stationweather.get_wind_speed_kt();
	double hdg = stationweather.get_wind_from_heading_deg();
	if (speed == 0) {
	  transmission += "  Winds light and variable";
	} else {
				// FIXME: get gust factor in somehow
	    char buf2[72];
	    sprintf(buf2, "%s %i %s %i %s", "  Winds ", int(speed),
		    " knots from ", int(hdg), " degrees");
	}
#endif

	string rwy_no = runways.search(ident, int(hdg));
	if(rwy_no != (string)"NN") {
	    transmission += "  Landing and departing runway ";
	    transmission += rwy_no;
	    //cout << "in atis.cxx, r.rwy_no = " << rwy_no << " r.id = " << r->id << " r.heading = " << r->heading << endl;
	}

	// Anything else?

	transmission += "  Advise controller on initial contact you have ";
	transmission += phonetic_id_string;
}
