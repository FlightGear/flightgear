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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 **************************************************************************/

/**************************************************************************
 * This file contains the class definitions for a (Top Level) traffic
 * manager for FlightGear. 
 **************************************************************************/

#ifndef _TRAFFICMGR_HXX_
#define _TRAFFICMGR_HXX_

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/xml/easyxml.hxx>

#include "SchedFlight.hxx"
#include "Schedule.hxx"


typedef vector<void *> IdList;
typedef vector<void *>::iterator IdListIterator;


class FGTrafficManager : public SGSubsystem, public XMLVisitor
{
private:
  ScheduleVector scheduledAircraft;
  ScheduleVectorIterator currAircraft, currAircraftClosest;
  string value;

  string mdl, livery, registration, callsign, fltrules, 
    port, timeString, departurePort, departureTime, arrivalPort, arrivalTime,
    repeat, acType, airline, m_class, flighttype;
  int cruiseAlt;
  double radius, offset;
  bool heavy;

  IdList releaseList;
    
  FGScheduledFlightVec flights;

public:
  FGTrafficManager();
  
  void init();
  void update(double time);
  void release(void *ref);
  bool isReleased(void *id);

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
