// FGApproach - a class to provide approach control at larger airports.
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

#include "approach.hxx"
#include "ATCdisplay.hxx"
#include <Airports/runways.hxx>

#include <simgear/misc/sg_path.hxx>

#ifdef FG_WEATHERCM
# include <WeatherCM/FGLocalWeatherDatabase.h>
#else
# include <Environment/environment_mgr.hxx>
# include <Environment/environment.hxx>
#endif


PlaneApp::PlaneApp()
:
  ident(""),
  lon(0.0),
  lat(0.0),
  alt(0.0),
  hdg(0.0),
  dist(0.0),
  brg(0.0),
  spd(0.0),
  contact(0),
  wpn(0),
  dnwp(-999.),
  dcc(0.0),
  dnc(0.0),
  aalt(0.0),
  ahdg(0.0),
  on_crs(true),
  tlm(0.0)
{
}

//Constructor
FGApproach::FGApproach() :
  bucket(0),
  active_runway(""),
  active_rw_hdg(0.0),
  display(false),
  displaying(false),
  num_planes(0),
  transmission(""),
  first(true),
  trans_ident(""),
  approach_failed(false)
{
  comm1_node = fgGetNode("/radios/comm[0]/frequencies/selected-mhz", true);
  comm2_node = fgGetNode("/radios/comm[1]/frequencies/selected-mhz", true);

  lon_node = fgGetNode("/position/longitude-deg", true);
  lat_node = fgGetNode("/position/latitude-deg", true);
  elev_node = fgGetNode("/position/altitude-ft", true);
}

//Destructor
FGApproach::~FGApproach(){
}

void FGApproach::Init() {
  display    = false;
}

// ============================================================================
// the main update function
// ============================================================================
void FGApproach::Update() {

  int wpn;
  double course, d;

  update_plane_dat();
  if ( active_runway == "" ) get_active_runway();
  
  for ( int i=0; i<num_planes; i++ ) {
    
    if ( planes[i].contact == 0) {
      double comm1_freq = comm1_node->getDoubleValue();
      if ( (int)(comm1_freq*100.0 + 0.5) == freq ) planes[i].contact = 1;
	  //cout << "comm1 = " << (int)(comm1_freq*100.0 + 0.5) << " freq = " << freq << '\n';
    }
    else if ( planes[i].contact == 1 ) {
      if ( planes[i].wpn == 0 ) {    // calculate initial waypoints
	wpn = planes[i].wpn;
	// airport
	planes[i].wpts[wpn][0] = active_rw_hdg;
	planes[i].wpts[wpn][1] = 0.0;
	planes[i].wpts[wpn][2] = elev;
	planes[i].wpts[wpn][4] = 0.0;
	planes[i].wpts[wpn][5] = 0.0;
	wpn += 1;

	planes[i].wpts[wpn][0] = active_rw_hdg + 180.0;
	if ( planes[i].wpts[wpn][0] > 360.0 ) planes[i].wpts[wpn][0] -= 360.0;
	planes[i].wpts[wpn][1] = 5;
	planes[i].wpts[wpn][2] = elev + 1000.0;
	calc_hd_course_dist(planes[i].wpts[wpn][0],   planes[i].wpts[wpn][1],
			    planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1],
			    &course, &d);
	planes[i].wpts[wpn][4] = course;
	planes[i].wpts[wpn][5] = d;
	wpn += 1;
	
	planes[i].wpts[wpn][0] = planes[i].brg;
	planes[i].wpts[wpn][1] = planes[i].dist;
	planes[i].wpts[wpn][2] = planes[i].alt;
	calc_hd_course_dist(planes[i].wpts[wpn][0],   planes[i].wpts[wpn][1],
			    planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1],
			    &course, &d);
	planes[i].wpts[wpn][4] = course;
	planes[i].wpts[wpn][5] = d;
	wpn += 1;

	planes[i].wpn = wpn;

	planes[i].ahdg = planes[i].wpts[wpn-1][4];
	cout << endl;
	cout << "Contact " << planes[i].wpn << endl;
	cout << "Turn to heading   = " << (int)(planes[i].ahdg) << endl;
	cout << endl;
	planes[i].on_crs = true;
      }

      // reached waypoint?
      if ( fabs(planes[i].dnc) < 0.3 && planes[i].dnwp < 1.0 ) {
	planes[i].wpn -= 1;
	wpn = planes[i].wpn-1;
	planes[i].ahdg = planes[i].wpts[wpn][4];
	cout << endl;
	cout << "Next waypoint = " << planes[i].wpn << endl;
	cout << "New heading   = " << planes[i].ahdg << endl;
	cout << endl;
	planes[i].on_crs = true;
      }

      // update assigned parameters
      wpn = planes[i].wpn-1;            // this is the current waypoint

      planes[i].dcc  = calc_psl_dist(planes[i].brg, planes[i].dist,
				     planes[i].wpts[wpn][0], planes[i].wpts[wpn][1],
				     planes[i].wpts[wpn][4]);
      planes[i].dnc  = calc_psl_dist(planes[i].brg, planes[i].dist,
				     planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1],
				     planes[i].wpts[wpn-1][4]);
      calc_hd_course_dist(planes[i].brg, planes[i].dist, 
			  planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1],
			  &course, &d);
      planes[i].dnwp = d;

      //cout << planes[i].brg << " " << planes[i].dist << " " << planes[i].wpts[wpn+1][0] 
      //<< " " << planes[i].wpts[wpn+1][1] << " " << planes[i].wpts[wpn+1][4] 
      //cout << " distance to current course = " << planes[i].dcc << endl;

      // come off course ?
      if ( fabs(planes[i].dcc) > 0.5 && planes[i].on_crs) {
	wpn = wpn-1;
	if ( planes[i].wpts[wpn][4] < 0) {
	  planes[i].ahdg += 30.0;
	}
	else {
	  planes[i].ahdg -= 30.0;
	}
	planes[i].on_crs = false;

	cout << endl;
	cout << "Your are " << planes[i].dcc << " miles off the asigned course: " << endl;
	cout << "New heading = " << (int)(planes[i].ahdg) << endl;
	cout << endl;
      }
      else if ( fabs(planes[i].dcc) < 0.1 && !planes[i].on_crs) {
	planes[i].ahdg = fabs(planes[i].wpts[wpn][4]);
	planes[i].on_crs = true;
	
	cout << endl;
	cout << "New heading = " << (int)(planes[i].ahdg) << endl;
	cout << endl;
	}
      
      // In range of tower?
      if ( planes[i].wpn == 2 && planes[i].dnwp < 3. ) {
	cout << endl;
	cout << "Contact Tower";
	cout << endl;
	planes[i].contact = 2;
      }
    }
  }

}

// ============================================================================
// get active runway
// ============================================================================
void FGApproach::get_active_runway() {

#ifdef FG_WEATHERCM
  sgVec3 position = { lat, lon, elev };
  FGPhysicalProperty stationweather = WeatherDatabase->get(position);
#else
  FGEnvironment stationweather =
    globals->get_environment_mgr()->getEnvironment(lat, lon, elev);
#endif

  SGPath path( globals->get_fg_root() );
  path.append( "Airports" );
  path.append( "runways.mk4" );
  FGRunways runways( path.c_str() );
  
  //Set the heading to into the wind
#ifdef FG_WEATHERCM
  double wind_x = stationweather.Wind[0];
  double wind_y = stationweather.Wind[1];
  
  double speed = sqrt( wind_x*wind_x + wind_y*wind_y ) * SG_METER_TO_NM / (60.0*60.0);
  double hdg;
  
  //If no wind use 270degrees
  if(speed == 0) {
    hdg = 270;
  } else {
    // //normalize the wind to get the direction
    //wind_x /= speed; wind_y /= speed;
    
    hdg = - atan2 ( wind_x, wind_y ) * SG_RADIANS_TO_DEGREES ;
    if (hdg < 0.0)
      hdg += 360.0;
  }
#else
  double hdg = stationweather.get_wind_from_heading_deg();
#endif

  FGRunway runway;
  //if ( runways.search( "EGNX", int(hdg), &runway) ) {
  if ( runways.search( ident, int(hdg), &runway) ) {
    active_runway = runway.rwy_no;
    active_rw_hdg = runway.heading;
    //cout << "Active runway is: " << active_runway << "  heading = " 
    // << active_rw_hdg << endl;
  }
  else cout << "FGRunways search failed" << endl;

}

// ========================================================================
// update infos about plane
// ========================================================================
void FGApproach::update_plane_dat() {
  
  //cout << "Update Approach " << ident << "   " << num_planes << " registered" << endl;
  // update plane positions
  for (int i=0; i<num_planes; i++) {
    planes[i].lon = lon_node->getDoubleValue();
    planes[i].lat = lat_node->getDoubleValue();
    planes[i].alt = elev_node->getDoubleValue();
//    Point3D aircraft = sgGeodToCart( Point3D(planes[i].lon*SGD_DEGREES_TO_RADIANS, 
//					     planes[i].lat*SGD_DEGREES_TO_RADIANS, 
//					     planes[i].alt*SG_FEET_TO_METER) );
    double course, distance;
    calc_gc_course_dist(Point3D(lon*SGD_DEGREES_TO_RADIANS, lat*SGD_DEGREES_TO_RADIANS, 0.0),
			Point3D(planes[i].lon*SGD_DEGREES_TO_RADIANS,planes[i].lat*SGD_DEGREES_TO_RADIANS, 0.0 ),
			&course, &distance);
    planes[i].dist = distance/SG_NM_TO_METER;
    planes[i].brg  = 360.0-course*SGD_RADIANS_TO_DEGREES;

    //cout << "Plane Id: " << planes[i].ident << "  Distance to " << ident 
    //<< " is " << planes[i].dist << " m" << endl;
    
    //if (first) {
    //transmission = ident;
    //globals->get_ATC_display()->RegisterRepeatingMessage(transmission);
    //first = false;
    //}
  }   
}

// =======================================================================
// Add plane to Approach list
// =======================================================================
void FGApproach::AddPlane(string pid) {
  for ( int i=0; i<num_planes; i++) {
    if ( planes[i].ident == pid) {
      //cout << "Plane already registered: " << ident << " " << num_planes << endl;
      return;
    }
  }
  planes[num_planes].ident = pid;
  ++num_planes;
  //cout << "Plane added to list: " << ident << " " << num_planes << endl;
  return;
}

// ========================================================================
// closest distance between a point and a straigt line in 2 dim.
// ========================================================================
double FGApproach::calc_psl_dist(const double &h1, const double &d1,
				 const double &h2, const double &d2,
				 const double &h3)
{
  double a1 = h1 * SGD_DEGREES_TO_RADIANS;
  double a2 = h2 * SGD_DEGREES_TO_RADIANS;
  double a3 = h3 * SGD_DEGREES_TO_RADIANS;
  double x1 = cos(a1) * d1;
  double y1 = sin(a1) * d1;
  double x2 = cos(a2) * d2;
  double y2 = sin(a2) * d2;
  double x3 = cos(a3);
  double y3 = sin(a3);
  
  // formula: dis = sqrt( (v1-v2)**2 - ((v1-v2)*v3)**2 ); vi = (xi,yi)
  double val1   = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
  double val2   = ((x1-x2)*x3 + (y1-y2)*y3) * ((x1-x2)*x3 + (y1-y2)*y3);
  double dis    = val1 - val2;
  // now get sign for offset 
  //cout << x1 << " " << x2 << " " << y1 << " " << y2 << " " 
  //     << x3 << " " << y3 << " " 
  //     << val1 << " " << val2 << " " << dis << endl;
  x3 *= sqrt(val2);
  y3 *= sqrt(val2);
  if ( x3*(x1-x2) < 0.0 && y3*(y1-y2) < 0.0) {
    x3 *= -1.0;
    y3 *= -1.0;
  }
  //cout << x3 << " " << y3 << endl;
  double dis1   = x1-x2-x3;
  double dis2   = y1-y2-y3;
  dis = sqrt(dis);
  if (atan2(dis2,dis1) < a3) dis *= -1.0;
  //cout << dis1 << " " << dis2 << " " << atan2(dis2,dis1)*SGD_RADIANS_TO_DEGREES << " " << h3
  //     << " " << sqrt(dis1*dis1 + dis2*dis2) << " " << dis << endl;
  //cout << atan2(dis2,dis1)*SGD_RADIANS_TO_DEGREES << " " << dis << endl;

  return dis;
}

// ========================================================================
// get heading and distance between two points; point1 ---> point2
// ========================================================================
void FGApproach::calc_hd_course_dist(const double &h1, const double &d1, 
				     const double &h2, const double &d2,
				     double *course, double *dist)
{
  double a1 = h1 * SGD_DEGREES_TO_RADIANS;
  double a2 = h2 * SGD_DEGREES_TO_RADIANS;
  double x1 = cos(a1) * d1;
  double y1 = sin(a1) * d1;
  double x2 = cos(a2) * d2;
  double y2 = sin(a2) * d2;
  	   
  *dist   = sqrt( (y2-y1)*(y2-y1) + (x2-x1)*(x2-x1) );
  *course = atan2( (y2-y1), (x2-x1) ) * SGD_RADIANS_TO_DEGREES;
  if ( *course < 0 ) *course = *course+360;
  //cout << x1 << " " << y1 << " " << x2 << " " << y2 << " " << *dist << " " << *course << endl;
}



int FGApproach::RemovePlane() {

  // first check if anything has to be done
  int i;
  bool rmplane = false;
  for (i=0; i<num_planes; i++) {
    if (planes[i].dist > range*SG_NM_TO_METER) {
      rmplane = true;
      break;
    }
  }
  if (!rmplane) return num_planes;

  // now make a copy of the plane list
  PlaneApp tmp[max_planes];
  for (i=0; i<num_planes; i++) {
    tmp[i] = planes[i];
  }
  
  int np = 0;
  // now check which planes are still in range
  for (i=0; i<num_planes; i++) {
    if (tmp[i].dist <= range*SG_NM_TO_METER) {
      planes[np] = tmp[i];
      np += 1;
    }
  }
  num_planes = np;
  return num_planes;
}
