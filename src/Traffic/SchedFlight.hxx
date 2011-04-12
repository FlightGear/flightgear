/* -*- Mode: C++ -*- *****************************************************
 * SchedFlight.hxx
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
 * ScheduledFlight is a class that is used by FlightGear's Traffic Manager
 * A scheduled flight can be assigned to a schedule, which can be assigned 
 * to an aircraft. The traffic manager decides for each schedule which
 * scheduled flight (if any) is currently active. I no scheduled flights
 * are found active, it tries to position the aircraft associated with this
 * schedule at departure airport of the next scheduled flight.
 * The class ScheduledFlight is a software implimentation of this.
 * In summary, this class stores arrival and departure information, as well
 * as some administrative data, such as the callsign of this particular
 * flight (used in future ATC scenarios), under which flight rules the 
 * flight is taking place, as well as a requested initial cruise altitude.
 * Finally, the class contains a repeat period, wich indicates after how
 * many seconds a flight should repeat in this schedule (which is usually
 * after either a day or a week). If this value is zero, this flight won't
 * repeat. 
 **************************************************************************/

#ifndef _FGSCHEDFLIGHT_HXX_
#define _FGSCHEDFLIGHT_HXX_


using namespace std;

using std::vector;


class FGScheduledFlight
{
private:
  string callsign;
  string fltRules;
  FGAirport *departurePort;
  FGAirport *arrivalPort;
  string depId;
  string arrId;
  string requiredAircraft;
  time_t departureTime;
  time_t arrivalTime;
  time_t repeatPeriod;
  int cruiseAltitude;
  
  bool initialized;
  bool available;

 
 
public:
  FGScheduledFlight();
  FGScheduledFlight(const FGScheduledFlight &other);
  //  FGScheduledFlight(const string);
  FGScheduledFlight(const string& cs,
		     const string& fr,
		     const string& depPrt,
		     const string& arrPrt,
		     int cruiseAlt,
		     const string& deptime,
		     const string& arrtime,
		     const string& rep,
                     const string& reqAC
		     );
  ~FGScheduledFlight();

  void update();
   bool initializeAirports();
  
  void adjustTime(time_t now);

  time_t getDepartureTime() { return departureTime; };
  time_t getArrivalTime  () { return arrivalTime;   };
  
  void setDepartureAirport(string port) { depId = port; };
  void setArrivalAirport  (string port) { arrId = port; };
  FGAirport *getDepartureAirport();
  FGAirport *getArrivalAirport  ();

  int getCruiseAlt() { return cruiseAltitude; };

  bool operator<(const FGScheduledFlight &other) const  
  { 
    return (departureTime < other.departureTime); 
  };
  string& getFlightRules() { return fltRules; };

  time_t processTimeString(const string& time);
  const string& getCallSign() {return callsign; };
  const string& getRequirement() { return requiredAircraft; }

  void lock() { available = false; };
  void release() { available = true; };

  bool isAvailable() { return available; };

  void setCallSign(string val)    { callsign = val; };
  void setFlightRules(string val) { fltRules = val; };
};

typedef vector<FGScheduledFlight*>           FGScheduledFlightVec;
typedef vector<FGScheduledFlight*>::iterator FGScheduledFlightVecIterator;

typedef std::map < std::string, FGScheduledFlightVec > FGScheduledFlightMap;

bool compareScheduledFlights(FGScheduledFlight *a, FGScheduledFlight *b);


#endif
