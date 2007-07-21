// runwayprefs.cxx - class implementations corresponding to runwayprefs.hxx
// assignments by the AI code
//
// Written by Durk Talsma, started January 2005.
//
// Copyright (C) 2004 Durk Talsma.
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
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>
//#include <algorithm>

#include <simgear/compiler.h>

//#include <plib/sg.h>
//#include <plib/ul.h>

//#include <Environment/environment_mgr.hxx>
//#include <Environment/environment.hxx>
//#include <simgear/misc/sg_path.hxx>
//#include <simgear/props/props.hxx>
//#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/debug/logstream.hxx>
#include <Main/globals.hxx>
//#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>

#include "runwayprefs.hxx"
#include "simple.hxx"

/******************************************************************************
 * ScheduleTime
 ***************e*************************************************************/
void ScheduleTime::clear()
{ 
  start.clear();
  end.clear();
  scheduleNames.clear();
}


ScheduleTime::ScheduleTime(const ScheduleTime &other) 
{
  //timeVec   start;
  timeVecConstIterator i;
  for (i = other.start.begin(); i != other.start.end(); i++)
    start.push_back(*i);
   for (i = other.end.begin(); i != other.end.end(); i++)
    end.push_back(*i);
   stringVecConstIterator k;
   for (k = other.scheduleNames.begin(); k != other.scheduleNames.end(); k++)
     scheduleNames.push_back(*k);
  
  //timeVec   end;
  //stringVec scheduleNames;
  tailWind = other.tailWind;
  crssWind = other.tailWind;
}


ScheduleTime & ScheduleTime::operator= (const ScheduleTime &other) 
{
  //timeVec   start;
  clear();
  timeVecConstIterator i;
  for (i = other.start.begin(); i != other.start.end(); i++)
    start.push_back(*i);
   for (i = other.end.begin(); i != other.end.end(); i++)
    end.push_back(*i);
   stringVecConstIterator k;
   for (k = other.scheduleNames.begin(); k != other.scheduleNames.end(); k++)
     scheduleNames.push_back(*k);
  
  //timeVec   end;
  //stringVec scheduleNames;
  tailWind = other.tailWind;
  crssWind = other.tailWind;
  return *this;
}
string ScheduleTime::getName(time_t dayStart)
{
  if ((start.size() != end.size()) || (start.size() != scheduleNames.size()))
    {
      SG_LOG( SG_GENERAL, SG_INFO, "Unable to parse schedule times" );
      exit(1);
    }
  else
    {
      int nrItems = start.size();
      //cerr << "Nr of items to process: " << nrItems << endl;
      if (nrItems > 0)
  	{
  	  for (unsigned int i = 0; i < start.size(); i++)
  	    {
	      //cerr << i << endl;
  	      if ((dayStart >= start[i]) && (dayStart <= end[i]))
		return scheduleNames[i];
	    }
  	}
      //couldn't find one so return 0;
      //cerr << "Returning 0 " << endl;
    }
    return string(0);
}			      
/******************************************************************************
 * RunwayList
 *****************************************************************************/

RunwayList::RunwayList(const RunwayList &other)
{
  type = other.type;
  stringVecConstIterator i;
  for (i = other.preferredRunways.begin(); i != other.preferredRunways.end(); i++)
    preferredRunways.push_back(*i);
}
RunwayList& RunwayList::operator= (const RunwayList &other)
{
  type = other.type;
  preferredRunways.clear();
  stringVecConstIterator i;
  for (i = other.preferredRunways.begin(); i != other.preferredRunways.end(); i++)
    preferredRunways.push_back(*i);
  return *this;
}
void RunwayList::set(const string &tp, const string &lst)
{
  //weekday          = atoi(timeCopy.substr(0,1).c_str());
  //    timeOffsetInDays = weekday - currTimeDate->getGmt()->tm_wday;
  //    timeCopy = timeCopy.substr(2,timeCopy.length());
  type = tp;
  string rwys = lst;
  string rwy;
  while (rwys.find(",") != string::npos)
    {
      rwy = rwys.substr(0, rwys.find(",",0));
      //cerr << "adding runway [" << rwy << "] to the list " << endl;
      preferredRunways.push_back(rwy);
      rwys.erase(0, rwys.find(",",0)+1); // erase until after the first whitspace
      while (rwys[0] == ' ')
	rwys.erase(0, 1); // Erase any leading whitespaces.
      //cerr << "Remaining runway list " << rwys;
    } 
  preferredRunways.push_back(rwys);
  //exit(1);
}

void RunwayList::clear() 
{
  type = "";
  preferredRunways.clear();
}
/****************************************************************************
 *
 ***************************************************************************/

RunwayGroup::RunwayGroup(const RunwayGroup &other)
{
  name = other.name; 
  RunwayListVecConstIterator i;
  for (i = other.rwyList.begin(); i != other.rwyList.end(); i++)
    rwyList.push_back(*i);
  choice[0] = other.choice[0];
  choice[1] = other.choice[1];
  nrActive = other.nrActive;
}
RunwayGroup& RunwayGroup:: operator= (const RunwayGroup &other)
{ 
  rwyList.clear();
  name = other.name; 
  RunwayListVecConstIterator i;
  for (i = other.rwyList.begin(); i != other.rwyList.end(); i++)
    rwyList.push_back(*i); 
  choice[0] = other.choice[0];
  choice[1] = other.choice[1];
  nrActive = other.nrActive;
  return *this;
}

void RunwayGroup::setActive(const string &aptId, 
			    double windSpeed, 
			    double windHeading, 
			    double maxTail, 
			    double maxCross,
			    stringVec *currentlyActive)
{

  FGRunway rwy;
  int activeRwys = rwyList.size(); // get the number of runways active
  int nrOfPreferences;
  // bool found = true;
  // double heading;
  double hdgDiff;
  double crossWind;
  double tailWind;
  string name;
  //stringVec names;
  int bestMatch = 0, bestChoice = 0;

  if (activeRwys > 0)
    {  
      // Now downward iterate across all the possible preferences
      // starting by the least preferred choice working toward the most preferred choice

      nrOfPreferences = rwyList[0].getRwyList()->size();
      bool validSelection = true;
      bool foundValidSelection = false;
      for (int i = nrOfPreferences-1; i >= 0; i--)
	{
	  int match = 0;
	  

	  // Test each runway listed in the preference to see if it's possible to use
	  // If one runway of the selection isn't allowed, we need to exclude this
	  // preference, however, we don't want to stop right there, because we also
	  // don't want to randomly swap runway preferences, unless there is a need to. 
	  //
	  validSelection = true;
	  for (int j = 0; j < activeRwys; j++)
	    {
	     
	      name = rwyList[j].getRwyList(i);
	      //cerr << "Name of Runway: " << name;
	      if (globals->get_runways()->search( aptId, 
						  name, 
						  &rwy))
		{
		  //cerr << "Succes" << endl;
		  hdgDiff = fabs(windHeading - rwy._heading);
		  //cerr << "Wind Heading: " << windHeading << "Runway Heading: " <<rwy._heading << endl;
		  //cerr << "Wind Speed  : " << windSpeed << endl;
		  if (hdgDiff > 180)
		    hdgDiff = 360 - hdgDiff;
		  //cerr << "Heading diff: " << hdgDiff << endl;
		  hdgDiff *= ((2*M_PI)/360.0); // convert to radians
		  crossWind = windSpeed * sin(hdgDiff);
		  tailWind  = -windSpeed * cos(hdgDiff);
		  //cerr << ". Tailwind : " << tailWind;
		  //cerr << ". Crosswnd : " << crossWind;
		  if ((tailWind > maxTail) || (crossWind > maxCross))
		    {
		      //cerr << ". [Invalid] " << endl;
		      validSelection = false;
		   }
		  else 
		    {
		      //cerr << ". [Valid] ";
		  }
		}else {
		SG_LOG( SG_GENERAL, SG_INFO, "Failed to find runway " << name << " at " << aptId );
		exit(1);
	      }
              //cerr << endl;
	    }
	  if (validSelection) 
	    {
	      //cerr << "Valid selection  : " << i << endl;;
	      foundValidSelection = true;
	      for (stringVecIterator it = currentlyActive->begin(); 
		   it != currentlyActive->end(); it++)
		{
		  if ((*it) == name)
		    match++;
		}
	      if (match >= bestMatch) {
		bestMatch = match;
		bestChoice = i;
	      }
	    } 
	  //cerr << "Preference " << i << " bestMatch " << bestMatch << " choice " << bestChoice << endl;
	}
      if (foundValidSelection)
	{
	  //cerr << "Valid runay selection : " << bestChoice << endl;
	  nrActive = activeRwys;
	  active = bestChoice;
	  return;
	}
      // If this didn't work, due to heavy winds, try again
      // but select only one landing and one takeoff runway. 
      choice[0] = 0;
      choice[1] = 0;
      for (int i = activeRwys-1;  i; i--)
	{
	  if (rwyList[i].getType() == string("landing"))
	    choice[0] = i;
	  if (rwyList[i].getType() == string("takeoff"))
	    choice[1] = i;
	}
      //cerr << "Choosing " << choice[0] << " for landing and " << choice[1] << "for takeoff" << endl;
      nrOfPreferences = rwyList[0].getRwyList()->size();
      for (int i = 0; i < nrOfPreferences; i++)
	{
	  bool validSelection = true;
	  for (int j = 0; j < 2; j++)
	    {
	      //cerr << "I J " << i << " " << j << endl;
	      name = rwyList[choice[j]].getRwyList(i);
	      //cerr << "Name of Runway: " << name << endl;
	      if (globals->get_runways()->search( aptId, 
						  name, 
						  &rwy))
		{
		  //cerr << "Succes" << endl;
		  hdgDiff = fabs(windHeading - rwy._heading);
		  if (hdgDiff > 180)
		    hdgDiff = 360 - hdgDiff;
		  hdgDiff *= ((2*M_PI)/360.0); // convert to radians
		  crossWind = windSpeed * sin(hdgDiff);
		  tailWind  = -windSpeed * cos(hdgDiff);
		  if ((tailWind > maxTail) || (crossWind > maxCross))
		    validSelection = false;
		}else {
		  SG_LOG( SG_GENERAL, SG_INFO, "Failed to find runway " << name << " at " << aptId );
		  exit(1);
		}

	    }
	  if (validSelection)
	    {
	      //cerr << "Valid runay selection : " << i << endl;
	      active = i;
	      nrActive = 2;
	      return;
	    }
	}
    }
  active = -1;
  nrActive = 0;
}

void RunwayGroup::getActive(int i, string &name, string &type)
{
  if (i == -1)
    {
      return;
    }
  if (nrActive == (int)rwyList.size())
    {
      name = rwyList[i].getRwyList(active);
      type = rwyList[i].getType();
    }
  else
    { 
      name = rwyList[choice[i]].getRwyList(active);
      type = rwyList[choice[i]].getType();
    }
}
/*****************************************************************************
 * FGRunway preference
 ****************************************************************************/
FGRunwayPreference::FGRunwayPreference(FGAirport* ap) : 
  _ap(ap)
{
  //cerr << "Running default Constructor" << endl;
  initialized = false;
}

FGRunwayPreference::FGRunwayPreference(const FGRunwayPreference &other)
{
  initialized = other.initialized;

  comTimes = other.comTimes; // Commercial Traffic;
  genTimes = other.genTimes; // General Aviation;
  milTimes = other.milTimes; // Military Traffic;

  PreferenceListConstIterator i;
  for (i = other.preferences.begin(); i != other.preferences.end(); i++)
    preferences.push_back(*i);
}
  
FGRunwayPreference & FGRunwayPreference::operator= (const FGRunwayPreference &other)
{
  initialized = other.initialized;
  
  comTimes = other.comTimes; // Commercial Traffic;
  genTimes = other.genTimes; // General Aviation;
  milTimes = other.milTimes; // Military Traffic;
  
  PreferenceListConstIterator i;
  preferences.clear();
  for (i = other.preferences.begin(); i != other.preferences.end(); i++)
    preferences.push_back(*i);
  return *this;
}

ScheduleTime *FGRunwayPreference::getSchedule(const char *trafficType)
{
  if (!(strcmp(trafficType, "com"))) {
    return &comTimes;
  }
  if (!(strcmp(trafficType, "gen"))) {
    return &genTimes;
  }
  if (!(strcmp(trafficType, "mil"))) {
    return &milTimes;
  }
  return 0;
}

RunwayGroup *FGRunwayPreference::getGroup(const string &groupName)
{
  PreferenceListIterator i = preferences.begin();
  if (preferences.begin() == preferences.end())
    return 0;
  while (!(i == preferences.end() || i->getName() == groupName))
    i++;
  if (i != preferences.end())
    return &(*i);
  else
    return 0;
}

 string FGRunwayPreference::getId() { 
   return _ap->getId(); 
 };
