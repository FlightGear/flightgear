// realwx_ctrl.cxx -- Process real weather data
//
// Written by David Megginson, started February 2002.
// Rewritten by Torsten Dreyer, August 2010, August 2011
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "realwx_ctrl.hxx"

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/structure/commands.hxx>

#include "metarproperties.hxx"
#include "metarairportfilter.hxx"
#include "fgmetar.hxx"
#include <Network/HTTPClient.hxx>
#include <Main/fg_props.hxx>

namespace Environment {


/* -------------------------------------------------------------------------------- */

class MetarDataHandler {
public:
    virtual void handleMetarData( const std::string & data ) = 0;
};

class MetarRequester {
public:
    virtual void requestMetar( MetarDataHandler * metarDataHandler, const std::string & id ) = 0;
};

/* -------------------------------------------------------------------------------- */

class LiveMetarProperties : public MetarProperties, MetarDataHandler {
public:
    LiveMetarProperties( SGPropertyNode_ptr rootNode, MetarRequester * metarRequester );
    virtual ~LiveMetarProperties();
    virtual void update( double dt );

    virtual double getTimeToLive() const { return _timeToLive; }
    virtual void resetTimeToLive()
    { _timeToLive = 0.00; _pollingTimer = 0.0; }

    // implementation of MetarDataHandler
    virtual void handleMetarData( const std::string & data );

    static const unsigned MAX_POLLING_INTERVAL_SECONDS = 10;
    static const unsigned DEFAULT_TIME_TO_LIVE_SECONDS = 900;

private:
    double _timeToLive;
    double _pollingTimer;
    MetarRequester * _metarRequester;
};

typedef SGSharedPtr<LiveMetarProperties> LiveMetarProperties_ptr;

LiveMetarProperties::LiveMetarProperties( SGPropertyNode_ptr rootNode, MetarRequester * metarRequester ) :
    MetarProperties( rootNode ),
    _timeToLive(0.0),
    _pollingTimer(0.0),
    _metarRequester(metarRequester)
{
    _tiedProperties.Tie("time-to-live", &_timeToLive );
}

LiveMetarProperties::~LiveMetarProperties()
{
    _tiedProperties.Untie();
}

void LiveMetarProperties::update( double dt )
{
    _timeToLive -= dt;
    _pollingTimer -= dt;
    if( _timeToLive <= 0.0 ) {
        _timeToLive = 0.0;
        std::string stationId = getStationId();
        if( stationId.empty() ) return;
        if( _pollingTimer > 0.0 ) return;
        _metarRequester->requestMetar( this, stationId );
        _pollingTimer = MAX_POLLING_INTERVAL_SECONDS;
    }
}

void LiveMetarProperties::handleMetarData( const std::string & data )
{
    SG_LOG( SG_ENVIRONMENT, SG_INFO, "LiveMetarProperties::handleMetarData() received METAR for " << getStationId() << ": " << data );
    _timeToLive = DEFAULT_TIME_TO_LIVE_SECONDS;
    setMetar( data );
}

/* -------------------------------------------------------------------------------- */

class BasicRealWxController : public RealWxController
{
public:
    BasicRealWxController( SGPropertyNode_ptr rootNode, MetarRequester * metarRequester );
    virtual ~BasicRealWxController ();

    virtual void init ();
    virtual void reinit ();
    virtual void shutdown ();
    
    /**
     * Create a metar-property binding at the specified property path,
     * and initiate a request for the specified station-ID (which may be
     * empty). If the property path is already mapped, the station ID
     * will be updated.
     */
    void addMetarAtPath(const string& propPath, const string& icao);
  
    void removeMetarAtPath(const string& propPath);
protected:
    void bind();
    void unbind();
    void update( double dt );

    void checkNearbyMetar();

    long getMetarMaxAgeMin() const { return _max_age_n == NULL ? 0 : _max_age_n->getLongValue(); }

    SGPropertyNode_ptr _rootNode;
    SGPropertyNode_ptr _ground_elevation_n;
    SGPropertyNode_ptr _max_age_n;

    bool _enabled;
    bool _wasEnabled;
    simgear::TiedPropertyList _tiedProperties;
    typedef std::vector<LiveMetarProperties_ptr> MetarPropertiesList;
    MetarPropertiesList _metarProperties;
    MetarRequester* _requester;

};

static bool commandRequestMetar(const SGPropertyNode* arg)
{
  SGSubsystemGroup* envMgr = (SGSubsystemGroup*) globals->get_subsystem("environment");
  if (!envMgr) {
    return false;
  }
  
  BasicRealWxController* self = (BasicRealWxController*) envMgr->get_subsystem("realwx");
  if (!self) {
    return false;
  }
  
  string icao(arg->getStringValue("station"));
  boost::to_upper(icao);
  string path = arg->getStringValue("path");
  self->addMetarAtPath(path, icao);
  return true;
}
  
static bool commandClearMetar(const SGPropertyNode* arg)
{
  SGSubsystemGroup* envMgr = (SGSubsystemGroup*) globals->get_subsystem("environment");
  if (!envMgr) {
    return false;
  }
  
  BasicRealWxController* self = (BasicRealWxController*) envMgr->get_subsystem("realwx");
  if (!self) {
    return false;
  }
  
  string path = arg->getStringValue("path");
  self->removeMetarAtPath(path);
  return true;
}
  
/* -------------------------------------------------------------------------------- */
/*
Properties
 ~/enabled: bool              Enables/Disables the realwx controller
 ~/metar[1..n]: string        Target property path for metar data
 */

BasicRealWxController::BasicRealWxController( SGPropertyNode_ptr rootNode, MetarRequester * metarRequester ) :
  _rootNode(rootNode),
  _ground_elevation_n( fgGetNode( "/position/ground-elev-m", true )),
  _max_age_n( fgGetNode( "/environment/params/metar-max-age-min", false ) ),
  _enabled(true),
  _wasEnabled(false),
  _requester(metarRequester)
{
    // at least instantiate MetarProperties for /environment/metar
    _metarProperties.push_back( new LiveMetarProperties( 
            fgGetNode( rootNode->getStringValue("metar", "/environment/metar"), true ), metarRequester ));

    BOOST_FOREACH( SGPropertyNode_ptr n, rootNode->getChildren("metar") ) {
        SGPropertyNode_ptr metarNode = fgGetNode( n->getStringValue(), true );
        addMetarAtPath(metarNode->getPath(), "");
    }
  
    SGCommandMgr::instance()->addCommand("request-metar", commandRequestMetar);
    SGCommandMgr::instance()->addCommand("clear-metar", commandClearMetar);
}

BasicRealWxController::~BasicRealWxController()
{
  //SGCommandMgr::instance()->removeCommand("request-metar");
}

void BasicRealWxController::init()
{
    _wasEnabled = false;
    
    checkNearbyMetar();
    update(0); // fetch data ASAP
    
    globals->get_event_mgr()->addTask("checkNearbyMetar", this,
                                      &BasicRealWxController::checkNearbyMetar, 60 );
}

void BasicRealWxController::reinit()
{
    _wasEnabled = false;
    checkNearbyMetar();
    update(0); // fetch data ASAP
}
    
void BasicRealWxController::shutdown()
{
    globals->get_event_mgr()->removeTask("checkNearbyMetar");
}

void BasicRealWxController::bind()
{
    _tiedProperties.setRoot( _rootNode );
    _tiedProperties.Tie( "enabled", &_enabled );
}

void BasicRealWxController::unbind()
{
    _tiedProperties.Untie();
}

void BasicRealWxController::update( double dt )
{  
  if( _enabled ) {
    bool firstIteration = !_wasEnabled;
    // clock tick for every METAR in stock
    BOOST_FOREACH(LiveMetarProperties* p, _metarProperties) {
      // first round? All received METARs are outdated
      if( firstIteration ) p->resetTimeToLive();
      p->update(dt);
    }

    _wasEnabled = true;
  } else {
    _wasEnabled = false;
  }
}

void BasicRealWxController::addMetarAtPath(const string& propPath, const string& icao)
{
  // check for duplicate entries
  BOOST_FOREACH( LiveMetarProperties_ptr p, _metarProperties ) {
    if( p->get_root_node()->getPath() == propPath ) {
      // already exists
      if (p->getStationId() != icao) {
        p->setStationId(icao);
        p->resetTimeToLive();
      }
      
      return;
    }
  } // of exitsing metar properties iteration

  SGPropertyNode_ptr metarNode = fgGetNode(propPath, true);
  SG_LOG( SG_ENVIRONMENT, SG_INFO, "Adding metar properties at " << propPath );
  LiveMetarProperties_ptr p(new LiveMetarProperties( metarNode, _requester ));
  _metarProperties.push_back(p);
  p->setStationId(icao);
}

void BasicRealWxController::removeMetarAtPath(const string &propPath)
{
  MetarPropertiesList::iterator it = _metarProperties.begin();
  for (; it != _metarProperties.end(); ++it) {
    LiveMetarProperties_ptr p(*it);
    if( p->get_root_node()->getPath() == propPath ) {
      _metarProperties.erase(it);
      // final ref will drop, and delete the MetarProperties, when we return
      return;
    }
  }
  
  SG_LOG(SG_ENVIRONMENT, SG_WARN, "no metar properties at " << propPath);
}
  
void BasicRealWxController::checkNearbyMetar()
{
    try {
      const SGGeod & pos = globals->get_aircraft_position();

      // check nearest airport
      SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "NoaaMetarRealWxController::update(): (re) checking nearby airport with METAR" );

      FGAirport * nearestAirport = FGAirport::findClosest(pos, 10000.0, MetarAirportFilter::instance() );
      if( nearestAirport == NULL ) {
          SG_LOG(SG_ENVIRONMENT,SG_WARN,"RealWxController::update can't find airport with METAR within 10000NM"  );
          return;
      }

      SG_LOG(SG_ENVIRONMENT, SG_DEBUG, 
          "NoaaMetarRealWxController::update(): nearest airport with METAR is: " << nearestAirport->ident() );

      // if it has changed, invalidate the associated METAR
      if( _metarProperties[0]->getStationId() != nearestAirport->ident() ) {
          SG_LOG(SG_ENVIRONMENT, SG_INFO, 
              "NoaaMetarRealWxController::update(): nearest airport with METAR has changed. Old: '" << 
              _metarProperties[0]->getStationId() <<
              "', new: '" << nearestAirport->ident() << "'" );
          _metarProperties[0]->setStationId( nearestAirport->ident() );
          _metarProperties[0]->resetTimeToLive();
      }
    }
    catch( sg_exception & ) {
      return;
    }
    
}

/* -------------------------------------------------------------------------------- */

class NoaaMetarRealWxController : public BasicRealWxController, MetarRequester {
public:
    NoaaMetarRealWxController( SGPropertyNode_ptr rootNode );

    // implementation of MetarRequester
    virtual void requestMetar( MetarDataHandler * metarDataHandler, const std::string & id );

private:
    
};

NoaaMetarRealWxController::NoaaMetarRealWxController( SGPropertyNode_ptr rootNode ) :
  BasicRealWxController(rootNode, this )
{
}

void NoaaMetarRealWxController::requestMetar( MetarDataHandler * metarDataHandler, const std::string & id )
{
    class NoaaMetarGetRequest : public simgear::HTTP::Request
    {
    public:
        NoaaMetarGetRequest(MetarDataHandler* metarDataHandler, const string& stationId ) :
              Request("http://weather.noaa.gov/pub/data/observations/metar/stations/" + stationId + ".TXT"),
              _fromProxy(false),
              _metarDataHandler(metarDataHandler)
          {
          }

          virtual string_list requestHeaders() const
          {
              string_list reply;
              reply.push_back("X-TIME");
              return reply;
          }

          virtual std::string header(const std::string& name) const
          {
              string reply;

              if( name == "X-TIME" ) {
                  std::ostringstream buf;
                  buf <<  globals->get_time_params()->get_cur_time();
                  reply = buf.str();
              }

              return reply;
          }

          virtual void responseHeader(const string& key, const string& value)
          {
              if (key == "x-metarproxy") {
                  _fromProxy = true;
              }
          }

          virtual void gotBodyData(const char* s, int n)
          {
              _metar += string(s, n);
          }

          virtual void responseComplete()
          {
              if (responseCode() == 200) {
                  _metarDataHandler->handleMetarData( simgear::strutils::simplify(_metar) );
              } else {
                  SG_LOG(SG_ENVIRONMENT, SG_WARN, "metar download failed:" << url() << ": reason:" << responseReason());
              }
          }
        
        virtual void failed()
        {
            SG_LOG(SG_ENVIRONMENT, SG_INFO, "metar download failure");
        }

//          bool fromMetarProxy() const
//          { return _fromProxy; }
    private:  
        string _metar;
        bool _fromProxy;
        MetarDataHandler * _metarDataHandler;
    };

    string upperId = boost::to_upper_copy(id);

    SG_LOG(SG_ENVIRONMENT, SG_INFO,
        "NoaaMetarRealWxController::update(): spawning load request for station-id '" << upperId << "'" );
    FGHTTPClient* http = static_cast<FGHTTPClient*>(globals->get_subsystem("http"));
    if (http) {
        http->makeRequest(new NoaaMetarGetRequest(metarDataHandler, upperId));
    }
}

/* -------------------------------------------------------------------------------- */

RealWxController * RealWxController::createInstance( SGPropertyNode_ptr rootNode )
{
  return new NoaaMetarRealWxController( rootNode );
}

/* -------------------------------------------------------------------------------- */

} // namespace Environment
