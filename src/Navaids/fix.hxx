// fix.hxx -- fix class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_FIX_HXX
#define _FG_FIX_HXX


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/misc/sgstream.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( SG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#elif defined( __BORLANDC__ )
#  include <iostream>
#else
#  include <istream.h>
#endif

#if ! defined( SG_HAVE_NATIVE_SGI_COMPILERS )
SG_USING_STD(istream);
#endif

#include STL_STRING
SG_USING_STD(string);


class FGFix {

    string ident;
    double lon, lat;

public:

    inline FGFix(void) {}
    inline ~FGFix(void) {}

    inline string get_ident() const { return ident; }
    inline double get_lon() const { return lon; }
    inline double get_lat() const { return lat; }

    friend istream& operator>> ( istream&, FGFix& );
};


inline istream&
operator >> ( istream& in, FGFix& f )
{
    in >> f.ident >> f.lat >> f.lon;

    // cout << "id = " << f.ident << endl;

    // return in >> skipeol;
    return in;
}


#endif // _FG_FIX_HXX
