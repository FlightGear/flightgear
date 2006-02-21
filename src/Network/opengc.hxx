
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

#include STL_STRING

#include <FDM/flight.hxx>
#include <Main/fg_props.hxx>

#include "protocol.hxx"
#include "opengc_data.hxx"

class FGOpenGC : public FGProtocol, public FGInterface {

    ogcFGData buf;
    int length;
    
    // Environment
    SGPropertyNode *press_node;
    SGPropertyNode *temp_node;
    SGPropertyNode *wind_dir_node;
    SGPropertyNode *wind_speed_node;
    SGPropertyNode *magvar_node;
    
    // Position on the Geod
    SGPropertyNode *p_latitude;
    SGPropertyNode *p_longitude;
    SGPropertyNode *p_elev_node;
    //SGPropertyNode *p_altitude;
    SGPropertyNode *p_altitude_agl;
    
    // Orientation
    SGPropertyNode	*p_pitch;
    SGPropertyNode	*p_bank;
    SGPropertyNode	*p_heading;
    SGPropertyNode	*p_yaw;
    SGPropertyNode	*p_yaw_rate;
    
    // Flight Parameters
    SGPropertyNode	*vel_kcas;
    SGPropertyNode	*p_vvi;
    SGPropertyNode	*p_mach;
    
    // Control surfaces
    SGPropertyNode *p_left_aileron;
    SGPropertyNode *p_right_aileron;
    SGPropertyNode *p_elevator;
    SGPropertyNode *p_elevator_trim;
    SGPropertyNode *p_rudder;
    SGPropertyNode *p_flaps;
    SGPropertyNode *p_flaps_cmd;
    
    // GEAR System
    SGPropertyNode *p_park_brake;
    
    // Engines
    SGPropertyNode *egt0_node;
    SGPropertyNode *egt1_node;
    SGPropertyNode *egt2_node;
    SGPropertyNode *egt3_node;
    
    SGPropertyNode *epr0_node;
    SGPropertyNode *epr1_node;
    SGPropertyNode *epr2_node;
    SGPropertyNode *epr3_node;
    
    SGPropertyNode *n10_node;
    SGPropertyNode *n11_node;
    SGPropertyNode *n12_node;
    SGPropertyNode *n13_node;
    
    SGPropertyNode *n20_node;
    SGPropertyNode *n21_node;
    SGPropertyNode *n22_node;
    SGPropertyNode *n23_node;
    
    SGPropertyNode *oil_temp0;
    SGPropertyNode *oil_temp1;
    SGPropertyNode *oil_temp2;
    SGPropertyNode *oil_temp3;
   
    // Fuel System
    SGPropertyNode *tank0_node;
    SGPropertyNode *tank1_node;
    SGPropertyNode *tank2_node;
    SGPropertyNode *tank3_node;
    SGPropertyNode *tank4_node;
    SGPropertyNode *tank5_node;
    SGPropertyNode *tank6_node;
    SGPropertyNode *tank7_node;
    // Boost pumps; Center tank has only override pumps; boosts are in the
    // four main wing tanks 1->4
//    SGPropertyNode	*boost1_node;
//    SGPropertyNode	*boost2_node;
//    SGPropertyNode	*boost3_node;
//    SGPropertyNode	*boost4_node;
//    SGPropertyNode	*boost5_node;
//    SGPropertyNode	*boost6_node;
//    SGPropertyNode	*boost7_node;
//    SGPropertyNode	*boost8_node;
    // Override pumps
//    SGPropertyNode	*ovride0_node;
//    SGPropertyNode	*ovride1_node;
//    SGPropertyNode	*ovride2_node;
//    SGPropertyNode	*ovride3_node;
//    SGPropertyNode	*ovride4_node;
//    SGPropertyNode	*ovride5_node;
    // X_Feed valves
//    SGPropertyNode	*x_feed0_node;
//    SGPropertyNode	*x_feed1_node;
//    SGPropertyNode	*x_feed2_node;
//    SGPropertyNode	*x_feed3_node;
    
    // Aero numbers
    SGPropertyNode *p_alphadot;
    SGPropertyNode *p_betadot;

public:

    FGOpenGC();
    ~FGOpenGC();

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();

    void collect_data( const FGInterface *fdm, ogcFGData *data );
};

#endif // _FG_OPENGC_HXX



