// FGAIAircraft - FGAIBase-derived class creates an AI airplane
//
// Written by David Culp, started October 2003.
//
// Copyright (C) 2003  David P. Culp - davidculp2@comcast.net
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/point3d.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <string>
#include <math.h>

SG_USING_STD(string);

#include "AIAircraft.hxx"

//
// accel, decel, climb_rate, descent_rate, takeoff_speed, climb_speed,
// cruise_speed, descent_speed, land_speed
//
const FGAIAircraft::PERF_STRUCT FGAIAircraft::settings[] = {
    // light aircraft
    {2.0, 2.0,  450.0, 1000.0,  70.0,  80.0, 100.0,  80.0,  60.0},
    // ww2_fighter
    {4.0, 2.0, 3000.0, 1500.0, 110.0, 180.0, 250.0, 200.0, 100.0},
    // jet_transport
    {5.0, 2.0, 3000.0, 1500.0, 140.0, 300.0, 430.0, 300.0, 130.0},
    // jet_fighter
    {7.0, 3.0, 4000.0, 2000.0, 150.0, 350.0, 500.0, 350.0, 150.0}
};


FGAIAircraft *FGAIAircraft::_self = NULL;

FGAIAircraft::FGAIAircraft(FGAIManager* mgr) {
   manager = mgr;   
   _self = this;
   _type_str = "aircraft";
   _otype = otAircraft;
   fp = 0;
   fp_count = 0;
   use_perf_vs = true;

   // set heading and altitude locks
   hdg_lock = false;
   alt_lock = false;
}


FGAIAircraft::~FGAIAircraft() {
    if (fp) delete fp;
    _self = NULL;
}


bool FGAIAircraft::init() {
   return FGAIBase::init();
}

void FGAIAircraft::bind() {
    FGAIBase::bind();

    props->tie("controls/gear/gear-down",
               SGRawValueFunctions<bool>(FGAIAircraft::_getGearDown));

/*
    props->getNode("controls/lighting/landing-lights", true)
           ->alias("controls/gear/gear-down");
*/
}

void FGAIAircraft::unbind() {
    FGAIBase::unbind();

    props->untie("controls/gear/gear-down");
//    props->getNode("controls/lighting/landing-lights")->unalias();
}


void FGAIAircraft::update(double dt) {

   Run(dt);
   Transform();
   FGAIBase::update(dt);
}

void FGAIAircraft::SetPerformance(const PERF_STRUCT *ps) {
   
   performance = ps;
} 


void FGAIAircraft::Run(double dt) {

   FGAIAircraft::dt = dt;

   if (fp) ProcessFlightPlan();
	
   double turn_radius_ft;
   double turn_circum_ft;
   double speed_north_deg_sec;
   double speed_east_deg_sec;
   double ft_per_deg_lon;
   double ft_per_deg_lat;
   double dist_covered_ft;
   double alpha;

   // get size of a degree at this latitude
   ft_per_deg_lat = 366468.96 - 3717.12 * cos(pos.lat()/SG_RADIANS_TO_DEGREES);
   ft_per_deg_lon = 365228.16 * cos(pos.lat() / SG_RADIANS_TO_DEGREES);

   // adjust speed
   double speed_diff = tgt_speed - speed;
   if (fabs(speed_diff) > 0.2) {
     if (speed_diff > 0.0) speed += performance->accel * dt;
     if (speed_diff < 0.0) speed -= performance->decel * dt;
   } 
   
   // convert speed to degrees per second
   speed_north_deg_sec = cos( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lat;
   speed_east_deg_sec  = sin( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lon;

   // set new position
   pos.setlat( pos.lat() + speed_north_deg_sec * dt);
   pos.setlon( pos.lon() + speed_east_deg_sec * dt); 

   // adjust heading based on current bank angle
   if (roll != 0.0) {
     turn_radius_ft = 0.088362 * speed * speed
                       / tan( fabs(roll) / SG_RADIANS_TO_DEGREES );
     turn_circum_ft = SGD_2PI * turn_radius_ft;
     dist_covered_ft = speed * 1.686 * dt; 
     alpha = dist_covered_ft / turn_circum_ft * 360.0;
     hdg += alpha * sign( roll );
     if ( hdg > 360.0 ) hdg -= 360.0;
     if ( hdg < 0.0) hdg += 360.0;
   }

   // adjust target bank angle if heading lock engaged
   if (hdg_lock) {
     double bank_sense = 0.0;
     double diff = fabs(hdg - tgt_heading);
     if (diff > 180) diff = fabs(diff - 360);
     double sum = hdg + diff;
     if (sum > 360.0) sum -= 360.0;
     if (fabs(sum - tgt_heading) < 1.0) { 
       bank_sense = 1.0;   // right turn
     } else {
       bank_sense = -1.0;  // left turn
     } 
     if (diff < 30) {
         tgt_roll = diff * bank_sense; 
     } else {
         tgt_roll = 30.0 * bank_sense;
     }
   }

   // adjust bank angle, use 9 degrees per second
   double bank_diff = tgt_roll - roll;
   if (fabs(bank_diff) > 0.2) {
     if (bank_diff > 0.0) roll += 9.0 * dt;
     if (bank_diff < 0.0) roll -= 9.0 * dt;
   }

   // adjust altitude (meters) based on current vertical speed (fpm)
   altitude += vs / 60.0 * dt;
   pos.setelev(altitude * SG_FEET_TO_METER);  
   double altitude_ft = altitude;

   // find target vertical speed if altitude lock engaged
   if (alt_lock && use_perf_vs) {
     if (altitude_ft < tgt_altitude) {
       tgt_vs = tgt_altitude - altitude_ft;
       if (tgt_vs > performance->climb_rate)
         tgt_vs = performance->climb_rate;
     } else {
       tgt_vs = tgt_altitude - altitude_ft;
       if (tgt_vs  < (-performance->descent_rate))
         tgt_vs = -performance->descent_rate;
     }
   }

   if (alt_lock && !use_perf_vs) {
     double max_vs = 2*(tgt_altitude - altitude);
     if ((fabs(tgt_altitude - altitude) < 1500.0) && 
         (fabs(max_vs) < fabs(tgt_vs))) tgt_vs = max_vs;
   }   

   // adjust vertical speed
   double vs_diff = tgt_vs - vs;
   if (fabs(vs_diff) > 10.0) {
     if (vs_diff > 0.0) {
       vs += 400.0 * dt;
       if (vs > tgt_vs) vs = tgt_vs;
     } else {
       vs -= 300.0 * dt;
       if (vs < tgt_vs) vs = tgt_vs;
     }
   }   
   
   // match pitch angle to vertical speed
   pitch = vs * 0.005;

   //###########################//
   // do calculations for radar //
   //###########################//

   // copy values from the AIManager
   double user_latitude  = manager->get_user_latitude();
   double user_longitude = manager->get_user_longitude();
   double user_altitude  = manager->get_user_altitude();
   double user_heading   = manager->get_user_heading();
   double user_pitch     = manager->get_user_pitch();
   double user_yaw       = manager->get_user_yaw();
   double user_speed     = manager->get_user_speed();

   // calculate range to target in feet and nautical miles
   double lat_range = fabs(pos.lat() - user_latitude) * ft_per_deg_lat;
   double lon_range = fabs(pos.lon() - user_longitude) * ft_per_deg_lon;
   double range_ft = sqrt( lat_range*lat_range + lon_range*lon_range );
   range = range_ft / 6076.11549;

   // calculate bearing to target
   if (pos.lat() >= user_latitude) {   
      bearing = atan2(lat_range, lon_range) * SG_RADIANS_TO_DEGREES;
        if (pos.lon() >= user_longitude) {
           bearing = 90.0 - bearing;
        } else {
           bearing = 270.0 + bearing;
        }
   } else {
      bearing = atan2(lon_range, lat_range) * SG_RADIANS_TO_DEGREES;
        if (pos.lon() >= user_longitude) {
           bearing = 180.0 - bearing;
        } else {
           bearing = 180.0 + bearing;
        }
   }

   // calculate look left/right to target, without yaw correction
   horiz_offset = bearing - user_heading;
   if (horiz_offset > 180.0) horiz_offset -= 360.0;
   if (horiz_offset < -180.0) horiz_offset += 360.0;

   // calculate elevation to target
   elevation = atan2( altitude_ft - user_altitude, range_ft )
                      * SG_RADIANS_TO_DEGREES;
   
   // calculate look up/down to target
   vert_offset = elevation + user_pitch;

/* this calculation needs to be fixed, but it isn't important anyway
   // calculate range rate
   double recip_bearing = bearing + 180.0;
   if (recip_bearing > 360.0) recip_bearing -= 360.0;
   double my_horiz_offset = recip_bearing - hdg;
   if (my_horiz_offset > 180.0) my_horiz_offset -= 360.0;
   if (my_horiz_offset < -180.0) my_horiz_offset += 360.0;
   rdot = (-user_speed * cos( horiz_offset * SG_DEGREES_TO_RADIANS ))
               + (-speed * 1.686 * cos( my_horiz_offset * SG_DEGREES_TO_RADIANS ));
*/
   
   // now correct look left/right for yaw
   horiz_offset += user_yaw;

   // calculate values for radar display
   y_shift = range * cos( horiz_offset * SG_DEGREES_TO_RADIANS);
   x_shift = range * sin( horiz_offset * SG_DEGREES_TO_RADIANS);
   rotation = hdg - user_heading;
   if (rotation < 0.0) rotation += 360.0; 

}


void FGAIAircraft::AccelTo(double speed) {
   tgt_speed = speed;
}


void FGAIAircraft::PitchTo(double angle) {
   tgt_pitch = angle;
   alt_lock = false;
}


void FGAIAircraft::RollTo(double angle) {
   tgt_roll = angle;
   hdg_lock = false; 
}


void FGAIAircraft::YawTo(double angle) {
   tgt_yaw = angle;
}


void FGAIAircraft::ClimbTo(double altitude) {
   tgt_altitude = altitude;
   alt_lock = true;
}


void FGAIAircraft::TurnTo(double heading) {
   tgt_heading = heading;
   hdg_lock = true;
}


double FGAIAircraft::sign(double x) {
  if ( x < 0.0 ) { return -1.0; }
  else { return 1.0; }
}

void FGAIAircraft::SetFlightPlan(FGAIFlightPlan *f) {
  fp = f;
}

void FGAIAircraft::ProcessFlightPlan( void ) {
  FGAIFlightPlan::waypoint* prev = 0; // the one behind you
  FGAIFlightPlan::waypoint* curr = 0; // the one ahead
  FGAIFlightPlan::waypoint* next = 0; // the next plus 1
  prev = fp->getPreviousWaypoint();
  curr = fp->getCurrentWaypoint();
  next = fp->getNextWaypoint();
  ++fp_count;

  if (!prev) {  //beginning of flightplan, do this initialization once
    fp->IncrementWaypoint();
    prev = fp->getPreviousWaypoint(); //first waypoint
    curr = fp->getCurrentWaypoint();  //second waypoint
    next = fp->getNextWaypoint();     //third waypoint (might not exist!) 
    setLatitude(prev->latitude);
    setLongitude(prev->longitude);
    setSpeed(prev->speed);
    setAltitude(prev->altitude);
    setHeading(fp->getBearing(prev->latitude, prev->longitude, curr));
    if (next) fp->setLeadDistance(speed, hdg, curr, next);
    
    if (curr->crossat > -1000.0) { //start descent/climb now
        use_perf_vs = false;
        tgt_vs = (curr->crossat - prev->altitude)/
                 (fp->getDistanceToGo(pos.lat(), pos.lon(), curr)/
                                      6076.0/prev->speed*60.0);
        tgt_altitude = curr->crossat;
    } else {
        use_perf_vs = true;
        tgt_altitude = prev->altitude;
    }
    alt_lock = hdg_lock = true;
    //cout << "First waypoint:  " << prev->name << endl;
    //cout << "  Target speed:    " << tgt_speed << endl;
    //cout << "  Target altitude: " << tgt_altitude << endl;
    //cout << "  Target heading:  " << tgt_heading << endl << endl;       
    return;  
  } // end of initialization

  // let's only process the flight plan every 11 time steps
  if (fp_count < 11) { 
    return;
  } else {
    fp_count = 0;

    // check to see if we've reached the lead point for our next turn
    double dist_to_go = fp->getDistanceToGo(pos.lat(), pos.lon(), curr); 
    double lead_dist = fp->getLeadDistance();
    if (lead_dist < (2*speed)) lead_dist = 2*speed;  //don't skip over the waypoint
    //cout << "dist_to_go: " << dist_to_go << ",  lead_dist: " << lead_dist << endl;

    if ( dist_to_go < lead_dist ) {
      if (curr->name == "END") {  //end of the flight plan, so terminate
        setDie(true);
        return;
      }
      // we've reached the lead-point for the waypoint ahead 
      if (next) tgt_heading = fp->getBearing(curr, next);  
      fp->IncrementWaypoint();
      prev = fp->getPreviousWaypoint();
      curr = fp->getCurrentWaypoint();
      next = fp->getNextWaypoint();
      if (next) fp->setLeadDistance(speed, tgt_heading, curr, next);
      if (curr->crossat > -1000.0) {
        use_perf_vs = false;
        tgt_vs = (curr->crossat - altitude)/
                 (fp->getDistanceToGo(pos.lat(), pos.lon(), curr)/6076.0/speed*60.0);
        tgt_altitude = curr->crossat;
      } else {
        use_perf_vs = true;
        tgt_altitude = prev->altitude;
      }
      tgt_speed = prev->speed;
      hdg_lock = alt_lock = true;
      //cout << "Crossing waypoint: " << prev->name << endl;
      //cout << "  Target speed:    " << tgt_speed << endl;
      //cout << "  Target altitude: " << tgt_altitude << endl;
      //cout << "  Target heading:  " << tgt_heading << endl << endl;       
    } else {
        double calc_bearing = fp->getBearing(pos.lat(), pos.lon(), curr);
        double hdg_error = calc_bearing - tgt_heading;
        if (fabs(hdg_error) > 1.0) {
          TurnTo( calc_bearing ); 
        }
    }
     
  }

}


