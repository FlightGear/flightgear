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

#ifndef _EXTERNAL_NET_HXX
#define _EXTERNAL_NET_HXX

#include <plib/netSocket.h>

#include "flight.hxx"


class FGExternalNet: public FGInterface {

private:

    int data_port;
    int cmd_port;
    string host;

    netSocket data_client;
    netSocket data_server;

    bool valid;

public:

    // Constructor
    FGExternalNet::FGExternalNet( double dt );

    // Destructor
    FGExternalNet::~FGExternalNet();

    // Reset flight params to a specific position
    void init();

    // update the fdm
    void update( int multiloop );

};


#endif // _EXTERNAL_NET_HXX


