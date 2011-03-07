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

#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <algorithm>
#if defined(ENABLE_THREADS)
#include <OpenThreads/Thread>
#include <simgear/threads/SGQueue.hxx>
#endif


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

    class MetarLoadRequest {
    public:
        MetarLoadRequest( const string & stationId ) :
            _stationId(stationId),
            _proxyHost(fgGetNode("/sim/presets/proxy/host", true)->getStringValue()),
            _proxyPort(fgGetNode("/sim/presets/proxy/port", true)->getStringValue()),
            _proxyAuth(fgGetNode("/sim/presets/proxy/authentication", true)->getStringValue())
        {}
        MetarLoadRequest( const MetarLoadRequest & other ) :
            _stationId(other._stationId),
            _proxyHost(other._proxyAuth),
            _proxyPort(other._proxyPort),
            _proxyAuth(other._proxyAuth)
        {}
        string _stationId;
        string _proxyHost;
        string _proxyPort;
        string _proxyAuth;
    private:
    };

    class MetarLoadResponse {
    public:
        MetarLoadResponse( const string & stationId, const string metar ) {
            _stationId = stationId;
            _metar = metar;
        }
        MetarLoadResponse( const MetarLoadResponse & other ) {
            _stationId = other._stationId;
            _metar = other._metar;
        }
        string _stationId;
        string _metar;
    };
private:
    double _positionTimeToLive;
    double _requestTimer;

#if defined(ENABLE_THREADS)
     class MetarLoadThread : public OpenThreads::Thread {
     public:
        MetarLoadThread( long maxAge );
        virtual ~MetarLoadThread( ) { stop(); } 
        void requestMetar( const MetarLoadRequest & metarRequest, bool background = true );
        bool hasMetar() { return _responseQueue.size() > 0; }
        MetarLoadResponse getMetar() { return _responseQueue.pop(); }
        virtual void run();
        void stop();
     private:
        void fetch( const MetarLoadRequest & );
        long _maxAge;
        long _minRequestInterval;
        volatile bool _stop;
        SGBlockingQueue <MetarLoadRequest> _requestQueue;
        SGBlockingQueue <MetarLoadResponse> _responseQueue;
     };

     MetarLoadThread * _metarLoadThread;
#endif
};

NoaaMetarRealWxController::NoaaMetarRealWxController( SGPropertyNode_ptr rootNode ) :
  BasicRealWxController(rootNode),
  _positionTimeToLive(0.0),
  _requestTimer(0.0)
{
#if defined(ENABLE_THREADS)
    _metarLoadThread = new MetarLoadThread(getMetarMaxAgeMin());
    _metarLoadThread->start();
#endif
}

void NoaaMetarRealWxController::shutdown()
{
#if defined(ENABLE_THREADS)
    if( _metarLoadThread ) {
        delete _metarLoadThread;
        _metarLoadThread = NULL;
    }
#endif // ENABLE_THREADS
}

NoaaMetarRealWxController::~NoaaMetarRealWxController()
{
}

void NoaaMetarRealWxController::update( bool first, double dt )
{
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

        for( MetarPropertiesList::iterator it = _metarProperties.begin(); 
            it != _metarProperties.end(); it++ ) {

                if( (*it)->getTimeToLive() > 0.0 ) continue;
                const std::string & stationId = (*it)->getStationId();
                if( stationId.empty() ) continue;

                SG_LOG(SG_ALL, SG_INFO, 
                    "NoaaMetarRealWxController::update(): spawning load request for station-id '" << stationId << "'" );
            
                MetarLoadRequest request( stationId );
                // load the metar for the nearest airport in the foreground if the fdm is uninitialized
                // to make sure a metar is received
                // before the automatic runway selection code runs. All subsequent calls
                // run in the background
                bool background = fgGetBool("/sim/fdm-initialized", false ) || it != _metarProperties.begin();
                _metarLoadThread->requestMetar( request, background );
        }
    }

    // pick all the received responses from the result queue and update the associated
    // property tree
    while( _metarLoadThread->hasMetar() ) {
        MetarLoadResponse metar = _metarLoadThread->getMetar();
        SG_LOG( SG_ALL, SG_INFO, "NoaaMetarRwalWxController::update() received METAR for " << metar._stationId << ": " << metar._metar );
        for( MetarPropertiesList::iterator it = _metarProperties.begin(); 
            it != _metarProperties.end(); it++ ) {
                if( (*it)->getStationId() != metar._stationId )
                    continue;
                (*it)->setTimeToLive(900);
                (*it)->setMetar( metar._metar );
        }
    }
}

/* -------------------------------------------------------------------------------- */

#if defined(ENABLE_THREADS)
NoaaMetarRealWxController::MetarLoadThread::MetarLoadThread( long maxAge ) :
  _maxAge(maxAge),
  _minRequestInterval(2000),
  _stop(false)
{
}

void NoaaMetarRealWxController::MetarLoadThread::requestMetar( const MetarLoadRequest & metarRequest, bool background )
{
    if( background ) {
        if( _requestQueue.size() > 10 ) {
            SG_LOG(SG_ALL,SG_ALERT,
                "NoaaMetarRealWxController::MetarLoadThread::requestMetar() more than 10 outstanding METAR requests, dropping " 
                << metarRequest._stationId );
            return;
        }

        _requestQueue.push( metarRequest );
    } else {
        fetch( metarRequest );
    }
}

void NoaaMetarRealWxController::MetarLoadThread::stop()
{
    // set stop flag and wake up the thread with an empty request
    _stop = true;
    MetarLoadRequest request("");
    requestMetar(request);
    join();
}

void NoaaMetarRealWxController::MetarLoadThread::run()
{
    SGTimeStamp lastRun = SGTimeStamp::fromSec(0);
    for( ;; ) {
        SGTimeStamp dt = SGTimeStamp::now() - lastRun;

        long delayMs = _minRequestInterval - dt.getSeconds() * 1000;
        while (( delayMs > 0 ) && !_stop)
        {
            // sleep no more than 3 seconds at a time, otherwise shutdown response is too slow
            long sleepMs = (delayMs>3000) ? 3000 : delayMs; 
            microSleep( sleepMs * 1000 );
            delayMs -= sleepMs;
        }

        if (_stop)
            break;

        lastRun = SGTimeStamp::now();
 
        const MetarLoadRequest request = _requestQueue.pop();

        if (( request._stationId.size() == 0 ) || _stop)
            break;

        fetch( request );
    }
}

void NoaaMetarRealWxController::MetarLoadThread::fetch( const MetarLoadRequest & request )
{
   SGSharedPtr<FGMetar> result = NULL;

    try {
        result = new FGMetar( request._stationId, request._proxyHost, request._proxyPort, request._proxyAuth );
        _minRequestInterval = 2000;
    } catch (const sg_io_exception& e) {
        SG_LOG( SG_GENERAL, SG_WARN, "NoaaMetarRealWxController::fetchMetar(): can't get METAR for " 
                                    << request._stationId << ":" << e.getFormattedMessage().c_str() );
        _minRequestInterval += _minRequestInterval/2; 
        if( _minRequestInterval > 30000 )
            _minRequestInterval = 30000;
        return;
    }

    string reply = result->getData();
    std::replace(reply.begin(), reply.end(), '\n', ' ');
    string metar = simgear::strutils::strip( reply );

    if( metar.empty() ) {
        SG_LOG( SG_GENERAL, SG_WARN, "NoaaMetarRealWxController::fetchMetar(): dropping empty METAR for " 
                                    << request._stationId );
        return;
    }

    if( _maxAge && result->getAge_min() > _maxAge ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "NoaaMetarRealWxController::fetchMetar(): dropping outdated METAR " 
                                     << metar );
        return;
    }

    MetarLoadResponse response( request._stationId, metar );
    _responseQueue.push( response );
}
#endif

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
