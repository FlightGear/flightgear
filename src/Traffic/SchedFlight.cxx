/******************************************************************************
 * SchedFlight.cxx
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
 * TODO:
 * - Check the code for known portability issues
 *
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>


#include <string>
#include <vector>

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/xml/easyxml.hxx>

#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <Airports/simple.hxx>
#include <Main/fg_init.hxx>   // That's pretty ugly, but I need fgFindAirportID



#include <Main/globals.hxx>

#include "SchedFlight.hxx"


/******************************************************************************
 * FGScheduledFlight stuff
 *****************************************************************************/

FGScheduledFlight::FGScheduledFlight()
{
    departureTime  = 0;
    arrivalTime    = 0;
    cruiseAltitude = 0;
    repeatPeriod   = 0;
    initialized = false;
    available = true;
}
  
FGScheduledFlight::FGScheduledFlight(const FGScheduledFlight &other)
{
  callsign          = other.callsign;
  fltRules          = other.fltRules;
  departurePort     = other.departurePort;
  depId             = other.depId;
  arrId             = other.arrId;
  departureTime     = other.departureTime;
  cruiseAltitude    = other.cruiseAltitude;
  arrivalPort       = other.arrivalPort;
  arrivalTime       = other.arrivalTime;
  repeatPeriod      = other.repeatPeriod;
  initialized       = other.initialized;
  requiredAircraft  = other.requiredAircraft;
  available         = other.available;
}

FGScheduledFlight::FGScheduledFlight(const string& cs,
		   const string& fr,
		   const string& depPrt,
		   const string& arrPrt,
		   int cruiseAlt,
		   const string& deptime,
		   const string& arrtime,
		   const string& rep,
                   const string& reqAC)
{
  callsign          = cs;
  fltRules          = fr;
  //departurePort.setId(depPrt);
  //arrivalPort.setId(arrPrt);
  depId = depPrt;
  arrId = arrPrt;
  //cerr << "Constructor: departure " << depId << ". arrival " << arrId << endl;
  //departureTime     = processTimeString(deptime);
  //arrivalTime       = processTimeString(arrtime);
  cruiseAltitude    = cruiseAlt;
  requiredAircraft  = reqAC;

  // Process the repeat period string
  if (rep.find("WEEK",0) != string::npos)
    {
      repeatPeriod = 7*24*60*60; // in seconds
    }
  else if (rep.find("Hr", 0) != string::npos)
    {
      repeatPeriod = 60*60*atoi(rep.substr(0,2).c_str());
    }
  else
    {
      repeatPeriod = 365*24*60*60;
      SG_LOG( SG_GENERAL, SG_ALERT, "Unknown repeat period in flight plan "
                                    "of flight '" << cs << "': " << rep );
    }

  // What we still need to do is preprocess the departure and
  // arrival times. 
  departureTime = processTimeString(deptime);
  arrivalTime   = processTimeString(arrtime);
  //departureTime += rand() % 300; // Make sure departure times are not limited to 5 minute increments.
  if (departureTime > arrivalTime)
    {
      departureTime -= repeatPeriod;
    }
  initialized = false;
  available   = true;
}


FGScheduledFlight:: ~FGScheduledFlight()
{
}

time_t FGScheduledFlight::processTimeString(const string& theTime)
{
  int weekday;
  int timeOffsetInDays;
  int targetHour;
  int targetMinute;
  int targetSecond;

  tm targetTimeDate;
  SGTime* currTimeDate = globals->get_time_params();
 
  string timeCopy = theTime;


  // okay first split theTime string into
  // weekday, hour, minute, second;
  // Check if a week day is specified 
  if (timeCopy.find("/",0) != string::npos)
    {
      weekday          = atoi(timeCopy.substr(0,1).c_str());
      timeOffsetInDays = weekday - currTimeDate->getGmt()->tm_wday;
      timeCopy = timeCopy.substr(2,timeCopy.length());
    }
  else 
    {
      timeOffsetInDays = 0;
    }
  // TODO: verify status of each token.
  targetHour   = atoi(timeCopy.substr(0,2).c_str());
  targetMinute = atoi(timeCopy.substr(3,5).c_str());
  targetSecond = atoi(timeCopy.substr(6,8).c_str());
  targetTimeDate.tm_year  = currTimeDate->getGmt()->tm_year;
  targetTimeDate.tm_mon   = currTimeDate->getGmt()->tm_mon;
  targetTimeDate.tm_mday  = currTimeDate->getGmt()->tm_mday;
  targetTimeDate.tm_hour  = targetHour;
  targetTimeDate.tm_min   = targetMinute;
  targetTimeDate.tm_sec   = targetSecond;

  time_t processedTime = sgTimeGetGMT(&targetTimeDate);
  processedTime += timeOffsetInDays*24*60*60;
  if (processedTime < currTimeDate->get_cur_time())
    {
      processedTime += repeatPeriod;
    }
  //tm *temp = currTimeDate->getGmt();
  //char buffer[512];
  //sgTimeFormatTime(&targetTimeDate, buffer);
  //cout << "Scheduled Time " << buffer << endl; 
  //cout << "Time :" << time(NULL) << " SGTime : " << sgTimeGetGMT(temp) << endl;
  return processedTime;
}

void FGScheduledFlight::update()
{
  departureTime += repeatPeriod;
  arrivalTime  += repeatPeriod;
}

void FGScheduledFlight::adjustTime(time_t now)
{
  //cerr << "1: Adjusting schedule please wait: " << now 
  //   << " " << arrivalTime << " " << arrivalTime+repeatPeriod << endl;
  // Make sure that the arrival time is in between 
  // the current time and the next repeat period.
  while ((arrivalTime < now) || (arrivalTime > now+repeatPeriod))
    {
      if (arrivalTime < now)
	{
	  departureTime += repeatPeriod;
	  arrivalTime   += repeatPeriod;
	}
      else if (arrivalTime > now+repeatPeriod)
	{
	  departureTime -= repeatPeriod;
	  arrivalTime   -= repeatPeriod;
	}
      //      cerr << "2: Adjusting schedule please wait: " << now 
      //   << " " << arrivalTime << " " << arrivalTime+repeatPeriod << endl;
    }
}


FGAirport *FGScheduledFlight::getDepartureAirport()
{
  if (!(initialized))
    {
      initializeAirports();
    }
  if (initialized)
    return departurePort;
  else
    return 0;
}
FGAirport * FGScheduledFlight::getArrivalAirport  ()
{
   if (!(initialized))
    {
      initializeAirports();
    }
   if (initialized)
     return arrivalPort;
   else
     return 0;
}

// Upon the first time of requesting airport information
// for this scheduled flight, these data need to be
// looked up in the main FlightGear database. 
// Missing or bogus Airport codes are currently ignored,
// but we should improve that. The best idea is probably to cancel
// this flight entirely by removing it from the schedule, if one
// of the airports cannot be found. 
bool FGScheduledFlight::initializeAirports()
{
  //cerr << "Initializing using : " << depId << " " << arrId << endl;
  departurePort = FGAirport::findByIdent(depId);
  if(departurePort == NULL)
    {
      SG_LOG( SG_GENERAL, SG_DEBUG, "Traffic manager could not find departure airport : " << depId);
      return false;
    }
  arrivalPort = FGAirport::findByIdent(arrId);
  if(arrivalPort == NULL)
    {
      SG_LOG( SG_GENERAL, SG_DEBUG, "Traffic manager could not find arrival airport   : " << arrId);
      return false;
    }

  //cerr << "Found : " << departurePort->getId() << endl;
  //cerr << "Found : " << arrivalPort->getId() << endl;
  initialized = true;
  return true;
}


bool compareScheduledFlights(FGScheduledFlight *a, FGScheduledFlight *b) 
{ 
  return (*a) < (*b); 
};
