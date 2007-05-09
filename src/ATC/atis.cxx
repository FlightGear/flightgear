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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


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

#include <simgear/misc/sg_path.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Airports/runways.hxx>

#include "atis.hxx"
#include "commlist.hxx"
#include "ATCutils.hxx"
#include "ATCmgr.hxx"

FGATIS::FGATIS() :
	transmission(""),
	trans_ident(""),
	atis_failed(false),
	refname("atis")
	//type(ATIS)
{
	_vPtr = globals->get_ATC_mgr()->GetVoicePointer(ATIS);
	_voiceOK = (_vPtr == NULL ? false : true);
	_type = ATIS;
}

FGATIS::~FGATIS() {
}

// Main update function - checks whether we are displaying or not the correct message.
void FGATIS::Update(double dt) {
	if(_display) {
		if(_displaying) {
			// Check if we need to update the message
			// - basically every hour and if the weather changes significantly at the station
		} else {
			// We need to get and display the message
			UpdateTransmission();
			//cout << "ATIS.CXX - calling ATCMgr to render transmission..." << endl;
			Render(transmission, refname, true);
			_displaying = true;
		}
	} else {
		// We shouldn't be displaying
		if(_displaying) {
			//cout << "ATIS.CXX - calling NoRender()..." << endl;
			NoRender(refname);
			_displaying = false;
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
	
	FGEnvironment stationweather =
            ((FGEnvironmentMgr *)globals->get_subsystem("environment"))
              ->getEnvironment(lat, lon, 0.0);
	
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
	temp = (int)stationweather.get_temperature_degc();
	
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
	visibility = stationweather.get_visibility_m();
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
	double P = fgGetDouble("/environment/pressure-sea-level-inhg");
	if(ident.substr(0,2) == "EG" && fgGetBool("/sim/atc/use-millibars") == true) {
		// Convert to millibars for the UK!
		P *= 33.864;
		sprintf(buf, "%.0f", P);
	} else {
		sprintf(buf, "%.2f", P);
	}		
	transmission += " / Altimeter ";
	tempstr1 = buf;
	transmission += ConvertNumToSpokenDigits(tempstr1);
	
	// Based on the airport-id and wind get the active runway
	//FGRunway *r;
	double speed = stationweather.get_wind_speed_kt();
	double hdg = stationweather.get_wind_from_heading_deg();
	if (speed == 0) {
		hdg = 270;	// This forces West-facing rwys to be used in no-wind situations
		          	// which is consistent with Flightgear's initial setup.
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
	
	string rwy_no = globals->get_runways()->search(ident, int(hdg));
	if(rwy_no != "NN") {
		transmission += " / Landing_and_departing_runway ";
		transmission += ConvertRwyNumToSpokenString(atoi(rwy_no.c_str()));
		//cout << "in atis.cxx, r.rwy_no = " << rwy_no << " r.id = " << r->id << " r.heading = " << r->heading << endl;
	}
	
	// Anything else?
	
	transmission += " / Advise_controller_on_initial_contact_you_have ";
	transmission += phonetic_id_string;
	transmission += " /// ";
}
