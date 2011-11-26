// radio.cxx -- implementation of FGRadio
// Class to manage radio propagation using the ITM model
// Written by Adrian Musceac, started August 2011.
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

#include <math.h>
#include <stdlib.h>
#include <deque>
#include "radio.hxx"
#include <Scenery/scenery.hxx>

#define WITH_POINT_TO_POINT 1
#include "itm.cpp"


FGRadio::FGRadio() {
	
	/** radio parameters (which should probably be set for each radio) */
	
	_receiver_sensitivity = -110.0;	// typical AM receiver sensitivity seems to be 0.8 microVolt at 12dB SINAD
	
	/** AM transmitter power in dBm.
	*  	Note this value is calculated from the typical final transistor stage output
	*  	small aircraft have portable transmitters which operate at 36 dBm output (4 Watts) others operate in the range 10-20 W
	*  	later possibly store this value in aircraft description
	*  	ATC comms usually operate high power equipment, thus making the link asymetrical; this is taken care of in propagation routines
	*	Typical output powers for ATC ground equipment, VHF-UHF:
	*	40 dBm - 10 W (ground, clearance)
	*	44 dBm - 20 W (tower)
	*	47 dBm - 50 W (center, sectors)
	*	50 dBm - 100 W (center, sectors)
	*	53 dBm - 200 W (sectors, on directional arrays)
	**/
	_transmitter_power = 43.0;
	
	/** pilot plane's antenna gain + AI aircraft antenna gain
	* 	real-life gain for conventional monopole/dipole antenna
	**/
	_antenna_gain = 2.0;
	_propagation_model = 2; //  choose between models via option: realistic radio on/off
	
}

FGRadio::~FGRadio() 
{
}


double FGRadio::getFrequency(int radio) {
	double freq = 118.0;
	switch (radio) {
		case 1:
			freq = fgGetDouble("/instrumentation/comm[0]/frequencies/selected-mhz");
			break;
		case 2:
			freq = fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz");
			break;
		default:
			freq = fgGetDouble("/instrumentation/comm[0]/frequencies/selected-mhz");
			
	}
	return freq;
}

/*** TODO: receive multiplayer chat message and voice
***/
void FGRadio::receiveChat(SGGeod tx_pos, double freq, string text, int ground_to_air) {

}

/*** TODO: receive navaid 
***/
double FGRadio::receiveNav(SGGeod tx_pos, double freq, int transmission_type) {
	
	// typical VOR/LOC transmitter power appears to be 200 Watt ~ 53 dBm
	// vor/loc typical sensitivity between -107 and -101 dBm
	// glideslope sensitivity between -85 and -81 dBm
	if ( _propagation_model == 1) {
		return LOS_calculate_attenuation(tx_pos, freq, 1);
	}
	else if ( _propagation_model == 2) {
		return ITM_calculate_attenuation(tx_pos, freq, 1);
	}
	
	return -1;

}

/*** Receive ATC radio communication as text
***/
void FGRadio::receiveATC(SGGeod tx_pos, double freq, string text, int ground_to_air) {

	
	double comm1 = getFrequency(1);
	double comm2 = getFrequency(2);
	if ( !(fabs(freq - comm1) <= 0.0001) &&  !(fabs(freq - comm2) <= 0.0001) ) {
		//cerr << "Frequency not tuned: " << freq << " Radio1: " << comm1 << " Radio2: " << comm2 << endl;
		return;
	}
	else {
	
		if ( _propagation_model == 0) {
			fgSetString("/sim/messages/atc", text.c_str());
		}
		else if ( _propagation_model == 1 ) {
			// TODO: free space, round earth
			double signal = LOS_calculate_attenuation(tx_pos, freq, ground_to_air);
			if (signal <= 0.0) {
				SG_LOG(SG_GENERAL, SG_BULK, "Signal below receiver minimum sensitivity: " << signal);
				//cerr << "Signal below receiver minimum sensitivity: " << signal << endl;
				return;
			}
			else {
				SG_LOG(SG_GENERAL, SG_BULK, "Signal completely readable: " << signal);
				//cerr << "Signal completely readable: " << signal << endl;
				fgSetString("/sim/messages/atc", text.c_str());
				/** write signal strength above threshold to the property tree
				*	to implement a simple S-meter just divide by 3 dB per grade (VHF norm)
				**/
				fgSetDouble("/sim/radio/comm1-signal", signal);
			}
		}
		else if ( _propagation_model == 2 ) {
			// Use ITM propagation model
			double signal = ITM_calculate_attenuation(tx_pos, freq, ground_to_air);
			if (signal <= 0.0) {
				SG_LOG(SG_GENERAL, SG_BULK, "Signal below receiver minimum sensitivity: " << signal);
				//cerr << "Signal below receiver minimum sensitivity: " << signal << endl;
				return;
			}
			if ((signal > 0.0) && (signal < 12.0)) {
				/** for low SNR values implement a way to make the conversation
				*	hard to understand but audible
				*	in the real world, the receiver AGC fails to capture the slope
				*	and the signal, due to being amplitude modulated, decreases volume after demodulation
				*	the workaround below is more akin to what would happen on a FM transmission
				*	therefore the correct way would be to work on the volume
				**/
				/*
				string hash_noise = " ";
				int reps = (int) (fabs(floor(signal - 11.0)) * 2);
				int t_size = text.size();
				for (int n = 1; n <= reps; ++n) {
					int pos = rand() % (t_size -1);
					text.replace(pos,1, hash_noise);
				}
				*/
				double volume = (fabs(signal - 12.0) / 12);
				double old_volume = fgGetDouble("/sim/sound/voices/voice/volume");
				SG_LOG(SG_GENERAL, SG_BULK, "Usable signal at limit: " << signal);
				//cerr << "Usable signal at limit: " << signal << endl;
				fgSetDouble("/sim/sound/voices/voice/volume", volume);
				fgSetString("/sim/messages/atc", text.c_str());
				fgSetDouble("/sim/radio/comm1-signal", signal);
				fgSetDouble("/sim/sound/voices/voice/volume", old_volume);
			}
			else {
				SG_LOG(SG_GENERAL, SG_BULK, "Signal completely readable: " << signal);
				//cerr << "Signal completely readable: " << signal << endl;
				fgSetString("/sim/messages/atc", text.c_str());
				/** write signal strength above threshold to the property tree
				*	to implement a simple S-meter just divide by 3 dB per grade (VHF norm)
				**/
				fgSetDouble("/sim/radio/comm1-signal", signal);
			}
			
		}
		
	}
	
}

/***  Implement radio attenuation		
	  based on the Longley-Rice propagation model
***/
double FGRadio::ITM_calculate_attenuation(SGGeod pos, double freq, int transmission_type) {

	
	
	/** ITM default parameters 
		TODO: take them from tile materials (especially for sea)?
	**/
	double eps_dielect=15.0;
	double sgm_conductivity = 0.005;
	double eno = 301.0;
	double frq_mhz;
	if( (freq < 118.0) || (freq > 137.0) )
		frq_mhz = 125.0; 	// sane value, middle of bandplan
	else
		frq_mhz = freq;
	int radio_climate = 5;		// continental temperate
	int pol=1;	// assuming vertical polarization although this is more complex in reality
	double conf = 0.90;	// 90% of situations and time, take into account speed
	double rel = 0.90;	
	double dbloss;
	char strmode[150];
	int errnum;
	
	double tx_pow = _transmitter_power;
	double ant_gain = _antenna_gain;
	double signal = 0.0;
	
	if(transmission_type == 1)
		tx_pow = _transmitter_power + 6.0;

	if((transmission_type == 1) || (transmission_type == 3))
		ant_gain = _antenna_gain + 3.0; //pilot plane's antenna gain + ground station antenna gain
	
	double link_budget = tx_pow - _receiver_sensitivity + ant_gain;	

	FGScenery * scenery = globals->get_scenery();
	
	double own_lat = fgGetDouble("/position/latitude-deg");
	double own_lon = fgGetDouble("/position/longitude-deg");
	double own_alt_ft = fgGetDouble("/position/altitude-ft");
	double own_alt= own_alt_ft * SG_FEET_TO_METER;
	
	
	//cerr << "ITM:: pilot Lat: " << own_lat << ", Lon: " << own_lon << ", Alt: " << own_alt << endl;
	
	SGGeod own_pos = SGGeod::fromDegM( own_lon, own_lat, own_alt );
	SGGeod max_own_pos = SGGeod::fromDegM( own_lon, own_lat, SG_MAX_ELEVATION_M );
	SGGeoc center = SGGeoc::fromGeod( max_own_pos );
	SGGeoc own_pos_c = SGGeoc::fromGeod( own_pos );
	
	/** 	position of sender radio antenna (HAAT)
			sender can be aircraft or ground station
	**/
	double ATC_HAAT = 30.0;
	double Aircraft_HAAT = 5.0;
	double sender_alt_ft,sender_alt;
	double transmitter_height=0.0;
	double receiver_height=0.0;
	SGGeod sender_pos = pos;
	
	sender_alt_ft = sender_pos.getElevationFt();
	sender_alt = sender_alt_ft * SG_FEET_TO_METER;
	SGGeod max_sender_pos = SGGeod::fromGeodM( pos, SG_MAX_ELEVATION_M );
	SGGeoc sender_pos_c = SGGeoc::fromGeod( sender_pos );
	//cerr << "ITM:: sender Lat: " << parent->getLatitude() << ", Lon: " << parent->getLongitude() << ", Alt: " << sender_alt << endl;
	
	double point_distance= 90.0; // regular SRTM is 90 meters
	double course = SGGeodesy::courseRad(own_pos_c, sender_pos_c);
	double distance_m = SGGeodesy::distanceM(own_pos, sender_pos);
	double probe_distance = 0.0;
	/** If distance larger than this value (300 km), assume reception imposssible */
	if (distance_m > 300000)
		return -1.0;
	/** If above 8000 meters, consider LOS mode and calculate free-space att */
	if (own_alt > 8000) {
		dbloss = 20 * log10(distance_m) +20 * log10(frq_mhz) -27.55;
		SG_LOG(SG_GENERAL, SG_BULK,
			"ITM Free-space mode:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm, free-space attenuation");
		//cerr << "ITM Free-space mode:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm, free-space attenuation" << endl;
		signal = link_budget - dbloss;
		return signal;
	}
	
		
	double max_points = distance_m / point_distance;
	deque<double> _elevations;

	double elevation_under_pilot = 0.0;
	if (scenery->get_elevation_m( max_own_pos, elevation_under_pilot, NULL )) {
		receiver_height = own_alt - elevation_under_pilot + 3; //assume antenna located 3 meters above ground
	}

	double elevation_under_sender = 0.0;
	if (scenery->get_elevation_m( max_sender_pos, elevation_under_sender, NULL )) {
		transmitter_height = sender_alt - elevation_under_sender;
	}
	else {
		transmitter_height = sender_alt;
	}
	
	if(transmission_type == 1) 
		transmitter_height += ATC_HAAT;
	else
		transmitter_height += Aircraft_HAAT;
	
	SG_LOG(SG_GENERAL, SG_BULK,
			"ITM:: RX-height: " << receiver_height << " meters, TX-height: " << transmitter_height << " meters, Distance: " << distance_m << " meters");
	//cerr << "ITM:: RX-height: " << receiver_height << " meters, TX-height: " << transmitter_height << " meters, Distance: " << distance_m << " meters" << endl;
	
	unsigned int e_size = (deque<unsigned>::size_type)max_points;
	
	while (_elevations.size() <= e_size) {
		probe_distance += point_distance;
		SGGeod probe = SGGeod::fromGeoc(center.advanceRadM( course, probe_distance ));
		
		double elevation_m = 0.0;
	
		if (scenery->get_elevation_m( probe, elevation_m, NULL )) {
			if((transmission_type == 3) || (transmission_type == 4)) {
				_elevations.push_back(elevation_m);
			}
			else {
				 _elevations.push_front(elevation_m);
			}
		}
		else {
			if((transmission_type == 3) || (transmission_type == 4)) {
				_elevations.push_back(elevation_m);
			}
			else {
			_elevations.push_front(0.0);
			}
		}
	}
	if((transmission_type == 3) || (transmission_type == 4)) {
		_elevations.push_front(elevation_under_pilot);
		_elevations.push_back(elevation_under_sender);
	}
	else {
		_elevations.push_back(elevation_under_pilot);
		_elevations.push_front(elevation_under_sender);
	}
	
	
	double max_alt_between=0.0;
	for( deque<double>::size_type i = 0; i < _elevations.size(); i++ ) {
		if (_elevations[i] > max_alt_between) {
			max_alt_between = _elevations[i];
		}
	}
	
	double num_points= (double)_elevations.size();
	//cerr << "ITM:: Max alt between: " << max_alt_between << ", num points:" << num_points << endl;
	_elevations.push_front(point_distance);
	_elevations.push_front(num_points -1);
	int size = _elevations.size();
	double itm_elev[size];
	for(int i=0;i<size;i++) {
		itm_elev[i]=_elevations[i];
		//cerr << "ITM:: itm_elev: " << _elevations[i] << endl;
	}

	
	/** first Fresnel zone radius
		frequency in the middle of the bandplan, more accuracy is not necessary
	*/
	double fz_clr= 8.657 * sqrt(distance_m / 0.125);
	
	// TODO: If we clear the first Fresnel zone, we are into line of sight territory

	// else we need to calculate point to point link loss
	if((transmission_type == 3) || (transmission_type == 4)) {
		// the sender and receiver roles are switched
		point_to_point(itm_elev, receiver_height, transmitter_height,
			eps_dielect, sgm_conductivity, eno, frq_mhz, radio_climate,
			pol, conf, rel, dbloss, strmode, errnum);
		
	}
	else {
		point_to_point(itm_elev, transmitter_height, receiver_height,
			eps_dielect, sgm_conductivity, eno, frq_mhz, radio_climate,
			pol, conf, rel, dbloss, strmode, errnum);
	}
	SG_LOG(SG_GENERAL, SG_BULK,
			"ITM:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm, " << strmode << ", Error: " << errnum);
	cerr << "ITM:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm, " << strmode << ", Error: " << errnum << endl;
	
	//if (errnum == 4)	// if parameters are outside sane values for lrprop, the alternative method is used
	//	return -1;
	signal = link_budget - dbloss;
	return signal;

}

/*** implement simple LOS propagation model (WIP)
***/
double FGRadio::LOS_calculate_attenuation(SGGeod pos, double freq, int transmission_type) {
	double frq_mhz;
	if( (freq < 118.0) || (freq > 137.0) )
		frq_mhz = 125.0; 	// sane value, middle of bandplan
	else
		frq_mhz = freq;
	double dbloss;
	double tx_pow = _transmitter_power;
	double ant_gain = _antenna_gain;
	double signal = 0.0;
	double ATC_HAAT = 30.0;
	double Aircraft_HAAT = 5.0;
	double sender_alt_ft,sender_alt;
	double transmitter_height=0.0;
	double receiver_height=0.0;
	double own_lat = fgGetDouble("/position/latitude-deg");
	double own_lon = fgGetDouble("/position/longitude-deg");
	double own_alt_ft = fgGetDouble("/position/altitude-ft");
	double own_alt= own_alt_ft * SG_FEET_TO_METER;
	
	if(transmission_type == 1)
		tx_pow = _transmitter_power + 6.0;

	if((transmission_type == 1) || (transmission_type == 3))
		ant_gain = _antenna_gain + 3.0; //pilot plane's antenna gain + ground station antenna gain
	
	double link_budget = tx_pow - _receiver_sensitivity + ant_gain;	

	//cerr << "ITM:: pilot Lat: " << own_lat << ", Lon: " << own_lon << ", Alt: " << own_alt << endl;
	
	SGGeod own_pos = SGGeod::fromDegM( own_lon, own_lat, own_alt );
	
	SGGeod sender_pos = pos;
	
	sender_alt_ft = sender_pos.getElevationFt();
	sender_alt = sender_alt_ft * SG_FEET_TO_METER;
	
	receiver_height = own_alt;
	transmitter_height = sender_alt;
	
	double distance_m = SGGeodesy::distanceM(own_pos, sender_pos);
	
	if(transmission_type == 1) 
		transmitter_height += ATC_HAAT;
	else
		transmitter_height += Aircraft_HAAT;
	
	/** radio horizon calculation with wave bending k=4/3 */
	double receiver_horizon = 4.12 * sqrt(receiver_height);
	double transmitter_horizon = 4.12 * sqrt(transmitter_height);
	double total_horizon = receiver_horizon + transmitter_horizon;
	
	if (distance_m > total_horizon) {
		return -1;
	}
	
	// free-space loss (distance calculation should be changed)
	dbloss = 20 * log10(distance_m) +20 * log10(frq_mhz) -27.55;
	signal = link_budget - dbloss;
	SG_LOG(SG_GENERAL, SG_BULK,
			"LOS:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm ");
	//cerr << "LOS:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm " << endl;
	return signal;
	
}
