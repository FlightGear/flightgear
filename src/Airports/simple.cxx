//
// simple.cxx -- a really simplistic class to manage airport ID,
//               lat, lon of the center of one of it's runways, and 
//               elevation in feet.
//
// Written by Curtis Olson, started April 1998.
// Updated by Durk Talsma, started December, 2004.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef _MSC_VER
#  define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <algorithm>

#include <simgear/compiler.h>

#include <plib/sg.h>
#include <plib/ul.h>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/debug/logstream.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>

#include STL_STRING

#include "simple.hxx"

SG_USING_STD(sort);
SG_USING_STD(random_shuffle);


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
			    double maxCross)
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

  if (activeRwys > 0)
    {
      nrOfPreferences = rwyList[0].getRwyList()->size();
      for (int i = 0; i < nrOfPreferences; i++)
	{
	  bool validSelection = true;
	  for (int j = 0; j < activeRwys; j++)
	    {
	      //cerr << "I J " << i << " " << j << endl;
	      name = rwyList[j].getRwyList(i);
	      //cerr << "Name of Runway: " << name << endl;
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
		  //cerr << "Tailwind : " << tailWind << endl;
		  //cerr << "Crosswnd : " << crossWind << endl;
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
	      nrActive = activeRwys;
	      active = i;
	      return;
	    }
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
		  //cerr << "Wind Heading: " << windHeading << "Runway Heading: " <<rwy._heading << endl;
		  //cerr << "Wind Speed  : " << windSpeed << endl;
		  if (hdgDiff > 180)
		    hdgDiff = 360 - hdgDiff;
		  //cerr << "Heading diff: " << hdgDiff << endl;
		  hdgDiff *= ((2*M_PI)/360.0); // convert to radians
		  crossWind = windSpeed * sin(hdgDiff);
		  tailWind  = -windSpeed * cos(hdgDiff);
		  //cerr << "Tailwind : " << tailWind << endl;
		  //cerr << "Crosswnd : " << crossWind << endl;
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
  //RunwayListVectorIterator i; // = rwlist.begin();
  //stringVecIterator j;
  //for (i = rwyList.begin(); i != rwyList.end(); i++)
  //  {
  //    cerr << i->getType();
  //    for (j = i->getRwyList()->begin(); j != i->getRwyList()->end(); j++)
  //	{                                 
  //	  cerr << (*j);
  //	}
  //    cerr << endl;
  //  }
  //for (int

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
FGRunwayPreference::FGRunwayPreference()
{
  //cerr << "Running default Constructor" << endl;
  initialized = false;
}

FGRunwayPreference::FGRunwayPreference(const FGRunwayPreference &other)
{
  initialized = other.initialized;
  value = other.value;
  scheduleName = other.scheduleName;

  comTimes = other.comTimes; // Commercial Traffic;
  genTimes = other.genTimes; // General Aviation;
  milTimes = other.milTimes; // Military Traffic;
  currTimes= other.currTimes; // Needed for parsing;

  rwyList = other.rwyList;
  rwyGroup = other.rwyGroup;
  PreferenceListConstIterator i;
  for (i = other.preferences.begin(); i != other.preferences.end(); i++)
    preferences.push_back(*i);
}
  
FGRunwayPreference & FGRunwayPreference::operator= (const FGRunwayPreference &other)
{
  initialized = other.initialized;
  value = other.value;
  scheduleName = other.scheduleName;
  
  comTimes = other.comTimes; // Commercial Traffic;
  genTimes = other.genTimes; // General Aviation;
  milTimes = other.milTimes; // Military Traffic;
  currTimes= other.currTimes; // Needed for parsing;
  
  rwyList = other.rwyList;
  rwyGroup = other.rwyGroup;
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

void  FGRunwayPreference::startXML () {
  //  cout << "Start XML" << endl;
}

void  FGRunwayPreference::endXML () {
  cout << "End XML" << endl;
}

void  FGRunwayPreference::startElement (const char * name, const XMLAttributes &atts) {
  //cout << "StartElement " << name << endl;
  value = string("");
  if (!(strcmp(name, "wind"))) {
    //cerr << "Will be processing Wind" << endl;
    for (int i = 0; i < atts.size(); i++)
      {
  	//cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl; 
  	//attname = atts.getName(i);
  	if (atts.getName(i) == string("tail")) {
  	  //cerr << "Tail Wind = " << atts.getValue(i) << endl;
  	  currTimes.setTailWind(atof(atts.getValue(i)));
  	}	
  	if (atts.getName(i) == string("cross")) {
  	  //cerr << "Cross Wind = " << atts.getValue(i) << endl;
  	  currTimes.setCrossWind(atof(atts.getValue(i)));
  	}
     }
  }
    if (!(strcmp(name, "time"))) {
      //cerr << "Will be processing time" << endl;	
    for (int i = 0; i < atts.size(); i++)
      {
  	if (atts.getName(i) == string("start")) {
  	  //cerr << "Start Time = " << atts.getValue(i) << endl;
  	  currTimes.addStartTime(processTime(atts.getValue(i)));
  	}
  	if (atts.getName(i) == string("end")) {
  	  //cerr << "End time = " << atts.getValue(i) << endl;
  	  currTimes.addEndTime(processTime(atts.getValue(i)));
  	}
  	if (atts.getName(i) == string("schedule")) {
  	  //cerr << "Schedule Name  = " << atts.getValue(i) << endl;
  	  currTimes.addScheduleName(atts.getValue(i));
  	}	
    }
  }
  if (!(strcmp(name, "takeoff"))) {
    rwyList.clear();
  }
  if  (!(strcmp(name, "landing")))
    {
      rwyList.clear();
    }
  if (!(strcmp(name, "schedule"))) {
    for (int i = 0; i < atts.size(); i++)
      {
  	//cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl; 
  	//attname = atts.getName(i);
  	if (atts.getName(i) == string("name")) {
  	  //cerr << "Schedule name = " << atts.getValue(i) << endl;
  	  scheduleName = atts.getValue(i);
  	}
      }
  }
}

//based on a string containing hour and minute, return nr seconds since day start.
time_t FGRunwayPreference::processTime(const string &tme)
{
  string hour   = tme.substr(0, tme.find(":",0));
  string minute = tme.substr(tme.find(":",0)+1, tme.length());

  //cerr << "hour = " << hour << " Minute = " << minute << endl;
  return (atoi(hour.c_str()) * 3600 + atoi(minute.c_str()) * 60);
}

void  FGRunwayPreference::endElement (const char * name) {
  //cout << "End element " << name << endl;
  if (!(strcmp(name, "rwyuse"))) {
    initialized = true;
  }
  if (!(strcmp(name, "com"))) { // Commercial Traffic
    //cerr << "Setting time table for commerical traffic" << endl;
    comTimes = currTimes;
    currTimes.clear();
  }
  if (!(strcmp(name, "gen"))) { // General Aviation
    //cerr << "Setting time table for general aviation" << endl;
    genTimes = currTimes;
    currTimes.clear();
  }  
  if (!(strcmp(name, "mil"))) { // Military Traffic
    //cerr << "Setting time table for military traffic" << endl;
    genTimes = currTimes;
    currTimes.clear();
  }

  if (!(strcmp(name, "takeoff"))) {
    //cerr << "Adding takeoff: " << value << endl;
    rwyList.set(name, value);
    rwyGroup.add(rwyList);
  }
  if (!(strcmp(name, "landing"))) {
    //cerr << "Adding landing: " << value << endl;
    rwyList.set(name, value);
    rwyGroup.add(rwyList);
  }
  if (!(strcmp(name, "schedule"))) {
    //cerr << "Adding schedule" << scheduleName << endl;
    rwyGroup.setName(scheduleName);
    //rwyGroup.addRunways(rwyList);
    preferences.push_back(rwyGroup);
    rwyGroup.clear();
    //exit(1);
  }
}

void  FGRunwayPreference::data (const char * s, int len) {
  string token = string(s,len);
  //cout << "Character data " << string(s,len) << endl;
  //if ((token.find(" ") == string::npos && (token.find('\n')) == string::npos))
  //  value += token;
  //else
  //  value = string("");
  value += token;
}

void  FGRunwayPreference::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void  FGRunwayPreference::warning (const char * message, int line, int column) {
  cout << "Warning: " << message << " (" << line << ',' << column << ')'   
       << endl;
}

void  FGRunwayPreference::error (const char * message, int line, int column) {
  cout << "Error: " << message << " (" << line << ',' << column << ')'
       << endl;
}

/*****************************************************************************
 * Helper function for parsing position string
 ****************************************************************************/
double processPosition(const string &pos)
{
  string prefix;
  string subs;
  string degree;
  string decimal;
  int sign = 1;
  double value;
  subs = pos;
  prefix= subs.substr(0,1);
  if (prefix == string("S") || (prefix == string("W")))
    sign = -1;
  subs    = subs.substr(1, subs.length());
  degree  = subs.substr(0, subs.find(" ",0));
  decimal = subs.substr(subs.find(" ",0), subs.length());
  
	      
  //cerr << sign << " "<< degree << " " << decimal << endl;
  value = sign * (atof(degree.c_str()) + atof(decimal.c_str())/60.0);
  //cerr << value <<endl;
  //exit(1);
  return value;
}


/*********************************************************************************
 * FGParking
 ********************************************************************************/
FGParking::FGParking(double lat,
		     double lon,
		     double hdg,
		     double rad,
		     int idx,
		     const string &name,
		     const string &tpe,
		     const string &codes)
{
  latitude     = lat;
  longitude    = lon;
  heading      = hdg;
  parkingName  = name;
  index        = idx;
  type         = tpe;
  airlineCodes = codes;
}


/***************************************************************************
 * FGAirport
 ***************************************************************************/
FGAirport::FGAirport() : _longitude(0), _latitude(0), _elevation(0)
{
  lastUpdate = 0;
  for (int i = 0; i < 10; i++)
    {
      avWindHeading [i] = 0;
      avWindSpeed   [i] = 0;
    }
}

FGAirport::FGAirport(const FGAirport& other)
{
  _id = other._id;
  _longitude = other._longitude;
  _latitude  = other._latitude;
  _elevation = other._elevation;
  _name      = other._name;
  _has_metar = other._has_metar;
  for (FGParkingVecConstIterator ip= other.parkings.begin(); ip != other.parkings.end(); ip++)
    parkings.push_back(*(ip));
  rwyPrefs = other.rwyPrefs;
  lastUpdate = other.lastUpdate;
  
  stringVecConstIterator il;
  for (il = other.landing.begin(); il != other.landing.end(); il++)
    landing.push_back(*il);
  for (il = other.takeoff.begin(); il != other.takeoff.end(); il++)
    takeoff.push_back(*il);
  lastUpdate = other.lastUpdate;
  for (int i = 0; i < 10; i++)
    {
      avWindHeading [i] = other.avWindHeading[i];
      avWindSpeed   [i] = other.avWindSpeed  [i];
    }
}

FGAirport::FGAirport(const string &id, double lon, double lat, double elev, const string &name, bool has_metar)
{
  _id = id;
  _longitude = lon;
  _latitude  = lat;
  _elevation = elev;
  _name      = name;
  _has_metar = has_metar; 
  lastUpdate = 0;
  for (int i = 0; i < 10; i++)
    {
      avWindHeading [i] = 0;
      avWindSpeed   [i] = 0;
    }
 
}

// Initialization required after XMLRead
void FGAirport::init() 
{
  // This may seem a bit weird to first randomly shuffle the parkings
  // and then sort them again. However, parkings are sorted here by ascending 
  // radius. Since many parkings have similar radii, with each radius class they will
  // still be allocated relatively systematically. Randomizing prior to sorting will
  // prevent any initial orderings to be destroyed, leading (hopefully) to a more 
  // naturalistic gate assignment. 
  random_shuffle(parkings.begin(), parkings.end());
  sort(parkings.begin(), parkings.end());
  // add the gate positions to the ground network. 
  groundNetwork.addNodes(&parkings);
  groundNetwork.init();
}

bool FGAirport::getAvailableParking(double *lat, double *lon, double *heading, int *gateId, double rad, const string &flType, const string &acType, const string &airline)
{
  bool found = false;
  bool available = false;
  //string gateType;

  FGParkingVecIterator i;
//   if (flType == "cargo")
//     {
//       gateType = "RAMP_CARGO";
//     }
//   else if (flType == "ga")
//     {
//       gateType = "RAMP_GA";
//     }
//   else gateType = "GATE";
  
  if (parkings.begin() == parkings.end())
    {
      //cerr << "Could not find parking spot at " << _id << endl;
      *lat = _latitude;
      *lon = _longitude;
      *heading = 0;
      found = true;
    }
  else
    {
      // First try finding a parking with a designated airline code
      for (i = parkings.begin(); !(i == parkings.end() || found); i++)
	{
	  //cerr << "Gate Id: " << i->getIndex()
	  //     << " Type  : " << i->getType()
	  //     << " Codes : " << i->getCodes()
	  //     << " Radius: " << i->getRadius()
	  //     << " Name  : " << i->getName()
          //     << " Available: " << i->isAvailable() << endl;
	  available = true;
	  // Taken by another aircraft
	  if (!(i->isAvailable()))
	    {
	      available = false;
	      continue;
	    }
	  // No airline codes, so skip
	  if (i->getCodes().empty())
	    {
	      available = false;
	      continue;
	    }
	  else // Airline code doesn't match
	    if (i->getCodes().find(airline, 0) == string::npos)
	      {
		available = false;
		continue;
	      }
	  // Type doesn't match
	  if (i->getType() != flType)
	    {
	      available = false;
	      continue;
	    }
	  // too small
	  if (i->getRadius() < rad)
	    {
	      available = false;
	      continue;
	    }
	  
	  if (available)
	    {
	      *lat     = i->getLatitude ();
	      *lon     = i->getLongitude();
	      *heading = i->getHeading  ();
	      *gateId  = i->getIndex    ();
	      i->setAvailable(false);
	      found = true;
	    }
	}
      // then try again for those without codes. 
      for (i = parkings.begin(); !(i == parkings.end() || found); i++)
	{
	  available = true;
	  if (!(i->isAvailable()))
	    {
	      available = false;
	      continue;
	    }
	  if (!(i->getCodes().empty()))
	    {
	      if ((i->getCodes().find(airline,0) == string::npos))
	  {
	    available = false;
	    continue;
	  }
	    }
	  if (i->getType() != flType)
	    {
	      available = false;
	      continue;
	    }
	      
	  if (i->getRadius() < rad)
	    {
	      available = false;
	      continue;
	    }
	  
	  if (available)
	    {
	      *lat     = i->getLatitude ();
	      *lon     = i->getLongitude();
	      *heading = i->getHeading  ();
	      *gateId  = i->getIndex    ();
	      i->setAvailable(false);
	      found = true;
	    }
	} 
      // And finally once more if that didn't work. Now ignore the airline codes, as a last resort
      for (i = parkings.begin(); !(i == parkings.end() || found); i++)
	{
	  available = true;
	  if (!(i->isAvailable()))
	    {
	      available = false;
	      continue;
	    }
	  if (i->getType() != flType)
	    {
	      available = false;
	      continue;
	    }
	  
	  if (i->getRadius() < rad)
	    {
	      available = false;
	      continue;
	    }
	  
	  if (available)
	    {
	      *lat     = i->getLatitude ();
	      *lon     = i->getLongitude();
	      *heading = i->getHeading  ();
	      *gateId  = i->getIndex    ();
	      i->setAvailable(false);
	      found = true;
	    }
	}
    }
  if (!found)
    {
      //cerr << "Traffic overflow at" << _id 
      //	   << ". flType = " << flType 
      //	   << ". airline = " << airline 
      //	   << " Radius = " <<rad
      //	   << endl;
      *lat = _latitude;
      *lon = _longitude;
      *heading = 0;
      *gateId  = -1;
      //exit(1);
    }
  return found;
}

void FGAirport::getParking (int id, double *lat, double* lon, double *heading)
{
  if (id < 0)
    {
      *lat = _latitude;
      *lon = _longitude;
      *heading = 0;
    }
  else
    {
      FGParkingVecIterator i = parkings.begin();
      for (i = parkings.begin(); i != parkings.end(); i++)
	{
	  if (id == i->getIndex())
	    {
	      *lat     = i->getLatitude();
	      *lon     = i->getLongitude();
	      *heading = i->getLongitude();
	    }
	}
    }
} 

FGParking *FGAirport::getParking(int i) 
{ 
  if (i < (int)parkings.size()) 
    return &(parkings[i]); 
  else 
    return 0;
}
string FGAirport::getParkingName(int i) 
{ 
  if (i < (int)parkings.size() && i >= 0) 
    return (parkings[i].getName()); 
  else 
    return string("overflow");
}
void FGAirport::releaseParking(int id)
{
  if (id >= 0)
    {
      
      FGParkingVecIterator i = parkings.begin();
      for (i = parkings.begin(); i != parkings.end(); i++)
	{
	  if (id == i->getIndex())
	    {
	      i -> setAvailable(true);
	    }
	}
    }
}
  
void  FGAirport::startXML () {
  //cout << "Start XML" << endl;
}

void  FGAirport::endXML () {
  //cout << "End XML" << endl;
}

void  FGAirport::startElement (const char * name, const XMLAttributes &atts) {
  // const char *attval;
  FGParking park;
  FGTaxiNode taxiNode;
  FGTaxiSegment taxiSegment;
  int index = 0;
  taxiSegment.setIndex(index);
  //cout << "Start element " << name << endl;
  string attname;
  string value;
  string gateName;
  string gateNumber;
  string lat;
  string lon;
  if (name == string("Parking"))
    {
      for (int i = 0; i < atts.size(); i++)
	{
	  //cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl; 
	  attname = atts.getName(i);
	  if (attname == string("index"))
	    park.setIndex(atoi(atts.getValue(i)));
	  else if (attname == string("type"))
	    park.setType(atts.getValue(i));
	 else if (attname == string("name"))
	   gateName = atts.getValue(i);
	  else if (attname == string("number"))
	    gateNumber = atts.getValue(i);
	  else if (attname == string("lat"))
	   park.setLatitude(atts.getValue(i));
	  else if (attname == string("lon"))
	    park.setLongitude(atts.getValue(i)); 
	  else if (attname == string("heading"))
	    park.setHeading(atof(atts.getValue(i)));
	  else if (attname == string("radius")) {
	    string radius = atts.getValue(i);
	    if (radius.find("M") != string::npos)
	      radius = radius.substr(0, radius.find("M",0));
	    //cerr << "Radius " << radius <<endl;
	    park.setRadius(atof(radius.c_str()));
	  }
	   else if (attname == string("airlineCodes"))
	     park.setCodes(atts.getValue(i));
	}
      park.setName((gateName+gateNumber));
      parkings.push_back(park);
    }
  if (name == string("node")) 
    {
      for (int i = 0; i < atts.size() ; i++)
	{
	  attname = atts.getName(i);
	  if (attname == string("index"))
	    taxiNode.setIndex(atoi(atts.getValue(i)));
	  if (attname == string("lat"))
	    taxiNode.setLatitude(atts.getValue(i));
	  if (attname == string("lon"))
	    taxiNode.setLongitude(atts.getValue(i));
	}
      groundNetwork.addNode(taxiNode);
    }
  if (name == string("arc")) 
    {
      taxiSegment.setIndex(++index);
      for (int i = 0; i < atts.size() ; i++)
	{
	  attname = atts.getName(i);
	  if (attname == string("begin"))
	    taxiSegment.setStartNodeRef(atoi(atts.getValue(i)));
	  if (attname == string("end"))
	    taxiSegment.setEndNodeRef(atoi(atts.getValue(i)));
	}
      groundNetwork.addSegment(taxiSegment);
    }
  // sort by radius, in asending order, so that smaller gates are first in the list
}

void  FGAirport::endElement (const char * name) {
  //cout << "End element " << name << endl;

}

void  FGAirport::data (const char * s, int len) {
  string token = string(s,len);
  //cout << "Character data " << string(s,len) << endl;
  //if ((token.find(" ") == string::npos && (token.find('\n')) == string::npos))
    //value += token;
  //else
    //value = string("");
}

void  FGAirport::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void  FGAirport::warning (const char * message, int line, int column) {
  cout << "Warning: " << message << " (" << line << ',' << column << ')'   
       << endl;
}

void  FGAirport::error (const char * message, int line, int column) {
  cout << "Error: " << message << " (" << line << ',' << column << ')'
       << endl;
}

void FGAirport::setRwyUse(const FGRunwayPreference& ref)
{
  rwyPrefs = ref;
  //cerr << "Exiting due to not implemented yet" << endl;
  //exit(1);
}
void FGAirport::getActiveRunway(const string &trafficType, int action, string &runway)
{
  double windSpeed;
  double windHeading;
  double maxTail;
  double maxCross;
  string name;
  string type;

  if (!(rwyPrefs.available()))
    {
      runway = chooseRunwayFallback();
      return; // generic fall back goes here
    }
  else
    {
      RunwayGroup *currRunwayGroup = 0;
      int nrActiveRunways = 0;
      time_t dayStart = fgGetLong("/sim/time/utc/day-seconds");
      if (((dayStart - lastUpdate) > 600) || trafficType != prevTrafficType)
	{
	  landing.clear();
	  takeoff.clear();
	  //lastUpdate = dayStart;
	  prevTrafficType = trafficType;

	  FGEnvironment 
	    stationweather = ((FGEnvironmentMgr *) globals->get_subsystem("environment"))
	    ->getEnvironment(getLatitude(), 
			     getLongitude(), 
			     getElevation());
	  
	  windSpeed = stationweather.get_wind_speed_kt();
	  windHeading = stationweather.get_wind_from_heading_deg();
	  double averageWindSpeed   = 0;
	  double averageWindHeading = 0;
	  double cosHeading         = 0;
	  double sinHeading         = 0;
	  // Initialize at the beginning of the next day or startup
	  if ((lastUpdate == 0) || (dayStart < lastUpdate))
	    {
	      for (int i = 0; i < 10; i++)
		{
		  avWindHeading [i] = windHeading;
		  avWindSpeed   [i] = windSpeed;
		}
	    }
	  else
	    {
	      if (windSpeed != avWindSpeed[9]) // update if new metar data 
		{
		  // shift the running average
		  for (int i = 0; i < 9 ; i++)
		    {
		      avWindHeading[i] = avWindHeading[i+1];
		      avWindSpeed  [i] = avWindSpeed  [i+1];
		    }
		} 
	      avWindHeading[9] = windHeading;
	      avWindSpeed  [9] = windSpeed;
	    }
	  
	  for (int i = 0; i < 10; i++)
	    {
	      averageWindSpeed   += avWindSpeed   [i];
	      //averageWindHeading += avWindHeading [i];
	      cosHeading += cos(avWindHeading[i] * SG_DEGREES_TO_RADIANS);
	      sinHeading += sin(avWindHeading[i] * SG_DEGREES_TO_RADIANS);
	    }
	  averageWindSpeed   /= 10;
	  //averageWindHeading /= 10;
	  cosHeading /= 10;
	  sinHeading /= 10;
	  averageWindHeading = atan2(sinHeading, cosHeading) *SG_RADIANS_TO_DEGREES;
	  if (averageWindHeading < 0)
	    averageWindHeading += 360.0;
	  //cerr << "Wind Heading " << windHeading << " average " << averageWindHeading << endl;
	  //cerr << "Wind Speed   " << windSpeed   << " average " << averageWindSpeed   << endl;
	  lastUpdate = dayStart;
	      //if (wind_speed == 0) {
	  //  wind_heading = 270;	 This forces West-facing rwys to be used in no-wind situations
	    // which is consistent with Flightgear's initial setup.
	  //}
	  
	  //string rwy_no = globals->get_runways()->search(apt->getId(), int(wind_heading));
	  string scheduleName;
	  //cerr << "finding active Runway for" << _id << endl;
	  //cerr << "Nr of seconds since day start << " << dayStart << endl;
	  ScheduleTime *currSched;
	  //cerr << "A"<< endl;
	  currSched = rwyPrefs.getSchedule(trafficType.c_str());
	  if (!(currSched))
	    return;   
	  //cerr << "B"<< endl;
	  scheduleName = currSched->getName(dayStart);
	  maxTail  = currSched->getTailWind  ();
	  maxCross = currSched->getCrossWind ();
	  //cerr << "SChedule anme = " << scheduleName << endl;
	  if (scheduleName.empty())
	    return;
	  //cerr << "C"<< endl;
	  currRunwayGroup = rwyPrefs.getGroup(scheduleName); 
	  //cerr << "D"<< endl;
	  if (!(currRunwayGroup))
	    return;
	  nrActiveRunways = currRunwayGroup->getNrActiveRunways();
	  //cerr << "Nr of Active Runways = " << nrActiveRunways << endl; 
	  currRunwayGroup->setActive(_id, averageWindSpeed, averageWindHeading, maxTail, maxCross); 
	  nrActiveRunways = currRunwayGroup->getNrActiveRunways();
	  for (int i = 0; i < nrActiveRunways; i++)
	    {
	      type = "unknown"; // initialize to something other than landing or takeoff
	      currRunwayGroup->getActive(i, name, type);
	      if (type == "landing")
		{
		  landing.push_back(name);
		  //cerr << "Landing " << name << endl; 
		}
	      if (type == "takeoff")
		{
		  takeoff.push_back(name);
		  //cerr << "takeoff " << name << endl;
		}
	    }
	}
      if (action == 1) // takeoff 
	{
	  int nr = takeoff.size();
	  if (nr)
	    {
	      runway = takeoff[(rand() %  nr)];
	    }
	  else
	    { // Fallback
	      runway = chooseRunwayFallback();
	    }
	} 
      if (action == 2) // landing
	{
	  int nr = landing.size();
	  if (nr)
	    {
	      runway = landing[(rand() % nr)];
	    }
	  else
	    {  //fallback
	       runway = chooseRunwayFallback();
	    }
	}
      
      //runway = globals->get_runways()->search(_id, int(windHeading));
      //cerr << "Seleceted runway: " << runway << endl;
    }
}

string FGAirport::chooseRunwayFallback()
{   
  FGEnvironment 
    stationweather = ((FGEnvironmentMgr *) globals->get_subsystem("environment"))
    ->getEnvironment(getLatitude(), 
		     getLongitude(),
		     getElevation());
  
  double windSpeed = stationweather.get_wind_speed_kt();
  double windHeading = stationweather.get_wind_from_heading_deg();
  if (windSpeed == 0) {
    windHeading = 270;	// This forces West-facing rwys to be used in no-wind situations
    //which is consistent with Flightgear's initial setup.
  }
  
   return globals->get_runways()->search(_id, int(windHeading));
}



/**************************************************************************
 * FGTaxiNode
 *************************************************************************/
FGTaxiNode::FGTaxiNode()
{
}

/***************************************************************************
 * FGTaxiSegment
 **************************************************************************/
FGTaxiSegment::FGTaxiSegment()
{
}

void FGTaxiSegment::setStart(FGTaxiNodeVector *nodes)
{
  FGTaxiNodeVectorIterator i = nodes->begin();
  while (i != nodes->end())
    {
      if (i->getIndex() == startNode)
	{
	  start = i->getAddress();
	  i->addSegment(this);
	  return;
	}
      i++;
    }
}

void FGTaxiSegment::setEnd(FGTaxiNodeVector *nodes)
{
  FGTaxiNodeVectorIterator i = nodes->begin();
  while (i != nodes->end())
    {
      if (i->getIndex() == endNode)
	{
	  end = i->getAddress();
	  return;
	}
      i++;
    }
}

// There is probably a computationally cheaper way of 
// doing this.
void FGTaxiSegment::setTrackDistance()
{
  double course;
  SGWayPoint first  (start->getLongitude(),
		     start->getLatitude(),
		     0);
  SGWayPoint second (end->getLongitude(),
		     end->getLatitude(),
		     0);
  first.CourseAndDistance(second, &course, &length);
  
}

bool FGTaxiRoute::next(int *val) 
{ 
  //for (intVecIterator i = nodes.begin(); i != nodes.end(); i++)
  //  cerr << "FGTaxiRoute contains : " << *(i) << endl;
  //cerr << "Offset from end: " << nodes.end() - currNode << endl;
  //if (currNode != nodes.end())
  //  cerr << "true" << endl;
  //else
  //  cerr << "false" << endl;
      
  if (currNode == nodes.end())
    return false;
  *val = *(currNode); 
  currNode++;
  return true;
};
/***************************************************************************
 * FGGroundNetwork()
 **************************************************************************/

FGGroundNetwork::FGGroundNetwork()
{
  hasNetwork = false;
  foundRoute = false;
  totalDistance = 0;
  maxDistance = 0;
}

void FGGroundNetwork::addSegment(const FGTaxiSegment &seg)
{
  segments.push_back(seg);
}

void FGGroundNetwork::addNode(const FGTaxiNode &node)
{
  nodes.push_back(node);
}

void FGGroundNetwork::addNodes(FGParkingVec *parkings)
{
  FGTaxiNode n;
  FGParkingVecIterator i = parkings->begin();
  while (i != parkings->end())
    {
      n.setIndex(i->getIndex());
      n.setLatitude(i->getLatitude());
      n.setLongitude(i->getLongitude());
      nodes.push_back(n);

      i++;
    }
}



void FGGroundNetwork::init()
{
  hasNetwork = true;
  FGTaxiSegmentVectorIterator i = segments.begin();
  while(i != segments.end()) {
    //cerr << "initializing node " << i->getIndex() << endl;
    i->setStart(&nodes);
    i->setEnd  (&nodes);
    i->setTrackDistance();
    //cerr << "Track distance = " << i->getLength() << endl;
    //cerr << "Track ends at"      << i->getEnd()->getIndex() << endl;
    i++;
  }
  //exit(1);
}

int FGGroundNetwork::findNearestNode(double lat, double lon)
{
  double minDist = HUGE_VAL;
  double course, dist;
  int index;
  SGWayPoint first  (lon,
		     lat,
		     0);
  
  for (FGTaxiNodeVectorIterator 
	 itr = nodes.begin();
       itr != nodes.end(); itr++)
    {
      double course;
      SGWayPoint second (itr->getLongitude(),
			 itr->getLatitude(),
			 0);
      first.CourseAndDistance(second, &course, &dist);
      if (dist < minDist)
	{
	  minDist = dist;
	  index = itr->getIndex();
	  //cerr << "Minimum distance of " << minDist << " for index " << index << endl;
	}
    }
  return index;
}

FGTaxiNode *FGGroundNetwork::findNode(int idx)
{
  for (FGTaxiNodeVectorIterator 
	 itr = nodes.begin();
       itr != nodes.end(); itr++)
    {
      if (itr->getIndex() == idx)
	return itr->getAddress();
    }
  return 0;
}

FGTaxiRoute FGGroundNetwork::findShortestRoute(int start, int end) 
{
  foundRoute = false;
  totalDistance = 0;
  FGTaxiNode *firstNode = findNode(start);
  FGTaxiNode *lastNode  = findNode(end);
  //prevNode = prevPrevNode = -1;
  //prevNode = start;
  routes.clear();
  traceStack.clear();
  trace(firstNode, end, 0, 0);
  FGTaxiRoute empty;
  
  if (!foundRoute)
    {
      SG_LOG( SG_GENERAL, SG_INFO, "Failed to find route from waypoint " << start << " to " << end );
      exit(1);
    }
  sort(routes.begin(), routes.end());
  //for (intVecIterator i = route.begin(); i != route.end(); i++)
  //  {
  //    rte->push_back(*i);
  //  }
  
  if (routes.begin() != routes.end())
    return *(routes.begin());
  else
    return empty;
}


void FGGroundNetwork::trace(FGTaxiNode *currNode, int end, int depth, double distance)
{
  traceStack.push_back(currNode->getIndex());
  totalDistance += distance;
  //cerr << "Starting trace " << depth << " total distance: " << totalDistance<< endl;
  //<< currNode->getIndex() << endl;

  // If the current route matches the required end point we found a valid route
  // So we can add this to the routing table
  if (currNode->getIndex() == end)
    {
      //cerr << "Found route : " <<  totalDistance << "" << " " << *(traceStack.end()-1) << endl;
      routes.push_back(FGTaxiRoute(traceStack,totalDistance));
      traceStack.pop_back();
      if (!(foundRoute))
	maxDistance = totalDistance;
      else
	if (totalDistance < maxDistance)
	  maxDistance = totalDistance;
      foundRoute = true;
      totalDistance -= distance;
      return;
    }
 

  // search if the currentNode has been encountered before
  // if so, we should step back one level, because it is
  // rather rediculous to proceed further from here. 
  // if the current node has not been encountered before,
  // i should point to traceStack.end()-1; and we can continue
  // if i is not traceStack.end, the previous node was found, 
  // and we should return. 
  // This only works at trace levels of 1 or higher though
  if (depth > 0) {
    intVecIterator i = traceStack.begin();
    while ((*i) != currNode->getIndex()) {
      //cerr << "Route so far : " << (*i) << endl;
      i++;
    }
    if (i != traceStack.end()-1) {
      traceStack.pop_back();
      totalDistance -= distance;
      return;
    }
    // If the total distance from start to the current waypoint
    // is longer than that of a route we can also stop this trace 
    // and go back one level. 
    if ((totalDistance > maxDistance) && foundRoute)
      {
	//cerr << "Stopping rediculously long trace: " << totalDistance << endl;
	traceStack.pop_back();
	totalDistance -= distance;
	return;
      }
  }
  
  //cerr << "2" << endl;
  if (currNode->getBeginRoute() != currNode->getEndRoute())
    {
      //cerr << "3" << endl;
      for (FGTaxiSegmentPointerVectorIterator 
	     i = currNode->getBeginRoute();
	   i != currNode->getEndRoute();
	   i++)
	{
	  //cerr << (*i)->getLenght() << endl;
	  trace((*i)->getEnd(), end, depth+1, (*i)->getLength());
	//  {
	//      // cerr << currNode -> getIndex() << " ";
	//      route.push_back(currNode->getIndex());
	//      return true;
	//    }
	}
    }
  else
    {
      SG_LOG( SG_GENERAL, SG_DEBUG, "4" );
    }
  traceStack.pop_back();
  totalDistance -= distance;
  return;
}



/******************************************************************************
 * FGAirportList
 *****************************************************************************/

// Populates a list of subdirectories of $FG_ROOT/Airports/AI so that
// the add() method doesn't have to try opening 2 XML files in each of
// thousands of non-existent directories.  FIXME: should probably add
// code to free this list after parsing of apt.dat is finished;
// non-issue at the moment, however, as there are no AI subdirectories
// in the base package.
FGAirportList::FGAirportList()
{
    ulDir* d;
    ulDirEnt* dent;
    SGPath aid( globals->get_fg_root() );
    aid.append( "/Airports/AI" );
    if((d = ulOpenDir(aid.c_str())) == NULL)
        return;
    while((dent = ulReadDir(d)) != NULL) {
        SG_LOG( SG_GENERAL, SG_DEBUG, "Dent: " << dent->d_name );
        ai_dirs.insert(dent->d_name);
    }
    ulCloseDir(d);
}


FGAirportList::~FGAirportList( void ) {
    for(unsigned int i = 0; i < airports_array.size(); ++i) {
        delete airports_array[i];
    }
}


// add an entry to the list
void FGAirportList::add( const string &id, const double longitude,
                         const double latitude, const double elevation,
                         const string &name, const bool has_metar )
{
    FGRunwayPreference rwyPrefs;
    FGAirport* a = new FGAirport(id, longitude, latitude, elevation, name, has_metar);
    SGPath parkpath( globals->get_fg_root() );
    parkpath.append( "/Airports/AI/" );
    parkpath.append(id);
    parkpath.append("parking.xml"); 
    
    SGPath rwyPrefPath( globals->get_fg_root() );
    rwyPrefPath.append( "/Airports/AI/" );
    rwyPrefPath.append(id);
    rwyPrefPath.append("rwyuse.xml");
    if (ai_dirs.find(id.c_str()) != ai_dirs.end()
        && parkpath.exists()) 
    {
        try {
            readXML(parkpath.str(),*a);
	    a->init();
        } 
        catch  (const sg_exception &e) {
            //cerr << "unable to read " << parkpath.str() << endl;
        }
    }
    if (ai_dirs.find(id.c_str()) != ai_dirs.end()
        && rwyPrefPath.exists()) 
    {
        try {
            readXML(rwyPrefPath.str(), rwyPrefs);
            a->setRwyUse(rwyPrefs);
        }
        catch  (const sg_exception &e) {
            //cerr << "unable to read " << rwyPrefPath.str() << endl;
            //exit(1);
        }
    }
    
    airports_by_id[a->getId()] = a;
    // try and read in an auxilary file
    
    airports_array.push_back( a );
    SG_LOG( SG_GENERAL, SG_BULK, "Adding " << id << " pos = " << longitude
            << ", " << latitude << " elev = " << elevation );
}


// search for the specified id
FGAirport* FGAirportList::search( const string& id) {
    airport_map_iterator itr = airports_by_id.find(id); 
    return(itr == airports_by_id.end() ? NULL : itr->second);
}


// search for first subsequent alphabetically to supplied id
const FGAirport* FGAirportList::findFirstById( const string& id, bool exact ) {
    airport_map_iterator itr;
    if(exact) {
        itr = airports_by_id.find(id);
    } else {
        itr = airports_by_id.lower_bound(id);
    }
    if(itr == airports_by_id.end()) {
        return(NULL);
    } else {
        return(itr->second);
    }
}


// search for the airport nearest the specified position
FGAirport* FGAirportList::search( double lon_deg, double lat_deg,
                                 bool with_metar ) {
    int closest = -1;
    double min_dist = 360.0;
    unsigned int i;
    for ( i = 0; i < airports_array.size(); ++i ) {
        // crude manhatten distance based on lat/lon difference
        double d = fabs(lon_deg - airports_array[i]->getLongitude())
            + fabs(lat_deg - airports_array[i]->getLatitude());
        if ( d < min_dist ) {
            if ( !with_metar || (with_metar&&airports_array[i]->getMetar()) ) {
                closest = i;
                min_dist = d;
            }
        }
    }

    return ( closest > -1 ? airports_array[closest] : NULL );
}


int
FGAirportList::size () const
{
    return airports_array.size();
}

const FGAirport *FGAirportList::getAirport( unsigned int index ) const
{
    if(index < airports_array.size()) {
        return(airports_array[index]);
    } else {
        return(NULL);
    }
}


/**
 * Mark the specified airport record as not having metar
 */
void FGAirportList::no_metar( const string &id ) {
    if(airports_by_id.find(id) != airports_by_id.end()) { 
        airports_by_id[id]->setMetar(false);
    }
}


/**
 * Mark the specified airport record as (yes) having metar
 */
void FGAirportList::has_metar( const string &id ) {
    if(airports_by_id.find(id) != airports_by_id.end()) { 
        airports_by_id[id]->setMetar(true);
    }
}
