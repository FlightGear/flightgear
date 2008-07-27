// ray.hxx -- "RayWoodworth" chair protocol via CIO-DDA06/Jr driver
//
// Written by Alexander Perry, May 2000
//
// Copyright (C) 2000 Alexander Perry, alex.perry@ieee.org
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


#ifndef _FG_RAY_HXX
#define _FG_RAY_HXX


#include <stdio.h>		// FILE

#include <simgear/compiler.h>

#include <string>

#include "protocol.hxx"

using std::string;


class FGRAY : public FGProtocol {

    char buf[ 20 ];
    int length;

    double chair_heading;
    double chair_rising;
    double chair_height;
    double chair_vertical[2];
    FILE *chair_FILE;

public:

    FGRAY();
    ~FGRAY();

    bool gen_message();
    bool parse_message();
 
    // process work for this port
    bool process();
};


#endif // _FG_RAY_HXX
