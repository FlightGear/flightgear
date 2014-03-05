/*
Encode an ATIS into spoken words
Copyright (C) 2014 Torsten Dreyer

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __ATIS_ENCODER_HXX
#define __ATIS_ENCODER_HXX

#include <string>
#include <Airports/airport.hxx>
#include <simgear/props/props.hxx>
#include <map>

class ATCSpeech {
public:
    static const char * getSpokenDigit( int i );
    static std::string getSpokenNumber( std::string number );
    static std::string getSpokenNumber( int number, bool leadingZero = false, int digits = 1 );
    static std::string getSpokenAltitude( int altitude );
};

class ATISInformationProvider {
public:
    virtual ~ATISInformationProvider() {}
    virtual bool isValid() = 0;
    virtual std::string airportId() = 0;

    static long makeAtisTime( int day, int hour, int minute ) {
      return  100*100l* day + 100l * hour + minute;
    }
    inline int getAtisTimeDay( long atisTime ) { return atisTime / (100l*100l); }
    inline int getAtisTimeHour( long atisTime ) { return (atisTime % (100l*100l)) / 100l; }
    inline int getAtisTimeMinute( long atisTime ) { return atisTime % 100l; }
    virtual long getTime() = 0; // see makeAtisTime

    virtual int getWindDeg() = 0;
    virtual int getWindMinDeg() = 0;
    virtual int getWindMaxDeg() = 0;
    virtual int getWindSpeedKt() = 0;
    virtual int getGustsKt() = 0;
    virtual int getQnh() = 0;
    virtual bool isCavok() = 0;
    virtual int getVisibilityMeters() = 0;
    virtual std::string getPhenomena() = 0;

    typedef std::map<int,std::string> CloudEntries;
    virtual CloudEntries getClouds() = 0;
    virtual int getTemperatureDeg() = 0;
    virtual int getDewpointDeg() = 0;
    virtual std::string getTrend() = 0;
};

class ATISEncoder : public ATCSpeech {
public:
  ATISEncoder();
  virtual ~ATISEncoder();
  virtual std::string encodeATIS( ATISInformationProvider * atisInformationProvider );

protected:
  virtual std::string getAtisId( SGPropertyNode_ptr );
  virtual std::string getAirportName( SGPropertyNode_ptr );
  virtual std::string getTime( SGPropertyNode_ptr );
  virtual std::string getApproachType( SGPropertyNode_ptr );
  virtual std::string getLandingRunway( SGPropertyNode_ptr );
  virtual std::string getTakeoffRunway( SGPropertyNode_ptr );
  virtual std::string getTransitionLevel( SGPropertyNode_ptr );
  virtual std::string getWindDirection( SGPropertyNode_ptr );
  virtual std::string getWindMinDirection( SGPropertyNode_ptr );
  virtual std::string getWindMaxDirection( SGPropertyNode_ptr );
  virtual std::string getWindspeedKnots( SGPropertyNode_ptr );
  virtual std::string getGustsKnots( SGPropertyNode_ptr );
  virtual std::string getCavok( SGPropertyNode_ptr );
  virtual std::string getVisibilityMetric( SGPropertyNode_ptr );
  virtual std::string getPhenomena( SGPropertyNode_ptr );
  virtual std::string getClouds( SGPropertyNode_ptr );
  virtual std::string getTemperatureDeg( SGPropertyNode_ptr );
  virtual std::string getDewpointDeg( SGPropertyNode_ptr );
  virtual std::string getQnh( SGPropertyNode_ptr );
  virtual std::string getInhg( SGPropertyNode_ptr );
  virtual std::string getTrend( SGPropertyNode_ptr );

  typedef std::string (ATISEncoder::*handler_t)( SGPropertyNode_ptr baseNode );
  typedef std::map<std::string, handler_t > HandlerMap;
  HandlerMap handlerMap;

  SGPropertyNode_ptr atisSchemaNode;

  std::string processTokens( SGPropertyNode_ptr baseNode );
  std::string processToken( SGPropertyNode_ptr baseNode );

  std::string processTextToken( SGPropertyNode_ptr baseNode );
  std::string processTokenToken( SGPropertyNode_ptr baseNode );
  std::string processIfToken( SGPropertyNode_ptr baseNode );
  bool checkEmptyCondition( SGPropertyNode_ptr node, bool isEmpty );
  bool checkEqualsCondition( SGPropertyNode_ptr node, bool isEmpty );

  FGAirportRef airport;
  ATISInformationProvider * _atis;
};

#endif
