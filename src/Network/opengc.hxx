
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
    SGPropertyNode *mag_var_node;
    
    // Position
    SGPropertyNode *p_latitude;
    SGPropertyNode *p_longitude;
    SGPropertyNode *p_alt_node;
    SGPropertyNode *p_altitude;
    SGPropertyNode *p_altitude_agl;
    
    SGPropertyNode *egt0_node;
    SGPropertyNode *egt1_node;
    SGPropertyNode *egt2_node;
    SGPropertyNode *egt3_node;
        
    // Control surfaces
    SGPropertyNode *p_left_aileron;
    SGPropertyNode *p_right_aileron;
    SGPropertyNode *p_elevator;
    SGPropertyNode *p_elevator_trim;
    SGPropertyNode *p_rudder;
    SGPropertyNode *p_flaps;
    SGPropertyNode *p_flaps_cmd;
    
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



