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
#elif defined( __BORLANDC__ )
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
#include "transmission.hxx"

//DCL - a complete guess for now.
#define FG_APPROACH_DEFAULT_RANGE 100

// Contains all the information about a plane that the approach control needs
const int    max_planes = 20;  // max number of planes on the stack
const int    max_wp = 10;      // max number of waypoints for approach phase
const double max_ta = 130;     // max turning angle for plane during approach
const double tbm    = 2.0;     // min time (in sec) between two messages
const double lfl    = 10.0;    // length of final leg

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
  double turn_rate;      // standard turning rate of the plane in seconds per degree
  double desc_rate;      // standard descent rate of the plane in feets per minute
  double clmb_rate;      // standard climb rate of the plane in feets per minute

  // additional variables if contact has been established
  int    wpn;                 // number of waypoints
  double wpts[max_wp][6];     // assigned waypoints for approach phase 
                              // first wp in list is airport
                              // last waypoint point at which contact was established
                              // second index: 0 = bearing to airport
                              // second index: 1 = distance to airport
                              // second index: 2 = alt 
                              // second index: 3 = ETA
                              // second index: 4 = heading to next waypoint
                              // second index: 5 = distance to next waypoint

  double dnwp;           // distance to next waypoint
  double dcc;            // closest distance to current assigned course
  double dnc;            // closest distance to course from next to next to next wp
  double aalt;           // assigned altitude
  double ahdg;           // assigned heading
  bool   on_crs;         // is the plane on course?
  bool   wp_change;      // way point has changed
  double tlm;            // time when last message was sent
  TransCode lmc;         // code of last message
};


class FGApproach : public FGATC {

  int      bucket;

  string active_runway;         
  double active_rw_hdg;
  double active_rw_lon;
  double active_rw_lat;
  double active_rw_len;

  int      num_planes;          // number of planes on the stack
  PlaneApp planes[max_planes];  // Array of planes
  string   transmission;
  bool     first;

  SGPropertyNode *comm1_node;
  SGPropertyNode *comm2_node;

  SGPropertyNode *atcmenu_node;
  SGPropertyNode *atcopt0_node;
  SGPropertyNode *atcopt1_node;
  SGPropertyNode *atcopt2_node;
  SGPropertyNode *atcopt3_node;
  SGPropertyNode *atcopt4_node;
  SGPropertyNode *atcopt5_node;
  SGPropertyNode *atcopt6_node;
  SGPropertyNode *atcopt7_node;
  SGPropertyNode *atcopt8_node;
  SGPropertyNode *atcopt9_node;

  // for failure modeling
  string trans_ident;	// transmitted ident
  bool approach_failed;	// approach failed?

public:

  FGApproach(void);
  ~FGApproach(void);

  void Init();

  void Update(double dt);

  // Add new plane to stack if not already registered 
  // Input:  pid - id of plane (name) 
  // Output: "true" if added; "false" if already existend
  void AddPlane(const string& pid);

  // Remove plane from stack if out of range
  int RemovePlane();
  
  inline double get_bucket() const { return bucket; }
  inline int get_pnum() const { return num_planes; }
  inline const string& get_trans_ident() { return trans_ident; }
  
private:

  void calc_wp( const int &i);

  void update_plane_dat();

  void get_active_runway();

  void update_param(const int &i);

  double round_alt( bool hl, double alt );

  double angle_diff_deg( const double &a1, const double &a2);

// ========================================================================
// get point2 given starting point1 and course and distance
// input:  point1 = heading in degrees, distance
// input:  course in degrees, distance
// output: point2 = heading in degrees, distance
// ========================================================================
  void calc_cd_head_dist(const double &h1, const double &d1,
			 const double &course, const double &dist,
			 double *h2, double *d2);


// ========================================================================
// get heading and distance between two points; point2 ---> point1
// input:  point1 = heading in degrees, distance
// input:  point2 = heading in degrees, distance
// output: course in degrees, distance
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
  SGPropertyNode *hdg_node;
  SGPropertyNode *speed_node;
  SGPropertyNode *etime_node;
  
  //Update the transmission string
  void UpdateTransmission(void);
  
  friend istream& operator>> ( istream&, FGApproach& );
};

#endif // _FG_APPROACH_HXX
