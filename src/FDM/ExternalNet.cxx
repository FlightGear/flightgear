// ExternalNet.hxx -- an net interface to an external flight dynamics model
//
// Written by Curtis Olson, started November 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - curt@flightgear.org
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
// $Id$

#include <simgear/debug/logstream.hxx>

#include "ExternalNet.hxx"


FGExternalNet::FGExternalNet( double dt ) {
    set_delta_t( dt );

    valid = true;

    // client sends data
    if ( ! data_client.open( false ) ) {
	SG_LOG( SG_FLIGHT, SG_ALERT, "Error opening client data channel" );
	valid = false;
    }
    // fire and forget
    data_client.setBlocking( false );

    // server receives data
    if ( ! data_server.open( false ) ) {
	SG_LOG( SG_FLIGHT, SG_ALERT, "Error opening client server channel" );
	valid = false;
    }
    // we want to block for incoming data in order to syncronize frame
    // rates.
    data_server.setBlocking( true );
}


FGExternalNet::~FGExternalNet() {
    data_client.close();
    data_server.close();
}


// Initialize the ExternalNet flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGExternalNet::init() {
    // cout << "FGExternalNet::init()" << endl;

    // Explicitly call the superclass's
    // init method first.
    common_init();
}


// Run an iteration of the EOM.  This is a NOP here because the flight
// model values are getting filled in elsewhere (most likely from some
// external source.)
void FGExternalNet::update( int multiloop ) {
    // cout << "FGExternalNet::update()" << endl;
}
