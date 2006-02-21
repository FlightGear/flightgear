// util.cxx - general-purpose utility functions.
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#include <simgear/compiler.h>

#include <math.h>

#include <vector>
SG_USING_STD(vector);

#include <simgear/debug/logstream.hxx>

#include "fg_io.hxx"
#include "fg_props.hxx"
#include "globals.hxx"
#include "util.hxx"


void
fgDefaultWeatherValue (const char * propname, double value)
{
    unsigned int i;

    SGPropertyNode * branch = fgGetNode("/environment/config/boundary", true);
    vector<SGPropertyNode_ptr> entries = branch->getChildren("entry");
    for (i = 0; i < entries.size(); i++) {
        entries[i]->setDoubleValue(propname, value);
    }

    branch = fgGetNode("/environment/config/aloft", true);
    entries = branch->getChildren("entry");
    for (i = 0; i < entries.size(); i++) {
        entries[i]->setDoubleValue(propname, value);
    }
}


void
fgSetupWind (double min_hdg, double max_hdg, double speed, double gust)
{
                                // Initialize to a reasonable state
  fgDefaultWeatherValue("wind-from-heading-deg", min_hdg);
  fgDefaultWeatherValue("wind-speed-kt", speed);

  SG_LOG(SG_GENERAL, SG_INFO, "WIND: " << min_hdg << '@' <<
         speed << " knots" << endl);

                                // Now, add some variety to the layers
  min_hdg += 10;
  if (min_hdg > 360)
      min_hdg -= 360;
  speed *= 1.1;
  fgSetDouble("/environment/config/boundary/entry[1]/wind-from-heading-deg",
              min_hdg);
  fgSetDouble("/environment/config/boundary/entry[1]/wind-speed-kt",
              speed);

  min_hdg += 20;
  if (min_hdg > 360)
      min_hdg -= 360;
  speed *= 1.1;
  fgSetDouble("/environment/config/aloft/entry[0]/wind-from-heading-deg",
              min_hdg);
  fgSetDouble("/environment/config/aloft/entry[0]/wind-speed-kt",
              speed);

  min_hdg += 10;
  if (min_hdg > 360)
      min_hdg -= 360;
  speed *= 1.1;
  fgSetDouble("/environment/config/aloft/entry[1]/wind-from-heading-deg",
              min_hdg);
  fgSetDouble("/environment/config/aloft/entry[1]/wind-speed-kt",
              speed);

  min_hdg += 10;
  if (min_hdg > 360)
      min_hdg -= 360;
  speed *= 1.1;
  fgSetDouble("/environment/config/aloft/entry[2]/wind-from-heading-deg",
              min_hdg);
  fgSetDouble("/environment/config/aloft/entry[2]/wind-speed-kt",
              speed);
}


void
fgExit (int status)
{
    SG_LOG(SG_GENERAL, SG_INFO, "Exiting FlightGear with status " << status);

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

