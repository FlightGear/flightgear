// FGAIFlightPlan - class for loading and storing  AI flight plans
// Written by David Culp, started May 2004
// - davidculp2@comcast.net
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



#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/route/waypoint.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/constants.h>
#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <simgear/props/props.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/fg_init.hxx>
#include <Airports/simple.hxx>
#include <Airports/runways.hxx>


#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

#include "AIFlightPlan.hxx"


FGAIFlightPlan::FGAIFlightPlan(string filename)
{
  int i;
  start_time = 0;
  SGPath path( globals->get_fg_root() );
  path.append( ("/Data/AI/FlightPlans/" + filename).c_str() );
  SGPropertyNode root;

  try {
      readProperties(path.str(), &root);
  } catch (const sg_exception &e) {
      SG_LOG(SG_GENERAL, SG_ALERT,
       "Error reading AI flight plan: " << path.str());
       // cout << path.str() << endl;
      return;
  }

  SGPropertyNode * node = root.getNode("flightplan");
  for (i = 0; i < node->nChildren(); i++) { 
     //cout << "Reading waypoint " << i << endl;        
     waypoint* wpt = new waypoint;
     SGPropertyNode * wpt_node = node->getChild(i);
     wpt->name      = wpt_node->getStringValue("name", "END");
     wpt->latitude  = wpt_node->getDoubleValue("lat", 0);
     wpt->longitude = wpt_node->getDoubleValue("lon", 0);
     wpt->altitude  = wpt_node->getDoubleValue("alt", 0);
     wpt->speed     = wpt_node->getDoubleValue("ktas", 0);
     wpt->crossat   = wpt_node->getDoubleValue("crossat", -10000);
     wpt->gear_down = wpt_node->getBoolValue("gear-down", false);
     wpt->flaps_down= wpt_node->getBoolValue("flaps-down", false);
     wpt->on_ground = wpt_node->getBoolValue("on-ground", false);

     if (wpt->name == "END") wpt->finished = true;
     else wpt->finished = false;

     waypoints.push_back( wpt );
   }

  wpt_iterator = waypoints.begin();
  //cout << waypoints.size() << " waypoints read." << endl;
}


// This is a modified version of the constructor,
// Which not only reads the waypoints from a 
// Flight plan file, but also adds the current
// Position computed by the traffic manager, as well
// as setting speeds and altitude computed by the
// traffic manager. 
FGAIFlightPlan::FGAIFlightPlan(FGAIModelEntity *entity,
			       double course,
			       time_t start,
			       FGAirport *dep,
			       FGAirport *arr)
{
  start_time = start;
  bool useInitialWayPoint = true;
  bool useCurrentWayPoint = false;
  SGPath path( globals->get_fg_root() );
  path.append( "/Data/AI/FlightPlans" );
  path.append( entity->path );
  SGPropertyNode root;
  
  // This is a bit of a hack:
  // Normally the value of course will be used to evaluate whether
  // or not a waypoint will be used for midair initialization of 
  // an AI aircraft. However, if a course value of 999 will be passed
  // when an update request is received, which will by definition always be
  // on the ground and should include all waypoints.
  if (course == 999) 
    {
      useInitialWayPoint = false;
      useCurrentWayPoint = true;
    }

  try {
    readProperties(path.str(), &root);
    
    SGPropertyNode * node = root.getNode("flightplan");
  
    //waypoints.push_back( init_waypoint );
    for (int i = 0; i < node->nChildren(); i++) { 
      //cout << "Reading waypoint " << i << endl;
      waypoint* wpt = new waypoint;
      SGPropertyNode * wpt_node = node->getChild(i);
      wpt->name      = wpt_node->getStringValue("name", "END");
      wpt->latitude  = wpt_node->getDoubleValue("lat", 0);
      wpt->longitude = wpt_node->getDoubleValue("lon", 0);
      wpt->altitude  = wpt_node->getDoubleValue("alt", 0);
      wpt->speed     = wpt_node->getDoubleValue("ktas", 0);
      //wpt->speed     = speed;
      wpt->crossat   = wpt_node->getDoubleValue("crossat", -10000);
      wpt->gear_down = wpt_node->getBoolValue("gear-down", false);
      wpt->flaps_down= wpt_node->getBoolValue("flaps-down", false);
      
      if (wpt->name == "END") wpt->finished = true;
      else wpt->finished = false;
      waypoints.push_back(wpt);
    }
  }
  catch (const sg_exception &e) {
    //SG_LOG(SG_GENERAL, SG_ALERT,
    // "Error reading AI flight plan: ");
    // cout << path.str() << endl;
    // cout << "Trying to create this plan dynamically" << endl;
    // cout << "Route from " << dep->id << " to " << arr->id << endl;
       create(dep,arr, entity->altitude, entity->speed);
       // Now that we have dynamically created a flight plan,
       // we need to add some code that pops any waypoints already past.
       //return;
  }
  waypoint* init_waypoint   = new waypoint;
  init_waypoint->name       = string("initial position");
  init_waypoint->latitude   = entity->latitude;
  init_waypoint->longitude  = entity->longitude;
  init_waypoint->altitude   = entity->altitude;
  init_waypoint->speed      = entity->speed;
  init_waypoint->crossat    = - 10000;
  init_waypoint->gear_down  = false;
  init_waypoint->flaps_down = false;
  init_waypoint->finished   = false;

  wpt_vector_iterator i = waypoints.begin();
  while (i != waypoints.end())
    {
      //cerr << "Checking status of each waypoint: " << (*i)->name << endl;
       SGWayPoint first(init_waypoint->longitude, 
		       init_waypoint->latitude, 
		       init_waypoint->altitude);
      SGWayPoint curr ((*i)->longitude, 
		       (*i)->latitude, 
		       (*i)->altitude);
      double crse, crsDiff;
      double dist;
      curr.CourseAndDistance(first, &crse, &dist);
      
      dist *= SG_METER_TO_NM;
      
      // We're only interested in the absolute value of crsDiff
      // wich should fall in the 0-180 deg range.
      crsDiff = fabs(crse-course);
      if (crsDiff > 180)
	crsDiff = 360-crsDiff;
      // These are the three conditions that we consider including
      // in our flight plan:
      // 1) current waypoint is less then 100 miles away OR
      // 2) curren waypoint is ahead of us, at any distance
     
      if ((dist > 20.0) && (crsDiff > 90.0) && ((*i)->name != string ("EOF")))
	{
	  //useWpt = false;
	  // Once we start including waypoints, we have to continue, even though
	  // one of the following way point would suffice. 
	  // so once is the useWpt flag is set to true, we cannot reset it to false.
	  //cerr << "Discarding waypoint: " << (*i)->name 
	  //   << ": Course difference = " << crsDiff
	  //  << "Course = " << course
	  // << "crse   = " << crse << endl;
	}
      else
	useCurrentWayPoint = true;
      
      if (useCurrentWayPoint)
	{
	  if ((dist > 100.0) && (useInitialWayPoint))
	    {
	      //waypoints.push_back(init_waypoint);;
	      waypoints.insert(i, init_waypoint);
	      //cerr << "Using waypoint : " << init_waypoint->name <<  endl;
	    }
	  //if (useInitialWayPoint)
	  // {
	  //    (*i)->speed = dist; // A hack
	  //  }
	  //waypoints.push_back( wpt );
	  //cerr << "Using waypoint : " << (*i)->name 
	  //  << ": course diff : " << crsDiff 
	  //   << "Course = " << course
	  //   << "crse   = " << crse << endl
	  //    << "distance      : " << dist << endl;
	  useInitialWayPoint = false;
	  i++;
	}
      else 
	{
	  //delete wpt;
	  delete *(i);
	  i = waypoints.erase(i);
	}
    }
  //for (i = waypoints.begin(); i != waypoints.end(); i++)
  //  cerr << "Using waypoint : " << (*i)->name << endl;
  wpt_iterator = waypoints.begin();
  //cout << waypoints.size() << " waypoints read." << endl;
}




FGAIFlightPlan::~FGAIFlightPlan()
{
  waypoints.clear();
}


FGAIFlightPlan::waypoint*
FGAIFlightPlan::getPreviousWaypoint( void )
{
  if (wpt_iterator == waypoints.begin()) {
    return 0;
  } else {
    wpt_vector_iterator prev = wpt_iterator;
    return *(--prev);
  }
}

FGAIFlightPlan::waypoint*
FGAIFlightPlan::getCurrentWaypoint( void )
{
  return *wpt_iterator;
}

FGAIFlightPlan::waypoint*
FGAIFlightPlan::getNextWaypoint( void )
{
  if (wpt_iterator == waypoints.end()) {
    return 0;
  } else {
    wpt_vector_iterator next = wpt_iterator;
    return *(++next);
  }
}

void FGAIFlightPlan::IncrementWaypoint( void )
{
  wpt_iterator++;
}

// gives distance in feet from a position to a waypoint
double FGAIFlightPlan::getDistanceToGo(double lat, double lon, waypoint* wp){
   // get size of a degree at the present latitude
   // this won't work over large distances
   double ft_per_deg_lat = 366468.96 - 3717.12 * cos(lat / SG_RADIANS_TO_DEGREES);
   double ft_per_deg_lon = 365228.16 * cos(lat / SG_RADIANS_TO_DEGREES);
   double lat_diff_ft = fabs(wp->latitude - lat) * ft_per_deg_lat;
   double lon_diff_ft = fabs(wp->longitude - lon) * ft_per_deg_lon;
   return sqrt((lat_diff_ft * lat_diff_ft) + (lon_diff_ft * lon_diff_ft));
}

// sets distance in feet from a lead point to the current waypoint
void FGAIFlightPlan::setLeadDistance(double speed, double bearing, 
                                     waypoint* current, waypoint* next){
  double turn_radius = 0.1911 * speed * speed; // an estimate for 25 degrees bank
  double inbound = bearing;
  double outbound = getBearing(current, next);
  double diff = fabs(inbound - outbound);
  if (diff > 180.0) diff = 360.0 - diff;
  lead_distance = turn_radius * sin(diff * SG_DEGREES_TO_RADIANS); 
}

void FGAIFlightPlan::setLeadDistance(double distance_ft){
  lead_distance = distance_ft;
}


double FGAIFlightPlan::getBearing(waypoint* first, waypoint* second){
  return getBearing(first->latitude, first->longitude, second);
}


double FGAIFlightPlan::getBearing(double lat, double lon, waypoint* wp){
  double course, distance;
 //  double latd = lat;
//   double lond = lon;
//   double latt = wp->latitude;
//   double lont = wp->longitude;
//   double ft_per_deg_lat = 366468.96 - 3717.12 * cos(lat/SG_RADIANS_TO_DEGREES);
//   double ft_per_deg_lon = 365228.16 * cos(lat/SG_RADIANS_TO_DEGREES);

//   if (lond < 0.0) {
//     lond+=360.0;
//     lont+=360;
//   }
//   if (lont < 0.0) {
//     lond+=360.0;
//     lont+=360.0;
//   }
//   latd+=90.0;
//   latt+=90.0;

//   double lat_diff = (latt - latd) * ft_per_deg_lat;
//   double lon_diff = (lont - lond) * ft_per_deg_lon;
//   double angle = atan(fabs(lat_diff / lon_diff)) * SG_RADIANS_TO_DEGREES;

//   bool southerly = true;
//   if (latt > latd) southerly = false;
//   bool easterly = false;
//   if (lont > lond) easterly = true;
//   if (southerly && easterly) return 90.0 + angle;
//   if (!southerly && easterly) return 90.0 - angle;
//   if (southerly && !easterly) return 270.0 - angle;
//   if (!southerly && !easterly) return 270.0 + angle; 
  SGWayPoint sgWp(wp->longitude,wp->latitude, wp->altitude, SGWayPoint::WGS84, string("temp"));
  sgWp.CourseAndDistance(lon, lat, wp->altitude, &course, &distance);
  return course;
  // Omit a compiler warning.
 
}

/* FGAIFlightPlan::create()
 * dynamically create a flight plan for AI traffic, based on data provided by the
 * Traffic Manager, when reading a filed flightplan failes. (DT, 2004/07/10) 
 *
 * Probably need to split this into separate functions for different parts of the flight

 * once the code matures a bit more.
 *
 */ 
void FGAIFlightPlan::create(FGAirport *dep, FGAirport *arr, double alt, double speed)
{
double wind_speed;
  double wind_heading;
  FGRunway rwy;
  double lat, lon, az;
  double lat2, lon2, az2;
  int direction;
  
  //waypoints.push_back(wpt);
  // Create the outbound taxi leg, for now simplified as a 
  // Direct route from the airport center point to the start
  // of the runway.
  ///////////////////////////////////////////////////////////
    //cerr << "Cruise Alt << " << alt << endl;
    // Temporary code to add some small random variation to aircraft parking positions;
  direction = (rand() % 360);
geo_direct_wgs_84 ( 0, dep->_latitude, dep->_longitude, direction, 
      100,
      &lat2, &lon2, &az2 );
  waypoint *wpt = new waypoint;
  wpt->name      = dep->_id; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = dep->_elevation + 19; // probably need to add some model height to it
  wpt->speed     = 15; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt);
  
  // Get the current active runway, based on code from David Luff
  FGEnvironment 
    stationweather = ((FGEnvironmentMgr *) globals->get_subsystem("environment"))
    ->getEnvironment(dep->_latitude, dep->_longitude, dep->_elevation);
  
  wind_speed = stationweather.get_wind_speed_kt();
  wind_heading = stationweather.get_wind_from_heading_deg();
  if (wind_speed == 0) {
    wind_heading = 270;	// This forces West-facing rwys to be used in no-wind situations
                        // which is consistent with Flightgear's initial setup.
  }
  
  string rwy_no = globals->get_runways()->search(dep->_id, int(wind_heading));
  if (!(globals->get_runways()->search(dep->_id, (int) wind_heading, &rwy )))
    {
      cout << "Failed to find runway for " << dep->_id << endl;
      // Hmm, how do we handle a potential error like this?
      exit(1);
    }

 
  double heading = rwy._heading;
  double azimuth = heading + 180.0;
  while ( azimuth >= 360.0 ) { azimuth -= 360.0; }
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		      rwy._length * SG_FEET_TO_METER * 0.5 - 5.0,
		      &lat2, &lon2, &az2 );
  
  //Add the runway startpoint;
  wpt = new waypoint;
  wpt->name      = rwy._id;
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = dep->_elevation + 19;
  wpt->speed     = 15; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt);

  //Next: The point on the runway where we begin to accelerate to take-off speed
  //100 meters down the runway seems to work. Shorter distances cause problems with
  // the turn with larger aircraft
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		      rwy._length * SG_FEET_TO_METER * 0.5 - 105.0,
		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "accel";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = dep->_elevation + 19;
  wpt->speed     = speed; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 
  
  lat = lat2;
  lon = lon2;
  az  = az2;

 //Next: the Start of Climb
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
		      2560 * SG_FEET_TO_METER,
		      &lat2, &lon2, &az2 );

  wpt = new waypoint;
  wpt->name      = "SOC";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = alt + 19;
  wpt->speed     = speed; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt); 

//Next: the Top of Climb
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
		      20*SG_NM_TO_METER,
		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "10000ft climb";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = 10000;
  wpt->speed     = speed; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt); 



  //Beginning of Decent
  stationweather = ((FGEnvironmentMgr *)globals->get_subsystem("environment"))
    ->getEnvironment(arr->_latitude, arr->_longitude, arr->_elevation);

  wind_speed = stationweather.get_wind_speed_kt();
  wind_heading = stationweather.get_wind_from_heading_deg();

  if (wind_speed == 0) {
    wind_heading = 270;	// This forces West-facing rwys to be used in no-wind situations
                        // which is consistent with Flightgear's initial setup.
  }

  rwy_no = globals->get_runways()->search(arr->_id, int(wind_heading));
  //cout << "Using runway # " << rwy_no << " for departure at " << dep->_id << endl;
  
   if (!(globals->get_runways()->search(arr->_id, (int) wind_heading, &rwy )))
    {
      cout << "Failed to find runway for " << arr->_id << endl;
      // Hmm, how do we handle a potential error like this?
      exit(1);
    }
  //cerr << "Done" << endl;
 heading = rwy._heading;
 azimuth = heading + 180.0;
 while ( azimuth >= 360.0 ) { azimuth -= 360.0; }

 

 geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		     100000,
		     &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "BOD"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = 10000;
  wpt->speed     = speed; 
  wpt->crossat   = alt +19;
  wpt->gear_down = false;
  wpt->flaps_down= false;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt); 

  // Ten thousand ft. Slowing down to 240 kts
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		     20*SG_NM_TO_METER,
		     &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "Dec 10000ft"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = arr->_elevation + 19;
  wpt->speed     = 240; 
  wpt->crossat   = 10000;
  wpt->gear_down = false;
  wpt->flaps_down= false;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt);  

  // Three thousand ft. Slowing down to 160 kts
  geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		     8*SG_NM_TO_METER,
		     &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "DEC 3000ft"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = arr->_elevation + 19;
  wpt->speed     = 160; 
  wpt->crossat   = 3000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = false;
  waypoints.push_back(wpt); 
  //Runway Threshold
 geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		     rwy._length*0.45 * SG_FEET_TO_METER,
		     &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "Threshold"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = arr->_elevation + 19;
  wpt->speed     = 15; 
  wpt->crossat   = arr->_elevation + 19;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 

 //Full stop at the runway centerpoint
 geo_direct_wgs_84 ( 0, rwy._lat, rwy._lon, azimuth, 
		     rwy._length*0.45,
		     &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "Center"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = rwy._lat;
  wpt->longitude = rwy._lon;
  wpt->altitude  = arr->_elevation + 19;
  wpt->speed     = 15; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 

direction = (rand() % 360);
geo_direct_wgs_84 ( 0, arr->_latitude, arr->_longitude, direction, 
  100,
  &lat2, &lon2, &az2 );

  // Add the final destination waypoint
  wpt = new waypoint;
  wpt->name      = arr->_id; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = arr->_elevation+19;
  wpt->speed     = 15; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  waypoints.push_back(wpt); 

  // And finally one more named "END"
  wpt = new waypoint;
  wpt->name      = "END"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = 19;
  wpt->speed     = 15; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = true;
  wpt->on_ground = true;
  waypoints.push_back(wpt);

 // And finally one more named "EOF"
  wpt = new waypoint;
  wpt->name      = "EOF"; //wpt_node->getStringValue("name", "END");
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = 19;
  wpt->speed     = 15; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = true;
  wpt->on_ground  = true;
  waypoints.push_back(wpt);
}
