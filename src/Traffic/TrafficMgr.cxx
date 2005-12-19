/******************************************************************************
 * TrafficMGr.cxx
 * Written by Durk Talsma, started May 5, 2004.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 **************************************************************************/
 
/* This a prototype version of a top-level flight plan manager for Flightgear.
 * It parses the fgtraffic.txt file and determine for a specific time/date, 
 * where each aircraft listed in this file is at the current time. 
 * 
 * I'm currently assuming the following simplifications:
 * 1) The earth is a perfect sphere
 * 2) Each aircraft flies a perfect great circle route.
 * 3) Each aircraft flies at a constant speed (with infinite accelerations and
 *    decelerations) 
 * 4) Each aircraft leaves at exactly the departure time. 
 * 5) Each aircraft arrives at exactly the specified arrival time. 
 *
 *
 *****************************************************************************/
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>


#include <string>
#include <vector>
#include <algorithm>

#include <plib/sg.h>

#include <simgear/compiler.h>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/route/waypoint.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/xml/easyxml.hxx>

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIBase.hxx>
#include <Airports/simple.hxx>
#include <Main/fg_init.hxx>   // That's pretty ugly, but I need fgFindAirportID



#include "TrafficMgr.hxx"

SG_USING_STD(sort);
 
/******************************************************************************
 * TrafficManager
 *****************************************************************************/
FGTrafficManager::FGTrafficManager()
{
}


void FGTrafficManager::init()
{ 
  //cerr << "Initializing Schedules" << endl;
  time_t now = time(NULL) + fgGetLong("/sim/time/warp");
  currAircraft = scheduledAircraft.begin();
  while (currAircraft != scheduledAircraft.end())
    {
      if (!(currAircraft->init()))
	{
	  currAircraft=scheduledAircraft.erase(currAircraft);
	  //cerr << "Erasing " << currAircraft->getRegistration() << endl;
	}
      else 
	{
	  currAircraft++;
	}
    }
  //cerr << "Sorting by distance " << endl;
  sort(scheduledAircraft.begin(), scheduledAircraft.end());
  currAircraft = scheduledAircraft.begin();
  currAircraftClosest = scheduledAircraft.begin();
  //cerr << "Done initializing schedules" << endl;
}

void FGTrafficManager::update(double something)
{
  time_t now = time(NULL) + fgGetLong("/sim/time/warp");
  if (scheduledAircraft.size() == 0)
	  return;
  if(currAircraft == scheduledAircraft.end())
    {
      //cerr << "resetting schedule " << endl;
      currAircraft = scheduledAircraft.begin();
    }
  if (!(currAircraft->update(now)))
    {
      // after proper initialization, we shouldnt get here.
      // But let's make sure
      cerr << "Failed to update aircraft schedule in traffic manager" << endl;
    }
  currAircraft++;
}

void FGTrafficManager::release(int id)
{
  releaseList.push_back(id);
}

bool FGTrafficManager::isReleased(int id)
{
  IdListIterator i = releaseList.begin();
  while (i != releaseList.end())
    {
      if ((*i) == id)
	{
	  releaseList.erase(i);
	  return true;
	}
      i++;
    }
  return false;
}

void  FGTrafficManager::startXML () {
  //cout << "Start XML" << endl;
}

void  FGTrafficManager::endXML () {
  //cout << "End XML" << endl;
}

void  FGTrafficManager::startElement (const char * name, const XMLAttributes &atts) {
  const char * attval;
  //cout << "Start element " << name << endl;
  //FGTrafficManager temp;
  //for (int i = 0; i < atts.size(); i++)
  //  if (string(atts.getName(i)) == string("include"))
  attval = atts.getValue("include");
  if (attval != 0)
      {
	//cout << "including " << attval << endl;
	SGPath path = 
	  globals->get_fg_root();
	path.append("/Traffic/");
	path.append(attval);
	readXML(path.str(), *this);
      }
  //  cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl; 
}

void  FGTrafficManager::endElement (const char * name) {
  //cout << "End element " << name << endl;
  string element(name);
  if (element == string("model"))
    mdl = value;
  else if (element == string("livery"))
    livery = value;
  else if (element == string("registration"))
    registration = value;
  else if (element == string("airline"))
    airline = value;
  else if (element == string("actype"))
    acType = value;
  else if (element == string("flighttype"))
    flighttype = value;
  else if (element == string("radius"))
    radius = atoi(value.c_str());
  else if (element == string("offset"))
    offset = atoi(value.c_str());
  else if (element == string("performance-class"))
    m_class = value;
  else if (element == string("heavy"))
    {
      if(value == string("true"))
	heavy = true;
      else
	heavy = false;
    }
  else if (element == string("callsign"))
    callsign = value;
  else if (element == string("fltrules"))
    fltrules = value;
  else if (element == string("port"))
    port = value;
  else if (element == string("time"))
    timeString = value;
  else if (element == string("departure"))
    {
      departurePort = port;
      departureTime = timeString;
    }
  else if (element == string("cruise-alt"))
    cruiseAlt = atoi(value.c_str());
  else if (element == string("arrival"))
    {
      arrivalPort = port;
      arrivalTime = timeString;
    }
  else if (element == string("repeat"))
    repeat = value;
  else if (element == string("flight"))
    {
      // We have loaded and parsed all the information belonging to this flight
      // so we temporarily store it. 
      //cerr << "Pusing back flight " << callsign << endl;
      //cerr << callsign  <<  " " << fltrules     << " "<< departurePort << " " <<  arrivalPort << " "
      //   << cruiseAlt <<  " " << departureTime<< " "<< arrivalTime   << " " << repeat << endl;
      flights.push_back(FGScheduledFlight(callsign,
					  fltrules,
					  departurePort,
					  arrivalPort,
					  cruiseAlt,
					  departureTime,
					  arrivalTime,
					  repeat));
    }
  else if (element == string("aircraft"))
    {
      //cerr << "Pushing back aircraft " << registration << endl;
      scheduledAircraft.push_back(FGAISchedule(mdl, 
					       livery, 
					       registration, 
					       heavy,
					       acType, 
					       airline, 
					       m_class, 
					       flighttype,
					       radius,
					       offset,
					       flights));
      while(flights.begin() != flights.end())
	flights.pop_back();
    }
}

void  FGTrafficManager::data (const char * s, int len) {
  string token = string(s,len);
  //cout << "Character data " << string(s,len) << endl;
  if ((token.find(" ") == string::npos && (token.find('\n')) == string::npos))
      value += token;
  else
    value = string("");
}

void  FGTrafficManager::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void  FGTrafficManager::warning (const char * message, int line, int column) {
  cout << "Warning: " << message << " (" << line << ',' << column << ')'   
       << endl;
}

void  FGTrafficManager::error (const char * message, int line, int column) {
  cout << "Error: " << message << " (" << line << ',' << column << ')'
       << endl;
}
