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

#include <string>
#include <vector>

using std::string;
using std::vector;
using std::sort;
using std::random_shuffle;

#include "simple.hxx"
#include "dynamics.hxx"

FGAirportDynamics::FGAirportDynamics(FGAirport* ap) :
  _ap(ap), rwyPrefs(ap) {
  lastUpdate = 0;
  for (int i = 0; i < 10; i++)
     {
       //avWindHeading [i] = 0;
       //avWindSpeed   [i] = 0;
     }
}

// Note that the ground network should also be copied
FGAirportDynamics::FGAirportDynamics(const FGAirportDynamics& other) :
  rwyPrefs(other.rwyPrefs)
{
  for (FGParkingVecConstIterator ip= other.parkings.begin(); ip != other.parkings.end(); ip++)
    parkings.push_back(*(ip));
  // rwyPrefs = other.rwyPrefs;
  lastUpdate = other.lastUpdate;
  
  stringVecConstIterator il;
  for (il = other.landing.begin(); il != other.landing.end(); il++)
    landing.push_back(*il);
  for (il = other.takeoff.begin(); il != other.takeoff.end(); il++)
    takeoff.push_back(*il);
  lastUpdate = other.lastUpdate;
  for (int i = 0; i < 10; i++)
    {
      //avWindHeading [i] = other.avWindHeading[i];
      //avWindSpeed   [i] = other.avWindSpeed  [i];
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
  groundNetwork.setTowerController(&towerController);
  groundNetwork.setParent(_ap);
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
      //cerr << "Could not find parking spot at " << _ap->getId() << endl;
      *lat = _ap->getLatitude();
      *lon = _ap->getLongitude();
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
	    {
	      //cerr << "Code = " << airline << ": Codes " << i->getCodes();
	      if (i->getCodes().find(airline, 0) == string::npos)
		{
		  available = false;
		  //cerr << "Unavailable" << endl;
		  continue;
		}
	      else
		{
		  //cerr << "Available" << endl;
		}
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
      //cerr << "Traffic overflow at" << _ap->getId() 
      //	   << ". flType = " << flType 
      //	   << ". airline = " << airline 
      //	   << " Radius = " <<rad
      //	   << endl;
      *lat = _ap->getLatitude();
      *lon = _ap->getLongitude();
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
      *lat = _ap->getLatitude();
      *lon = _ap->getLongitude();
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

FGParking *FGAirportDynamics::getParking(int id) 
{ 
    FGParkingVecIterator i = parkings.begin();
    for (i = parkings.begin(); i != parkings.end(); i++)
	{
	  if (id == i->getIndex()) {
               return &(*i);
          }
        }
    return 0;
}
string FGAirportDynamics::getParkingName(int id) 
{ 
    FGParkingVecIterator i = parkings.begin();
    for (i = parkings.begin(); i != parkings.end(); i++)
	{
	  if (id == i->getIndex()) {
               return i->getName();
          }
        }

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
      if ((abs((long)(dayStart - lastUpdate)) > 600) || trafficType != prevTrafficType)
	{
	  landing.clear();
	  takeoff.clear();
	  lastUpdate = dayStart;
	  prevTrafficType = trafficType;

	  FGEnvironment 
	    stationweather = ((FGEnvironmentMgr *) globals->get_subsystem("environment"))
	    ->getEnvironment(getLatitude(), 
			     getLongitude(), 
			     getElevation());
	  
	  windSpeed = stationweather.get_wind_speed_kt();
	  windHeading = stationweather.get_wind_from_heading_deg();
	  string scheduleName;
	  //cerr << "finding active Runway for" << _ap->getId() << endl;
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

        // Keep a history of the currently active runways, to ensure
        // that an already established selection of runways will not
        // be overridden once a more preferred selection becomes 
        // available as that can lead to random runway swapping.
	if (trafficType == "com") {
          currentlyActive = &comActive;
        } else if (trafficType == "gen") {
          currentlyActive = &genActive;
        } else if (trafficType == "mil") {
          currentlyActive = &milActive;
        } else if (trafficType == "ul") {
          currentlyActive = &ulActive;
        }
	  // 
	  currRunwayGroup->setActive(_ap->getId(), 
				     windSpeed, 
				     windHeading, 
				     maxTail, 
				     maxCross, 
				     currentlyActive); 

	  // Note that I SHOULD keep multiple lists in memory, one for 
	  // general aviation, one for commercial and one for military
	  // traffic.
	  currentlyActive->clear();
	  nrActiveRunways = currRunwayGroup->getNrActiveRunways();
          //cerr << "Choosing runway for " << trafficType << endl;
	  for (int i = 0; i < nrActiveRunways; i++)
	    {
	      type = "unknown"; // initialize to something other than landing or takeoff
	      currRunwayGroup->getActive(i, name, type);
	      if (type == "landing")
		{
		  landing.push_back(name);
		  currentlyActive->push_back(name);
		  //cerr << "Landing " << name << endl; 
		}
	      if (type == "takeoff")
		{
		  takeoff.push_back(name);
		  currentlyActive->push_back(name);
		  //cerr << "takeoff " << name << endl;
		}
	    }
          //cerr << endl;
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
      //runway = globals->get_runways()->search(_ap->getId(), int(windHeading));
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
  
   return globals->get_runways()->search(_ap->getId(), int(windHeading));
}

void FGAirportDynamics::addParking(FGParking& park) {
  parkings.push_back(park);
}

double FGAirportDynamics::getLatitude() const {
  return _ap->getLatitude();
}

double FGAirportDynamics::getLongitude() const {
  return _ap->getLongitude();
}

double FGAirportDynamics::getElevation() const {
  return _ap->getElevation();
}

const string& FGAirportDynamics::getId() const {
  return _ap->getId();
}
