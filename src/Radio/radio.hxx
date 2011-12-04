// radio.hxx -- FGRadio: class to manage radio propagation
//
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
	bool isOperable() const
		{ return _operable; }
	bool _operable; ///< is the unit serviceable, on, powered, etc
	
	double _receiver_sensitivity;
	double _transmitter_power;
	double _tx_antenna_height;
	double _rx_antenna_height;
	double _rx_antenna_gain;
	double _tx_antenna_gain;
	double _rx_line_losses;
	double _tx_line_losses;
	FGRadioAntenna* _TX_antenna;
	FGRadioAntenna* _RX_antenna;
	double _terrain_sampling_distance;
	int _polarization;
	std::map<string, double[2]> _mat_database;
	SGPropertyNode *_root_node;
	int _propagation_model; /// 0 none, 1 round Earth, 2 ITM
	double polarization_loss();
	double ITM_calculate_attenuation(SGGeod tx_pos, double freq, int ground_to_air);
	double LOS_calculate_attenuation(SGGeod tx_pos, double freq, int ground_to_air);
	void clutterLoss(double freq, double distance_m, double itm_elev[], std::deque<string> materials,
			double transmitter_height, double receiver_height, int p_mode,
			double horizons[], double &clutter_loss);
	void get_material_properties(string mat_name, double &height, double &density);
	
public:

    FGRadioTransmission();
    ~FGRadioTransmission();

    // a couple of setters and getters for convenience
    void setFrequency(double freq, int radio);
    double getFrequency(int radio);
    void setTxPower(double txpower) { _transmitter_power = txpower; };
    void setRxSensitivity(double sensitivity) { _receiver_sensitivity = sensitivity; };
    void setTxAntennaHeight(double tx_antenna_height) { _tx_antenna_height = tx_antenna_height; };
    void setRxAntennaHeight(double rx_antenna_height) { _rx_antenna_height = rx_antenna_height; };
    void setTxAntennaGain(double tx_antenna_gain) { _tx_antenna_gain = tx_antenna_gain; };
    void setRxAntennaGain(double rx_antenna_gain) { _rx_antenna_gain = rx_antenna_gain; };
    void setTxLineLosses(double tx_line_losses) { _tx_line_losses = tx_line_losses; };
    void setRxLineLosses(double rx_line_losses) { _rx_line_losses = rx_line_losses; };
    void setPropagationModel(int model) { _propagation_model = model; };
    void setPolarization(int polarization) { _polarization = polarization; };
    // accessory functions for unit conversions
    double watt_to_dbm(double power_watt);
    double dbm_to_watt(double dbm);
    double dbm_to_microvolt(double dbm);
    
    
    // transmission_type: 0 for air to ground 1 for ground to air, 2 for air to air, 3 for pilot to ground, 4 for pilot to air
    void receiveATC(SGGeod tx_pos, double freq, string text, int transmission_type);
    void receiveChat(SGGeod tx_pos, double freq, string text, int transmission_type);
    // returns signal quality
    // transmission_type: 0 for VOR, 1 for ILS
    double receiveNav(SGGeod tx_pos, double freq, int transmission_type);
    
};


