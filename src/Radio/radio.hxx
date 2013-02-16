// radio.hxx -- FGRadio: class to manage radio propagation
//
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


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <deque>
#include <Main/fg_props.hxx>

#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>
#include "antenna.hxx"

using std::string;


class FGRadioTransmission 
{
private:
	
	double _receiver_sensitivity;
	double _transmitter_power;
	double _tx_antenna_height;
	double _rx_antenna_height;
	double _rx_antenna_gain;
	double _tx_antenna_gain;
	double _rx_line_losses;
	double _tx_line_losses;
	
	double _terrain_sampling_distance;
	int _polarization;
	std::map<string, double[2]> _mat_database;
	SGPropertyNode *_root_node;
	int _propagation_model; /// 0 none, 1 round Earth, 2 ITM
	double polarization_loss();
	
	
/***  Implement radio attenuation		
*	  based on the Longley-Rice propagation model
*	ground_to_air: 0 for air to ground 1 for ground to air, 2 for air to air, 3 for pilot to ground, 4 for pilot to air
*	@param: transmitter position, frequency, flag to indicate if the transmission is from a ground station
*	@return: signal level above receiver treshhold sensitivity
***/
	double ITM_calculate_attenuation(SGGeod tx_pos, double freq, int ground_to_air);
	
/*** a simple alternative LOS propagation model (WIP)
*	@param: transmitter position, frequency, flag to indicate if the transmission is from a ground station
*	@return: signal level above receiver treshhold sensitivity
***/
	double LOS_calculate_attenuation(SGGeod tx_pos, double freq, int ground_to_air);
	
/*** Calculate losses due to vegetation and urban clutter (WIP)
*	 We are only worried about clutter loss, terrain influence 
*	 on the first Fresnel zone is calculated in the ITM functions
*	@param: frequency, elevation data, terrain type, horizon distances, calculated loss
*	@return: none
***/
	void calculate_clutter_loss(double freq, double itm_elev[], std::deque<string*> &materials,
			double transmitter_height, double receiver_height, int p_mode,
			double horizons[], double &clutter_loss);
	
/*** 	Temporary material properties database
*		@param: terrain type, median clutter height, radiowave attenuation factor
*		@return: none
***/
	void get_material_properties(string* mat_name, double &height, double &density);
	
	
public:

    FGRadioTransmission();
    ~FGRadioTransmission();

    /// a couple of setters and getters for convenience, call after initializing
    /// frequency is in MHz, sensitivity in dBm, antenna gain and losses in dB, transmitter power in dBm
    /// polarization can be: 0 horizontal, 1 vertical
    void setFrequency(double freq, int radio);
    double getFrequency(int radio);
    inline void setTxPower(double txpower) { _transmitter_power = txpower; };
    inline void setRxSensitivity(double sensitivity) { _receiver_sensitivity = sensitivity; };
    inline void setTxAntennaHeight(double tx_antenna_height) { _tx_antenna_height = tx_antenna_height; };
    inline void setRxAntennaHeight(double rx_antenna_height) { _rx_antenna_height = rx_antenna_height; };
    inline void setTxAntennaGain(double tx_antenna_gain) { _tx_antenna_gain = tx_antenna_gain; };
    inline void setRxAntennaGain(double rx_antenna_gain) { _rx_antenna_gain = rx_antenna_gain; };
    inline void setTxLineLosses(double tx_line_losses) { _tx_line_losses = tx_line_losses; };
    inline void setRxLineLosses(double rx_line_losses) { _rx_line_losses = rx_line_losses; };
    inline void setPropagationModel(int model) { _propagation_model = model; };
    inline void setPolarization(int polarization) { _polarization = polarization; };
    
    /// static convenience functions for unit conversions
    static double watt_to_dbm(double power_watt);
    static double dbm_to_watt(double dbm);
    static double dbm_to_microvolt(double dbm);
    
    
/*** Receive ATC radio communication as text
*	transmission_type: 0 for air to ground 1 for ground to air, 2 for air to air, 3 for pilot to ground, 4 for pilot to air
*	@param: transmitter position, frequency, ATC text, flag to indicate whether the transmission comes from an ATC groundstation
*	@return: none
***/
    void receiveATC(SGGeod tx_pos, double freq, string text, int transmission_type);
    
/*** TODO: receive multiplayer chat message and voice
*	@param: transmitter position, frequency, ATC text, flag to indicate whether the transmission comes from an ATC groundstation
*	@return: none
***/
    void receiveChat(SGGeod tx_pos, double freq, string text, int transmission_type);
    
/*** TODO: receive navaid 
*	@param: transmitter position, frequency, flag 
*	@return: signal level above receiver treshhold sensitivity
***/
    double receiveNav(SGGeod tx_pos, double freq, int transmission_type);
    
/*** Call this function to receive an arbitrary signal
*	for instance via the Nasal radioTransmission() function
*	returns the signal value above receiver sensitivity treshhold
*	@param: transmitter position, object heading in degrees (for antenna), object pitch angle in degrees 
*	@return: signal level above receiver treshhold sensitivity
***/
    double receiveBeacon(SGGeod &tx_pos, double heading, double pitch);
};


