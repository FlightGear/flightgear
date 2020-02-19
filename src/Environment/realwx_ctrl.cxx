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
#include <cctype>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/io/HTTPMemoryRequest.hxx>
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

class MetarRequester;

/* -------------------------------------------------------------------------------- */

class LiveMetarProperties : public MetarProperties {
public:
    LiveMetarProperties( SGPropertyNode_ptr rootNode, MetarRequester * metarRequester, int maxAge );
    virtual ~LiveMetarProperties();
    virtual void update( double dt );

    virtual double getTimeToLive() const { return _timeToLive; }
    virtual void resetTimeToLive()
    { _timeToLive = 0.00; _pollingTimer = 0.0; }

    // implementation of MetarDataHandler
    virtual void handleMetarData( const std::string & data );
    virtual void handleMetarFailure();
  
    static const unsigned MAX_POLLING_INTERVAL_SECONDS = 10;
    static const unsigned DEFAULT_TIME_TO_LIVE_SECONDS = 900;

private:
    double _timeToLive;
    double _pollingTimer;
    MetarRequester * _metarRequester;
    int _maxAge;
    bool _failure;
};

typedef SGSharedPtr<LiveMetarProperties> LiveMetarProperties_ptr;

class MetarRequester {
public:
    virtual void requestMetar( LiveMetarProperties_ptr metarDataHandler, const std::string & id ) = 0;
};


LiveMetarProperties::LiveMetarProperties( SGPropertyNode_ptr rootNode, MetarRequester * metarRequester, int maxAge ) :
    MetarProperties( rootNode ),
    _timeToLive(0.0),
    _pollingTimer(0.0),
    _metarRequester(metarRequester),
    _maxAge(maxAge),
    _failure(false)
{
    _tiedProperties.Tie("time-to-live", &_timeToLive );
    _tiedProperties.Tie("failure", &_failure);
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
        invalidate();
        std::string stationId = getStationId();
        if( stationId.empty() ) return;
        if( _pollingTimer > 0.0 ) return;
        _metarRequester->requestMetar( this, stationId );
        _pollingTimer = MAX_POLLING_INTERVAL_SECONDS;
    }
}

void LiveMetarProperties::handleMetarData( const std::string & data )
{
    SG_LOG( SG_ENVIRONMENT, SG_DEBUG, "LiveMetarProperties::handleMetarData() received METAR for " << getStationId() << ": " << data );
    _timeToLive = DEFAULT_TIME_TO_LIVE_SECONDS;
    
    SGSharedPtr<FGMetar> m;
    try {
        m = new FGMetar(data.c_str());
    }
    catch( sg_io_exception  &) {
        SG_LOG( SG_ENVIRONMENT, SG_ALERT, "Can't parse metar: " << data );
        _failure = true;
        return;
    }

    if (_maxAge && (m->getAge_min() > _maxAge)) {
        // METAR is older than max-age, ignore
        SG_LOG( SG_ENVIRONMENT, SG_ALERT, "Ignoring outdated METAR for " << getStationId() << " (see /environment/params/metar-max-age-min)");
        return;
    }
  
    _failure = false;
    setMetar( m );
}

void LiveMetarProperties::handleMetarFailure()
{
  _failure = true;
}
  
/* -------------------------------------------------------------------------------- */

class BasicRealWxController : public RealWxController
{
public:
    BasicRealWxController( SGPropertyNode_ptr rootNode, MetarRequester * metarRequester );
    virtual ~BasicRealWxController ();

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void shutdown() override;
    void unbind() override;
    void update(double dt) override;
    
    /**
     * Create a metar-property binding at the specified property path,
     * and initiate a request for the specified station-ID (which may be
     * empty). If the property path is already mapped, the station ID
     * will be updated.
     */
    void addMetarAtPath(const string& propPath, const string& icao);
  
    void removeMetarAtPath(const string& propPath);

    typedef std::vector<LiveMetarProperties_ptr> MetarPropertiesList;
    MetarPropertiesList::iterator findMetarAtPath(const string &propPath);

protected:
    void checkNearbyMetar();

    long getMetarMaxAgeMin() const { return _max_age_n == NULL ? 0 : _max_age_n->getLongValue(); }

    SGPropertyNode_ptr _rootNode;
    SGPropertyNode_ptr _ground_elevation_n;
    SGPropertyNode_ptr _max_age_n;

    bool _enabled;
    bool _wasEnabled;
    simgear::TiedPropertyList _tiedProperties;
    MetarPropertiesList _metarProperties;
    MetarRequester* _requester;
};

static bool commandRequestMetar(const SGPropertyNode * arg, SGPropertyNode * root)
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
  std::transform(icao.begin(), icao.end(), icao.begin(), static_cast<int(*)(int)>(std::toupper));

  string path = arg->getStringValue("path");
  self->addMetarAtPath(path, icao);
  return true;
}
  
static bool commandClearMetar(const SGPropertyNode * arg, SGPropertyNode * root)
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
    
    globals->get_commands()->addCommand("request-metar", commandRequestMetar);
    globals->get_commands()->addCommand("clear-metar", commandClearMetar);
}

BasicRealWxController::~BasicRealWxController()
{
    globals->get_commands()->removeCommand("request-metar");
    globals->get_commands()->removeCommand("clear-metar");
}

void BasicRealWxController::init()
{
    _wasEnabled = false;
    
    // at least instantiate MetarProperties for /environment/metar
    SGPropertyNode_ptr metarNode = fgGetNode( _rootNode->getStringValue("metar", "/environment/metar"), true );
    _metarProperties.push_back( new LiveMetarProperties(metarNode,
                                                        _requester,
                                                        getMetarMaxAgeMin()));
    
    for( auto n : _rootNode->getChildren("metar") ) {
        SGPropertyNode_ptr metarNode = fgGetNode( n->getStringValue(), true );
        addMetarAtPath(metarNode->getPath(), "");
    }

    checkNearbyMetar();
    update(0); // fetch data ASAP
    
    globals->get_event_mgr()->addTask("checkNearbyMetar", this,
                                      &BasicRealWxController::checkNearbyMetar, 10 );
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
    for(auto p : _metarProperties) {
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
  MetarPropertiesList::iterator it = findMetarAtPath(propPath);
  if( it != _metarProperties.end() ) {
    SG_LOG( SG_ENVIRONMENT, SG_INFO, "Reusing metar properties at " << propPath << " for " << icao);
    // already exists
    if ((*it)->getStationId() != icao) {
      (*it)->setStationId(icao);
      (*it)->resetTimeToLive();
    }
    return;
  }

  SGPropertyNode_ptr metarNode = fgGetNode(propPath, true);
  SG_LOG( SG_ENVIRONMENT, SG_INFO, "Adding metar properties at " << propPath << " for " << icao);
  LiveMetarProperties_ptr p(new LiveMetarProperties( metarNode, _requester, getMetarMaxAgeMin() ));
  _metarProperties.push_back(p);
  p->setStationId(icao);
}

void BasicRealWxController::removeMetarAtPath(const string &propPath)
{
  MetarPropertiesList::iterator it = findMetarAtPath( propPath );
  if( it != _metarProperties.end() ) {
    SG_LOG(SG_ENVIRONMENT, SG_INFO, "removing metar properties at " << propPath);
    _metarProperties.erase(it);
  } else {
    SG_LOG(SG_ENVIRONMENT, SG_WARN, "no metar properties at " << propPath);
  }
}

BasicRealWxController::MetarPropertiesList::iterator BasicRealWxController::findMetarAtPath(const string &propPath)
{
  // don not compare unprocessed property path
  // /foo/bar[0]/baz equals /foo/bar/baz
  SGPropertyNode_ptr n = fgGetNode(propPath,false);
  if( false == n.valid() ) // trivial: node does not exist
    return _metarProperties.end();

  MetarPropertiesList::iterator it = _metarProperties.begin();
  while( it != _metarProperties.end() &&
         (*it)->get_root_node()->getPath() != n->getPath() )
    ++it;

  return it;
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

class NoaaMetarRealWxController : public BasicRealWxController, MetarRequester
{
public:
    NoaaMetarRealWxController( SGPropertyNode_ptr rootNode );

    // implementation of MetarRequester
    virtual void requestMetar( LiveMetarProperties_ptr metarDataHandler, const std::string & id );

    virtual ~NoaaMetarRealWxController()
    {
    }

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "noaa-metar-real-wx-controller"; }

private:
    std::string noaa_base_url;
};

NoaaMetarRealWxController::NoaaMetarRealWxController( SGPropertyNode_ptr rootNode ) :
  BasicRealWxController(rootNode, this )
{
    // default to hardcoded URL for compatibility
    noaa_base_url = "https://tgftp.nws.noaa.gov/data/observations/metar/stations/[station].TXT";

    // override with environment/realwx/metar-url (if present)
    SGPropertyNode *urlNode = _rootNode->getNode("metar-url", false);
    if (urlNode != nullptr)
        noaa_base_url = urlNode->getStringValue();
}

void NoaaMetarRealWxController::requestMetar
(
  LiveMetarProperties_ptr metarDataHandler,
  const std::string& id
)
{
  class NoaaMetarGetRequest:
    public simgear::HTTP::MemoryRequest
  {
    public:
      NoaaMetarGetRequest( LiveMetarProperties_ptr metarDataHandler,
                           const std::string& stationId, 
                           const std::string &base_url):
        MemoryRequest( simgear::strutils::replace(base_url, "[station]",stationId) ),
        _metarDataHandler(metarDataHandler)
      {
        std::ostringstream buf;
        buf <<  globals->get_time_params()->get_cur_time();
        requestHeader("X-TIME") = buf.str();
      }

      virtual void onDone()
      {
        if( responseCode() != 200 )
        {
          SG_LOG
          (
            SG_ENVIRONMENT,
            SG_WARN,
            "metar download failed:" << url() << ": reason:" << responseReason()
          );
          return;
        }

        _metarDataHandler->handleMetarData
        (
          simgear::strutils::simplify(responseBody())
        );
      }

      virtual void onFail()
      {
        SG_LOG(SG_ENVIRONMENT, SG_INFO, "metar download failure");
        _metarDataHandler->handleMetarFailure();
      }

    private:
      LiveMetarProperties_ptr _metarDataHandler;
  };

  string upperId = id;
  std::transform(upperId.begin(), upperId.end(), upperId.begin(), static_cast<int(*)(int)>(std::toupper));

  SG_LOG
  (
    SG_ENVIRONMENT,
    SG_INFO,
    "NoaaMetarRealWxController::update(): "
    "spawning load request for station-id '" << upperId << "'"
  );
  FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
  if (http) {
      http->makeRequest(new NoaaMetarGetRequest(metarDataHandler, upperId, noaa_base_url));
  }
}

// Register the subsystem.
#if 0
SGSubsystemMgr::Registrant<NoaaMetarRealWxController> registrantNoaaMetarRealWxController(
    SGSubsystemMgr::GENERAL,
    {{"environment", SGSubsystemMgr::Dependency::SOFT},
     {"FGHTTPClient", SGSubsystemMgr::Dependency::SOFT},
     {"realwx", SGSubsystemMgr::Dependency::SOFT}});
#endif


/* -------------------------------------------------------------------------------- */
    
RealWxController * RealWxController::createInstance( SGPropertyNode_ptr rootNode )
{
  return new NoaaMetarRealWxController( rootNode );
}
    
RealWxController::~RealWxController()
{
}

/* -------------------------------------------------------------------------------- */

} // namespace Environment
