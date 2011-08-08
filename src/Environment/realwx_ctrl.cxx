// realwx_ctrl.cxx -- Process real weather data
//
// Written by David Megginson, started February 2002.
// Rewritten by Torsten Dreyer, August 2010
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
#include "metarproperties.hxx"
#include "metarairportfilter.hxx"
#include "fgmetar.hxx"

#include <Main/fg_props.hxx>

#include <boost/foreach.hpp>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/timing/sg_time.hxx>

#include <algorithm>

using simgear::PropertyList;

namespace Environment {

/* -------------------------------------------------------------------------------- */

class LiveMetarProperties : public MetarProperties {
public:
    LiveMetarProperties( SGPropertyNode_ptr rootNode );
    virtual ~LiveMetarProperties();
    virtual void update( double dt );

    virtual double getTimeToLive() const { return _timeToLive; }
    virtual void setTimeToLive( double value ) { _timeToLive = value; }
private:
    double _timeToLive;

};

typedef SGSharedPtr<LiveMetarProperties> LiveMetarProperties_ptr;

LiveMetarProperties::LiveMetarProperties( SGPropertyNode_ptr rootNode ) :
    MetarProperties( rootNode ),
    _timeToLive(0.0)
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
    if( _timeToLive < 0.0 ) _timeToLive = 0.0;
}

/* -------------------------------------------------------------------------------- */

class BasicRealWxController : public RealWxController
{
public:
    BasicRealWxController( SGPropertyNode_ptr rootNode );
    virtual ~BasicRealWxController ();

    virtual void init ();
    virtual void reinit ();

protected:
    void bind();
    void unbind();
    void update( double dt );

    virtual void update( bool first, double dt ) = 0;

    long getMetarMaxAgeMin() const { return _max_age_n == NULL ? 0 : _max_age_n->getLongValue(); }

    SGPropertyNode_ptr _rootNode;
    SGPropertyNode_ptr _longitude_n;
    SGPropertyNode_ptr _latitude_n;
    SGPropertyNode_ptr _ground_elevation_n;
    SGPropertyNode_ptr _max_age_n;

    bool _enabled;
    bool __enabled;
    simgear::TiedPropertyList _tiedProperties;
    typedef std::vector<LiveMetarProperties_ptr> MetarPropertiesList;
    MetarPropertiesList _metarProperties;
    
};

/* -------------------------------------------------------------------------------- */
/*
Properties
 ~/enabled: bool              Enables/Disables the realwx controller
 ~/metar[1..n]: string        Target property path for metar data
 */

BasicRealWxController::BasicRealWxController( SGPropertyNode_ptr rootNode ) :
  _rootNode(rootNode),
  _longitude_n( fgGetNode( "/position/longitude-deg", true )),
  _latitude_n( fgGetNode( "/position/latitude-deg", true )),
  _ground_elevation_n( fgGetNode( "/position/ground-elev-m", true )),
  _max_age_n( fgGetNode( "/environment/params/metar-max-age-min", false ) ),
  _enabled(true),
  __enabled(false)
{
    // at least instantiate MetarProperties for /environment/metar
    _metarProperties.push_back( new LiveMetarProperties( 
            fgGetNode( rootNode->getStringValue("metar", "/environment/metar"), true ) ) );

    PropertyList metars = rootNode->getChildren("metar");
    for( PropertyList::size_type i = 1; i < metars.size(); i++ ) {
       SG_LOG( SG_ALL, SG_INFO, "Adding metar properties at " << metars[i]->getStringValue() );
        _metarProperties.push_back( new LiveMetarProperties( 
            fgGetNode( metars[i]->getStringValue(), true )));
    }
}

BasicRealWxController::~BasicRealWxController()
{
}

void BasicRealWxController::init()
{
    __enabled = false;
    update(0); // fetch data ASAP
}

void BasicRealWxController::reinit()
{
    __enabled = false;
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
    bool firstIteration = !__enabled; // first iteration after being enabled?

    // clock tick for every METAR in stock
    for( MetarPropertiesList::iterator it = _metarProperties.begin();
          it != _metarProperties.end(); it++ ) {
      // first round? All received METARs are outdated
      if( firstIteration ) (*it)->setTimeToLive( 0.0 );
      (*it)->update(dt);
    }

    update( firstIteration, dt );
    __enabled = true;
  } else {
    __enabled = false;
  }
}

/* -------------------------------------------------------------------------------- */

class NoaaMetarRealWxController : public BasicRealWxController {
public:
    NoaaMetarRealWxController( SGPropertyNode_ptr rootNode );
    virtual ~NoaaMetarRealWxController();
    virtual void update (bool first, double delta_time_sec);
    virtual void shutdown ();

    /**
     * callback from MetarGetRequest when a download succeeds
     */
    void gotMetar(const string& stationId, const string& metar);
private:
    double _positionTimeToLive;
    double _requestTimer;

    simgear::HTTP::Client _http;
};

class MetarGetRequest : public simgear::HTTP::Request
{
public:
    MetarGetRequest(NoaaMetarRealWxController* con, const string& s) :
        Request(""),
        stationId(s),
        fromProxy(false),
        wxController(con)
    {
        setUrl("http://weather.noaa.gov/pub/data/observations/metar/stations/" + stationId + ".TXT");
    }
    
    virtual string_list requestHeaders() const
    {
        string_list r;
        r.push_back("X-Time");
        return r;
    }
    
    virtual string header(const string& name) const
    {
        if (name == "X-Time") {
            char buf[16];
            snprintf(buf, 16, "%ld", globals->get_time_params()->get_cur_time());
            return buf;
        }
        
        return Request::header(name);     
    }
    
    virtual void responseHeader(const string& key, const string& value)
    {
        if (key == "x-metarproxy") {
            fromProxy = true;
        }
    }
    
    virtual void gotBodyData(const char* s, int n)
    {
        metar += string(s, n);
    }
    
    virtual void responseComplete()
    {
        if (responseCode() == 200) {
            wxController->gotMetar(stationId, metar);
        } else {
            SG_LOG(SG_IO, SG_WARN, "metar download failed:" << url() << ": reason:" << responseReason());
        }
    }
    
    bool fromMetarProxy() const
        { return fromProxy; }
private:  
    string stationId;
    string metar;
    bool fromProxy;
    NoaaMetarRealWxController* wxController;
};



NoaaMetarRealWxController::NoaaMetarRealWxController( SGPropertyNode_ptr rootNode ) :
  BasicRealWxController(rootNode),
  _positionTimeToLive(0.0),
  _requestTimer(0.0)
{
    string proxyHost(fgGetString("/sim/presets/proxy/host"));
    int proxyPort(fgGetInt("/sim/presets/proxy/port"));
    string proxyAuth(fgGetString("/sim/presets/proxy/auth"));
    
    if (!proxyHost.empty()) {
        _http.setProxy(proxyHost, proxyPort, proxyAuth);
    }
}

void NoaaMetarRealWxController::shutdown()
{
}

NoaaMetarRealWxController::~NoaaMetarRealWxController()
{
}

void NoaaMetarRealWxController::update( bool first, double dt )
{
    _http.update();
    
    _positionTimeToLive -= dt;
    _requestTimer -= dt;

    if( _positionTimeToLive <= 0.0 ) {
        // check nearest airport
        SG_LOG(SG_ALL, SG_INFO, "NoaaMetarRealWxController::update(): (re) checking nearby airport with METAR" );
        _positionTimeToLive = 60.0;

        SGGeod pos = SGGeod::fromDeg(_longitude_n->getDoubleValue(), _latitude_n->getDoubleValue());

        FGAirport * nearestAirport = FGAirport::findClosest(pos, 10000.0, MetarAirportFilter::instance() );
        if( nearestAirport == NULL ) {
            SG_LOG(SG_ALL,SG_WARN,"RealWxController::update can't find airport with METAR within 10000NM"  );
            return;
        }

        SG_LOG(SG_ALL, SG_INFO, 
            "NoaaMetarRealWxController::update(): nearest airport with METAR is: " << nearestAirport->ident() );

        // if it has changed, invalidate the associated METAR
        if( _metarProperties[0]->getStationId() != nearestAirport->ident() ) {
            SG_LOG(SG_ALL, SG_INFO, 
                "NoaaMetarRealWxController::update(): nearest airport with METAR has changed. Old: '" << 
                _metarProperties[0]->getStationId() <<
                "', new: '" << nearestAirport->ident() << "'" );
            _metarProperties[0]->setStationId( nearestAirport->ident() );
            _metarProperties[0]->setTimeToLive( 0.0 );
        }
    }
  
    if( _requestTimer <= 0.0 ) {
        _requestTimer = 10.0;
        
        BOOST_FOREACH(LiveMetarProperties* p, _metarProperties) {
            if( p->getTimeToLive() > 0.0 ) continue;
            const std::string & stationId = p->getStationId();
            if( stationId.empty() ) continue;

            SG_LOG(SG_ALL, SG_INFO, 
                "NoaaMetarRealWxController::update(): spawning load request for station-id '" << stationId << "'" );
            
            _http.makeRequest(new MetarGetRequest(this, stationId));
        } // of MetarProperties iteration
    }
}

void NoaaMetarRealWxController::gotMetar(const string& stationId, const string& metar)
{
    SG_LOG( SG_ALL, SG_INFO, "NoaaMetarRwalWxController::update() received METAR for " << stationId << ": " << metar );
    BOOST_FOREACH(LiveMetarProperties* p, _metarProperties) {
        if (p->getStationId() != stationId)
            continue;
            
        p->setTimeToLive(900);
        p->setMetar( metar );
    }
}

/* -------------------------------------------------------------------------------- */

RealWxController * RealWxController::createInstance( SGPropertyNode_ptr rootNode )
{
//    string dataSource = rootNode->getStringValue("data-source", "noaa" );
//    if( dataSource == "nwx" ) {
//      return new NwxMetarRealWxController( rootNode );
//    } else {
      return new NoaaMetarRealWxController( rootNode );
//    }
}

/* -------------------------------------------------------------------------------- */

} // namespace Environment
