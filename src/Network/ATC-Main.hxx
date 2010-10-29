// ATC-Main.hxx -- FGFS interface to ATC 610x hardware
//
// Written by Curtis Olson, started January 2002.
//
// Copyright (C) 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_ATC_MAIN_HXX
#define _FG_ATC_MAIN_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/misc/sg_path.hxx>

#include <Main/fg_props.hxx>

#include "protocol.hxx"

#include "ATC-Inputs.hxx"
#include "ATC-Outputs.hxx"


class FGATCMain : public FGProtocol {

    FGATCInput *input0;         // board0 input interface class
    FGATCInput *input1;         // board1 input interface class
    FGATCOutput *output0;       // board0 output interface class
    FGATCOutput *output1;       // board1 output interface class

    SGPath input0_path;
    SGPath input1_path;
    SGPath output0_path;
    SGPath output1_path;

    int board;

    int lock0_fd;
    int lock1_fd;

public:

    FGATCMain() :
        input0(NULL),
        input1(NULL),
        output0(NULL),
        output1(NULL),
        input0_path(""),
        input1_path(""),
        output0_path(""),
        output1_path("")
    { }

    ~FGATCMain() {
        delete input0;
        delete input1;
        delete output0;
        delete output1;
    }

    // Open and initialize ATC 610x hardware
    bool open();

    void init_config();

    bool process();

    bool close();

    inline void set_path_names( const SGPath &in0, const SGPath &in1,
                                const SGPath &out0, const SGPath &out1 )
    {
        input0_path = in0;
        input1_path = in1;
        output0_path = out0;
        output1_path = out1;
    }
};


#endif // _FG_ATC_MAIN_HXX
