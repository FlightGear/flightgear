
//// opengc.hxx - Network interface program to send sim data onto a LAN
//
// Created by: 	J. Wojnaroski  -- castle@mminternet.com
// Date:		21 Nov 2001 
//
// 
// Adapted from original network code developed by C. Olson
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
//


#ifndef _FG_OPENGC_HXX
#define _FG_OPENGC_HXX

#include <simgear/compiler.h>

#include <string>

#include <Main/fg_props.hxx>

#include "protocol.hxx"
#include "opengc_data.hxx"

class FlightProperties;

class FGOpenGC : public FGProtocol
{

    ogcFGData buf;
    FlightProperties* fdm;
    
    // Environment
    SGPropertyNode_ptr press_node;
    SGPropertyNode_ptr temp_node;
    SGPropertyNode_ptr wind_dir_node;
    SGPropertyNode_ptr wind_speed_node;
    SGPropertyNode_ptr magvar_node;
    
    // Position on the Geod
    SGPropertyNode_ptr p_latitude;
    SGPropertyNode_ptr p_longitude;
    SGPropertyNode_ptr p_elev_node;
    //SGPropertyNode_ptr p_altitude;
    SGPropertyNode_ptr p_altitude_agl;
    
    // Orientation
    SGPropertyNode_ptr p_pitch;
    SGPropertyNode_ptr p_bank;
    SGPropertyNode_ptr p_heading;
    SGPropertyNode_ptr p_yaw;
    SGPropertyNode_ptr p_yaw_rate;
    
    // Flight Parameters
    SGPropertyNode_ptr vel_kcas;
    SGPropertyNode_ptr p_vvi;
    SGPropertyNode_ptr p_mach;
    
    // Control surfaces
    SGPropertyNode_ptr p_left_aileron;
    SGPropertyNode_ptr p_right_aileron;
    SGPropertyNode_ptr p_elevator;
    SGPropertyNode_ptr p_elevator_trim;
    SGPropertyNode_ptr p_rudder;
    SGPropertyNode_ptr p_flaps;
    SGPropertyNode_ptr p_flaps_cmd;
    
    // GEAR System
    SGPropertyNode_ptr p_park_brake;
    
    // Engines
    SGPropertyNode_ptr egt0_node;
    SGPropertyNode_ptr egt1_node;
    SGPropertyNode_ptr egt2_node;
    SGPropertyNode_ptr egt3_node;
    
    SGPropertyNode_ptr epr0_node;
    SGPropertyNode_ptr epr1_node;
    SGPropertyNode_ptr epr2_node;
    SGPropertyNode_ptr epr3_node;
    
    SGPropertyNode_ptr n10_node;
    SGPropertyNode_ptr n11_node;
    SGPropertyNode_ptr n12_node;
    SGPropertyNode_ptr n13_node;
    
    SGPropertyNode_ptr n20_node;
    SGPropertyNode_ptr n21_node;
    SGPropertyNode_ptr n22_node;
    SGPropertyNode_ptr n23_node;
    
    SGPropertyNode_ptr oil_temp0;
    SGPropertyNode_ptr oil_temp1;
    SGPropertyNode_ptr oil_temp2;
    SGPropertyNode_ptr oil_temp3;
   
    // Fuel System
    SGPropertyNode_ptr tank0_node;
    SGPropertyNode_ptr tank1_node;
    SGPropertyNode_ptr tank2_node;
    SGPropertyNode_ptr tank3_node;
    SGPropertyNode_ptr tank4_node;
    SGPropertyNode_ptr tank5_node;
    SGPropertyNode_ptr tank6_node;
    SGPropertyNode_ptr tank7_node;
    // Boost pumps; Center tank has only override pumps; boosts are in the
    // four main wing tanks 1->4
//    SGPropertyNode_ptr boost1_node;
//    SGPropertyNode_ptr boost2_node;
//    SGPropertyNode_ptr boost3_node;
//    SGPropertyNode_ptr boost4_node;
//    SGPropertyNode_ptr boost5_node;
//    SGPropertyNode_ptr boost6_node;
//    SGPropertyNode_ptr boost7_node;
//    SGPropertyNode_ptr boost8_node;
    // Override pumps
//    SGPropertyNode_ptr ovride0_node;
//    SGPropertyNode_ptr ovride1_node;
//    SGPropertyNode_ptr ovride2_node;
//    SGPropertyNode_ptr ovride3_node;
//    SGPropertyNode_ptr ovride4_node;
//    SGPropertyNode_ptr ovride5_node;
    // X_Feed valves
//    SGPropertyNode_ptr x_feed0_node;
//    SGPropertyNode_ptr x_feed1_node;
//    SGPropertyNode_ptr x_feed2_node;
//    SGPropertyNode_ptr x_feed3_node;
    
    // Aero numbers
    SGPropertyNode_ptr p_alphadot;
    SGPropertyNode_ptr p_betadot;

public:

    FGOpenGC();
    ~FGOpenGC();

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();

    void collect_data(ogcFGData *data );
};

#endif // _FG_OPENGC_HXX



