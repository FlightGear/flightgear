/* -*- Mode: C++ -*- *****************************************************
 * Schedule.hxx
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
 * This file contains the definition of the class Shedule.
 *
 * A schedule is basically a number of scheduled flights, wich can be
 * assigned to an AI aircraft. 
 **************************************************************************/

#ifndef _FGSCHEDULE_HXX_
#define _FGSCHEDULE_HXX_

#define TRAFFICTOAIDIST 150.0


class FGAISchedule
{
 private:
  string modelPath;
  string livery;
  string registration;
  string airline;
  string acType;
  string m_class;
  string flightType;
  bool heavy;
  FGScheduledFlightVec flights;
  float lat;
  float lon; 
  double radius;
  double groundOffset;
  double distanceToUser;
  int AIManagerRef;
  int score;
  bool firstRun;


 public:
  FGAISchedule();                                           // constructor
  FGAISchedule(string, string, string, bool, string, string, string, string, double, double, 
	       int, FGScheduledFlightVec);                  // construct & init
  FGAISchedule(const FGAISchedule &other);                  // copy constructor



  ~FGAISchedule(); //destructor

  bool update(time_t now);
  bool init();

  double getSpeed         ();
  //void setClosestDistanceToUser();
  void next();   // forces the schedule to move on to the next flight.

  time_t      getDepartureTime    () { return (*flights.begin())->getDepartureTime   (); };
  FGAirport * getDepartureAirport () { return (*flights.begin())->getDepartureAirport(); };
  FGAirport * getArrivalAirport   () { return (*flights.begin())->getArrivalAirport  (); };
  int         getCruiseAlt        () { return (*flights.begin())->getCruiseAlt       (); };
  double      getRadius           () { return radius; };
  double      getGroundOffset     () { return groundOffset;};
  const string& getFlightType     () { return flightType;};
  const string& getAirline        () { return airline; };
  const string& getAircraft       () { return acType; };
  const string& getCallSign       () { return (*flights.begin())->getCallSign (); };
  const string& getRegistration   () { return registration;};
  const string& getFlightRules    () { return (*flights.begin())->getFlightRules (); };
  bool getHeavy                   () { return heavy; };
  // used to sort in decending order of score: I've probably found a better way to
  // decending order sorting, but still need to test that.
  bool operator< (const FGAISchedule &other) const { return (score > other.score); };
  //void * getAiRef                 () { return AIManagerRef; };
  //FGAISchedule* getAddress        () { return this;};

};

typedef vector<FGAISchedule*>           ScheduleVector;
typedef vector<FGAISchedule*>::iterator ScheduleVectorIterator;

bool compareSchedules(FGAISchedule*a, FGAISchedule*b);

#endif

