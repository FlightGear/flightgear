// tmp.cxx -- stuff I don't know what to do with at the moment
//
// Written by Curtis Olson, started July 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/misc/sg_path.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

#include <FDM/flight.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "light.hxx"
#include "moonpos.hxx"
#include "sunpos.hxx"
#include "tmp.hxx"


// periodic time updater wrapper
void fgUpdateLocalTime() {
    static const SGPropertyNode *longitude
	= fgGetNode("/position/longitude-deg");
    static const SGPropertyNode *latitude
	= fgGetNode("/position/latitude-deg");

    SGPath zone( globals->get_fg_root() );
    zone.append( "Timezone" );

    SG_LOG(SG_GENERAL, SG_INFO, "updateLocal("
	   << longitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS
	   << ", "
	   << latitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS
	   << ", " << zone.str() << ")");
    globals->get_time_params()->updateLocal( longitude->getDoubleValue()
					       * SGD_DEGREES_TO_RADIANS, 
					     latitude->getDoubleValue()
					       * SGD_DEGREES_TO_RADIANS,
					     zone.str() );
}

