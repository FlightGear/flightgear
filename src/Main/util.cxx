// util.cxx - general-purpose utility functions.
// Copyright (C) 2002  Curtis L. Olson  - curt@me.umn.edu
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


#include <math.h>

#include <simgear/debug/logstream.hxx>

#include "fg_io.hxx"
#include "fg_props.hxx"
#include "globals.hxx"
#include "util.hxx"

#if defined(FG_NETWORK_OLK)
#include <NetworkOLK/network.h>
#endif


void
fgExit (int status)
{
    SG_LOG(SG_GENERAL, SG_INFO, "Exiting FlightGear with status " << status);

#if defined(FG_NETWORK_OLK)
    if (fgGetBool("/sim/networking/network-olk"))
	fgd_send_com("8", FGFS_host);
#endif

    globals->get_io()->shutdown_all();
    exit(status);
}


// Originally written by Alex Perry.
double
fgGetLowPass (double current, double target, double timeratio)
{
    if ( timeratio < 0.0 ) {
	if ( timeratio < -1.0 ) {
                                // time went backwards; kill the filter
                current = target;
        } else {
                                // ignore mildly negative time
        }
    } else if ( timeratio < 0.2 ) {
                                // Normal mode of operation; fast
                                // approximation to exp(-timeratio)
        current = current * (1.0 - timeratio) + target * timeratio;
    } else if ( timeratio > 5.0 ) {
                                // Huge time step; assume filter has settled
        current = target;
    } else {
                                // Moderate time step; non linear response
        double keep = exp(-timeratio);
        current = current * keep + target * (1.0 - keep);
    }

    return current;
}

// end of util.cxx

