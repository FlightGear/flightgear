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

static string_list read_allowed_paths;
static string_list write_allowed_paths;

/**
 * Allowed paths here are absolute, and may contain _one_ *,
 * which matches any string
 */
void fgInitAllowedPaths()
{
    if(SGPath("ygjmyfvhhnvdoesnotexist").realpath().utf8Str() == "ygjmyfvhhnvdoesnotexist"){
        // Forbid using this version of fgValidatePath() with older
        // (not normalizing non-existent files) versions of realpath(),
        // as that would be a security hole
        flightgear::fatalMessageBoxThenExit(
          "Nasal initialization error",
          "Version mismatch - please update simgear");
    }
    read_allowed_paths.clear();
    write_allowed_paths.clear();

    PathList read_paths = globals->get_extra_read_allowed_paths();
    read_paths.push_back(globals->get_fg_root());
    read_paths.push_back(globals->get_fg_home());

    for( PathList::const_iterator it = read_paths.begin(); it != read_paths.end(); ++it )
      {
        // if we get the initialization order wrong, better to have an
        // obvious error than a can-read-everything security hole...
          if (it->isNull()) {
              flightgear::fatalMessageBoxThenExit(
                "Nasal initialization error",
                "Empty string in FG_ROOT, FG_HOME, FG_AIRCRAFT, FG_SCENERY or "
                "--allow-nasal-read, or fgInitAllowedPaths() called too early");
          }
          read_allowed_paths.push_back(it->realpath().utf8Str() + "/*");
          read_allowed_paths.push_back(it->realpath().utf8Str());
      }
    
    std::string fg_home = globals->get_fg_home().realpath().utf8Str();
    write_allowed_paths.push_back(fg_home + "/*.sav");
    write_allowed_paths.push_back(fg_home + "/*.log");
    write_allowed_paths.push_back(fg_home + "/cache/*");
    write_allowed_paths.push_back(fg_home + "/Export/*");
    write_allowed_paths.push_back(fg_home + "/state/*.xml");
    write_allowed_paths.push_back(fg_home + "/aircraft-data/*.xml");
    write_allowed_paths.push_back(fg_home + "/Wildfire/*.xml");
    write_allowed_paths.push_back(fg_home + "/runtime-jetway/*.xml");
    write_allowed_paths.push_back(fg_home + "/Input/Joysticks/*.xml");
    
    // Check that it works
    std::string homePath = globals->get_fg_home().utf8Str();
    if(!fgValidatePath(homePath + "/../no.log",true).isNull() ||
        !fgValidatePath(homePath + "/no.logt",true).isNull() ||
        !fgValidatePath(homePath + "/nolog",true).isNull() ||
        !fgValidatePath(homePath + "no.log",true).isNull() ||
        !fgValidatePath(homePath + "\\..\\no.log",false).isNull() ||
        fgValidatePath(homePath + "/aircraft-data/yes..xml",true).isNull() ||
        fgValidatePath(homePath + "/.\\yes.bmp",false).isNull()) {
            flightgear::fatalMessageBoxThenExit(
              "Nasal initialization error",
              "The FG_HOME directory must not be inside any of the FG_ROOT, "
              "FG_AIRCRAFT, FG_SCENERY or --allow-nasal-read directories",
              "(check that you have not accidentally included an extra ':', "
              "as an empty part means the current directory)");
    }
}

/**
 * Check whether Nasal is allowed to access a path
 * Warning: because this always (not just on Windows) treats both \ and /
 * as path separators, and accepts relative paths (check-to-use race if
 * the current directory changes),
 * always use the returned path not the original one
 */
SGPath fgValidatePath (const SGPath& path, bool write)
{
    // Normalize the path (prevents ../../.. or symlink trickery)
    std::string normed_path = path.realpath().utf8Str();
    
    const string_list& allowed_paths(write ? write_allowed_paths : read_allowed_paths);
    size_t star_pos;
    
    // Check against each allowed pattern
    for( string_list::const_iterator it = allowed_paths.begin();
                                     it != allowed_paths.end();
                                   ++it )
    {
        star_pos = it->find('*');
        if (star_pos == std::string::npos) {
            if (!(it->compare(normed_path))) {
                return SGPath::fromUtf8(normed_path);
            }
        } else {
            if ((it->size()-1 <= normed_path.size()) /* long enough to be a potential match */
                && !(it->substr(0,star_pos)
                    .compare(normed_path.substr(0,star_pos))) /* before-star parts match */
                && !(it->substr(star_pos+1,it->size()-star_pos-1)
                    .compare(normed_path.substr(star_pos+1+normed_path.size()-it->size(),
                      it->size()-star_pos-1))) /* after-star parts match */) {
                return SGPath::fromUtf8(normed_path);
            }
        }
    }
    // no match found
    return SGPath();
}

// end of util.cxx

