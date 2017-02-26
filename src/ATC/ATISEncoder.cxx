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

#include "ATISEncoder.hxx"
#include <Airports/dynamics.hxx>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>
#include <boost/lexical_cast.hpp>

#include <map>
#include <vector>

using std::string;
using std::vector;
using simgear::PropertyList;

static string NO_ATIS("nil");
static string EMPTY("");
#define SPACE append(1,' ')

std::string ATCSpeech::getSpokenDigit( int i )
{
  string key = "n" + boost::lexical_cast<std::string>( i );
  return globals->get_locale()->getLocalizedString(key.c_str(), "atc", "" );
}

string ATCSpeech::getSpokenNumber( string number )
{
  string result;
  for( string::iterator it = number.begin(); it != number.end(); ++it ) {
    if( false == result.empty() )
      result.SPACE;
    result.append( getSpokenDigit( (*it) - '0' ));
  }
  return result;
}

string ATCSpeech::getSpokenNumber( int number, bool leadingZero, int digits )
{
  string_list spokenDigits;
  bool negative = false;
  if( number < 0 ) {
    negative = true;
    number = -number;
  }

  int n = 0;
  while( number > 0 ) {
    spokenDigits.push_back( getSpokenDigit(number%10) );
    number /= 10;
    n++;
  }

  if( digits > 0 ) {
    while( n++ < digits ) {
      spokenDigits.push_back( getSpokenDigit(0) );
    }
  }

  string result;
  if( negative ) {
    result.append( globals->get_locale()->getLocalizedString("minus", "atc", "minus" ) );
  }

  while( false == spokenDigits.empty() ) {
    if( false == result.empty() )
      result.SPACE;

    result.append( spokenDigits.back() );
    spokenDigits.pop_back();
  }

  return result;
}

string ATCSpeech::getSpokenAltitude( int altitude )
{
  string result;
  int thousands = altitude / 1000;
  int hundrets = (altitude % 1000) / 100;

  if( thousands > 0 ) {
    result.append( getSpokenNumber(thousands) );
    result.SPACE;
    result.append( getSpokenDigit(1000) );
    result.SPACE;
  }
  if( hundrets > 0 )
      result.append( getSpokenNumber(hundrets) )
            .SPACE
            .append( getSpokenDigit(100) );

  return result;
}

ATISEncoder::ATISEncoder()
{
  handlerMap.insert( std::make_pair( "text", &ATISEncoder::processTextToken ));
  handlerMap.insert( std::make_pair( "token", &ATISEncoder::processTokenToken ));
  handlerMap.insert( std::make_pair( "if", &ATISEncoder::processIfToken ));
  handlerMap.insert( std::make_pair( "section", &ATISEncoder::processTokens ));

  handlerMap.insert( std::make_pair( "id", &ATISEncoder::getAtisId ));
  handlerMap.insert( std::make_pair( "airport-name", &ATISEncoder::getAirportName ));
  handlerMap.insert( std::make_pair( "time", &ATISEncoder::getTime ));
  handlerMap.insert( std::make_pair( "approach-type", &ATISEncoder::getApproachType ));
  handlerMap.insert( std::make_pair( "rwy-land", &ATISEncoder::getLandingRunway ));
  handlerMap.insert( std::make_pair( "rwy-to", &ATISEncoder::getTakeoffRunway ));
  handlerMap.insert( std::make_pair( "transition-level", &ATISEncoder::getTransitionLevel ));
  handlerMap.insert( std::make_pair( "wind-dir", &ATISEncoder::getWindDirection ));
  handlerMap.insert( std::make_pair( "wind-from", &ATISEncoder::getWindMinDirection ));
  handlerMap.insert( std::make_pair( "wind-to", &ATISEncoder::getWindMaxDirection ));
  handlerMap.insert( std::make_pair( "wind-speed-kn", &ATISEncoder::getWindspeedKnots ));
  handlerMap.insert( std::make_pair( "gusts", &ATISEncoder::getGustsKnots ));
  handlerMap.insert( std::make_pair( "visibility-metric", &ATISEncoder::getVisibilityMetric ));
  handlerMap.insert( std::make_pair( "visibility-miles", &ATISEncoder::getVisibilityMiles ));
  handlerMap.insert( std::make_pair( "phenomena", &ATISEncoder::getPhenomena ));
  handlerMap.insert( std::make_pair( "clouds", &ATISEncoder::getClouds ));
  handlerMap.insert( std::make_pair( "clouds-brief", &ATISEncoder::getCloudsBrief ));
  handlerMap.insert( std::make_pair( "cavok", &ATISEncoder::getCavok ));
  handlerMap.insert( std::make_pair( "temperature-deg", &ATISEncoder::getTemperatureDeg ));
  handlerMap.insert( std::make_pair( "dewpoint-deg", &ATISEncoder::getDewpointDeg ));
  handlerMap.insert( std::make_pair( "qnh", &ATISEncoder::getQnh ));
  handlerMap.insert( std::make_pair( "inhg", &ATISEncoder::getInhg ));
  handlerMap.insert( std::make_pair( "inhg-integer", &ATISEncoder::getInhgInteger ));
  handlerMap.insert( std::make_pair( "inhg-fraction", &ATISEncoder::getInhgFraction ));
  handlerMap.insert( std::make_pair( "trend", &ATISEncoder::getTrend ));
}

ATISEncoder::~ATISEncoder()
{
}

SGPropertyNode_ptr findAtisTemplate( const std::string & stationId, SGPropertyNode_ptr atisSchemaNode )
{
  using simgear::strutils::starts_with;
  SGPropertyNode_ptr atisTemplate;

  PropertyList schemaNodes = atisSchemaNode->getChildren("atis-schema");
  for( PropertyList::iterator asit = schemaNodes.begin(); asit != schemaNodes.end(); ++asit ) {
    SGPropertyNode_ptr ppp = (*asit)->getNode("station-starts-with", false );
    atisTemplate = (*asit)->getNode("atis", false );
    if( false == atisTemplate.valid() ) continue; // no <atis> node - ignore entry

    PropertyList startsWithNodes = (*asit)->getChildren("station-starts-with");
    for( PropertyList::iterator swit = startsWithNodes.begin(); swit != startsWithNodes.end(); ++swit ) {
   
      if( starts_with( stationId,  (*swit)->getStringValue() ) ) {
        return atisTemplate;
      }
    }

  }

  return atisTemplate;
}

string ATISEncoder::encodeATIS( ATISInformationProvider * atisInformation )
{
  using simgear::strutils::lowercase;

  if( false == atisInformation->isValid() ) return NO_ATIS;

  airport = FGAirport::getByIdent( atisInformation->airportId() );
  if( false == airport.valid() ) {
    SG_LOG( SG_ATC, SG_WARN, "ATISEncoder: unknown airport id " << atisInformation->airportId() );
    return NO_ATIS;
  }

  _atis = atisInformation;

  // lazily load the schema file on the first call
  if( false == atisSchemaNode.valid() ) {
    atisSchemaNode = new SGPropertyNode();
    try
    {
      SGPath path = globals->resolve_maybe_aircraft_path("ATC/atis.xml");
      readProperties( path, atisSchemaNode );
    }
    catch (const sg_exception& e)
    {
      SG_LOG( SG_ATC, SG_ALERT, "ATISEncoder: Failed to load atis schema definition: " << e.getMessage());
      return NO_ATIS;
    }
  }

  string stationId = lowercase( airport->ident() );
  
  SGPropertyNode_ptr atisTemplate = findAtisTemplate( stationId, atisSchemaNode );;
  if( false == atisTemplate.valid() ) {
    SG_LOG(SG_ATC, SG_WARN, "no matching atis template for station " << stationId  );
    return NO_ATIS; // no template for this station!?
  }

  return processTokens( atisTemplate );
}

string ATISEncoder::processTokens( SGPropertyNode_ptr node )
{
  string result;
  if( node.valid() ) {
    for( int i = 0; i < node->nChildren(); i++ ) {
      result.append(processToken( node->getChild(i) ));
    }
  }
  return result;
}

string ATISEncoder::processToken( SGPropertyNode_ptr token ) 
{
  HandlerMap::iterator it = handlerMap.find( token->getName());
  if( it == handlerMap.end() ) {
    SG_LOG(SG_ATC, SG_WARN, "ATISEncoder: unknown token: " << token->getName() );
    return EMPTY;
  }
  handler_t h = it->second;
  return (this->*h)( token );
}

string ATISEncoder::processTextToken( SGPropertyNode_ptr token )
{
  return token->getStringValue();
}

string ATISEncoder::processTokenToken( SGPropertyNode_ptr token )
{
  HandlerMap::iterator it = handlerMap.find( token->getStringValue());
  if( it == handlerMap.end() ) {
    SG_LOG(SG_ATC, SG_WARN, "ATISEncoder: unknown token: " << token->getStringValue() );
    return EMPTY;
  }
  handler_t h = it->second;
  return (this->*h)( token );

  token->getStringValue();
}

string ATISEncoder::processIfToken( SGPropertyNode_ptr token )
{
  using namespace simgear::strutils;

  SGPropertyNode_ptr n;

  if( (n = token->getChild("empty", false )).valid() ) {
    return checkEmptyCondition( n, true) ? 
           processTokens(token->getChild("then",false)) : 
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("not-empty", false )).valid() ) {
    return checkEmptyCondition( n, false) ? 
           processTokens(token->getChild("then",false)) : 
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("contains", false )).valid() ) {
    return checkCondition( n, true, &contains, "contains") ?
           processTokens(token->getChild("then",false)) :
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("not-contains", false )).valid() ) {
    return checkCondition( n, false, &contains, "not-contains") ?
           processTokens(token->getChild("then",false)) :
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("ends-with", false )).valid() ) {
    return checkCondition( n, true, &ends_with, "ends-with") ?
           processTokens(token->getChild("then",false)) :
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("not-ends-with", false )).valid() ) {
    return checkCondition( n, false, &ends_with, "not-ends-with") ?
           processTokens(token->getChild("then",false)) :
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("equals", false )).valid() ) {
    return checkCondition( n, true, &equals, "equals") ?
           processTokens(token->getChild("then",false)) : 
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("not-equals", false )).valid() ) {
    return checkCondition( n, false, &equals, "not-equals") ?
           processTokens(token->getChild("then",false)) :
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("starts-with", false )).valid() ) {
    return checkCondition( n, true, &starts_with, "starts-with") ?
           processTokens(token->getChild("then",false)) :
           processTokens(token->getChild("else",false));
  }

  if( (n = token->getChild("not-starts-with", false )).valid() ) {
    return checkCondition( n, false, &starts_with, "not-starts-with") ?
           processTokens(token->getChild("then",false)) : 
           processTokens(token->getChild("else",false));
  }

  SG_LOG(SG_ATC, SG_WARN, "ATISEncoder: no valid token found for <if> element" );

  return EMPTY;
}

bool ATISEncoder::checkEmptyCondition( SGPropertyNode_ptr node, bool isEmpty ) 
{
  SGPropertyNode_ptr n1 = node->getNode( "token", false );
  if( false == n1.valid() ) {
      SG_LOG(SG_ATC, SG_WARN, "missing <token> node for (not)-empty"  );
      return false;
  }
  return processToken( n1 ).empty() == isEmpty;
}

bool ATISEncoder::checkCondition( SGPropertyNode_ptr node, bool notInverted,
    bool (*fp)(const string &, const string &), const string &name )
{
  using namespace simgear::strutils;

  SGPropertyNode_ptr n1 = node->getNode( "token", 0, false );
  SGPropertyNode_ptr n2 = node->getNode( "token", 1, false );

  if( n1.valid() && n2.valid() ) {
    bool comp = fp( processToken( n1 ), processToken( n2 ) );
    return comp == notInverted;
  }

  if( n1.valid() && !n2.valid() ) {
    SGPropertyNode_ptr t1 = node->getNode( "text", 0, false );
    if( t1.valid() ) {
      string n1s = lowercase( strip( processToken( n1 ) ) );
      string t1s = lowercase( strip( processTextToken( t1 ) ) );
      return fp( n1s, t1s ) == notInverted;
    }
    SG_LOG(SG_ATC, SG_WARN, "missing <token> or <text> node for " << name);
    return false;
  }

  SG_LOG(SG_ATC, SG_WARN, "missing <token> node for " << name);
  return false;
}

string ATISEncoder::getAtisId( SGPropertyNode_ptr )
{
  FGAirportDynamics * dynamics = airport->getDynamics();
  if( NULL != dynamics ) {
    dynamics->updateAtisSequence( 30*60, false );
    return dynamics->getAtisSequence();
  }
  return EMPTY;
}

string ATISEncoder::getAirportName( SGPropertyNode_ptr )
{
  return airport->getName();
}

string ATISEncoder::getTime( SGPropertyNode_ptr )
{
  return getSpokenNumber( _atis->getTime() % (100*100), true, 4 );
}

static inline FGRunwayRef findBestRunwayForWind( FGAirportRef airport, int windDeg, int windKt )
{
  struct FGAirport::FindBestRunwayForHeadingParams p;
  //TODO: ramp down the heading weight with wind speed
  p.ilsWeight = 4;
  return airport->findBestRunwayForHeading( windDeg, &p );
}

string ATISEncoder::getApproachType( SGPropertyNode_ptr )
{
  FGRunwayRef runway = findBestRunwayForWind( airport, _atis->getWindDeg(), _atis->getWindSpeedKt() );
  if( runway.valid() ) {
    if( NULL != runway->ILS() ) return globals->get_locale()->getLocalizedString("ils", "atc", "ils" );
    //TODO: any chance to find other approach types? localizer-dme, vor-dme, vor, ndb?
  }

  return globals->get_locale()->getLocalizedString("visual", "atc", "visual" );
}

string ATISEncoder::getLandingRunway( SGPropertyNode_ptr )
{
  FGRunwayRef runway = findBestRunwayForWind( airport, _atis->getWindDeg(), _atis->getWindSpeedKt() );
  if( runway.valid() ) {
    string runwayIdent = runway->ident();
    if(runwayIdent != "NN") {
      return getSpokenNumber(runwayIdent);
    }
  }
  return EMPTY;
}

string ATISEncoder::getTakeoffRunway( SGPropertyNode_ptr p )
{
  //TODO: if the airport has more than one runway, probably pick another one?
  return getLandingRunway( p );
}

string ATISEncoder::getTransitionLevel( SGPropertyNode_ptr )
{
  double hPa = _atis->getQnh();

  /* Transition level is the flight level above which aircraft must use standard pressure and below
   * which airport pressure settings must be used.
   * Following definitions are taken from German ATIS:
   *      QNH <=  977 hPa: TRL 80
   *      QNH <= 1013 hPa: TRL 70
   *      QNH >  1013 hPa: TRL 60
   * (maybe differs slightly for other countries...)
   */
  int tl;
  if (hPa <= 978) {
    tl = 80;
  } else if( hPa > 978 && hPa <= 1013 ) {
    tl = 70;
  } else if( hPa > 1013 && hPa <= 1046 ) {
    tl = 60;
  } else {
    tl = 50;
  }

  // add an offset to the transition level for high altitude airports (just guessing here,
  // seems reasonable)
  int e = int(airport->getElevation() / 1000.0);
  if (e >= 3) {
    // TL steps in 10(00)ft
    tl += (e-2)*10;
  }

  return getSpokenNumber(tl);
}

string ATISEncoder::getWindDirection( SGPropertyNode_ptr )
{
  string variable = globals->get_locale()->getLocalizedString("variable", "atc", "variable" );

  bool vrb = _atis->getWindMinDeg() == 0 && _atis->getWindMaxDeg() == 359;
  return vrb ? variable : getSpokenNumber( _atis->getWindDeg(), true, 3 );
}

string ATISEncoder::getWindMinDirection( SGPropertyNode_ptr )
{
  return getSpokenNumber( _atis->getWindMinDeg(), true, 3 );
}

string ATISEncoder::getWindMaxDirection( SGPropertyNode_ptr )
{
  return getSpokenNumber( _atis->getWindMaxDeg(), true, 3 );
}

string ATISEncoder::getWindspeedKnots( SGPropertyNode_ptr )
{
  return getSpokenNumber( _atis->getWindSpeedKt() );
}

string ATISEncoder::getGustsKnots( SGPropertyNode_ptr )
{
  int g = _atis->getGustsKt();
  return g > 0 ? getSpokenNumber( g ) : EMPTY;
}

string ATISEncoder::getCavok( SGPropertyNode_ptr )
{
  string CAVOK = globals->get_locale()->getLocalizedString("cavok", "atc", "cavok" );

  return _atis->isCavok() ? CAVOK : EMPTY;
}

string ATISEncoder::getVisibilityMetric( SGPropertyNode_ptr )
{
  string m = globals->get_locale()->getLocalizedString("meters", "atc", "meters" );
  string km = globals->get_locale()->getLocalizedString("kilometers", "atc", "kilometers" );
  string or_more = globals->get_locale()->getLocalizedString("ormore", "atc", "or more" );

  int v = _atis->getVisibilityMeters();
  string reply;
  if( v < 5000 ) return reply.append( getSpokenAltitude( v ) ).SPACE.append( m );
  if( v >= 9999 ) return reply.append( getSpokenNumber(10) ).SPACE.append( km ).SPACE.append(or_more);
  return reply.append( getSpokenNumber( v/1000 ).SPACE.append( km ) );
}

string ATISEncoder::getVisibilityMiles( SGPropertyNode_ptr )
{
  string feet = globals->get_locale()->getLocalizedString("feet", "atc", "feet" );

  int v = _atis->getVisibilityMeters();
  int vft = int( v * SG_METER_TO_FEET / 100 + 0.5 ) * 100; // Rounded to 100 ft
  int vsm = int( v * SG_METER_TO_SM + 0.5 );

  string reply;
  if( vsm < 1 ) return reply.append( getSpokenAltitude( vft ) ).SPACE.append( feet );
  if( vsm >= 10 ) return reply.append( getSpokenNumber(10) );
  return reply.append( getSpokenNumber( vsm ) );
}

string ATISEncoder::getPhenomena( SGPropertyNode_ptr )
{
  return _atis->getPhenomena();
}

string ATISEncoder::getClouds( SGPropertyNode_ptr )
{
  string FEET =  globals->get_locale()->getLocalizedString("feet", "atc", "feet" );
  string reply;

  ATISInformationProvider::CloudEntries cloudEntries = _atis->getClouds();

  for( ATISInformationProvider::CloudEntries::iterator it = cloudEntries.begin(); it != cloudEntries.end(); it++ ) {
    if( false == reply.empty() ) reply.SPACE;
    reply.append( it->second ).SPACE.append( getSpokenAltitude(it->first).SPACE.append( FEET ) );
  }
  return reply;
}

string ATISEncoder::getCloudsBrief( SGPropertyNode_ptr )
{
  string reply;

  ATISInformationProvider::CloudEntries cloudEntries = _atis->getClouds();

  for( ATISInformationProvider::CloudEntries::iterator it = cloudEntries.begin(); it != cloudEntries.end(); it++ ) {
    if( false == reply.empty() ) reply.append(",").SPACE;
    reply.append( it->second ).SPACE.append( getSpokenAltitude(it->first) );
  }
  return reply;
}

string ATISEncoder::getTemperatureDeg( SGPropertyNode_ptr )
{
  return getSpokenNumber( _atis->getTemperatureDeg() );
}

string ATISEncoder::getDewpointDeg( SGPropertyNode_ptr )
{
  return getSpokenNumber( _atis->getDewpointDeg() );
}

string ATISEncoder::getQnh( SGPropertyNode_ptr )
{
  return getSpokenNumber( _atis->getQnh() );
}

string ATISEncoder::getInhgInteger( SGPropertyNode_ptr )
{
  double qnh = _atis->getQnhInHg();
  return getSpokenNumber( (int)qnh, true, 2 );
}

string ATISEncoder::getInhgFraction( SGPropertyNode_ptr )
{
  double qnh = _atis->getQnhInHg();
  int f = int(100 * (qnh - int(qnh)) + 0.5);
  return getSpokenNumber( f, true, 2 );
}

string ATISEncoder::getInhg( SGPropertyNode_ptr node)
{
  string DECIMAL = globals->get_locale()->getLocalizedString("dp", "atc", "decimal" );
  return getInhgInteger(node)
    .SPACE.append(DECIMAL).SPACE
    .append(getInhgFraction(node));
}

string ATISEncoder::getTrend( SGPropertyNode_ptr )
{
  return _atis->getTrend();
}

