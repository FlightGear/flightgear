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
SG_USING_STD(cout);

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
#include "commlist.hxx"
//#include "atislist.hxx"
#include "ATCdisplay.hxx"
#include "ATCutils.hxx"
#include "ATCmgr.hxx"

// Constructor
FGATIS::FGATIS() :
display(false),
displaying(false),
transmission(""),
trans_ident(""),
atis_failed(false),
refname("atis")
//type(ATIS)
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
			//cout << "ATIS.CXX - calling ATCMgr to render transmission..." << endl;
			globals->get_ATC_mgr()->Render(transmission, refname, true);
			displaying = true;
		}
	} else {
		// We shouldn't be displaying
		if(displaying) {
			//cout << "ATIS.CXX - calling NoRender()..." << endl;
			globals->get_ATC_mgr()->NoRender(refname);
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
	
	// UK CAA radiotelephony manual indicated ATIS transmissions start with "This is"
	// Not sure about rest of the world though.
	transmission += "This_is ";
	// transmitted station name.
	transmission += name;
	// Add "Information"
	transmission += " information";
	
	//cout << "In atis.cxx, time_str = " << time_str << '\n';
	// Add the recording identifier
	// For now we will assume we only transmit every hour
	hours = atoi((time_str.substr(1,2)).c_str());	//Warning - this is fragile if the 
	//time string format changes
	//cout << "In atis.cxx, hours = " << hours << endl;
	phonetic_id = current_commlist->GetCallSign(ident, hours, 0);
	phonetic_id_string = GetPhoneticIdent(phonetic_id);
	transmission += " ";
	transmission += phonetic_id_string;
	
	// Output the recording time. - we'll just output the last whole hour for now.
	// FIXME - this only gets GMT time but that appears to be all the clock outputs for now
	//cout << "in atis.cxx, time = " << time_str << endl;
	transmission = transmission + " / Weather " + ConvertNumToSpokenDigits((time_str.substr(0,3) + "00")) + " hours zulu";
	
	// Get the temperature
	int temp;
	#ifdef FG_WEATHERCM
	temp = int(stationweather.Temperature - 273.15);
	#else
	temp = (int)stationweather.get_temperature_degc();
	#endif
	
	// HACK ALERT - at the moment the new environment subsystem returns bogus temperatures
	// FIXME - take out this hack when this gets fixed upstream
	if((temp < -50) || (temp > 60)) {
		temp = 15;
	}

	sprintf(buf, "%i", abs(temp));
	transmission += " / Temperature ";
	if(temp < 0) {
		transmission += "minus ";
	}
	string tempstr1 = buf;
	string tempstr2;
	transmission += ConvertNumToSpokenDigits(tempstr1);
	transmission += " degrees_Celsius";
	
	// Get the visibility
	#ifdef FG_WEATHERCM
	visibility = fgGetDouble("/environment/visibility-m");
	#else
	visibility = stationweather.get_visibility_m();
	#endif
	sprintf(buf, "%i", int(visibility/1600));
	transmission += " / Visibility ";
	tempstr1 = buf;
	transmission += ConvertNumToSpokenDigits(tempstr1);
	transmission += " miles";
	
	// Get the cloudbase
	// FIXME: kludge for now
	if (strcmp(fgGetString("/environment/clouds/layer[0]/type"), "clear")) {
		double cloudbase =
		fgGetDouble("/environment/clouds/layer[0]/elevation-ft");
		// For some reason the altitude returned doesn't seem to correspond to the actual cloud altitude.
		char buf3[10];
		char buf4[10];
		// cout << "cloudbase = " << cloudbase << endl;
		sprintf(buf3, "%i", int(cloudbase)/1000);
		sprintf(buf4, "%i", ((int)cloudbase % 1000)/100);
		transmission += " / Cloudbase";
		if(int(cloudbase)/1000) {
			tempstr1 = buf3;
			transmission = transmission + " " + ConvertNumToSpokenDigits(tempstr1) + " thousand";
		}
		if(((int)cloudbase % 1000)/100) {
			tempstr1 = buf4;
			transmission = transmission + " " + ConvertNumToSpokenDigits(tempstr1) + " hundred";
		}			
		transmission += " feet";
	}
	
	// Get the pressure / altimeter
	
	#ifndef FG_WEATHERCM
	double altimeter = stationweather.get_pressure_sea_level_inhg();
	sprintf(buf, "%.2f", altimeter);
	transmission += " / Altimeter ";
	tempstr1 = buf;
	transmission += ConvertNumToSpokenDigits(tempstr1);
	#endif
	
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
		transmission += " / Winds_light_and_variable";
	} else {
		// //normalize the wind to get the direction
		//wind_x /= speed; wind_y /= speed;
		
		hdg = - atan2 ( wind_x, wind_y ) * SG_RADIANS_TO_DEGREES ;
		if (hdg < 0.0)
			hdg += 360.0;
		
		//add a description of the wind to the transmission
		char buf5[10];
		char buf6[10];
		sprintf(buf5, "%i", int(speed));
		sprintf(buf6, "%i", int(hdg));
		tempstr1 = buf5;
		tempstr2 = buf6;
		transmission = transmission + " / Winds " + ConvertNumToSpokenDigits(tempstr1) + " knots from "
		                            + ConvertNumToSpokenDigits(tempstr2) + " degrees";
	}
	#else
	double speed = stationweather.get_wind_speed_kt();
	double hdg = stationweather.get_wind_from_heading_deg();
	if (speed == 0) {
		transmission += " / Winds_light_and_variable";
	} else {
		// FIXME: get gust factor in somehow
		char buf5[10];
		char buf6[10];
		sprintf(buf5, "%i", int(speed));
		sprintf(buf6, "%i", int(hdg));
		tempstr1 = buf5;
		tempstr2 = buf6;
		transmission = transmission + " / Winds " + ConvertNumToSpokenDigits(tempstr1) + " knots from "
		                            + ConvertNumToSpokenDigits(tempstr2) + " degrees";
	}
	#endif
	
	string rwy_no = runways.search(ident, int(hdg));
	if(rwy_no != (string)"NN") {
		transmission += " / Landing_and_departing_runway ";
		transmission += ConvertRwyNumToSpokenString(atoi(rwy_no.c_str()));
		//cout << "in atis.cxx, r.rwy_no = " << rwy_no << " r.id = " << r->id << " r.heading = " << r->heading << endl;
	}
	
	// Anything else?
	
	transmission += " / Advise_controller_on_initial_contact_you_have ";
	transmission += phonetic_id_string;
	transmission += " /// ";
}
