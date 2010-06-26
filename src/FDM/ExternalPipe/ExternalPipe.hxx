// ExternalPipe.hxx -- a "pipe" interface to an external flight dynamics model
//
// Written by Curtis Olson, started March 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$

#ifndef _EXTERNAL_PIPE_HXX
#define _EXTERNAL_PIPE_HXX

#include <stdio.h>              // FILE*, fopen(), fread(), fwrite(), et. al.

#include <simgear/timing/timestamp.hxx> // fine grained timing measurements

#include <Network/net_ctrls.hxx>
#include <Network/net_fdm.hxx>
#include <FDM/flight.hxx>


class FGExternalPipe: public FGInterface {

private:

    bool valid;

    string fifo_name_1;
    string fifo_name_2;
    FILE *pd1;
    FILE *pd2;
    string _protocol;

    FGNetCtrls ctrls;
    FGNetFDM fdm;
    char *buf;

    double last_weight;
    double last_cg_offset;

    vector <string> property_names;
    vector <SGPropertyNode_ptr> nodes;

    // Protocol specific init routines
    void init_binary();
    void init_property();

    // Protocol specific update routines
    void update_binary( double dt );
    void update_property( double dt );

    void process_set_command( const string_list &tokens );
public:

    // Constructor
    FGExternalPipe( double dt, string fifo_name, string protocol );

    // Destructor
    ~FGExternalPipe();

    // Reset flight params to a specific position
    void init();

    // update the fdm
    void update( double dt );

};


#endif // _EXTERNAL_PIPE_HXX
