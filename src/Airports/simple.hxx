// simple.hxx -- a really simplistic class to manage airport ID,
//                 lat, lon of the center of one of it's runways, and 
//                 elevation in feet.
//
// Written by Curtis Olson, started April 1998.
// Updated by Durk Talsma, started December 2004.
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


#ifndef _FG_SIMPLE_HXX
#define _FG_SIMPLE_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/xml/easyxml.hxx>

#include STL_STRING
#include <map>
#include <vector>

SG_USING_STD(string);
SG_USING_STD(map);
SG_USING_STD(vector);

typedef vector<string> stringVec;
typedef vector<string>::iterator stringVecIterator;
typedef vector<string>::const_iterator stringVecConstIterator;

typedef vector<time_t> timeVec;
typedef vector<time_t>::const_iterator timeVecConstIterator;
/***************************************************************************/
class ScheduleTime {
private:
  timeVec   start;
  timeVec   end;
  stringVec scheduleNames;
  double tailWind;
  double crssWind;
public:
  ScheduleTime() {};
  ScheduleTime(const ScheduleTime &other);
  ScheduleTime &operator= (const ScheduleTime &other);
  string getName(time_t dayStart);

  void clear();
  void addStartTime(time_t time)     { start.push_back(time);            };
  void addEndTime  (time_t time)     { end.  push_back(time);            };
  void addScheduleName(string sched) { scheduleNames.push_back(sched);   };
  void setTailWind(double wnd)  { tailWind = wnd;                        };
  void setCrossWind(double wnd) { tailWind = wnd;                        };

  double getTailWind()  { return tailWind;                               };
  double getCrossWind() { return crssWind;                               };
};

//typedef vector<ScheduleTime> ScheduleTimes;
/*****************************************************************************/

class RunwayList
{
private:
  string type;
  stringVec preferredRunways;
public:
  RunwayList() {};
  RunwayList(const RunwayList &other);
  RunwayList& operator= (const RunwayList &other);

  void set(string, string);
  void clear();

  string getType() { return type; };
  stringVec *getRwyList() { return &preferredRunways;    };
  string getRwyList(int j) { return preferredRunways[j]; };
};

typedef vector<RunwayList> RunwayListVec;
typedef vector<RunwayList>::iterator RunwayListVectorIterator;
typedef vector<RunwayList>::const_iterator RunwayListVecConstIterator;


/*****************************************************************************/

class RunwayGroup
{
private:
  string name;
  RunwayListVec rwyList;
  int active;
  //stringVec runwayNames;
  int choice[2];
  int nrActive;
public:
  RunwayGroup() {};
  RunwayGroup(const RunwayGroup &other);
  RunwayGroup &operator= (const RunwayGroup &other);

  void setName(string nm) { name = nm;                };
  void add(RunwayList list) { rwyList.push_back(list);};
  void setActive(string aptId, double windSpeed, double windHeading, double maxTail, double maxCross);

  int getNrActiveRunways() { return nrActive;};
  void getActive(int i, string *name, string *type);

  string getName() { return name; };
  void clear() { rwyList.clear(); }; 
  //void add(string, string);
};

typedef vector<RunwayGroup> PreferenceList;
typedef vector<RunwayGroup>::iterator PreferenceListIterator;
typedef vector<RunwayGroup>::const_iterator PreferenceListConstIterator;
/******************************************************************************/

class FGRunwayPreference  : public XMLVisitor {
private:
  string value;
  string scheduleName;

  ScheduleTime comTimes; // Commercial Traffic;
  ScheduleTime genTimes; // General Aviation;
  ScheduleTime milTimes; // Military Traffic;
  ScheduleTime currTimes; // Needed for parsing;

  RunwayList  rwyList;
  RunwayGroup rwyGroup;
  PreferenceList preferences;

  time_t processTime(string);
  bool initialized;

public:
  FGRunwayPreference();
  FGRunwayPreference(const FGRunwayPreference &other);
  
  FGRunwayPreference & operator= (const FGRunwayPreference &other);
  ScheduleTime *getSchedule(const char *trafficType);
  RunwayGroup *getGroup(const string groupName);
  bool available() { return initialized; };

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

class FGParking {
private:
  double latitude;
  double longitude;
  double heading;
  double radius;
  int index;
  string parkingName;
  string type;
  string airlineCodes;
 

  bool available;

  double processPosition(string pos);

public:
  FGParking() { available = true;};
  //FGParking(FGParking &other);
  FGParking(double lat,
	    double lon,
	    double hdg,
	    double rad,
	    int idx,
	    string name,
	    string tpe,
	    string codes);
  void setLatitude (string lat)  { latitude    = processPosition(lat);  };
  void setLongitude(string lon)  { longitude   = processPosition(lon);  };
  void setHeading  (double hdg)  { heading     = hdg;  };
  void setRadius   (double rad)  { radius      = rad;  };
  void setIndex    (int    idx)  { index       = idx;  };
  void setName     (string name) { parkingName = name; };
  void setType     (string tpe)  { type        = tpe;  };
  void setCodes    (string codes){ airlineCodes= codes;};

  bool isAvailable ()         { return available;};
  void setAvailable(bool val) { available = val; };
  
  double getLatitude () { return latitude;    };
  double getLongitude() { return longitude;   };
  double getHeading  () { return heading;     };
  double getRadius   () { return radius;      };
  int    getIndex    () { return index;       };
  string getType     () { return type;        };
  string getCodes    () { return airlineCodes;};
  string getName     () { return parkingName; };

  bool operator< (const FGParking &other) const {return radius < other.radius; };
};

typedef vector<FGParking> FGParkingVec;
typedef vector<FGParking>::iterator FGParkingVecIterator;
typedef vector<FGParking>::const_iterator FGParkingVecConstIterator;


class FGAirport : public XMLVisitor{
private:
  string _id;
  double _longitude;
  double _latitude;
  double _elevation;
  string _code;               // depricated and can be removed
  string _name;
  bool _has_metar;
  FGParkingVec parkings;
  FGRunwayPreference rwyPrefs;

 time_t lastUpdate;
  string prevTrafficType;
  stringVec landing;
  stringVec takeoff;

  // Experimental keep a running average of wind dir and speed to prevent
  // Erratic runway changes. 
  // Note: I should add these to the copy constructor and assigment operator to be
  // constistent
  double avWindHeading [10];
  double avWindSpeed   [10];

public:
  FGAirport();
  FGAirport(const FGAirport &other);
  //operator= (FGAirport &other);
  FGAirport(string id, double lon, double lat, double elev, string name, bool has_metar);
  void getActiveRunway(string trafficType, int action, string *runway);
  void chooseRunwayFallback(string *runway);
  bool getAvailableParking(double *lat, double *lon, double *heading, int *gate, double rad, string fltype, 
			   string acType, string airline);
  void getParking         (int id, double *lat, double* lon, double *heading);
  FGParking *getParking(int i); // { if (i < parkings.size()) return parkings[i]; else return 0;};
  void releaseParking(int id);
  string getParkingName(int i); 
  const string getId() const { return _id;};
  const string &getName() const { return _name;};
  FGAirport *getAddress() { return this; };
  double getLongitude() { return _longitude;};
  double getLatitude()  { return _latitude; };
  double getElevation() { return _elevation;};
  bool   getMetar()     { return _has_metar;};
  

  void setId(string id) { _id = id;};
  void setMetar(bool value) { _has_metar = value; };

  void setRwyUse(FGRunwayPreference& ref);

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

typedef map < string, FGAirport > airport_map;
typedef airport_map::iterator airport_map_iterator;
typedef airport_map::const_iterator const_airport_map_iterator;

typedef vector < FGAirport * > airport_list;


class FGAirportList {

private:

    airport_map airports_by_id;
    airport_list airports_array;

public:

    // Constructor (new)
    FGAirportList() {}

    // Destructor
    ~FGAirportList();

    // add an entry to the list
    void add( const string id, const double longitude, const double latitude,
              const double elevation, const string name, const bool has_metar );

    // search for the specified id.
    // Returns true if successful, otherwise returns false.
    // On success, airport data is returned thru "airport" pointer.
    // "airport" is not changed if "apt" is not found.
    FGAirport search( const string& id );

    // search for the airport closest to the specified position
    // (currently a linear inefficient search so it's probably not
    // best to use this at runtime.)  If with_metar is true, then only
    // return station id's marked as having metar data.
    FGAirport search( double lon_deg, double lat_deg, bool with_metar );

    // search and return a pointer;
    FGAirport* search( const string& id, FGAirport *result);

    /**
     * Return the number of airports in the list.
     */
    int size() const;


    /**
     * Return a specific airport, by position.
     */
    const FGAirport *getAirport( int index ) const;


    /**
     * Mark the specified airport record as not having metar
     */
    void no_metar( const string &id );

    /**
     * Mark the specified airport record as (yes) having metar
     */
    void has_metar( const string &id );

};


#endif // _FG_SIMPLE_HXX


