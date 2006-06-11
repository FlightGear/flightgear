// ACMS.hxx -- interface to the AIAircraft FDM
//
// Written by Erik Hofman, started October 2004
//
// Copyright (C) 2004 Erik Hofman <erik@ehofman.com>
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


#ifndef _ACMS_HXX
#define _ACMS_HXX

#include <simgear/props/props.hxx>

#include <FDM/flight.hxx>


class FGACMS: public FGInterface
{
public:
    FGACMS( double dt );
    ~FGACMS();

    // reset flight params to a specific position 
    void init();

    // update position based on properties
    void update( double dt );

private:

    SGPropertyNode_ptr _alt, _speed, _climb_rate;
    SGPropertyNode_ptr _pitch, _roll, _heading;
    SGPropertyNode_ptr _acc_lat, _acc_lon, _acc_down;
    SGPropertyNode_ptr _temp, _wow;
};


#endif // _ACMS_HXX
