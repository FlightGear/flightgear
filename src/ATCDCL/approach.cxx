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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "approach.hxx"
#include "transmission.hxx"
#include "transmissionlist.hxx"
#include "ATCDialog.hxx"

#include <Airports/runways.hxx>
#include <simgear/constants.h>
#include <simgear/math/polar3d.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>


#include <GUI/gui.h>

//Constructor
FGApproach::FGApproach(){
  comm1_node = fgGetNode("/instrumentation/comm[0]/frequencies/selected-mhz", true);
  comm2_node = fgGetNode("/instrumentation/comm[1]/frequencies/selected-mhz", true);
  
  _type = APPROACH;

  num_planes = 0;
  lon_node   = fgGetNode("/position/longitude-deg", true);
  lat_node   = fgGetNode("/position/latitude-deg", true);
  elev_node  = fgGetNode("/position/altitude-ft", true);
  hdg_node   = fgGetNode("/orientation/heading-deg", true);
  speed_node = fgGetNode("/velocities/airspeed-kt", true);
  etime_node = fgGetNode("/sim/time/elapsed-sec", true);

  first = true;
  active_runway = "";
  int i;
  for ( i=0; i<max_planes; i++) {
    planes[i].contact   = 0;
    planes[i].wpn       = 0;
    planes[i].dnwp      = -999.;
    planes[i].on_crs    = true;
    planes[i].turn_rate = 10.0;
    planes[i].desc_rate = 1000.0;
    planes[i].clmb_rate = 500.0;
    planes[i].tlm       = 0.0;
    planes[i].lmc.c1    = 0;
    planes[i].lmc.c2    = 0;
    planes[i].lmc.c3    = -1;
    planes[i].wp_change = false;
  }
}

//Destructor
FGApproach::~FGApproach(){
}

void FGApproach::Init() {
}



// ============================================================================
// the main update function
// ============================================================================
void FGApproach::Update(double dt) {
	
	const int max_trans = 20;
	FGTransmission tmissions[max_trans];
	int    wpn;
	atc_type station = APPROACH;
	TransCode code;
	TransPar TPar;
	int    i,j;
	//double course, d, 
	double adif, datp;
	//char   buf[10];
	string message;
	//static string atcmsg1[10];
	//static string atcmsg2[10];
	string mentry;
	string transm;
	TransPar tpars;
	//static bool TransDisplayed = false;
	
	update_plane_dat();
	if ( active_runway == "" ) get_active_runway();
	
	double comm1_freq = comm1_node->getDoubleValue();
	
	//bool DisplayTransmissions = true;
	
	for (i=0; i<num_planes; i++) {
		if ( planes[i].ident == "Player") { 
			station = APPROACH;
			tpars.station = name;
			tpars.callsign = "Player";
			tpars.airport = ident;
			
			//cout << "ident = " << ident << " name = " << name << '\n';
			
			int num_trans = 0;
			// is the frequency of the station tuned in?
			if ( freq == (int)(comm1_freq*100.0 + 0.5) ) {
				current_transmissionlist->query_station( station, tmissions, max_trans, num_trans );
				// loop over all transmissions for station
				for ( j=0; j<=num_trans-1; j++ ) {
					code = tmissions[j].get_code();
					//cout << "code is " << code.c1 << "  " << code.c2 << "  " << code.c3 << '\n';
					// select proper transmissions
					if(code.c3 != 2) {    // DCL - hack to prevent request crossing airspace being displayed since this isn't implemented yet.
					    if ( ( code.c2 == -1 && planes[i].lmc.c3 == 0 ) || 
						    ( code.c1 == 0  && code.c2 == planes[i].lmc.c2 ) ) {
						    mentry = current_transmissionlist->gen_text(station, code, tpars, false);
						    transm = current_transmissionlist->gen_text(station, code, tpars, true);
						    // is the transmission already registered?
						    if (!current_atcdialog->trans_reg( ident, transm, APPROACH )) {
							    current_atcdialog->add_entry( ident, transm, mentry, APPROACH, 0 );
						    }
					    }
					}
				}
			}
		}
	}
	
	for ( i=0; i<num_planes; i++ ) {
		//cout << "TPar.airport = " << TPar.airport << " TPar.station = " << TPar.station << " TPar.callsign = " << TPar.callsign << '\n';
		//if ( planes[i].ident == TPar.callsign && name == TPar.airport && TPar.station == "approach" ) {
			//if ( TPar.request && TPar.intention == "landing" && ident == TPar.intid) {
			if(planes[i].ident == "Player" && fgGetBool("/sim/atc/opt0")) {
				//cout << "Landing requested\n";
				fgSetBool("/sim/atc/opt0", false);
				planes[i].wpn = 0; 
				// ===========================
				// === calculate waypoints ===
				// ===========================
				calc_wp( i );  
				update_param( i );
				wpn = planes[i].wpn-1;
				planes[i].aalt = planes[i].wpts[wpn-1][2];
				planes[i].ahdg = planes[i].wpts[wpn][4];
				
				// generate the message
				code.c1 = 1;
				code.c2 = 1;
				code.c3 = 0;
				adif = angle_diff_deg( planes[i].hdg, planes[i].ahdg );
				tpars.station = name;
				tpars.callsign = "Player";
				if ( adif < 0 ) tpars.tdir = 1;
				else            tpars.tdir = 2;
				tpars.heading = planes[i].ahdg;
				if      (planes[i].alt-planes[i].aalt > 100.0)  tpars.VDir = 1;
				else if (planes[i].alt-planes[i].aalt < -100.0) tpars.VDir = 3;
				else tpars.VDir = 2;
				tpars.alt = planes[i].aalt;
				message = current_transmissionlist->gen_text(station, code, tpars, true );
				//cout << message << '\n';
				set_message(message);
				planes[i].lmc = code;
				planes[i].tlm = etime_node->getDoubleValue();
				planes[i].on_crs = true;
				planes[i].contact = 1;
			}
		//}
		
		//if(1) {
		if ( planes[i].contact == 1 ) {
			// =========================
			// === update parameters ===
			// =========================
			update_param( i );
			//cout << planes[i].brg << " " << planes[i].dist << " " << planes[i].wpts[wpn+1][0] 
			//<< " " << planes[i].wpts[wpn+1][1] << " " << planes[i].wpts[wpn+1][4] 
			//cout << wpn << " distance to current course = " << planes[i].dcc << endl;
			//cout << etime_node->getDoubleValue() << endl;
			
			// =========================
			// === reached waypoint? ===
			// =========================
			wpn = planes[i].wpn-2;
			adif = angle_diff_deg( planes[i].hdg, planes[i].wpts[wpn][4] ) 
			* SGD_DEGREES_TO_RADIANS;
			datp = 2*sin(fabs(adif)/2.0)*sin(fabs(adif)/2.0) *
			       planes[i].spd/3600. * planes[i].turn_rate + 
			       planes[i].spd/3600. * 3.0;
			//cout << adif/SGD_DEGREES_TO_RADIANS << " " 
			//     << datp << " " << planes[i].dnc << " " << planes[i].dcc <<endl;
			if ( fabs(planes[i].dnc) < datp ) {
			//if ( fabs(planes[i].dnc) < 0.3 && planes[i].dnwp < 1.0 ) {
				//cout << "Reached next waypoint!\n";
				planes[i].wpn -= 1;
				wpn = planes[i].wpn-1;
				planes[i].ahdg = planes[i].wpts[wpn][4];
				planes[i].aalt = planes[i].wpts[wpn-1][2];
				planes[i].wp_change = true;
				
				// generate the message
				adif = angle_diff_deg( planes[i].hdg, planes[i].ahdg );
				tpars.station = name;
				tpars.callsign = "Player";
				if ( adif < 0 ) tpars.tdir = 1;
				else            tpars.tdir = 2;
				tpars.heading = planes[i].ahdg;
				
				if ( wpn-1 != 0) { 
					code.c1 = 1;
					code.c2 = 1;
					code.c3 = 0;
					if      (planes[i].alt-planes[i].aalt > 100.0)  tpars.VDir = 1;
					else if (planes[i].alt-planes[i].aalt < -100.0) tpars.VDir = 3;
					else tpars.VDir = 2;
					tpars.alt = planes[i].aalt;
					message = current_transmissionlist->gen_text(station, code, tpars, true );
					//cout << "Approach transmitting...\n";
					//cout << message << endl;
					set_message(message);
				}
				else {
					code.c1 = 1;
					code.c2 = 3;
					code.c3 = 0;
					tpars.runway = active_runway;
					message = current_transmissionlist->gen_text(station, code, tpars, true);
					//cout << "Approach transmitting 2 ...\n";
					//cout << message << endl;
					set_message(message);
				}
				planes[i].lmc = code;
				planes[i].tlm = etime_node->getDoubleValue();
				planes[i].on_crs = true;
				
				update_param( i );
			}
			
			// =========================
			// === come off course ? ===
			// =========================
			if ( fabs(planes[i].dcc) > 1.0 && 
			   ( !planes[i].wp_change || etime_node->getDoubleValue() - planes[i].tlm > tbm ) ) {
				//cout << "Off course!\n";
				if ( planes[i].on_crs ) {
					if ( planes[i].dcc < 0) {
						planes[i].ahdg += 30.0;
					}
					else {
						planes[i].ahdg -= 30.0;
					}
					if (planes[i].ahdg > 360.0) planes[i].ahdg -= 360.0;
					else if (planes[i].ahdg < 0.0) planes[i].ahdg += 360.0;
				}
				//cout << planes[i].on_crs << " " 
				//     << angle_diff_deg( planes[i].hdg, planes[i].ahdg) << " "
				//     << etime_node->getDoubleValue() << " "
				//     << planes[i].tlm << endl;
				// generate the message
				if ( planes[i].on_crs || 
				   ( fabs(angle_diff_deg( planes[i].hdg, planes[i].ahdg )) >  30.0  && 
				    etime_node->getDoubleValue() - planes[i].tlm > tbm) ) {
					// generate the message
					code.c1 = 1;
					code.c2 = 4;
					code.c3 = 0;
					adif = angle_diff_deg( planes[i].hdg, planes[i].ahdg );
					tpars.station = name;
					tpars.callsign = "Player";
					tpars.miles   = fabs(planes[i].dcc);
					if ( adif < 0 ) tpars.tdir = 1;
					else            tpars.tdir = 2;
					tpars.heading = planes[i].ahdg;
					message = current_transmissionlist->gen_text(station, code, tpars, true);
					//cout << "Approach transmitting 3 ...\n";
					//cout << message << '\n';
					set_message(message);
					planes[i].lmc = code;
					planes[i].tlm = etime_node->getDoubleValue();
				}
				
				planes[i].on_crs = false;
			}
			else if ( !planes[i].on_crs ) {
				//cout << "Off course 2!\n";
				wpn = planes[i].wpn-1;
				adif = angle_diff_deg( planes[i].hdg, planes[i].wpts[wpn][4] ) 
				       * SGD_DEGREES_TO_RADIANS;
				datp = 2*sin(fabs(adif)/2.0)*sin(fabs(adif)/2.0) *
				planes[i].spd/3600. * planes[i].turn_rate + 
				planes[i].spd/3600. * 3.0;
				if ( fabs(planes[i].dcc) < datp ) { 
					planes[i].ahdg = fabs(planes[i].wpts[wpn][4]);
					
					// generate the message
					code.c1 = 1;
					code.c2 = 2;
					code.c3 = 0;
					tpars.station = name;
					tpars.callsign = "Player";
					if ( adif < 0 ) tpars.tdir = 1;
					else            tpars.tdir = 2;
					tpars.heading = planes[i].ahdg;
					message = current_transmissionlist->gen_text(station, code, tpars, true);
					//cout << "Approach transmitting 4 ...\n";
					//cout << message << '\n';
					set_message(message);
					planes[i].lmc = code;
					planes[i].tlm = etime_node->getDoubleValue();
					
					planes[i].on_crs = true;	  
				} 
			}
			else if ( planes[i].wp_change  ) {
				planes[i].wp_change = false;
			}
			
			// ===================================================================
			// === Less than two minutes away from touchdown? -> Contact Tower ===
			// ===================================================================
			if ( planes[i].wpn == 2 && planes[i].dnwp < planes[i].spd/60.*2.0 ) {
				
				double freq = 121.95;	// Hardwired - FIXME
				// generate message
				code.c1 = 1;
				code.c2 = 5;
				code.c3 = 0;
				tpars.station = name;
				tpars.callsign = "Player";
				tpars.freq    = freq;
				message = current_transmissionlist->gen_text(station, code, tpars, true);
				//cout << "Approach transmitting 5 ...\n";
				//cout << message << '\n';
				set_message(message);
				planes[i].lmc = code;
				planes[i].tlm = etime_node->getDoubleValue();
				
				planes[i].contact = 2;
			}
		}
	}
}


// ============================================================================
// update course parameters
// ============================================================================
void FGApproach::update_param( const int &i ) {
  
  double course, d;

  int wpn = planes[i].wpn-1;            // this is the current waypoint

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

}

// ============================================================================
// smallest difference between two angles in degree
// difference is negative if a1 > a2 and positive if a2 > a1
// ===========================================================================
double FGApproach::angle_diff_deg( const double &a1, const double &a2) {
  
  double a3 = a2 - a1;
  if (a3 < 180.0) a3 += 360.0;
  if (a3 > 180.0) a3 -= 360.0;

  return a3;
}

// ============================================================================
// calculate waypoints
// ============================================================================
void FGApproach::calc_wp( const int &i ) {
	
	int j;
	double course, d, cd, a1;
	
	int wpn = planes[i].wpn;
	// waypoint 0: Threshold of active runway
	calc_gc_course_dist(Point3D(lon*SGD_DEGREES_TO_RADIANS, lat*SGD_DEGREES_TO_RADIANS, 0.0),
	                    Point3D(active_rw_lon*SGD_DEGREES_TO_RADIANS,active_rw_lat*SGD_DEGREES_TO_RADIANS, 0.0 ),
	                    &course, &d);
	double d1 = active_rw_hdg+180.0;
	if ( d1 > 360.0 ) d1 -=360.0;
	calc_cd_head_dist(360.0-course*SGD_RADIANS_TO_DEGREES, d/SG_NM_TO_METER, 
	                  d1, active_rw_len/SG_NM_TO_METER/2.0, 
	                  &planes[i].wpts[wpn][0], &planes[i].wpts[wpn][1]);
	planes[i].wpts[wpn][2] = elev;
	planes[i].wpts[wpn][4] = 0.0;
	planes[i].wpts[wpn][5] = 0.0;
	wpn += 1;
	
	// ======================
	// horizontal navigation
	// ======================
	// waypoint 1: point for turning onto final
	calc_cd_head_dist(planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1], d1, lfl,
	                  &planes[i].wpts[wpn][0], &planes[i].wpts[wpn][1]);
	calc_hd_course_dist(planes[i].wpts[wpn][0],   planes[i].wpts[wpn][1],
	                    planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1],
	                    &course, &d);
	planes[i].wpts[wpn][4] = course;
	planes[i].wpts[wpn][5] = d;
	wpn += 1;
	
	// calculate course and distance from plane position to waypoint 1
	calc_hd_course_dist(planes[i].brg, planes[i].dist, planes[i].wpts[1][0], 
	                    planes[i].wpts[1][1], &course, &d);
	// check if airport is not between plane and waypoint 1 and
	// DCA to airport on course to waypoint 1 is larger than 10 miles
	double zero = 0.0;
	if ( fabs(angle_diff_deg( planes[i].wpts[1][0], planes[i].brg  )) < 90.0 ||
		calc_psl_dist( zero, zero, planes[i].brg, planes[i].dist, course ) > 10.0 ) {
		// check if turning angle at waypoint 1 would be > max_ta
		if ( fabs(angle_diff_deg( planes[i].wpts[1][4], course )) > max_ta ) {
			cd = calc_psl_dist(planes[i].brg, planes[i].dist,
			     planes[i].wpts[1][0], planes[i].wpts[1][1],
			     planes[i].wpts[1][4]);
			a1 = atan2(cd,planes[i].wpts[1][1]);
			planes[i].wpts[wpn][0] = planes[i].wpts[1][0] - a1/SGD_DEGREES_TO_RADIANS;
			if ( planes[i].wpts[wpn][0] < 0.0)   planes[i].wpts[wpn][0] += 360.0;   
			if ( planes[i].wpts[wpn][0] > 360.0) planes[i].wpts[wpn][0] -= 360.0;   
			planes[i].wpts[wpn][1] = fabs(cd) / sin(fabs(a1));
			calc_hd_course_dist(planes[i].wpts[wpn][0],   planes[i].wpts[wpn][1],
			                    planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1],
			                    &course, &d);
			planes[i].wpts[wpn][4] = course;
			planes[i].wpts[wpn][5] = d;
			wpn += 1;
			
			calc_hd_course_dist(planes[i].brg, planes[i].dist, planes[i].wpts[wpn-1][0], 
			                    planes[i].wpts[wpn-1][1], &course, &d);
		}
	} else {
		double leg = 10.0;
		a1 = atan2(planes[i].wpts[1][1], leg );
		
		if ( angle_diff_deg(planes[i].brg,planes[i].wpts[1][0]) < 0 ) 
			planes[i].wpts[wpn][0] = planes[i].wpts[1][0] + a1/SGD_DEGREES_TO_RADIANS;
		else planes[i].wpts[wpn][0] = planes[i].wpts[1][0] - a1/SGD_DEGREES_TO_RADIANS;
		
		planes[i].wpts[wpn][1] = sqrt( planes[i].wpts[1][1]*planes[i].wpts[1][1] + leg*leg );
		calc_hd_course_dist(planes[i].wpts[wpn][0],   planes[i].wpts[wpn][1],
		                    planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1],
		                    &course, &d);
		planes[i].wpts[wpn][4] = course;
		planes[i].wpts[wpn][5] = d;
		wpn += 1;
		
		calc_hd_course_dist(planes[i].brg, planes[i].dist,
		                    planes[i].wpts[wpn-1][0], planes[i].wpts[wpn-1][1],
		                    &course, &d);
	}
	
	planes[i].wpts[wpn][0] = planes[i].brg;
	planes[i].wpts[wpn][1] = planes[i].dist;
	planes[i].wpts[wpn][2] = planes[i].alt;
	planes[i].wpts[wpn][4] = course;
	planes[i].wpts[wpn][5] = d;
	wpn += 1;
	
	planes[i].wpn = wpn;
	
	// Now check if legs are too short or if legs can be shortend
	// legs must be at least 2 flight minutes long
	double mdist = planes[i].spd / 60.0 * 2.0;
	for ( j=2; j<wpn-1; ++j ) {
		if ( planes[i].wpts[j][1] < mdist) {
		}
	}
	
	// ====================
	// vertical navigation
	// ====================
	double alt = elev+3000.0;
	planes[i].wpts[1][2] = round_alt( true, alt );
	for ( j=2; j<wpn-1; ++j ) {
		double dalt = planes[i].alt - planes[i].wpts[j-1][2];
		if ( dalt > 0 ) {
			alt = planes[i].wpts[j-1][2] + 
			      (planes[i].wpts[j][5] / planes[i].spd) * 60.0 * planes[i].desc_rate;
			planes[i].wpts[j][2] = round_alt( false, alt );
			if ( planes[i].wpts[j][2] > planes[i].alt ) 
				planes[i].wpts[j][2] = round_alt( false, planes[i].alt );
		}
		else {
			planes[i].wpts[j][2] = planes[i].wpts[1][2];
		}
	}
	
	cout << "Plane position: " << planes[i].brg << " " << planes[i].dist << endl;
	for ( j=0; j<wpn; ++j ) {
		cout << "Waypoint " << j << endl;
		cout << "------------------" << endl;
		cout << planes[i].wpts[j][0] << "   " << planes[i].wpts[j][1]
		     << "   " << planes[i].wpts[j][2] << "   " << planes[i].wpts[j][5]; 
		cout << endl << endl;
	}
	
}


// ============================================================================
// round altitude value to next highest/lowest 500 feet
// ============================================================================
double FGApproach::round_alt( const bool hl, double alt ) {

  alt = alt/1000.0;
  if ( hl ) {
    if ( alt > (int)(alt)+0.5 ) alt = ((int)(alt)+1)*1000.0;
    else alt = ((int)(alt)+0.5)*1000.0;
  }
  else {
    if ( alt > (int)(alt)+0.5 ) alt = ((int)(alt)+0.5)*1000.0;
    else alt = ((int)(alt))*1000.0;
  }
  
  return alt;
}


// ============================================================================
// get active runway
// ============================================================================
void FGApproach::get_active_runway() {
	//cout << "Entering FGApproach::get_active_runway()\n";

  FGEnvironment stationweather =
      ((FGEnvironmentMgr *)globals->get_subsystem("environment"))
        ->getEnvironment(lat, lon, elev);

  double hdg = stationweather.get_wind_from_heading_deg();
  
  FGRunway runway;
  if ( globals->get_runways()->search( ident, int(hdg), &runway) ) {
    active_runway = runway._rwy_no;
    active_rw_hdg = runway._heading;
    active_rw_lon = runway._lon;
    active_rw_lat = runway._lat;
    active_rw_len = runway._length;
    //cout << "Active runway is: " << active_runway << "  heading = " 
    // << active_rw_hdg 
    // << " lon = " << active_rw_lon 
    // << " lat = " << active_rw_lat <<endl;
  }
  else cout << "FGRunways search failed\n";

}

// ========================================================================
// update infos about plane
// ========================================================================
void FGApproach::update_plane_dat() {
  
  //cout << "Update Approach " << ident << "   " << num_planes << " registered" << endl;
  // update plane positions
  int i;
  for (i=0; i<num_planes; i++) {
    planes[i].lon = lon_node->getDoubleValue();
    planes[i].lat = lat_node->getDoubleValue();
    planes[i].alt = elev_node->getDoubleValue();
    planes[i].hdg = hdg_node->getDoubleValue();
    planes[i].spd = speed_node->getDoubleValue();

    /*Point3D aircraft = sgGeodToCart( Point3D(planes[i].lon*SGD_DEGREES_TO_RADIANS, 
					     planes[i].lat*SGD_DEGREES_TO_RADIANS, 
					     planes[i].alt*SG_FEET_TO_METER) );*/
    double course, distance;
    calc_gc_course_dist(Point3D(lon*SGD_DEGREES_TO_RADIANS, lat*SGD_DEGREES_TO_RADIANS, 0.0),
			Point3D(planes[i].lon*SGD_DEGREES_TO_RADIANS,planes[i].lat*SGD_DEGREES_TO_RADIANS, 0.0 ),
			&course, &distance);
    planes[i].dist = distance/SG_NM_TO_METER;
    planes[i].brg  = 360.0-course*SGD_RADIANS_TO_DEGREES;

    //cout << "Plane Id: " << planes[i].ident << "  Distance to " << ident 
    // << " is " << planes[i].dist << " miles   " << "Bearing " << planes[i].brg << endl;
    
  }   
}

// =======================================================================
// Add plane to Approach list
// =======================================================================
void FGApproach::AddPlane(const string& pid) {

  int i;
  for ( i=0; i<num_planes; i++) {
    if ( planes[i].ident == pid) {
      //cout << "Plane already registered: " << planes[i].ident << ' ' << ident << ' ' << num_planes << endl;
      return;
    }
  }
  planes[num_planes].ident = pid;
  ++num_planes;
  //cout << "Plane added to list: " << ident << " " << num_planes << endl;
  return;
}

// ================================================================================
// closest distance between a point (h1,d1) and a straigt line (h2,d2,h3) in 2 dim.
// ================================================================================
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
  double da = fabs(atan2(y3,x3) - atan2(y1-y2,x1-x2));
  if ( da > SGD_PI ) da -= SGD_2PI;
  if ( fabs(da) > SGD_PI_2) {
    //if ( x3*(x1-x2) < 0.0 && y3*(y1-y2) < 0.0) {
    x3 *= -1.0;
    y3 *= -1.0;
  }
  //cout << x3 << " " << y3 << endl;
  double dis1   = x1-x2-x3;
  double dis2   = y1-y2-y3;
  dis = sqrt(dis);
  da = atan2(dis2,dis1);
  if ( da < 0.0 ) da  += SGD_2PI;
  if ( da < a3 )  dis *= -1.0;
  //cout << dis1 << " " << dis2 << " " << da*SGD_RADIANS_TO_DEGREES << " " << h3
  //     << " " << sqrt(dis1*dis1 + dis2*dis2) << " " << dis << endl;
  //cout << atan2(dis2,dis1)*SGD_RADIANS_TO_DEGREES << " " << dis << endl;

  return dis;
}


// ========================================================================
// Calculate new bear/dist given starting bear/dis, and offset radial,
// and distance.
// ========================================================================
void FGApproach::calc_cd_head_dist(const double &h1, const double &d1, 
				   const double &course, const double &dist,
				   double *h2, double *d2)
{
  double a1 = h1 * SGD_DEGREES_TO_RADIANS;
  double a2 = course * SGD_DEGREES_TO_RADIANS;
  double x1 = cos(a1) * d1;
  double y1 = sin(a1) * d1;
  double x2 = cos(a2) * dist;
  double y2 = sin(a2) * dist;
    
  *d2 = sqrt((x1+x2)*(x1+x2) + (y1+y2)*(y1+y2));
  *h2 = atan2( (y1+y2), (x1+x2) ) * SGD_RADIANS_TO_DEGREES;
  if ( *h2 < 0 ) *h2 = *h2+360;
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
  bool rmplane = false;
  int i;

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


void FGApproach::set_message(const string &msg)
{
  fgSetString("/sim/messages/approach", msg.c_str());
}

