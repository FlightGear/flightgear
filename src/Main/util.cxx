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


std::string
fgUnescape (const char *s)
{
    std::string r;
    while (*s) {
        if (*s != '\\') {
            r += *s++;
            continue;
        }
        if (!*++s)
            break;
        if (*s == '\\') {
            r += '\\';
        } else if (*s == 'n') {
            r += '\n';
        } else if (*s == 'r') {
            r += '\r';
        } else if (*s == 't') {
            r += '\t';
        } else if (*s == 'v') {
            r += '\v';
        } else if (*s == 'f') {
            r += '\f';
        } else if (*s == 'a') {
            r += '\a';
        } else if (*s == 'b') {
            r += '\b';
        } else if (*s == 'x') {
            if (!*++s)
                break;
            int v = 0;
            for (int i = 0; i < 2 && isxdigit(*s); i++, s++)
                v = v * 16 + (isdigit(*s) ? *s - '0' : 10 + tolower(*s) - 'a');
            r += v;
            continue;

        } else if (*s >= '0' && *s <= '7') {
            int v = *s++ - '0';
            for (int i = 0; i < 3 && *s >= '0' && *s <= '7'; i++, s++)
                v = v * 8 + *s - '0';
            r += v;
            continue;

        } else {
            r += *s;
        }
        s++;
    }
    return r;
}


// Write out path to validation node and read it back in. A Nasal
// listener is supposed to replace the path with a validated version
// or an empty string otherwise.
const char *fgValidatePath (const char *str, bool write)
{
    static SGPropertyNode_ptr r, w;
    if (!r) {
        r = fgGetNode("/sim/paths/validate/read", true);
        r->setAttribute(SGPropertyNode::READ, true);
        r->setAttribute(SGPropertyNode::WRITE, true);

        w = fgGetNode("/sim/paths/validate/write", true);
        w->setAttribute(SGPropertyNode::READ, true);
        w->setAttribute(SGPropertyNode::WRITE, true);
    }
    SGPropertyNode *prop = write ? w : r;
    prop->setStringValue(str);
    const char *result = prop->getStringValue();
    return result[0] ? result : 0;
}

// end of util.cxx

