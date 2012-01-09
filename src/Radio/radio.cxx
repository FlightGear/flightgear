// radio.cxx -- implementation of FGRadio
// Class to manage radio propagation using the ITM model
// Written by Adrian Musceac YO8RZZ, started August 2011.
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
#include <simgear/scene/material/mat.hxx>
#include <Scenery/scenery.hxx>
#include <boost/scoped_array.hpp>

#define WITH_POINT_TO_POINT 1
#include "itm.cpp"


FGRadioTransmission::FGRadioTransmission() {
	
	
	_receiver_sensitivity = -105.0;	// typical AM receiver sensitivity seems to be 0.8 microVolt at 12dB SINAD or less
	
	/** AM transmitter power in dBm.
	*	Typical output powers for ATC ground equipment, VHF-UHF:
	*	40 dBm - 10 W (ground, clearance)
	*	44 dBm - 20 W (tower)
	*	47 dBm - 50 W (center, sectors)
	*	50 dBm - 100 W (center, sectors)
	*	53 dBm - 200 W (sectors, on directional arrays)
	**/
	_transmitter_power = 43.0;
	
	_tx_antenna_height = 2.0; // TX antenna height above ground level
	
	_rx_antenna_height = 2.0; // RX antenna height above ground level
	
	
	_rx_antenna_gain = 1.0;	// maximum antenna gain expressed in dBi
	_tx_antenna_gain = 1.0;
	
	_rx_line_losses = 2.0;	// to be configured for each station
	_tx_line_losses = 2.0;
	
	_polarization = 1; // default vertical
	
	_propagation_model = 2; 
	
	_root_node = fgGetNode("sim/radio", true);
	_terrain_sampling_distance = _root_node->getDoubleValue("sampling-distance", 90.0); // regular SRTM is 90 meters
	
	
}

FGRadioTransmission::~FGRadioTransmission() 
{
}


double FGRadioTransmission::getFrequency(int radio) {
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


void FGRadioTransmission::receiveChat(SGGeod tx_pos, double freq, string text, int ground_to_air) {

}


double FGRadioTransmission::receiveNav(SGGeod tx_pos, double freq, int transmission_type) {
	
	// typical VOR/LOC transmitter power appears to be 100 - 200 Watt i.e 50 - 53 dBm
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


double FGRadioTransmission::receiveBeacon(SGGeod &tx_pos, double heading, double pitch) {
	
	// these properties should be set by an instrument
	_receiver_sensitivity = _root_node->getDoubleValue("station[0]/rx-sensitivity", _receiver_sensitivity);
	_transmitter_power = watt_to_dbm(_root_node->getDoubleValue("station[0]/tx-power-watt", _transmitter_power));
	_polarization = _root_node->getIntValue("station[0]/polarization", 1);
	_tx_antenna_height += _root_node->getDoubleValue("station[0]/tx-antenna-height", 0);
	_rx_antenna_height += _root_node->getDoubleValue("station[0]/rx-antenna-height", 0);
	_tx_antenna_gain += _root_node->getDoubleValue("station[0]/tx-antenna-gain", 0);
	_rx_antenna_gain += _root_node->getDoubleValue("station[0]/rx-antenna-gain", 0);
	
	double freq = _root_node->getDoubleValue("station[0]/frequency", 144.8);	// by default stay in the ham 2 meter band
	
	double comm1 = getFrequency(1);
	double comm2 = getFrequency(2);
	if ( !(fabs(freq - comm1) <= 0.0001) &&  !(fabs(freq - comm2) <= 0.0001) ) {
		return -1;
	}
	
	double signal = ITM_calculate_attenuation(tx_pos, freq, 1);
	
	return signal;
}



void FGRadioTransmission::receiveATC(SGGeod tx_pos, double freq, string text, int ground_to_air) {

	// adjust some default parameters in case the ATC code does not set them
	if(ground_to_air == 1) {
		_transmitter_power += 4.0;
		_tx_antenna_height += 30.0;
		_tx_antenna_gain += 2.0; 
	}
	
	double comm1 = getFrequency(1);
	double comm2 = getFrequency(2);
	if ( !(fabs(freq - comm1) <= 0.0001) &&  !(fabs(freq - comm2) <= 0.0001) ) {
		return;
	}
	else {
	
		if ( _propagation_model == 0) {		// skip propagation routines entirely
			fgSetString("/sim/messages/atc", text.c_str());
		}
		else if ( _propagation_model == 1 ) {		// Use free-space, round earth
			
			double signal = LOS_calculate_attenuation(tx_pos, freq, ground_to_air);
			if (signal <= 0.0) {
				return;
			}
			else {
				fgSetString("/sim/messages/atc", text.c_str());
			}
		}
		else if ( _propagation_model == 2 ) {	// Use ITM propagation model
			
			double signal = ITM_calculate_attenuation(tx_pos, freq, ground_to_air);
			if (signal <= 0.0) {
				return;
			}
			if ((signal > 0.0) && (signal < 12.0)) {
				/** for low SNR values need a way to make the conversation
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
				//double volume = (fabs(signal - 12.0) / 12);
				//double old_volume = fgGetDouble("/sim/sound/voices/voice/volume");
				
				//fgSetDouble("/sim/sound/voices/voice/volume", volume);
				fgSetString("/sim/messages/atc", text.c_str());
				//fgSetDouble("/sim/sound/voices/voice/volume", old_volume);
			}
			else {
				fgSetString("/sim/messages/atc", text.c_str());
			}
		}
	}
}


double FGRadioTransmission::ITM_calculate_attenuation(SGGeod pos, double freq, int transmission_type) {

	
	if((freq < 40.0) || (freq > 20000.0))	// frequency out of recommended range 
		return -1;
	/** ITM default parameters 
		TODO: take them from tile materials (especially for sea)?
	**/
	double eps_dielect=15.0;
	double sgm_conductivity = 0.005;
	double eno = 301.0;
	double frq_mhz = freq;
	
	int radio_climate = 5;		// continental temperate
	int pol= _polarization;	
	double conf = 0.90;	// 90% of situations and time, take into account speed
	double rel = 0.90;	
	double dbloss;
	char strmode[150];
	int p_mode = 0; // propgation mode selector: 0 LOS, 1 diffraction dominant, 2 troposcatter
	double horizons[2];
	int errnum;
	
	double clutter_loss = 0.0; 	// loss due to vegetation and urban
	double tx_pow = _transmitter_power;
	double ant_gain = _rx_antenna_gain + _tx_antenna_gain;
	double signal = 0.0;
	
	
	double link_budget = tx_pow - _receiver_sensitivity - _rx_line_losses - _tx_line_losses + ant_gain;	
	double signal_strength = tx_pow - _rx_line_losses - _tx_line_losses + ant_gain;	
	double tx_erp = dbm_to_watt(tx_pow + _tx_antenna_gain - _tx_line_losses);
	

	FGScenery * scenery = globals->get_scenery();
	
	double own_lat = fgGetDouble("/position/latitude-deg");
	double own_lon = fgGetDouble("/position/longitude-deg");
	double own_alt_ft = fgGetDouble("/position/altitude-ft");
	double own_heading = fgGetDouble("/orientation/heading-deg");
	double own_alt= own_alt_ft * SG_FEET_TO_METER;
	
	
	
	
	SGGeod own_pos = SGGeod::fromDegM( own_lon, own_lat, own_alt );
	SGGeod max_own_pos = SGGeod::fromDegM( own_lon, own_lat, SG_MAX_ELEVATION_M );
	SGGeoc center = SGGeoc::fromGeod( max_own_pos );
	SGGeoc own_pos_c = SGGeoc::fromGeod( own_pos );
	
	
	double sender_alt_ft,sender_alt;
	double transmitter_height=0.0;
	double receiver_height=0.0;
	SGGeod sender_pos = pos;
	
	sender_alt_ft = sender_pos.getElevationFt();
	sender_alt = sender_alt_ft * SG_FEET_TO_METER;
	SGGeod max_sender_pos = SGGeod::fromGeodM( pos, SG_MAX_ELEVATION_M );
	SGGeoc sender_pos_c = SGGeoc::fromGeod( sender_pos );
	
	
	double point_distance= _terrain_sampling_distance; 
	double course = SGGeodesy::courseRad(own_pos_c, sender_pos_c);
	double reverse_course = SGGeodesy::courseRad(sender_pos_c, own_pos_c);
	double distance_m = SGGeodesy::distanceM(own_pos, sender_pos);
	double probe_distance = 0.0;
	/** If distance larger than this value (300 km), assume reception imposssible to spare CPU cycles */
	if (distance_m > 300000)
		return -1.0;
	/** If above 8000 meters, consider LOS mode and calculate free-space att to spare CPU cycles */
	if (own_alt > 8000) {
		dbloss = 20 * log10(distance_m) +20 * log10(frq_mhz) -27.55;
		SG_LOG(SG_GENERAL, SG_BULK,
			"ITM Free-space mode:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm, free-space attenuation");
		//cerr << "ITM Free-space mode:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm, free-space attenuation" << endl;
		signal = link_budget - dbloss;
		return signal;
	}
	
		
	int max_points = (int)floor(distance_m / point_distance);
	//double delta_last = fmod(distance_m, point_distance);
	
	deque<double> elevations;
	deque<string*> materials;
	

	double elevation_under_pilot = 0.0;
	if (scenery->get_elevation_m( max_own_pos, elevation_under_pilot, NULL )) {
		receiver_height = own_alt - elevation_under_pilot; 
	}

	double elevation_under_sender = 0.0;
	if (scenery->get_elevation_m( max_sender_pos, elevation_under_sender, NULL )) {
		transmitter_height = sender_alt - elevation_under_sender;
	}
	else {
		transmitter_height = sender_alt;
	}
	
	
	transmitter_height += _tx_antenna_height;
	receiver_height += _rx_antenna_height;
	
	//cerr << "ITM:: RX-height: " << receiver_height << " meters, TX-height: " << transmitter_height << " meters, Distance: " << distance_m << " meters" << endl;
	_root_node->setDoubleValue("station[0]/rx-height", receiver_height);
	_root_node->setDoubleValue("station[0]/tx-height", transmitter_height);
	_root_node->setDoubleValue("station[0]/distance", distance_m / 1000);
	
	unsigned int e_size = (deque<unsigned>::size_type)max_points;
	
	while (elevations.size() <= e_size) {
		probe_distance += point_distance;
		SGGeod probe = SGGeod::fromGeoc(center.advanceRadM( course, probe_distance ));
		const SGMaterial *mat = 0;
		double elevation_m = 0.0;
	
		if (scenery->get_elevation_m( probe, elevation_m, &mat )) {
			if((transmission_type == 3) || (transmission_type == 4)) {
				elevations.push_back(elevation_m);
				if(mat) {
					const std::vector<string> mat_names = mat->get_names();
					string* name = new string(mat_names[0]);
					materials.push_back(name);
				}
				else {
					string* no_material = new string("None"); 
					materials.push_back(no_material);
				}
			}
			else {
				 elevations.push_front(elevation_m);
				 if(mat) {
				 	 const std::vector<string> mat_names = mat->get_names();
				 	 string* name = new string(mat_names[0]);
				 	 materials.push_front(name);
				}
				else {
					string* no_material = new string("None"); 
					materials.push_front(no_material);
				}
			}
		}
		else {
			if((transmission_type == 3) || (transmission_type == 4)) {
				elevations.push_back(0.0);
				string* no_material = new string("None"); 
				materials.push_back(no_material);
			}
			else {
				string* no_material = new string("None"); 
				elevations.push_front(0.0);
				materials.push_front(no_material);
			}
		}
	}
	if((transmission_type == 3) || (transmission_type == 4)) {
		elevations.push_front(elevation_under_pilot);
		//if (delta_last > (point_distance / 2) )			// only add last point if it's farther than half point_distance
			elevations.push_back(elevation_under_sender);
	}
	else {
		elevations.push_back(elevation_under_pilot);
		//if (delta_last > (point_distance / 2) )
			elevations.push_front(elevation_under_sender);
	}
	
	
	double num_points= (double)elevations.size();


	elevations.push_front(point_distance);
	elevations.push_front(num_points -1);

	int size = elevations.size();
        boost::scoped_array<double> itm_elev( new double[size] );

	for(int i=0;i<size;i++) {
		itm_elev[i]=elevations[i];
	}
	
	if((transmission_type == 3) || (transmission_type == 4)) {
		// the sender and receiver roles are switched
		ITM::point_to_point(itm_elev.get(), receiver_height, transmitter_height,
			eps_dielect, sgm_conductivity, eno, frq_mhz, radio_climate,
			pol, conf, rel, dbloss, strmode, p_mode, horizons, errnum);
		if( _root_node->getBoolValue( "use-clutter-attenuation", false ) )
			calculate_clutter_loss(frq_mhz, itm_elev.get(), materials, receiver_height, transmitter_height, p_mode, horizons, clutter_loss);
	}
	else {
		ITM::point_to_point(itm_elev.get(), transmitter_height, receiver_height,
			eps_dielect, sgm_conductivity, eno, frq_mhz, radio_climate,
			pol, conf, rel, dbloss, strmode, p_mode, horizons, errnum);
		if( _root_node->getBoolValue( "use-clutter-attenuation", false ) )
			calculate_clutter_loss(frq_mhz, itm_elev.get(), materials, transmitter_height, receiver_height, p_mode, horizons, clutter_loss);
	}
	
	double pol_loss = 0.0;
	// TODO: remove this check after we check a bit the axis calculations in this function
	if (_polarization == 1) {
		pol_loss = polarization_loss();
	}
	//SG_LOG(SG_GENERAL, SG_BULK,
	//		"ITM:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm, " << strmode << ", Error: " << errnum);
	//cerr << "ITM:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm, " << strmode << ", Error: " << errnum << endl;
	_root_node->setDoubleValue("station[0]/link-budget", link_budget);
	_root_node->setDoubleValue("station[0]/terrain-attenuation", dbloss);
	_root_node->setStringValue("station[0]/prop-mode", strmode);
	_root_node->setDoubleValue("station[0]/clutter-attenuation", clutter_loss);
	_root_node->setDoubleValue("station[0]/polarization-attenuation", pol_loss);
	//if (errnum == 4)	// if parameters are outside sane values for lrprop, bail out fast
	//	return -1;
	
	// temporary, keep this antenna radiation pattern code here
	double tx_pattern_gain = 0.0;
	double rx_pattern_gain = 0.0;
	double sender_heading = 270.0; // due West
	double tx_antenna_bearing = sender_heading - reverse_course * SGD_RADIANS_TO_DEGREES;
	double rx_antenna_bearing = own_heading - course * SGD_RADIANS_TO_DEGREES;
	double rx_elev_angle = atan((itm_elev[2] + transmitter_height - itm_elev[(int)itm_elev[0] + 2] + receiver_height) / distance_m) * SGD_RADIANS_TO_DEGREES;
	double tx_elev_angle = 0.0 - rx_elev_angle;
	if (_root_node->getBoolValue("use-tx-antenna-pattern", false)) {
		FGRadioAntenna* TX_antenna;
		TX_antenna = new FGRadioAntenna("Plot2");
		TX_antenna->set_heading(sender_heading);
		TX_antenna->set_elevation_angle(0);
		tx_pattern_gain = TX_antenna->calculate_gain(tx_antenna_bearing, tx_elev_angle);
		delete TX_antenna;
	}
	if (_root_node->getBoolValue("use-rx-antenna-pattern", false)) {
		FGRadioAntenna* RX_antenna;
		RX_antenna = new FGRadioAntenna("Plot2");
		RX_antenna->set_heading(own_heading);
		RX_antenna->set_elevation_angle(fgGetDouble("/orientation/pitch-deg"));
		rx_pattern_gain = RX_antenna->calculate_gain(rx_antenna_bearing, rx_elev_angle);
		delete RX_antenna;
	}
	
	signal = link_budget - dbloss - clutter_loss + pol_loss + rx_pattern_gain + tx_pattern_gain;
	double signal_strength_dbm = signal_strength - dbloss - clutter_loss + pol_loss + rx_pattern_gain + tx_pattern_gain;
	double field_strength_uV = dbm_to_microvolt(signal_strength_dbm);
	_root_node->setDoubleValue("station[0]/signal-dbm", signal_strength_dbm);
	_root_node->setDoubleValue("station[0]/field-strength-uV", field_strength_uV);
	_root_node->setDoubleValue("station[0]/signal", signal);
	_root_node->setDoubleValue("station[0]/tx-erp", tx_erp);

	//_root_node->setDoubleValue("station[0]/tx-pattern-gain", tx_pattern_gain);
	//_root_node->setDoubleValue("station[0]/rx-pattern-gain", rx_pattern_gain);

	for (unsigned i =0; i < materials.size(); i++) {
		delete materials[i];
	}
	
	return signal;

}


void FGRadioTransmission::calculate_clutter_loss(double freq, double itm_elev[], deque<string*> &materials,
	double transmitter_height, double receiver_height, int p_mode,
	double horizons[], double &clutter_loss) {
	
	double distance_m = itm_elev[0] * itm_elev[1]; // only consider elevation points
	unsigned mat_size = materials.size();
	if (p_mode == 0) {	// LOS: take each point and see how clutter height affects first Fresnel zone
		int mat = 0;
		int j=1; 
		for (int k=3;k < (int)(itm_elev[0]) + 2;k++) {
			
			double clutter_height = 0.0;	// mean clutter height for a certain terrain type
			double clutter_density = 0.0;	// percent of reflected wave
			if((unsigned)mat >= mat_size) {	//this tends to happen when the model interferes with the antenna (obstructs)
				//cerr << "Array index out of bounds 0-0: " << mat << " size: " << mat_size << endl;
				break;
			}
			get_material_properties(materials[mat], clutter_height, clutter_density);
			
			double grad = fabs(itm_elev[2] + transmitter_height - itm_elev[(int)itm_elev[0] + 2] + receiver_height) / distance_m;
			// First Fresnel radius
			double frs_rad = 548 * sqrt( (j * itm_elev[1] * (itm_elev[0] - j) * itm_elev[1] / 1000000) / (  distance_m * freq / 1000) );
			if (frs_rad <= 0.0) {	//this tends to happen when the model interferes with the antenna (obstructs)
				//cerr << "Frs rad 0-0: " << frs_rad << endl;
				continue;
			}
			//double earth_h = distance_m * (distance_m - j * itm_elev[1]) / ( 1000000 * 12.75 * 1.33 );	// K=4/3
			
			double min_elev = SGMiscd::min(itm_elev[2] + transmitter_height, itm_elev[(int)itm_elev[0] + 2] + receiver_height);
			double d1 = j * itm_elev[1];
			if ((itm_elev[2] + transmitter_height) > ( itm_elev[(int)itm_elev[0] + 2] + receiver_height) ) {
				d1 = (itm_elev[0] - j) * itm_elev[1];
			}
			double ray_height = (grad * d1) + min_elev;
			
			double clearance = ray_height - (itm_elev[k] + clutter_height) - frs_rad * 8/10;		
			double intrusion = fabs(clearance);
			
			if (clearance >= 0) {
				// no losses
			}
			else if (clearance < 0 && (intrusion < clutter_height)) {
				
				clutter_loss += clutter_density * (intrusion / (frs_rad * 2) ) * (freq/100) * (itm_elev[1]/100);
			}
			else if (clearance < 0 && (intrusion > clutter_height)) {
				clutter_loss += clutter_density * (clutter_height / (frs_rad * 2 ) ) * (freq/100) * (itm_elev[1]/100);
			}
			else {
				// no losses
			}
			j++;
			mat++;
		}
		
	}
	else if (p_mode == 1) {		// diffraction
		
		if (horizons[1] == 0.0) {	//	single horizon: same as above, except pass twice using the highest point
			int num_points_1st = (int)floor( horizons[0] * itm_elev[0]/ distance_m ); 
			int num_points_2nd = (int)ceil( (distance_m - horizons[0]) * itm_elev[0] / distance_m ); 
			//cerr << "Diffraction 1 horizon:: points1: " << num_points_1st << " points2: " << num_points_2nd << endl;
			int last = 1;
			/** perform the first pass */
			int mat = 0;
			int j=1; 
			for (int k=3;k < num_points_1st + 2;k++) {
				if (num_points_1st < 1)
					break;
				double clutter_height = 0.0;	// mean clutter height for a certain terrain type
				double clutter_density = 0.0;	// percent of reflected wave
				
				if((unsigned)mat >= mat_size) {		
					//cerr << "Array index out of bounds 1-1: " << mat << " size: " << mat_size << endl;
					break;
				}
				get_material_properties(materials[mat], clutter_height, clutter_density);
				
				double grad = fabs(itm_elev[2] + transmitter_height - itm_elev[num_points_1st + 2] + clutter_height) / distance_m;
				// First Fresnel radius
				double frs_rad = 548 * sqrt( (j * itm_elev[1] * (num_points_1st - j) * itm_elev[1] / 1000000) / ( num_points_1st * itm_elev[1] * freq / 1000) );
				if (frs_rad <= 0.0) {	
					//cerr << "Frs rad 1-1: " << frs_rad << endl;
					continue;
				}
				//double earth_h = distance_m * (distance_m - j * itm_elev[1]) / ( 1000000 * 12.75 * 1.33 );	// K=4/3
				
				double min_elev = SGMiscd::min(itm_elev[2] + transmitter_height, itm_elev[num_points_1st + 2] + clutter_height);
				double d1 = j * itm_elev[1];
				if ( (itm_elev[2] + transmitter_height) > (itm_elev[num_points_1st + 2] + clutter_height) ) {
					d1 = (num_points_1st - j) * itm_elev[1];
				}
				double ray_height = (grad * d1) + min_elev;
				
				double clearance = ray_height - (itm_elev[k] + clutter_height) - frs_rad * 8/10;		
				double intrusion = fabs(clearance);
				
				if (clearance >= 0) {
					// no losses
				}
				else if (clearance < 0 && (intrusion < clutter_height)) {
					
					clutter_loss += clutter_density * (intrusion / (frs_rad * 2) ) * (freq/100) * (itm_elev[1]/100);
				}
				else if (clearance < 0 && (intrusion > clutter_height)) {
					clutter_loss += clutter_density * (clutter_height / (frs_rad * 2 ) ) * (freq/100) * (itm_elev[1]/100);
				}
				else {
					// no losses
				}
				j++;
				mat++;
				last = k;
			}
			
			/** and the second pass */
			mat +=1;
			j =1; // first point is diffraction edge, 2nd the RX elevation
			for (int k=last+2;k < (int)(itm_elev[0]) + 2;k++) {
				if (num_points_2nd < 1)
					break;
				double clutter_height = 0.0;	// mean clutter height for a certain terrain type
				double clutter_density = 0.0;	// percent of reflected wave
				
				if((unsigned)mat >= mat_size) {		
					//cerr << "Array index out of bounds 1-2: " << mat << " size: " << mat_size << endl;
					break;
				}
				get_material_properties(materials[mat], clutter_height, clutter_density);
				
				double grad = fabs(itm_elev[last+1] + clutter_height - itm_elev[(int)itm_elev[0] + 2] + receiver_height) / distance_m;
				// First Fresnel radius
				double frs_rad = 548 * sqrt( (j * itm_elev[1] * (num_points_2nd - j) * itm_elev[1] / 1000000) / (  num_points_2nd * itm_elev[1] * freq / 1000) );
				if (frs_rad <= 0.0) {	
					//cerr << "Frs rad 1-2: " << frs_rad << " numpoints2 " << num_points_2nd << " j: " << j << endl;
					continue;
				}
				//double earth_h = distance_m * (distance_m - j * itm_elev[1]) / ( 1000000 * 12.75 * 1.33 );	// K=4/3
				
				double min_elev = SGMiscd::min(itm_elev[last+1] + clutter_height, itm_elev[(int)itm_elev[0] + 2] + receiver_height);
				double d1 = j * itm_elev[1];
				if ( (itm_elev[last+1] + clutter_height) > (itm_elev[(int)itm_elev[0] + 2] + receiver_height) ) { 
					d1 = (num_points_2nd - j) * itm_elev[1];
				}
				double ray_height = (grad * d1) + min_elev;
				
				double clearance = ray_height - (itm_elev[k] + clutter_height) - frs_rad * 8/10;		
				double intrusion = fabs(clearance);
				
				if (clearance >= 0) {
					// no losses
				}
				else if (clearance < 0 && (intrusion < clutter_height)) {
					
					clutter_loss += clutter_density * (intrusion / (frs_rad * 2) ) * (freq/100) * (itm_elev[1]/100);
				}
				else if (clearance < 0 && (intrusion > clutter_height)) {
					clutter_loss += clutter_density * (clutter_height / (frs_rad * 2 ) ) * (freq/100) * (itm_elev[1]/100);
				}
				else {
					// no losses
				}
				j++;
				mat++;
			}
			
		}
		else {	// double horizon: same as single horizon, except there are 3 segments
			
			int num_points_1st = (int)floor( horizons[0] * itm_elev[0] / distance_m ); 
			int num_points_2nd = (int)floor(horizons[1] * itm_elev[0] / distance_m ); 
			int num_points_3rd = (int)itm_elev[0] - num_points_1st - num_points_2nd; 
			//cerr << "Double horizon:: horizon1: " << horizons[0] << " horizon2: " << horizons[1] << " distance: " << distance_m << endl;
			//cerr << "Double horizon:: points1: " << num_points_1st << " points2: " << num_points_2nd << " points3: " << num_points_3rd << endl;
			int last = 1;
			/** perform the first pass */
			int mat = 0;
			int j=1; // first point is TX elevation, 2nd is obstruction elevation
			for (int k=3;k < num_points_1st +2;k++) {
				if (num_points_1st < 1)
					break;
				double clutter_height = 0.0;	// mean clutter height for a certain terrain type
				double clutter_density = 0.0;	// percent of reflected wave
				if((unsigned)mat >= mat_size) {		
					//cerr << "Array index out of bounds 2-1: " << mat << " size: " << mat_size << endl;
					break;
				}
				get_material_properties(materials[mat], clutter_height, clutter_density);
				
				double grad = fabs(itm_elev[2] + transmitter_height - itm_elev[num_points_1st + 2] + clutter_height) / distance_m;
				// First Fresnel radius
				double frs_rad = 548 * sqrt( (j * itm_elev[1] * (num_points_1st - j) * itm_elev[1] / 1000000) / (  num_points_1st * itm_elev[1] * freq / 1000) );
				if (frs_rad <= 0.0) {		
					//cerr << "Frs rad 2-1: " << frs_rad << " numpoints1 " << num_points_1st << " j: " << j << endl;
					continue;
				}
				//double earth_h = distance_m * (distance_m - j * itm_elev[1]) / ( 1000000 * 12.75 * 1.33 );	// K=4/3
				
				double min_elev = SGMiscd::min(itm_elev[2] + transmitter_height, itm_elev[num_points_1st + 2] + clutter_height);
				double d1 = j * itm_elev[1];
				if ( (itm_elev[2] + transmitter_height) > (itm_elev[num_points_1st + 2] + clutter_height) ) {
					d1 = (num_points_1st - j) * itm_elev[1];
				}
				double ray_height = (grad * d1) + min_elev;
				
				double clearance = ray_height - (itm_elev[k] + clutter_height) - frs_rad * 8/10;		
				double intrusion = fabs(clearance);
				
				if (clearance >= 0) {
					// no losses
				}
				else if (clearance < 0 && (intrusion < clutter_height)) {
					
					clutter_loss += clutter_density * (intrusion / (frs_rad * 2) ) * (freq/100) * (itm_elev[1]/100);
				}
				else if (clearance < 0 && (intrusion > clutter_height)) {
					clutter_loss += clutter_density * (clutter_height / (frs_rad * 2 ) ) * (freq/100) * (itm_elev[1]/100);
				}
				else {
					// no losses
				}
				j++;
				mat++;
				last = k;
			}
			mat +=1;
			/** and the second pass */
			int last2=1;
			j =1; // first point is 1st obstruction elevation, 2nd is 2nd obstruction elevation
			for (int k=last+2;k < num_points_1st + num_points_2nd +2;k++) {
				if (num_points_2nd < 1)
					break;
				double clutter_height = 0.0;	// mean clutter height for a certain terrain type
				double clutter_density = 0.0;	// percent of reflected wave
				if((unsigned)mat >= mat_size) {		
					//cerr << "Array index out of bounds 2-2: " << mat << " size: " << mat_size << endl;
					break;
				}
				get_material_properties(materials[mat], clutter_height, clutter_density);
				
				double grad = fabs(itm_elev[last+1] + clutter_height - itm_elev[num_points_1st + num_points_2nd + 2] + clutter_height) / distance_m;
				// First Fresnel radius
				double frs_rad = 548 * sqrt( (j * itm_elev[1] * (num_points_2nd - j) * itm_elev[1] / 1000000) / (  num_points_2nd * itm_elev[1] * freq / 1000) );
				if (frs_rad <= 0.0) {	
					//cerr << "Frs rad 2-2: " << frs_rad << " numpoints2 " << num_points_2nd << " j: " << j << endl;
					continue;
				}
				//double earth_h = distance_m * (distance_m - j * itm_elev[1]) / ( 1000000 * 12.75 * 1.33 );	// K=4/3
				
				double min_elev = SGMiscd::min(itm_elev[last+1] + clutter_height, itm_elev[num_points_1st + num_points_2nd +2] + clutter_height);
				double d1 = j * itm_elev[1];
				if ( (itm_elev[last+1] + clutter_height) > (itm_elev[num_points_1st + num_points_2nd + 2] + clutter_height) ) { 
					d1 = (num_points_2nd - j) * itm_elev[1];
				}
				double ray_height = (grad * d1) + min_elev;
				
				double clearance = ray_height - (itm_elev[k] + clutter_height) - frs_rad * 8/10;		
				double intrusion = fabs(clearance);
				
				if (clearance >= 0) {
					// no losses
				}
				else if (clearance < 0 && (intrusion < clutter_height)) {
					
					clutter_loss += clutter_density * (intrusion / (frs_rad * 2) ) * (freq/100) * (itm_elev[1]/100);
				}
				else if (clearance < 0 && (intrusion > clutter_height)) {
					clutter_loss += clutter_density * (clutter_height / (frs_rad * 2 ) ) * (freq/100) * (itm_elev[1]/100);
				}
				else {
					// no losses
				}
				j++;
				mat++;
				last2 = k;
			}
			
			/** third and final pass */
			mat +=1;
			j =1; // first point is 2nd obstruction elevation, 3rd is RX elevation
			for (int k=last2+2;k < (int)itm_elev[0] + 2;k++) {
				if (num_points_3rd < 1)
					break;
				double clutter_height = 0.0;	// mean clutter height for a certain terrain type
				double clutter_density = 0.0;	// percent of reflected wave
				if((unsigned)mat >= mat_size) {		
					//cerr << "Array index out of bounds 2-3: " << mat << " size: " << mat_size << endl;
					break;
				}
				get_material_properties(materials[mat], clutter_height, clutter_density);
				
				double grad = fabs(itm_elev[last2+1] + clutter_height - itm_elev[(int)itm_elev[0] + 2] + receiver_height) / distance_m;
				// First Fresnel radius
				double frs_rad = 548 * sqrt( (j * itm_elev[1] * (num_points_3rd - j) * itm_elev[1] / 1000000) / (  num_points_3rd * itm_elev[1] * freq / 1000) );
				if (frs_rad <= 0.0) {		
					//cerr << "Frs rad 2-3: " << frs_rad << " numpoints3 " << num_points_3rd << " j: " << j << endl;
					continue;
				}
				
				//double earth_h = distance_m * (distance_m - j * itm_elev[1]) / ( 1000000 * 12.75 * 1.33 );	// K=4/3
				
				double min_elev = SGMiscd::min(itm_elev[last2+1] + clutter_height, itm_elev[(int)itm_elev[0] + 2] + receiver_height);
				double d1 = j * itm_elev[1];
				if ( (itm_elev[last2+1] + clutter_height) > (itm_elev[(int)itm_elev[0] + 2] + receiver_height) ) { 
					d1 = (num_points_3rd - j) * itm_elev[1];
				}
				double ray_height = (grad * d1) + min_elev;
				
				double clearance = ray_height - (itm_elev[k] + clutter_height) - frs_rad * 8/10;		
				double intrusion = fabs(clearance);
				
				if (clearance >= 0) {
					// no losses
				}
				else if (clearance < 0 && (intrusion < clutter_height)) {
					
					clutter_loss += clutter_density * (intrusion / (frs_rad * 2) ) * (freq/100) * (itm_elev[1]/100);
				}
				else if (clearance < 0 && (intrusion > clutter_height)) {
					clutter_loss += clutter_density * (clutter_height / (frs_rad * 2 ) ) * (freq/100) * (itm_elev[1]/100);
				}
				else {
					// no losses
				}
				j++;
				mat++;
				
			}
			
		}
	}
	else if (p_mode == 2) {		//	troposcatter: ignore ground clutter for now... maybe do something with weather
		clutter_loss = 0.0;
	}
	
}


void FGRadioTransmission::get_material_properties(string* mat_name, double &height, double &density) {
	
	if(!mat_name)
		return;
	
	if(*mat_name == "Landmass") {
		height = 15.0;
		density = 0.2;
	}

	else if(*mat_name == "SomeSort") {
		height = 15.0;
		density = 0.2;
	}

	else if(*mat_name == "Island") {
		height = 15.0;
		density = 0.2;
	}
	else if(*mat_name == "Default") {
		height = 15.0;
		density = 0.2;
	}
	else if(*mat_name == "EvergreenBroadCover") {
		height = 20.0;
		density = 0.2;
	}
	else if(*mat_name == "EvergreenForest") {
		height = 20.0;
		density = 0.2;
	}
	else if(*mat_name == "DeciduousBroadCover") {
		height = 15.0;
		density = 0.3;
	}
	else if(*mat_name == "DeciduousForest") {
		height = 15.0;
		density = 0.3;
	}
	else if(*mat_name == "MixedForestCover") {
		height = 20.0;
		density = 0.25;
	}
	else if(*mat_name == "MixedForest") {
		height = 15.0;
		density = 0.25;
	}
	else if(*mat_name == "RainForest") {
		height = 25.0;
		density = 0.55;
	}
	else if(*mat_name == "EvergreenNeedleCover") {
		height = 15.0;
		density = 0.2;
	}
	else if(*mat_name == "WoodedTundraCover") {
		height = 5.0;
		density = 0.15;
	}
	else if(*mat_name == "DeciduousNeedleCover") {
		height = 5.0;
		density = 0.2;
	}
	else if(*mat_name == "ScrubCover") {
		height = 3.0;
		density = 0.15;
	}
	else if(*mat_name == "BuiltUpCover") {
		height = 30.0;
		density = 0.7;
	}
	else if(*mat_name == "Urban") {
		height = 30.0;
		density = 0.7;
	}
	else if(*mat_name == "Construction") {
		height = 30.0;
		density = 0.7;
	}
	else if(*mat_name == "Industrial") {
		height = 30.0;
		density = 0.7;
	}
	else if(*mat_name == "Port") {
		height = 30.0;
		density = 0.7;
	}
	else if(*mat_name == "Town") {
		height = 10.0;
		density = 0.5;
	}
	else if(*mat_name == "SubUrban") {
		height = 10.0;
		density = 0.5;
	}
	else if(*mat_name == "CropWoodCover") {
		height = 10.0;
		density = 0.1;
	}
	else if(*mat_name == "CropWood") {
		height = 10.0;
		density = 0.1;
	}
	else if(*mat_name == "AgroForest") {
		height = 10.0;
		density = 0.1;
	}
	else {
		height = 0.0;
		density = 0.0;
	}
	
}


double FGRadioTransmission::LOS_calculate_attenuation(SGGeod pos, double freq, int transmission_type) {
	
	double frq_mhz = freq;
	double dbloss;
	double tx_pow = _transmitter_power;
	double ant_gain = _rx_antenna_gain + _tx_antenna_gain;
	double signal = 0.0;
	
	double sender_alt_ft,sender_alt;
	double transmitter_height=0.0;
	double receiver_height=0.0;
	double own_lat = fgGetDouble("/position/latitude-deg");
	double own_lon = fgGetDouble("/position/longitude-deg");
	double own_alt_ft = fgGetDouble("/position/altitude-ft");
	double own_alt= own_alt_ft * SG_FEET_TO_METER;
	
	
	double link_budget = tx_pow - _receiver_sensitivity - _rx_line_losses - _tx_line_losses + ant_gain;	

	//cerr << "ITM:: pilot Lat: " << own_lat << ", Lon: " << own_lon << ", Alt: " << own_alt << endl;
	
	SGGeod own_pos = SGGeod::fromDegM( own_lon, own_lat, own_alt );
	
	SGGeod sender_pos = pos;
	
	sender_alt_ft = sender_pos.getElevationFt();
	sender_alt = sender_alt_ft * SG_FEET_TO_METER;
	
	receiver_height = own_alt;
	transmitter_height = sender_alt;
	
	double distance_m = SGGeodesy::distanceM(own_pos, sender_pos);
	
	
	transmitter_height += _tx_antenna_height;
	receiver_height += _rx_antenna_height;
	
	
	/** radio horizon calculation with wave bending k=4/3 */
	double receiver_horizon = 4.12 * sqrt(receiver_height);
	double transmitter_horizon = 4.12 * sqrt(transmitter_height);
	double total_horizon = receiver_horizon + transmitter_horizon;
	
	if (distance_m > total_horizon) {
		return -1;
	}
	double pol_loss = 0.0;
	if (_polarization == 1) {
		pol_loss = polarization_loss();
	}
	// free-space loss (distance calculation should be changed)
	dbloss = 20 * log10(distance_m) +20 * log10(frq_mhz) -27.55;
	signal = link_budget - dbloss + pol_loss;

	//cerr << "LOS:: Link budget: " << link_budget << ", Attenuation: " << dbloss << " dBm " << endl;
	return signal;
	
}

/*** calculate loss due to polarization mismatch
*	this function is only reliable for vertical polarization
*	due to the V-shape of horizontally polarized antennas
***/
double FGRadioTransmission::polarization_loss() {
	
	double theta_deg;
	double roll = fgGetDouble("/orientation/roll-deg");
	if (fabs(roll) > 85.0)
		roll = 85.0;
	double pitch = fgGetDouble("/orientation/pitch-deg");
	if (fabs(pitch) > 85.0)
		pitch = 85.0;
	double theta = fabs( atan( sqrt( 
		pow(tan(roll * SGD_DEGREES_TO_RADIANS), 2) + 
		pow(tan(pitch * SGD_DEGREES_TO_RADIANS), 2) )) * SGD_RADIANS_TO_DEGREES);
	
	if (_polarization == 0)
		theta_deg = 90.0 - theta;
	else
		theta_deg = theta;
	if (theta_deg > 85.0)	// we don't want to converge into infinity
		theta_deg = 85.0;
	
	double loss = 10 * log10( pow(cos(theta_deg * SGD_DEGREES_TO_RADIANS), 2) );
	//cerr << "Polarization loss: " << loss << " dBm " << endl;
	return loss;
}


double FGRadioTransmission::watt_to_dbm(double power_watt) {
	return 10 * log10(1000 * power_watt);	// returns dbm
}

double FGRadioTransmission::dbm_to_watt(double dbm) {
	return exp( (dbm-30) * log(10.0) / 10.0);	// returns Watts
}

double FGRadioTransmission::dbm_to_microvolt(double dbm) {
	return sqrt(dbm_to_watt(dbm) * 50) * 1000000;	// returns microvolts
}


