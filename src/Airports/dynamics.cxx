// dynamics.cxx - Code to manage the higher order airport ground activities
// Written by Durk Talsma, started December 2004.
//
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
#include <simgear/route/waypoint.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>
#include <simgear/xml/easyxml.hxx>

#include STL_STRING
#include <vector>

SG_USING_STD(string);
SG_USING_STD(vector);
SG_USING_STD(sort);
SG_USING_STD(random_shuffle);

#include "parking.hxx"
#include "groundnetwork.hxx"
#include "runwayprefs.hxx"
#include "dynamics.hxx"

/********** FGAirport Dynamics *********************************************/

FGAirportDynamics::FGAirportDynamics(double lat, double lon, double elev, string id) :
  _latitude(lat),
  _longitude(lon),
  _elevation(elev),
  _id(id)
{
  lastUpdate = 0;
  for (int i = 0; i < 10; i++)
    {
      avWindHeading [i] = 0;
      avWindSpeed   [i] = 0;
    }
}


// Note that the ground network should also be copied
FGAirportDynamics::FGAirportDynamics(const FGAirportDynamics& other) 
{
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

// Destructor
FGAirportDynamics::~FGAirportDynamics()
{
  
}


// Initialization required after XMLRead
void FGAirportDynamics::init() 
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

bool FGAirportDynamics::getAvailableParking(double *lat, double *lon, double *heading, int *gateId, double rad, const string &flType, const string &acType, const string &airline)
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

void FGAirportDynamics::getParking (int id, double *lat, double* lon, double *heading)
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
	      *heading = i->getHeading();
	    }
	}
    }
} 

FGParking *FGAirportDynamics::getParking(int i) 
{ 
  if (i < (int)parkings.size()) 
    return &(parkings[i]); 
  else 
    return 0;
}
string FGAirportDynamics::getParkingName(int i) 
{ 
  if (i < (int)parkings.size() && i >= 0) 
    return (parkings[i].getName()); 
  else 
    return string("overflow");
}
void FGAirportDynamics::releaseParking(int id)
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
  
void  FGAirportDynamics::startXML () {
  //cout << "Start XML" << endl;
}

void  FGAirportDynamics::endXML () {
  //cout << "End XML" << endl;
}

void  FGAirportDynamics::startElement (const char * name, const XMLAttributes &atts) {
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

void  FGAirportDynamics::endElement (const char * name) {
  //cout << "End element " << name << endl;

}

void  FGAirportDynamics::data (const char * s, int len) {
  string token = string(s,len);
  //cout << "Character data " << string(s,len) << endl;
  //if ((token.find(" ") == string::npos && (token.find('\n')) == string::npos))
    //value += token;
  //else
    //value = string("");
}

void  FGAirportDynamics::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void  FGAirportDynamics::warning (const char * message, int line, int column) {
  //cout << "Warning: " << message << " (" << line << ',' << column << ')'   
  //     << endl;
}

void  FGAirportDynamics::error (const char * message, int line, int column) {
  //cout << "Error: " << message << " (" << line << ',' << column << ')'
  //     << endl;
}

void FGAirportDynamics::setRwyUse(const FGRunwayPreference& ref)
{
  rwyPrefs = ref;
  //cerr << "Exiting due to not implemented yet" << endl;
  //exit(1);
}
void FGAirportDynamics::getActiveRunway(const string &trafficType, int action, string &runway)
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
	 //  double averageWindSpeed   = 0;
// 	  double averageWindHeading = 0;
// 	  double cosHeading         = 0;
// 	  double sinHeading         = 0;
// 	  // Initialize at the beginning of the next day or startup
// 	  if ((lastUpdate == 0) || (dayStart < lastUpdate))
// 	    {
// 	      for (int i = 0; i < 10; i++)
// 		{
// 		  avWindHeading [i] = windHeading;
// 		  avWindSpeed   [i] = windSpeed;
// 		}
// 	    }
// 	  else
// 	    {
// 	      if (windSpeed != avWindSpeed[9]) // update if new metar data 
// 		{
// 		  // shift the running average
// 		  for (int i = 0; i < 9 ; i++)
// 		    {
// 		      avWindHeading[i] = avWindHeading[i+1];
// 		      avWindSpeed  [i] = avWindSpeed  [i+1];
// 		    }
// 		} 
// 	      avWindHeading[9] = windHeading;
// 	      avWindSpeed  [9] = windSpeed;
// 	    }
	  
// 	  for (int i = 0; i < 10; i++)
// 	    {
// 	      averageWindSpeed   += avWindSpeed   [i];
// 	      //averageWindHeading += avWindHeading [i];
// 	      cosHeading += cos(avWindHeading[i] * SG_DEGREES_TO_RADIANS);
// 	      sinHeading += sin(avWindHeading[i] * SG_DEGREES_TO_RADIANS);
// 	    }
// 	  averageWindSpeed   /= 10;
// 	  //averageWindHeading /= 10;
// 	  cosHeading /= 10;
// 	  sinHeading /= 10;
// 	  averageWindHeading = atan2(sinHeading, cosHeading) *SG_RADIANS_TO_DEGREES;
// 	  if (averageWindHeading < 0)
// 	    averageWindHeading += 360.0;
// 	  //cerr << "Wind Heading " << windHeading << " average " << averageWindHeading << endl;
// 	  //cerr << "Wind Speed   " << windSpeed   << " average " << averageWindSpeed   << endl;
// 	  lastUpdate = dayStart;
// 	      //if (wind_speed == 0) {
// 	  //  wind_heading = 270;	 This forces West-facing rwys to be used in no-wind situations
// 	    // which is consistent with Flightgear's initial setup.
// 	  //}
	  
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

	  // 
	  currRunwayGroup->setActive(_id, 
				     windSpeed, 
				     windHeading, 
				     maxTail, 
				     maxCross, 
				     &currentlyActive); 

	  // Note that I SHOULD keep three lists in memory, one for 
	  // general aviation, one for commercial and one for military
	  // traffic.
	  currentlyActive.clear();
	  nrActiveRunways = currRunwayGroup->getNrActiveRunways();
	  for (int i = 0; i < nrActiveRunways; i++)
	    {
	      type = "unknown"; // initialize to something other than landing or takeoff
	      currRunwayGroup->getActive(i, name, type);
	      if (type == "landing")
		{
		  landing.push_back(name);
		  currentlyActive.push_back(name);
		  //cerr << "Landing " << name << endl; 
		}
	      if (type == "takeoff")
		{
		  takeoff.push_back(name);
		  currentlyActive.push_back(name);
		  //cerr << "takeoff " << name << endl;
		}
	    }
	}
      if (action == 1) // takeoff 
	{
	  int nr = takeoff.size();
	  if (nr)
	    {
	      // Note that the randomization below, is just a placeholder to choose between
	      // multiple active runways for this action. This should be
	      // under ATC control.
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

string FGAirportDynamics::chooseRunwayFallback()
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
