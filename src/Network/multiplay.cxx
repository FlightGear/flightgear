// multiplay.cxx -- protocol object for multiplay in Flightgear
//
// Written by Diarmuid Tyson, started February 2003.
// diarmuid.tyson@airservicesaustralia.com
//
// Copyright (C) 2003  Airservices Australia
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


#include <simgear/compiler.h>

#include STL_STRING

#include <iostream>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/model/placement.hxx>

#include <Scenery/scenery.hxx>

#include "multiplay.hxx"

SG_USING_STD(string);


// These constants are provided so that the ident command can list file versions.
const char sFG_MULTIPLAY_BID[] = "$Id$";
const char sFG_MULTIPLAY_HID[] = FG_MULTIPLAY_HID;


/******************************************************************
* Name: FGMultiplay
* Description: Constructor.  Initialises the protocol and stores
* host and port information.
******************************************************************/
FGMultiplay::FGMultiplay (const string &dir, const int rate, const string &host, const int port) {

  set_hz(rate);

  set_direction(dir);

  if (get_direction() == SG_IO_IN) {

    fgSetInt("/sim/multiplay/rxport", port);
    fgSetString("/sim/multiplay/rxhost", host.c_str());

  } else if (get_direction() == SG_IO_OUT) {

    fgSetInt("/sim/multiplay/txport", port);
    fgSetString("/sim/multiplay/txhost", host.c_str());

  }

}


/******************************************************************
* Name: ~FGMultiplay
* Description: Destructor.
******************************************************************/
FGMultiplay::~FGMultiplay () {
}


/******************************************************************
* Name: open
* Description: Enables the protocol.
******************************************************************/
bool FGMultiplay::open() {

    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
		<< "is already in use, ignoring" );
	return false;
    }

    set_enabled(true);

    return is_enabled();
}


/******************************************************************
* Name: process
* Description: Prompts the multiplayer mgr to either send
* or receive data over the network
******************************************************************/
bool FGMultiplay::process() {

  if (get_direction() == SG_IO_IN) {

    globals->get_multiplayer_rx_mgr()->ProcessData();

  } else if (get_direction() == SG_IO_OUT) {

    sgMat4 posTrans;
    sgCopyMat4(posTrans, globals->get_aircraft_model()->get3DModel()->get_POS());
    Point3D center = globals->get_scenery()->get_center();
    sgdVec3 PlayerPosition;
    sgdSetVec3(PlayerPosition, posTrans[3][0] + center[0],
               posTrans[3][1] + center[1], posTrans[3][2] + center[2]);
    sgQuat PlayerOrientation;
    sgMatrixToQuat(PlayerOrientation, posTrans);

    globals->get_multiplayer_tx_mgr()->SendMyPosition(PlayerOrientation, PlayerPosition);

  }

    return true;
}


/******************************************************************
* Name: close
* Description:  Closes the multiplayer mgrs to stop any further
* network processing
******************************************************************/
bool FGMultiplay::close() {

  if (get_direction() == SG_IO_IN) {

    globals->get_multiplayer_rx_mgr()->Close();

  } else if (get_direction() == SG_IO_OUT) {

    globals->get_multiplayer_tx_mgr()->Close();

  }

    return true;
}

