// approach.hxx -- Approach class
//
// Written by Alexander Kappes, started March 2002.
//
// Copyright (C) 2002  Alexander Kappes
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


#ifndef _FG_APPROACH_HXX
#define _FG_APPROACH_HXX

#include <stdio.h>

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <Main/fg_props.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#include <iomanip>
#elif defined( SG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#elif defined( __BORLANDC__ ) || (__APPLE__)
#  include <iostream>
#else
#  include <istream.h>
#include <iomanip.h>
#endif

#if ! defined( SG_HAVE_NATIVE_SGI_COMPILERS )
SG_USING_STD(istream);
#endif

SG_USING_STD(string);

#include "ATC.hxx"

//DCL - a complete guess for now.
#define FG_APPROACH_DEFAULT_RANGE 100

// Contains all information about a plane that the approach control needs
const int max_planes = 20;  // max number of planes on the stack
const int max_wp = 10;      // max number of waypoints for approach phase
struct PlaneApp {

  // variables for plane if it's on the radar
  string ident;          // indentification of plane
  double lon;            // longitude in degrees
  double lat;            // latitude in degrees
  double alt;            // Altitute above sea level in feet
  double hdg;            // heading of plane in degrees
  double dist;           // distance to airport in miles
  double brg;            // bearing relative to airport in degrees
  double spd;            // speed above ground
  int    contact;        // contact with approach established?
                         // 0 = no contact yet
                         // 1 = in contact
                         // 2 = handed off to tower

  // additional variables if contact has been established
  int    wpn;                 // number of waypoints
  double wpts[max_wp][6];     // assigned waypoints for approach phase 
                              // first wp in list is airport
                              // last waypoint point at which contact was established
                              // second index: 0 = bearing to airport
                              // second index: 1 = distance to aiport
                              // second index: 2 = alt 
                              // second index: 3 = ETA
                              // second index: 4 = heading to next waypoint
                              // second index: 5 = distance to next waypoint

  double dnwp;           // distance to next waypoint
  double dcc;            // closest distance to current assigned course
  double dnc;            // closest distance to course from next to next to next wp
  double aalt;           // assigned alt at next waypoint
  double ahdg;           // assigned heading
  bool   on_crs;         // is the plane on course?
  double tlm;            // time when last message was sent

  PlaneApp(void);
};


class FGApproach : public FGATC {

  char     type;
  double   lon, lat;
  double   elev;
  double   x, y, z;
  int      freq;
  int      bucket;
  double   range;

  string active_runway;         
  double active_rw_hdg;

  bool     display;		// Flag to indicate whether we should be outputting to the display.
  bool     displaying;	        // Flag to indicate whether we are outputting to the display.
  string   ident;		// Code of the airport its at.
  string   name;		// Name transmitted in the broadcast.
  int      num_planes;          // number of planes on the stack
  PlaneApp planes[max_planes];  // Array of planes
  string   transmission;
  bool     first;

  SGPropertyNode *comm1_node;
  SGPropertyNode *comm2_node;

  // for failure modeling
  string trans_ident;	// transmitted ident
  bool approach_failed;	// approach failed?

public:

  FGApproach(void);
  ~FGApproach(void);

  void Init();

  void Update();

  // Add new plane to stack if not already registered 
  // Input:  pid - id of plane (name) 
  // Output: "true" if added; "false" if already existend
  void AddPlane(string pid);

  // Remove plane from stack if out of range
  int RemovePlane();
  
  //Indicate that this instance should be outputting to the ATC display
  inline void SetDisplay(void) {display = true;}
  
  //Indicate that this instance should not be outputting to the ATC display
  inline void SetNoDisplay(void) {display = false;}
  
  inline char get_type() const { return type; }
  inline double get_lon() const { return lon; }
  inline double get_lat() const { return lat; }
  inline double get_elev() const { return elev; }
  inline double get_x() const { return x; }
  inline double get_y() const { return y; }
  inline double get_z() const { return z; }
  inline double get_bucket() const { return bucket; }
  inline int get_freq() const { return freq; }
  inline double get_range() const { return range; }
  inline int get_pnum() const { return num_planes; }
  inline const char* GetIdent() { return ident.c_str(); }
  inline string get_trans_ident() { return trans_ident; }
  inline string get_name() { return name; }
  inline atc_type GetType() { return APPROACH; }
  
private:

  void update_plane_dat();

  void get_active_runway();

// ========================================================================
// get heading and distance between two points; point2 ---> point1
// input: point1 = heading in degrees, distance
// input: point2 = heading in degrees, distance
// ouput: course in degrees, distance
// ========================================================================
  void calc_hd_course_dist(const double &h1, const double &d1,
			   const double &h2, const double &d2,
			   double *course, double *dist);

// ========================================================================
// closest distance between a point and a straigt line in 2 dim.
// the input variables are given in (heading, distance) 
// relative to a common point
// input:  point        = heading in degrees, distance
// input:  straigt line = anker vector (heading in degrees, distance), 
//                        heading of direction vector
// output: distance
// ========================================================================
  double calc_psl_dist(const double &h1, const double &d1,
		       const double &h2, const double &d2,
		       const double &h3);

  // Pointers to current users position
  SGPropertyNode *lon_node;
  SGPropertyNode *lat_node;
  SGPropertyNode *elev_node;
  
  //Update the transmission string
  void UpdateTransmission(void);
  
  friend istream& operator>> ( istream&, FGApproach& );
};


inline istream&
operator >> ( istream& in, FGApproach& a )
{
    double f;
    char ch;

    static bool first_time = true;
    static double julian_date = 0;
    static const double MJD0    = 2415020.0;
    if ( first_time ) {
	julian_date = sgTimeCurrentMJD(0, 0) + MJD0;
	first_time = false;
    }

    in >> a.type;
    
    if ( a.type == '[' )
      return in >> skipeol;

    in >> a.lat >> a.lon >> a.elev >> f >> a.range 
       >> a.ident;

    a.name = "";
    in >> ch;
    a.name += ch;
    while(1) {
	//in >> noskipws
	in.unsetf(ios::skipws);
	in >> ch;
	a.name += ch;
	if((ch == '"') || (ch == 0x0A)) {
	    break;
	}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
    }
    in.setf(ios::skipws);
    //cout << "approach.name = " << a.name << '\n';

    a.freq = (int)(f*100.0 + 0.5);

    // generate cartesian coordinates
    Point3D geod( a.lon * SGD_DEGREES_TO_RADIANS , a.lat * SGD_DEGREES_TO_RADIANS, 
		  a.elev * SG_FEET_TO_METER );
    Point3D cart = sgGeodToCart( geod );
    a.x = cart.x();
    a.y = cart.y();
    a.z = cart.z();

    // get bucket number
    SGBucket buck(a.lon, a.lat);
    a.bucket = buck.gen_index();

    a.trans_ident = a.ident;
    a.approach_failed = false;

    return in >> skipeol;
}

#endif // _FG_APPROACH_HXX
