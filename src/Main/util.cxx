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

#include <math.h>

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

// Normalize a path
// Unlike SGPath::realpath, does not require that the file already exists,
// but does require that it be below the starting point
static std::string fgNormalizePath (const std::string& path)
{
    string_list path_parts;
    char c;
    std::string normed_path = "", this_part = "";
    
    for (int pos = 0; ; pos++) {
        c = path[pos];
        if (c == '\\') { c = '/'; }
        if ((c == '/') || (c == 0)) {
            if ((this_part == "/..") || (this_part == "..")) {
                if (path_parts.empty()) { return ""; }
                path_parts.pop_back();
            } else if ((this_part != "/.") && (this_part != "/")) {
                path_parts.push_back(this_part);
            }
            this_part = "";
        }
        if (c == 0) { break; }
        this_part = this_part + c;
    }
    for( string_list::const_iterator it = path_parts.begin();
                                     it != path_parts.end();
                                   ++it )
    {
        normed_path.append(*it);
    }
    return normed_path;
 }

static string_list read_allowed_paths;
static string_list write_allowed_paths;

// Allowed paths here are absolute, and may contain _one_ *,
// which matches any string
// FG_SCENERY is deliberately not allowed, as it would make
// /sim/terrasync/scenery-dir a security hole
void fgInitAllowedPaths()
{
    read_allowed_paths.clear();
    write_allowed_paths.clear();
    std::string fg_root = fgNormalizePath(globals->get_fg_root());
    std::string fg_home = fgNormalizePath(globals->get_fg_home());
    read_allowed_paths.push_back(fg_root + "/*");
    read_allowed_paths.push_back(fg_home + "/*");
    string_list const aircraft_paths = globals->get_aircraft_paths();
    for( string_list::const_iterator it = aircraft_paths.begin();
                                     it != aircraft_paths.end();
                                   ++it )
    {
        read_allowed_paths.push_back(fgNormalizePath(*it) + "/*");
    }

    for( string_list::const_iterator it = read_allowed_paths.begin();
                                     it != read_allowed_paths.end();
                                   ++it )
    { // if we get the initialization order wrong, better to have an
      // obvious error than a can-read-everything security hole...
        if (!(it->compare("/*"))){
            flightgear::fatalMessageBox("Nasal initialization error",
                                    "Empty string in FG_ROOT, FG_HOME or FG_AIRCRAFT",
                                    "or fgInitAllowedPaths() called too early");
            exit(-1);
        }
    }
    write_allowed_paths.push_back(fg_home + "/*.sav");
    write_allowed_paths.push_back(fg_home + "/*.log");
    write_allowed_paths.push_back(fg_home + "/cache/*");
    write_allowed_paths.push_back(fg_home + "/Export/*");
    write_allowed_paths.push_back(fg_home + "/state/*.xml");
    write_allowed_paths.push_back(fg_home + "/aircraft-data/*.xml");
    write_allowed_paths.push_back(fg_home + "/Wildfire/*.xml");
    write_allowed_paths.push_back(fg_home + "/runtime-jetways/*.xml");
    write_allowed_paths.push_back(fg_home + "/Input/Joysticks/*.xml");
    
    // Check that it works
    if(!fgValidatePath(globals->get_fg_home() + "/../no.log",true).empty() ||
        !fgValidatePath(globals->get_fg_home() + "/no.lot",true).empty() ||
        !fgValidatePath(globals->get_fg_home() + "/nolog",true).empty() ||
        !fgValidatePath(globals->get_fg_home() + "no.log",true).empty() ||
        !fgValidatePath("..\\" + globals->get_fg_home() + "/no.log",false).empty() ||
        !fgValidatePath(std::string("/tmp/no.xml"),false).empty() ||
        fgValidatePath(globals->get_fg_home() + "/./ff/../Export\\yes..gg",true).empty() ||
        fgValidatePath(globals->get_fg_home() + "/aircraft-data/yes..xml",true).empty() ||
        fgValidatePath(globals->get_fg_root() + "/./\\yes.bmp",false).empty()) {
            flightgear::fatalMessageBox("Nasal initialization error",
                                    "fgInitAllowedPaths() does not work",
                                    "");
            exit(-1);
    }
}

// Check whether Nasal is allowed to access a path
std::string fgValidatePath (const std::string& path, bool write)
{
    const string_list& allowed_paths(write ? write_allowed_paths : read_allowed_paths);
    size_t star_pos;
    
    // Normalize the path (prevents ../../.. trickery)
    std::string normed_path = fgNormalizePath(path);

    // Check against each allowed pattern
    for( string_list::const_iterator it = allowed_paths.begin();
                                     it != allowed_paths.end();
                                   ++it )
    {
        star_pos = it->find('*');
        if (star_pos == std::string::npos) {
            if (!(it->compare(normed_path))) {
                return normed_path;
            }
        } else {
            if ((it->size()-1 <= normed_path.size()) /* long enough to be a potential match */
                && !(it->substr(0,star_pos)
                    .compare(normed_path.substr(0,star_pos))) /* before-star parts match */
                && !(it->substr(star_pos+1,it->size()-star_pos-1)
                    .compare(normed_path.substr(star_pos+1+normed_path.size()-it->size(),
                      it->size()-star_pos-1))) /* after-star parts match */) {
                return normed_path;
            }
        }
    }
    // no match found
    return "";
}
std::string fgValidatePath(const SGPath& path, bool write) { return fgValidatePath(path.str(),write); }
// end of util.cxx

