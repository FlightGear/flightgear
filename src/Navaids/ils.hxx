// ils.hxx -- navaid class
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


#ifndef _FG_ILS_HXX
#define _FG_ILS_HXX


#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sgstream.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( __BORLANDC__ ) || (__APPLE__)
#  include <iostream>
#else
#  include <istream.h>
#endif

SG_USING_STD(istream);


#define FG_ILS_DEFAULT_RANGE 18

class FGILS {

    char ilstype;
    char ilstypename[6];
    char aptcode[5];
    char rwyno[4];
    int  locfreq;
    char locident[5];		// official ident
    double locheading;
    double loclat;
    double loclon;
    double x, y, z;
    bool has_gs;
    double gselev;
    double gsangle;
    double gslat;
    double gslon;
    double gs_x, gs_y, gs_z;
    bool has_dme;
    double dmelat;
    double dmelon;
    double dme_x, dme_y, dme_z;
    double omlat;
    double omlon;
    double mmlat;
    double mmlon;
    double imlat;
    double imlon;

    // for failure modeling
    string trans_ident;		// transmitted ident
    bool loc_failed;		// localizer failed?
    bool gs_failed;		// glide slope failed?
    bool dme_failed;		// dme failed?

public:

    inline FGILS(void);
    inline ~FGILS(void) {}

    inline char get_ilstype() const { return ilstype; }
    inline char *get_ilstypename() { return ilstypename; }
    inline char *get_aptcode() { return aptcode; }
    inline char *get_rwyno() { return rwyno; }
    inline int get_locfreq() const { return locfreq; }
    inline char *get_locident() { return locident; }
    inline string get_trans_ident() { return trans_ident; }
    inline double get_locheading() const { return locheading; }
    inline double get_loclat() const { return loclat; }
    inline double get_loclon() const { return loclon; }
    inline double get_x() const { return x; }
    inline double get_y() const { return y; }
    inline double get_z() const { return z; }
    inline bool get_has_gs() const { return has_gs; }
    inline double get_gselev() const { return gselev; }
    inline double get_gsangle() const { return gsangle; }
    inline double get_gslat() const { return gslat; }
    inline double get_gslon() const { return gslon; }
    inline double get_gs_x() const { return gs_x; }
    inline double get_gs_y() const { return gs_y; }
    inline double get_gs_z() const { return gs_z; }
    inline bool get_has_dme() const { return has_dme; }
    inline double get_dmelat() const { return dmelat; }
    inline double get_dmelon() const { return dmelon; }
    inline double get_dme_x() const { return dme_x; }
    inline double get_dme_y() const { return dme_y; }
    inline double get_dme_z() const { return dme_z; }
    inline double get_omlat() const { return omlat; }
    inline double get_omlon() const { return omlon; }
    inline double get_mmlat() const { return mmlat; }
    inline double get_mmlon() const { return mmlon; }
    inline double get_imlat() const { return imlat; }
    inline double get_imlon() const { return imlon; }

    friend istream& operator>> ( istream&, FGILS& );
};


inline
FGILS::FGILS(void)
  : ilstype(0),
    locfreq(0),
    locheading(0.0),
    loclat(0.0),
    loclon(0.0),
    x(0.0), y(0.0), z(0.0),
    has_gs(false),
    gselev(0.0),
    gsangle(0.0),
    gslat(0.0),
    gslon(0.0),
    gs_x(0.0), gs_y(0.0), gs_z(0.0),
    has_dme(false),
    dmelat(0.0),
    dmelon(0.0),
    dme_x(0.0), dme_y(0.0), dme_z(0.0),
    omlat(0.0),
    omlon(0.0),
    mmlat(0.0),
    mmlon(0.0),
    imlat(0.0),
    imlon(0.0),
    trans_ident(""),
    loc_failed(false),
    gs_failed(false),
    dme_failed(false)
{
    ilstypename[0] = '\0';
    aptcode[0] = '\0';
    rwyno[0] = '\0';
    locident[0] = '\0';
}

    
inline istream&
operator >> ( istream& in, FGILS& i )
{
    double f;
    in >> i.ilstype;
    
    if ( i.ilstype == '[' )
          return in;

    in >> i.ilstypename >> i.aptcode >> i.rwyno 
       >> f >> i.locident >> i.locheading >> i.loclat >> i.loclon
       >> i.gselev >> i.gsangle >> i.gslat >> i.gslon
       >> i.dmelat >> i.dmelon
       >> i.omlat >> i.omlon
       >> i.mmlat >> i.mmlon
       >> i.imlat >> i.imlon;
	

    i.locfreq = (int)(f*100.0 + 0.5);

    // generate cartesian coordinates
    Point3D geod, cart;

    geod = Point3D( i.loclon * SGD_DEGREES_TO_RADIANS,
                    i.loclat * SGD_DEGREES_TO_RADIANS,
                    i.gselev * SG_FEET_TO_METER );
    cart = sgGeodToCart( geod );
    i.x = cart.x();
    i.y = cart.y();
    i.z = cart.z();

    if ( i.gslon < SG_EPSILON && i.gslat < SG_EPSILON ) {
	i.has_gs = false;
    } else {
	i.has_gs = true;

	geod = Point3D( i.gslon * SGD_DEGREES_TO_RADIANS,
                        i.gslat * SGD_DEGREES_TO_RADIANS,
                        i.gselev * SG_FEET_TO_METER );
	cart = sgGeodToCart( geod );
	i.gs_x = cart.x();
	i.gs_y = cart.y();
	i.gs_z = cart.z();
	// cout << "gs = " << cart << endl;
    }

    if ( i.dmelon < SG_EPSILON && i.dmelat < SG_EPSILON ) {
	i.has_dme = false;
    } else {
	i.has_dme = true;

	geod = Point3D( i.dmelon * SGD_DEGREES_TO_RADIANS,
                        i.dmelat * SGD_DEGREES_TO_RADIANS,
                        i.gselev * SG_FEET_TO_METER );
	cart = sgGeodToCart( geod );
	i.dme_x = cart.x();
	i.dme_y = cart.y();
	i.dme_z = cart.z();
	// cout << "dme = " << cart << endl;
    }

    i.trans_ident = "I";
    i.trans_ident += i.locident;
    i.loc_failed = i.gs_failed = i.dme_failed = false;
   
    // return in >> skipeol;
    return in;
}


#endif // _FG_ILS_HXX
