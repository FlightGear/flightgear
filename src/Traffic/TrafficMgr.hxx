/* -*- Mode: C++ -*- *****************************************************
 * TrafficMgr.hxx
 * Written by Durk Talsma. Started May 5, 2004
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

/**************************************************************************
 * This file contains the class definitions for a (Top Level) traffic
 * manager for FlightGear. 
 * 
 * This is traffic manager version II. The major difference from version 
 * I is that the Flight Schedules are decoupled from the AIAircraft
 * entities. This allows for a much greater flexibility in setting up
 * Irregular schedules. Traffic Manager II also makes no longer use of .xml
 * based configuration files. 
 * 
 * Here is a step plan to achieve the goal of creating Traffic Manager II
 * 
 * 1) Read aircraft data from a simple text file, like the one provided by
 *    Gabor Toth
 * 2) Create a new database structure of SchedFlights. This new database 
 *    should not be part of the Schedule class, but of TrafficManager itself
 * 3) Each aircraft should have a list of possible Flights it can operate
 *    (i.e. airline and AC type match). 
 * 4) Aircraft processing proceeds as current. During initialization, we seek 
 *    the most urgent flight that needs to be operated 
 * 5) Modify the getNextLeg function so that the next flight is loaded smoothly.
 
 **************************************************************************/

#ifndef _TRAFFICMGR_HXX_
#define _TRAFFICMGR_HXX_

#include <set>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/propertyObject.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>

#include "SchedFlight.hxx"
#include "Schedule.hxx"


typedef std::vector<int> IdList;
typedef std::vector<int>::iterator IdListIterator;

class Heuristic
{
public:
   std::string registration;
   unsigned int runCount;
   unsigned int hits;
   unsigned int lastRun;
};

typedef std::vector<Heuristic> heuristicsVector;
typedef std::vector<Heuristic>::iterator heuristicsVectorIterator;

typedef std::map < std::string, Heuristic> HeuristicMap;
typedef HeuristicMap::iterator             HeuristicMapIterator;




class FGTrafficManager : public SGSubsystem, public XMLVisitor
{
private:
  bool inited;
  bool doingInit;
  
  ScheduleVector scheduledAircraft;
  ScheduleVectorIterator currAircraft, currAircraftClosest;
  vector<string> elementValueStack;

  // record model paths which are missing, to avoid duplicate
  // warnings when parsing traffic schedules.
  std::set<std::string> missingModels;
    
  std::string mdl, livery, registration, callsign, fltrules, 
    port, timeString, departurePort, departureTime, arrivalPort, arrivalTime,
    repeat, acType, airline, m_class, flighttype, requiredAircraft, homePort;
  int cruiseAlt;
  int score, runCount, acCounter;
  double radius, offset;
  bool heavy;

  IdList releaseList;
    
  FGScheduledFlightMap flights;

  void readTimeTableFromFile(SGPath infilename);
  void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " ");

  simgear::PropertyObject<bool> enabled, aiEnabled, realWxEnabled, metarValid;
  
  void loadHeuristics();
  
  void initStep();
  void finishInit();
  void shutdown();
  
  // during incremental init, contains the XML files still be read in
  simgear::PathList schedulesToRead;
public:
  FGTrafficManager();
  ~FGTrafficManager();
  void init();
  void update(double time);
  void release(int ref);
  bool isReleased(int id);

  FGScheduledFlightVecIterator getFirstFlight(const string &ref) { return flights[ref].begin(); }
  FGScheduledFlightVecIterator getLastFlight(const string &ref) { return flights[ref].end(); }

  void endAircraft();
  
  // Some overloaded virtual XMLVisitor members
  virtual void startXML (); 
  virtual void endXML   ();
  virtual void startElement (const char * name, const XMLAttributes &atts);
  virtual void endElement (const char * name);
  virtual void data (const char * s, int len);
  virtual void pi (const char * target, const char * data);
  virtual void warning (const char * message, int line, int column);
  virtual void error (const char * message, int line, int column);
};

#endif
