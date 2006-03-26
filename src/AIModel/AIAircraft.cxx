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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/point3d.hxx>
#include <simgear/route/waypoint.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>

#include <string>
#include <math.h>
#include <time.h>
#ifdef _MSC_VER
#  include <float.h>
#  define finite _finite
#elif defined(__sun) || defined(sgi)
#  include <ieeefp.h>
#endif

SG_USING_STD(string);

#include "AIAircraft.hxx"
   static string tempReg;
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
    {7.0, 3.0, 4000.0, 2000.0, 150.0, 350.0, 500.0, 350.0, 150.0},
    // tanker
    {5.0, 2.0, 3000.0, 1500.0, 140.0, 300.0, 430.0, 300.0, 130.0}
};


FGAIAircraft::FGAIAircraft(FGAISchedule *ref) :
  FGAIBase(otAircraft) {
  trafficRef = ref;
  if (trafficRef)
    groundOffset = trafficRef->getGroundOffset();
  else
    groundOffset = 0;
   fp = 0;
   dt_count = 0;
   dt_elev_count = 0;
   use_perf_vs = true;
   isTanker = false;

   // set heading and altitude locks
   hdg_lock = false;
   alt_lock = false;
   roll = 0;
   headingChangeRate = 0.0;
}


FGAIAircraft::~FGAIAircraft() {
  //delete fp;
}

void FGAIAircraft::readFromScenario(SGPropertyNode* scFileNode) {
  if (!scFileNode)
    return;

  FGAIBase::readFromScenario(scFileNode);

  setPerformance(scFileNode->getStringValue("class", "jet_transport"));
  setFlightPlan(scFileNode->getStringValue("flightplan"),
                scFileNode->getBoolValue("repeat", false));
}

bool FGAIAircraft::init() {
   refuel_node = fgGetNode("systems/refuel/contact", true);
   return FGAIBase::init();
}

void FGAIAircraft::bind() {
    FGAIBase::bind();

    props->tie("controls/gear/gear-down",
               SGRawValueMethods<FGAIAircraft,bool>(*this,
                                              &FGAIAircraft::_getGearDown));
}

void FGAIAircraft::unbind() {
    FGAIBase::unbind();

    props->untie("controls/gear/gear-down");
}


void FGAIAircraft::update(double dt) {

   FGAIBase::update(dt);
   Run(dt);
   Transform();
}

void FGAIAircraft::setPerformance(const std::string& acclass)
{
  if (acclass == "light") {
    SetPerformance(&FGAIAircraft::settings[FGAIAircraft::LIGHT]);
  } else if (acclass == "ww2_fighter") {
    SetPerformance(&FGAIAircraft::settings[FGAIAircraft::WW2_FIGHTER]);
  } else if (acclass ==  "jet_transport") {
    SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
  } else if (acclass == "jet_fighter") {
    SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_FIGHTER]);
  } else if (acclass ==  "tanker") {
    SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
    SetTanker(true);
  } else {
    SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
  }
}

void FGAIAircraft::SetPerformance(const PERF_STRUCT *ps) {
   
   performance = ps;
} 


void FGAIAircraft::Run(double dt) {

   FGAIAircraft::dt = dt;

   if (fp) 
     {
       time_t now = time(NULL) + fgGetLong("/sim/time/warp");
       ProcessFlightPlan(dt, now);
       if (now < fp->getStartTime())
	 {
	   // Do execute Ground elev for inactive aircraft, so they
	   // Are repositioned to the correct ground altitude when the user flies within visibility range.
	   if (no_roll)
	     {
	       Transform();         // make sure aip is initialized.
	       getGroundElev(dt); // make sure it's exectuted first time around, so force a large dt value
	       //getGroundElev(dt); // Need to do this twice.
	       //cerr << trafficRef->getRegistration() << " Setting altitude to " << tgt_altitude;
	       doGroundAltitude();
	       //cerr << " Actual altitude " << altitude << endl;
	       // Transform(); 
	       pos.setelev(altitude * SG_FEET_TO_METER);
	     }
	   return;
	 }
     }
  
   double turn_radius_ft;
   double turn_circum_ft;
   double speed_north_deg_sec;
   double speed_east_deg_sec;
   double dist_covered_ft;
   double alpha;

   // adjust speed
   double speed_diff; //= tgt_speed - speed;
   if (!no_roll)
     {
       speed_diff = tgt_speed - speed;
     }
   else
     {
       speed_diff = groundTargetSpeed - speed;
     }
   if (fabs(speed_diff) > 0.2) {
     if (speed_diff > 0.0) speed += performance->accel * dt;
     if (speed_diff < 0.0) {
       if (no_roll) { // was (!no_roll) but seems more logical this way (ground brakes).
           speed -= performance->decel * dt * 3;
        } else {
           speed -= performance->decel * dt;
        }
     }
   } 
   
   // convert speed to degrees per second
   speed_north_deg_sec = cos( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lat;
   speed_east_deg_sec  = sin( hdg / SG_RADIANS_TO_DEGREES )
                          * speed * 1.686 / ft_per_deg_lon;

   // set new position
   pos.setlat( pos.lat() + speed_north_deg_sec * dt);
   pos.setlon( pos.lon() + speed_east_deg_sec * dt); 
   //if (!(finite(pos.lat()) && finite(pos.lon())))
   //  {
   //    cerr << "Position is not finite" << endl;
   //    cerr << "speed = " << speed << endl;
   //    cerr << "dt" << dt << endl;
   //    cerr << "heading " << hdg << endl;
   //    cerr << "speed east " << speed_east_deg_sec << endl;
   //    cerr << "speed nrth " << speed_north_deg_sec << endl;
   //    cerr << "deg_lat    " << ft_per_deg_lat << endl;
   //    cerr << "deg_lon    " << ft_per_deg_lon << endl;
   //  }

   // adjust heading based on current bank angle
   if (roll == 0.0) 
     roll = 0.01;
   if (roll != 0.0) {
     // double turnConstant;
     //if (no_roll)
     //  turnConstant = 0.0088362;
     //else 
     //  turnConstant = 0.088362;
     // If on ground, calculate heading change directly
     if (no_roll) {
       double headingDiff = fabs(hdg-tgt_heading);
      
       if (headingDiff > 180)
	 headingDiff = fabs(headingDiff - 360);
       groundTargetSpeed = tgt_speed - (tgt_speed * (headingDiff/45));
       if (sign(groundTargetSpeed) != sign(tgt_speed))
	 groundTargetSpeed = 0.21 * sign(tgt_speed); // to prevent speed getting stuck in 'negative' mode
       if (headingDiff > 30.0)
	 {
	   
	   headingChangeRate += dt * sign(roll); // invert if pushed backward
	   // Print some debug statements to find out why aircraft may get stuck
	   // forever turning
	   //if (trafficRef->getDepartureAirport()->getId() == string("EHAM"))
	   //  {
	   //cerr << "Turning : " << trafficRef->getRegistration()
	   //cerr <<  " Speed = " << speed << " Heading " << hdg 
	   //<< " Target Heading " << tgt_heading 
	   //	<< " Lead Distance " <<  fp->getLeadDistance()
	   //	<< " Distance to go " 
	   //	<< fp->getDistanceToGo(pos.lat(), pos.lon(), fp->getCurrentWaypoint())
	   //	<< "waypoint name " << fp->getCurrentWaypoint()->name
	   //	<< endl;
	   //}
	   if (headingChangeRate > 30) 
	     { 
	       headingChangeRate = 30;
	     }
	   else if (headingChangeRate < -30)
	     {
	       headingChangeRate = -30;
	     }
	 }
       else
	 {
	   if (fabs(headingChangeRate) > headingDiff)
	     headingChangeRate = headingDiff*sign(roll);
	   else
	     headingChangeRate += dt * sign(roll);
	 }
       hdg += headingChangeRate * dt;
       //cerr << "On ground. Heading: " << hdg << ". Target Heading: " << tgt_heading << ". Target speed: " << groundTargetSpeed << ". heading change rate" << headingChangeRate << endl;
     }
     else {
       if (fabs(speed) > 1.0) {
	 turn_radius_ft = 0.088362 * speed * speed
	   / tan( fabs(roll) / SG_RADIANS_TO_DEGREES );
       }
       else
	 {
	   turn_radius_ft = 1.0; // Check if turn_radius_ft == 0; this might lead to a division by 0.
	 }
       turn_circum_ft = SGD_2PI * turn_radius_ft;
       dist_covered_ft = speed * 1.686 * dt; 
       alpha = dist_covered_ft / turn_circum_ft * 360.0;
       hdg += alpha * sign(roll);
     }
     while ( hdg > 360.0 ) {
       hdg -= 360.0;
       spinCounter++;
     }
     while ( hdg < 0.0) 
       {
	 hdg += 360.0;
	 spinCounter--;
       }
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
     if ((fabs((double) spinCounter) > 1) && (diff > 30))
       {
	 tgt_speed *= 0.999; // Ugly hack: If aircraft get stuck, they will continually spin around.
	 // The only way to resolve this is to make them slow down. 
 	 //if (tempReg.empty())
 	 //  tempReg = trafficRef->getRegistration();
 	 //if (trafficRef->getRegistration() == tempReg)
 	 //  {
 	 //    cerr << trafficRef->getRegistration() 
	 //	  << " appears to be spinning: " << spinCounter << endl
	 //	  << " speed          " << speed << endl
	 //	  << " heading        " << hdg << endl
	 //	  << " lead distance  " << fp->getLeadDistance() << endl
	 //	  << " waypoint       " << fp->getCurrentWaypoint()->name
	 //	  << " target heading " << tgt_heading << endl
	 //	  << " lead in angle  " << fp->getLeadInAngle()<< endl
	 //	  << " roll           " << roll << endl
	 //	  << " target_roll    " << tgt_roll << endl;
	     
 	 //  }
       }
   }

   // adjust bank angle, use 9 degrees per second
   double bank_diff = tgt_roll - roll;
   if (fabs(bank_diff) > 0.2) {
     if (bank_diff > 0.0) roll += 9.0 * dt;
     if (bank_diff < 0.0) roll -= 9.0 * dt;
     //while (roll > 180) roll -= 360;
     //while (roll < 180) roll += 360;
   }

   // adjust altitude (meters) based on current vertical speed (fpm)
   altitude += vs / 60.0 * dt;
   pos.setelev(altitude * SG_FEET_TO_METER);  
   double altitude_ft = altitude;

   // adjust target Altitude, based on ground elevation when on ground
   if (no_roll)
     {
       getGroundElev(dt);
       doGroundAltitude();
     }
   else
     {
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
	 double max_vs = 4*(tgt_altitude - altitude);
	 double min_vs = 100;
	 if (tgt_altitude < altitude) min_vs = -100.0;
	 if ((fabs(tgt_altitude - altitude) < 1500.0) && 
	     (fabs(max_vs) < fabs(tgt_vs))) tgt_vs = max_vs;
	 if (fabs(tgt_vs) < fabs(min_vs)) tgt_vs = min_vs;
       }   
     }
   // adjust vertical speed
   double vs_diff = tgt_vs - vs;
   if (fabs(vs_diff) > 10.0) {
     if (vs_diff > 0.0) {
       vs += 900.0 * dt;
       if (vs > tgt_vs) vs = tgt_vs;
     } else {
       vs -= 400.0 * dt;
       if (vs < tgt_vs) vs = tgt_vs;
     }
   }   
   
   // match pitch angle to vertical speed
   if (vs > 0){
     pitch = vs * 0.005;
   } else {
     pitch = vs * 0.002;
   }

   //###########################//
   // do calculations for radar //
   //###########################//
   double range_ft2 = UpdateRadar(manager);

   //************************************//
   // Tanker code                        //
   //************************************//

   if ( isTanker) {
     if ( (range_ft2 < 250.0 * 250.0) &&
          (y_shift > 0.0)    &&
          (elevation > 0.0) ) {
       refuel_node->setBoolValue(true);
     } else {
       refuel_node->setBoolValue(false);
     } 
   }
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

void FGAIAircraft::setFlightPlan(const std::string& flightplan, bool repeat)
{
  if (!flightplan.empty()){
    FGAIFlightPlan* fp = new FGAIFlightPlan(flightplan);
    fp->setRepeat(repeat);
    SetFlightPlan(fp);
  }
}

void FGAIAircraft::SetFlightPlan(FGAIFlightPlan *f) {
  delete fp;
  fp = f;
}

void FGAIAircraft::ProcessFlightPlan( double dt, time_t now ) 
{
  bool eraseWaypoints;
  if (trafficRef)
    {
//       FGAirport *arr;
//       FGAirport *dep;
      eraseWaypoints = true;
//       cerr << trafficRef->getRegistration();
//       cerr << "Departure airport " << endl;
//       dep = trafficRef->getDepartureAirport();
//       if (dep)
// 	cerr << dep->getId() << endl;
//       cerr << "Arrival   airport " << endl;
//       arr = trafficRef->getArrivalAirport();
//       if (arr)
// 	cerr << arr->getId() <<endl << endl;;
     }
  else
    eraseWaypoints = false;

  //cerr << "Processing Flightplan" << endl;
  FGAIFlightPlan::waypoint* prev = 0; // the one behind you
  FGAIFlightPlan::waypoint* curr = 0; // the one ahead
  FGAIFlightPlan::waypoint* next = 0; // the next plus 1
  prev = fp->getPreviousWaypoint();
  curr = fp->getCurrentWaypoint();
  next = fp->getNextWaypoint();
  dt_count += dt;
  
  if (!prev) {  //beginning of flightplan, do this initialization once
    //setBank(0.0);
    spinCounter = 0;
    tempReg = "";
    //prev_dist_to_go = HUGE;
    //cerr << "Before increment " << curr-> name << endl;
    fp->IncrementWaypoint(eraseWaypoints); 
    //prev = fp->getPreviousWaypoint(); //first waypoint
    //curr = fp->getCurrentWaypoint();  //second waypoint
    //next = fp->getNextWaypoint();     //third waypoint (might not exist!) 
    //cerr << "After increment " << prev-> name << endl;
    if (!(fp->getNextWaypoint()) && trafficRef)
      {
	loadNextLeg();
      }
    //cerr << "After load " << prev-> name << endl;
    prev = fp->getPreviousWaypoint(); //first waypoint
    curr = fp->getCurrentWaypoint();  //second waypoint
    next = fp->getNextWaypoint();     //third waypoint (might not exist!) 
    //cerr << "After load " << prev-> name << endl;
    setLatitude(prev->latitude);
    setLongitude(prev->longitude);
    setSpeed(prev->speed);
    setAltitude(prev->altitude);
    if (prev->speed > 0.0)
      setHeading(fp->getBearing(prev->latitude, prev->longitude, curr));
    else
      {
	setHeading(fp->getBearing(curr->latitude, curr->longitude, prev));
      }
    // If next doesn't exist, as in incrementally created flightplans for 
    // AI/Trafficmanager created plans, 
    // Make sure lead distance is initialized otherwise
    if (next) 
      fp->setLeadDistance(speed, hdg, curr, next);
    
    if (curr->crossat > -1000.0) { //use a calculated descent/climb rate
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
    no_roll = prev->on_ground;
    if (no_roll)
      {
	Transform();         // make sure aip is initialized.
	getGroundElev(60.1); // make sure it's exectuted first time around, so force a large dt value
	//getGroundElev(60.1); // Need to do this twice.
	//cerr << trafficRef->getRegistration() << " Setting altitude to " << tgt_altitude << endl;
	doGroundAltitude(); //(tgt_altitude);
      }
    prevSpeed = 0;
    //cout << "First waypoint:  " << prev->name << endl;
    //cout << "  Target speed:    " << tgt_speed << endl;
    //cout << "  Target altitude: " << tgt_altitude << endl;
    //cout << "  Target heading:  " << tgt_heading << endl << endl;       
    //cerr << "Done Flightplan init" << endl;
    return;  
  } // end of initialization
  
  // let's only process the flight plan every 100 ms.
  if ((dt_count < 0.1) || (now < fp->getStartTime()))
    {
      //cerr  << "done fp dt" << endl;
      return;
    } else {
      dt_count = 0;
    }
  // check to see if we've reached the lead point for our next turn
  double dist_to_go = fp->getDistanceToGo(pos.lat(), pos.lon(), curr);
  
  //cerr << "2" << endl;
  double lead_dist = fp->getLeadDistance();
  //cerr << " Distance : " << dist_to_go << ": Lead distance " << lead_dist << endl;
  // experimental: Use fabs, because speed can be negative (I hope) during push_back.
  if (lead_dist < fabs(2*speed)) 
    {
      lead_dist = fabs(2*speed);  //don't skip over the waypoint
      //cerr << "Extending lead distance to " << lead_dist << endl;
    } 
//   FGAirport * apt = trafficRef->getDepartureAirport();
//   if ((dist_to_go > prev_dist_to_go) && trafficRef && apt)
//     {
//       if (apt->getId() == string("EHAM"))
// 	cerr << "Alert: " << trafficRef->getRegistration() << " is moving away from waypoint " << curr->name  << endl
// 	     << "Target heading : " << tgt_heading << "act heading " << hdg << " Tgt speed : " << tgt_speed << endl
// 	     << "Lead distance : " << lead_dist  << endl
// 	     << "Distance to go: " << dist_to_go << endl;
      
//     }
  prev_dist_to_go = dist_to_go;
  //cerr << "2" << endl;
  //if (no_roll)
  //  lead_dist = 10.0;
  //cout << "Leg : " << (fp->getLeg()-1) << ". dist_to_go: " << dist_to_go << ",  lead_dist: " << lead_dist << ", tgt_speed " << tgt_speed << ", tgt_heading " << tgt_heading << " speed " << speed << " hdg " << hdg << ". Altitude "  << altitude << " TAget alt :" << tgt_altitude << endl;
  
  if ( dist_to_go < lead_dist ) {
    //prev_dist_to_go = HUGE;
    // For traffic manager generated aircraft: 
    // check if the aircraft flies of of user range. And adjust the
    // Current waypoint's elevation according to Terrain Elevation
    if (curr->finished) {  //end of the flight plan
      {
         if (fp->getRepeat()) {
           fp->restart();
         } else {   
 	  setDie(true);
         } 

	//cerr << "Done die end of fp" << endl;
      }
      return;
    }
    
    // we've reached the lead-point for the waypoint ahead 
    //cerr << "4" << endl;
    //cerr << "Situation after lead point" << endl;
    //cerr << "Prviious: " << prev->name << endl;
    //cerr << "Current : " << curr->name << endl;
    //cerr << "Next    : " << next->name << endl;
    if (next) 
      {
	tgt_heading = fp->getBearing(curr, next);  
	spinCounter = 0;
      }
    fp->IncrementWaypoint(eraseWaypoints);
    if (!(fp->getNextWaypoint()) && trafficRef)
      {
	loadNextLeg();
      }
    prev = fp->getPreviousWaypoint();
    curr = fp->getCurrentWaypoint();
    next = fp->getNextWaypoint();
    // Now that we have incremented the waypoints, excute some traffic manager specific code
    // based on the name of the waypoint we just passed.
    if (trafficRef)
      {	
	double userLatitude  = fgGetDouble("/position/latitude-deg");
	double userLongitude = fgGetDouble("/position/longitude-deg");
	double course, distance;
	SGWayPoint current  (pos.lon(),
			     pos.lat(),
			     0);
	SGWayPoint user (   userLongitude,
			    userLatitude,
			    0);
	user.CourseAndDistance(current, &course, &distance);
	if ((distance * SG_METER_TO_NM) > TRAFFICTOAIDIST)
	  {
	    setDie(true);
	    //cerr << "done fp die out of range" << endl;
	    return;
	  }
	
	FGAirport * dep = trafficRef->getDepartureAirport();
	FGAirport * arr = trafficRef->getArrivalAirport();
	// At parking the beginning of the airport
	if (!( dep && arr))
	  {
	    setDie(true);
	    return;
	  }
	//if ((dep->getId() == string("EHAM") || (arr->getId() == string("EHAM"))))
	//  {
	//	cerr << trafficRef->getRegistration() 
	//	     << " Enroute from " << dep->getId() 
	//	     << " to "           << arr->getId()
	//	     << " just crossed " << prev->name
	//	     << " Assigned rwy     " << fp->getRunwayId()
	//	     << " " << fp->getRunway() << endl;
	// }
	//if ((dep->getId() == string("EHAM")) && (prev->name == "park2"))
	//  {
	//    cerr << "Schiphol ground " 
	//	 << trafficRef->getCallSign();
	//    if (trafficRef->getHeavy())
	//      cerr << "Heavy";
	//    cerr << ", is type " 
	//	 << trafficRef->getAircraft()
	//	 << " ready to go. IFR to "
	//	 << arr->getId() <<endl;
	//  }	
	if (prev->name == "park2")
	  {
	    dep->getDynamics()->releaseParking(fp->getGate());
	  }
	// Some debug messages, specific to testing the Logical networks.
	if ((arr->getId() == string("EHAM")) && (prev->name  == "Center"))
	  {
	    
	    cerr << "Schiphol ground " 
		 << trafficRef->getRegistration() << " "
		 << trafficRef->getCallSign();
	    if (trafficRef->getHeavy())
	      cerr << "Heavy";
	    cerr << ", arriving from " << dep->getName() ;
	    cerr << " landed runway " 
		 << fp->getRunway()
		 << " request taxi to gate " 
		 << arr->getDynamics()->getParkingName(fp->getGate()) 
		 << endl;
	  }
	if (prev->name == "END")
	  fp->setTime(trafficRef->getDepartureTime());
	//cerr << "5" << endl;
      }
    if (next) 
      {
	//cerr << "Current waypoint" << curr->name << endl;
	//cerr << "Next waypoint" << next->name << endl;
	fp->setLeadDistance(speed, tgt_heading, curr, next);
      }
    //cerr << "5.1" << endl;
    if (!(prev->on_ground)) { // only update the tgt altitude from flightplan if not on the ground
      tgt_altitude = prev->altitude;
      if (curr->crossat > -1000.0) {
	//cerr << "5.1a" << endl;
	use_perf_vs = false;
	tgt_vs = (curr->crossat - altitude)/
	  (fp->getDistanceToGo(pos.lat(), pos.lon(), curr)/6076.0/speed*60.0);
	//cerr << "5.1b" << endl;
	tgt_altitude = curr->crossat;
      } else {
	//cerr << "5.1c" << endl;
	use_perf_vs = true;
	//cerr << "5.1d" << endl;	
	
	//cerr << "Setting target altitude : " <<tgt_altitude << endl;
      }
    }
    //cerr << "6" << endl;
    tgt_speed = prev->speed;
    hdg_lock = alt_lock = true;
    no_roll = prev->on_ground;
    //cout << "Crossing waypoint: " << prev->name << endl;
    //cout << "  Target speed:    " << tgt_speed << endl;
    //cout << "  Target altitude: " << tgt_altitude << endl;
    //cout << "  Target heading:  " << tgt_heading << endl << endl;       
  } else {
    
    double calc_bearing = fp->getBearing(pos.lat(), pos.lon(), curr);
    //cerr << "Bearing = " << calc_bearing << endl;
    if (speed < 0)
      {
	calc_bearing +=180;
	if (calc_bearing > 360)
	  calc_bearing -= 360;
      }
    if (finite(calc_bearing))
      {
	double hdg_error = calc_bearing - tgt_heading;
	if (fabs(hdg_error) > 1.0) {
	  TurnTo( calc_bearing ); 
	}
      }
    else
      {
	cerr << "calc_bearing is not a finite number : "
	     << "Speed " << speed
	     << "pos : " << pos.lat() << ", " << pos.lon()
	     << "waypoint " << curr->latitude << ", " << curr->longitude << endl;
	cerr << "waypoint name " << curr->name;
	exit(1);
      }
    double speed_diff = speed - prevSpeed;
    // Update the lead distance calculation if speed has changed sufficiently
    // to prevent spinning (hopefully);
    if (fabs(speed_diff) > 10)
      { 
	prevSpeed = speed;
	fp->setLeadDistance(speed, tgt_heading, curr, next);
      }
    
    //cerr << "Done Processing FlightPlan"<< endl;
  } // if (dt count) else
}

  bool FGAIAircraft::_getGearDown() const {
   return ((props->getFloatValue("position/altitude-agl-ft") < 900.0)
            && (props->getFloatValue("velocities/airspeed-kt")
                 < performance->land_speed*1.25));
}


void FGAIAircraft::loadNextLeg() 
{
  //delete fp;
  //time_t now = time(NULL) + fgGetLong("/sim/time/warp");
  
  
  //FGAIModelEntity entity;
  //entity.m_class = "jet_transport";
  //entity.path = modelPath.c_str();
  //entity.flightplan = "none";
  //entity.latitude = _getLatitude();
  //entity.longitude = _getLongitude();
 //entity.altitude = trafficRef->getCruiseAlt() * 100; // convert from FL to feet
  //entity.speed = 450; // HACK ALERT
  //entity.fp = new FGAIFlightPlan(&entity, courseToDest, i->getDepartureTime(), dep, arr);
  int leg;
  if ((leg = fp->getLeg())  == 10)
    {
      trafficRef->next();
      leg = 1;
      fp->setLeg(leg);
      
      //cerr << "Resetting leg : " << leg << endl;
    }
  //{
  //leg++;
  //fp->setLeg(leg);
  //cerr << "Creating leg number : " << leg << endl;
  FGAirport *dep = trafficRef->getDepartureAirport();
  FGAirport *arr = trafficRef->getArrivalAirport();
  if (!(dep && arr))
    {
      setDie(true);
      //cerr << "Failed to get airport in AIAircraft::ProcessFlightplan()" << endl;
      //if (dep)
      //  cerr << "Departure " << dep->getId() << endl;
      //if (arr) 
      //  cerr << "Arrival   " << arr->getId() << endl;
    }
  else
    {
      double cruiseAlt = trafficRef->getCruiseAlt() * 100;
      //cerr << "Creating new leg using " << cruiseAlt << " as cruise altitude."<< endl;
      
      fp->create (dep,
		  arr,
		  leg,
		  cruiseAlt, //(trafficRef->getCruiseAlt() * 100), // convert from FL to feet
		  trafficRef->getSpeed(),
		  _getLatitude(),
		  _getLongitude(),
		  false,
		  trafficRef->getRadius(),
		  trafficRef->getFlightType(), 
		  acType,
		  company);
      //prev = fp->getPreviousWaypoint();
      //curr = fp->getCurrentWaypoint();
      //next = fp->getNextWaypoint();
      //cerr << "25" << endl;
      //if (next) 
      //	{
      //  //cerr << "Next waypoint" << next->name << endl;
      //	  fp->setLeadDistance(speed, tgt_heading, curr, next);
      //	}
      //cerr << "25.1" << endl;
      //if (curr->crossat > -1000.0) {
      //	//cerr << "25.1a" << endl;
      //	use_perf_vs = false;
      //	
      //	tgt_vs = (curr->crossat - altitude)/
      //	  (fp->getDistanceToGo(pos.lat(), pos.lon(), curr)/6076.0/speed*60.0);
      //	//cerr << "25.1b" << endl;
      //	tgt_altitude = curr->crossat;
      //} else {
      //	//cerr << "25.1c" << endl;
      //	use_perf_vs = true;
      //	//cerr << "25.1d" << endl;	
      //	tgt_altitude = prev->altitude;
      //	//cerr << "Setting target altitude : " <<tgt_altitude << endl;
      // }
      //cerr << "26" << endl;
      //tgt_speed = prev->speed;
      //hdg_lock = alt_lock = true;
      //no_roll = prev->on_ground;
      
    }
  //}
  //else
  //{
  //delete entity.fp;
  //entity.fp = new FGAIFlightPlan(&entity, 
  //				 999,  // A hack
  //				 trafficRef->getDepartureTime(), 
  //				 trafficRef->getDepartureAirport(), 
  //				 trafficRef->getArrivalAirport(),
  //				 false,
  //				 acType,
  //				 company);
  //SetFlightPlan(entity.fp);
}



// Note: This code is copied from David Luff's AILocalTraffic 
// Warning - ground elev determination is CPU intensive
// Either this function or the logic of how often it is called
// will almost certainly change.

void FGAIAircraft::getGroundElev(double dt) {
  dt_elev_count += dt;
  //return;
  if (dt_elev_count < (3.0) + (rand() % 10)) //Update minimally every three secs, but add some randomness to prevent all IA objects doing this in synchrony
     {
       return;
     }
   else
     {
       dt_elev_count = 0;
     }
  // It would be nice if we could set the correct tile center here in order to get a correct
  // answer with one call to the function, but what I tried in the two commented-out lines
  // below only intermittently worked, and I haven't quite groked why yet.
  //SGBucket buck(pos.lon(), pos.lat());
  //aip.getSGLocation()->set_tile_center(Point3D(buck.get_center_lon(), buck.get_center_lat(), 0.0));
  
  // Only do the proper hitlist stuff if we are within visible range of the viewer.
   if (!invisible) {
     double visibility_meters = fgGetDouble("/environment/visibility-m");	
     
     
     FGViewer* vw = globals->get_current_view();
     double 
       course, 
       distance;
     
     //Point3D currView(vw->getLongitude_deg(), 
     //		   vw->getLatitude_deg(), 0.0);
     SGWayPoint current  (pos.lon(),
			  pos.lat(),
			  0);
     SGWayPoint view (   vw->getLongitude_deg(),
			 vw->getLatitude_deg(),
			 0);
     view.CourseAndDistance(current, &course, &distance);
     if(distance > visibility_meters) {
       //aip.getSGLocation()->set_cur_elev_m(aptElev);
       return;
     }
     
     // FIXME: make sure the pos.lat/pos.lon values are in degrees ...
     double range = 500.0;
     if (!globals->get_tile_mgr()->scenery_available(pos.lat(), pos.lon(), range))
       {
	 // Try to shedule tiles for that position.
	 globals->get_tile_mgr()->update( aip.getSGLocation(), range );
       }
     
     // FIXME: make sure the pos.lat/pos.lon values are in degrees ...
     double alt;
     if (globals->get_scenery()->get_elevation_m(pos.lat(), pos.lon(),
						 20000.0, alt))
       tgt_altitude = alt * SG_METER_TO_FEET; 
     //cerr << "Target altitude : " << tgt_altitude << endl;
     // if (globals->get_scenery()->get_elevation_m(pos.lat(), pos.lon(),
     //                                            20000.0, alt))
     //    tgt_altitude = alt * SG_METER_TO_FEET;
     //cerr << "Target altitude : " << tgt_altitude << endl;
   }
}
   
void FGAIAircraft::doGroundAltitude()
{
  if (fabs(altitude - (tgt_altitude+groundOffset)) > 1000.0)
    altitude = (tgt_altitude + groundOffset);
  else
    altitude += 0.1 * ((tgt_altitude+groundOffset) - altitude);
}
