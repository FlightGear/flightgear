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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <cmath>

#include <cstdlib>

#include <vector>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGLimits.hxx>
#include <simgear/math/SGMisc.hxx>

#include <GUI/MessageBox.hxx>
#include "fg_io.hxx"
#include "fg_props.hxx"
#include "globals.hxx"
#include "util.hxx"

#ifdef OSG_LIBRARY_STATIC
#include "osgDB/Registry"
#endif

using std::vector;

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

namespace {
double innerLowPassPeriodic(double current, double target, double timeratio)
{
    auto currentSigned = SGMiscd::normalizePeriodic(-180.0, 180.0, current);
    auto targetSigned = SGMiscd::normalizePeriodic(-180.0, 180.0, target);
    if (fabs(currentSigned - targetSigned) > 180.0) {
        // value are 'opposite ends', bring them close together so
        // fgGetLowPass doesn't see the discontinuity
        // we offset target by 360 in the 'same direction' as current
        // via copysign
        targetSigned += copysign(360, currentSigned);
    }

    return fgGetLowPass(currentSigned, targetSigned, timeratio);
}
} // namespace

double
flightgear::lowPassPeriodicDegreesPositive(double current, double target, double timeratio)
{
    return SGMiscd::normalizePeriodic(0.0, 360.0, innerLowPassPeriodic(current, target, timeratio));
}

double
flightgear::lowPassPeriodicDegreesSigned(double current, double target, double timeratio)
{
    return SGMiscd::normalizePeriodic(-180.0, 180.0, innerLowPassPeriodic(current, target, timeratio));
}

double
flightgear::filterExponential(double current, double target, double timeratio)
{
   double factor = 2.0 / (timeratio + 1.0);
   return (target - current) * factor + current;
}


/**
 * Allowed paths here are absolute, and may contain _one_ *,
 * which matches any string
 */
void fgInitAllowedPaths()
{
    if(SGPath("ygjmyfvhhnvdoesnotexist").realpath().utf8Str() == "ygjmyfvhhnvdoesnotexist"){
        // Abort in case this is used with older versions of realpath()
        // that don't normalize non-existent files, as that would be a security
        // hole.
        flightgear::fatalMessageBoxThenExit(
          "Nasal initialization error",
          "Version mismatch - please update simgear");
    }
    SGPath::clearListOfAllowedPaths(false); // clear list of read-allowed paths
    SGPath::clearListOfAllowedPaths(true);  // clear list of write-allowed paths

    PathList read_paths = globals->get_extra_read_allowed_paths();
    read_paths.push_back(globals->get_fg_root());
    read_paths.push_back(globals->get_fg_home());

    for (const auto& path: read_paths) {
        // If we get the initialization order wrong, better to have an
        // obvious error than a can-read-everything security hole...
        if (path.isNull()) {
            flightgear::fatalMessageBoxThenExit(
                "Nasal initialization error",
                "Empty string in FG_ROOT, FG_HOME, FG_AIRCRAFT, FG_SCENERY or "
                "--allow-nasal-read, or fgInitAllowedPaths() called too early");
        }
        SGPath::addAllowedPathPattern(path.realpath().utf8Str() + "/*",
                                      false /* write */);
        SGPath::addAllowedPathPattern(path.realpath().utf8Str(), false);
    }

    const std::string fg_home = globals->get_fg_home().realpath().utf8Str();
    SGPath::addAllowedPathPattern(fg_home + "/*.sav", true /* write */);
    SGPath::addAllowedPathPattern(fg_home + "/*.log", true);
    SGPath::addAllowedPathPattern(fg_home + "/cache/*", true);
    SGPath::addAllowedPathPattern(fg_home + "/Export/*", true);
    SGPath::addAllowedPathPattern(fg_home + "/state/*.xml", true);
    SGPath::addAllowedPathPattern(fg_home + "/aircraft-data/*.xml", true);
    SGPath::addAllowedPathPattern(fg_home + "/Wildfire/*.xml", true);
    SGPath::addAllowedPathPattern(fg_home + "/runtime-jetways/*.xml", true);
    SGPath::addAllowedPathPattern(fg_home + "/Input/Joysticks/*.xml", true);

    // Check that it works
    const std::string homePath = globals->get_fg_home().utf8Str();
    if (! SGPath(homePath + "/../no.log").validate(true).isNull() ||
        ! SGPath(homePath + "/no.logt").validate(true).isNull() ||
        ! SGPath(homePath + "/nolog").validate(true).isNull() ||
        ! SGPath(homePath + "no.log").validate(true).isNull() ||
        ! SGPath(homePath + "\\..\\no.log").validate(false).isNull() ||
        SGPath(homePath + "/aircraft-data/yes..xml").validate(true).isNull() ||
        SGPath(homePath + "/.\\yes.bmp").validate(false).isNull()) {
            flightgear::fatalMessageBoxThenExit(
              "Nasal initialization error",
              "The FG_HOME directory must not be inside any of the FG_ROOT, "
              "FG_AIRCRAFT, FG_SCENERY or --allow-nasal-read directories",
              "(check that you have not accidentally included an extra ':', "
              "as an empty part means the current directory)");
    }
}

std::string generateAuthorsText(SGPropertyNode* authors)
{
    std::string result;
    for (auto a : authors->getChildren("author")) {
        const std::string name = a->getStringValue("name");
        if (name.empty())
            continue;

        if (!result.empty())
            result += ", ";
        result += a->getStringValue("name");
    }
    return result;
}

std::string flightgear::getAircraftAuthorsText()
{
    const auto authorsNode = fgGetNode("sim/authors");
    if (authorsNode) {
        // we have structured authors data
        return generateAuthorsText(authorsNode);
    }

    // if we hit this point, there is no strucutred authors data
    return fgGetString("/sim/author");
}

// end of util.cxx
