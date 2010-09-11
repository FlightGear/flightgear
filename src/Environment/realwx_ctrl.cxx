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
#include "tiedpropertylist.hxx"
#include "metarproperties.hxx"
#include "metarairportfilter.hxx"
#include "fgmetar.hxx"

#include <Main/fg_props.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <algorithm>
#if defined(ENABLE_THREADS)
#include <OpenThreads/Thread>
#include <simgear/threads/SGQueue.hxx>
#endif


namespace Environment {

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

    SGPropertyNode_ptr _rootNode;
    SGPropertyNode_ptr _longitude_n;
    SGPropertyNode_ptr _latitude_n;
    SGPropertyNode_ptr _ground_elevation_n;

    bool _enabled;
    TiedPropertyList _tiedProperties;
    MetarProperties  _metarProperties;
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
  _enabled(true),
  _metarProperties( fgGetNode( rootNode->getStringValue("metar", "/environment/metar"), true ) )
{
}

BasicRealWxController::~BasicRealWxController()
{
}

void BasicRealWxController::init()
{
    update(0); // fetch data ASAP
}

void BasicRealWxController::reinit()
{
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

/* -------------------------------------------------------------------------------- */

class NoaaMetarRealWxController : public BasicRealWxController {
public:
    NoaaMetarRealWxController( SGPropertyNode_ptr rootNode );
    virtual ~NoaaMetarRealWxController();
    virtual void update (double delta_time_sec);

    class MetarLoadRequest {
    public:
        MetarLoadRequest( const string & stationId ) {
            _stationId = stationId;
            _proxyHost = fgGetNode("/sim/presets/proxy/host", true)->getStringValue();
            _proxyPort = fgGetNode("/sim/presets/proxy/port", true)->getStringValue();
            _proxyAuth = fgGetNode("/sim/presets/proxy/authentication", true)->getStringValue();
        }
        string _stationId;
        string _proxyHost;
        string _proxyPort;
        string _proxyAuth;
    private:
    };
private:
    double _metarTimeToLive;
    double _positionTimeToLive;
    double _minimumRequestInterval;
    
    SGPropertyNode_ptr _metarDataNode;
    SGPropertyNode_ptr _metarValidNode;
    SGPropertyNode_ptr _metarStationIdNode;


#if defined(ENABLE_THREADS)
     class MetarLoadThread : public OpenThreads::Thread {
     public:
         MetarLoadThread( NoaaMetarRealWxController & controller );
         void requestMetar( const MetarLoadRequest & metarRequest );
         bool hasMetar() { return _responseQueue.size() > 0; }
         string getMetar() { return _responseQueue.pop(); }
         virtual void run();
     private:
        NoaaMetarRealWxController & _controller;
        SGBlockingQueue <MetarLoadRequest> _requestQueue;
        SGBlockingQueue <string> _responseQueue;
     };

     MetarLoadThread * _metarLoadThread;
#endif
};

NoaaMetarRealWxController::NoaaMetarRealWxController( SGPropertyNode_ptr rootNode ) :
  BasicRealWxController(rootNode),
  _metarTimeToLive(0.0),
  _positionTimeToLive(0.0),
  _minimumRequestInterval(0.0),
  _metarDataNode(_metarProperties.get_root_node()->getNode("data",true)),
  _metarValidNode(_metarProperties.get_root_node()->getNode("valid",true)),
  _metarStationIdNode(_metarProperties.get_root_node()->getNode("station-id",true))
{
#if defined(ENABLE_THREADS)
    _metarLoadThread = new MetarLoadThread(*this);
    _metarLoadThread->start();
#endif
}

NoaaMetarRealWxController::~NoaaMetarRealWxController()
{
#if defined(ENABLE_THREADS)
    if( _metarLoadThread ) {
        MetarLoadRequest request("");
        _metarLoadThread->requestMetar(request);
        _metarLoadThread->join();
        delete _metarLoadThread;
    }
#endif // ENABLE_THREADS
}

void NoaaMetarRealWxController::update( double dt )
{
    if( !_enabled )
        return;

    if( _metarLoadThread->hasMetar() )
        _metarDataNode->setStringValue( _metarLoadThread->getMetar() );

    _metarTimeToLive -= dt;
    _positionTimeToLive -= dt;
    _minimumRequestInterval -= dt;

    bool valid = _metarValidNode->getBoolValue();
    string stationId = valid ? _metarStationIdNode->getStringValue() : "";


    if( _metarTimeToLive <= 0.0 ) {
        valid = false;
        _metarTimeToLive = 900;
        _positionTimeToLive = 0;
    }

    if( _positionTimeToLive <= 0.0 || valid == false ) {
        _positionTimeToLive = 60.0;

        SGGeod pos = SGGeod::fromDeg(_longitude_n->getDoubleValue(), _latitude_n->getDoubleValue());

        FGAirport * nearestAirport = FGAirport::findClosest(pos, 10000.0, MetarAirportFilter::instance() );
        if( nearestAirport == NULL ) {
            SG_LOG(SG_ALL,SG_WARN,"RealWxController::update can't find airport with METAR within 10000NM"  );
            return;
        }

        if( stationId != nearestAirport->ident() ) {
            valid = false;
            stationId = nearestAirport->ident();
        }

    }

    if( !valid ) {
        if( _minimumRequestInterval <= 0 && stationId.length() > 0 ) {
            MetarLoadRequest request( stationId );
            _metarLoadThread->requestMetar( request );
            _minimumRequestInterval = 10;
        }
    }

}

/* -------------------------------------------------------------------------------- */

#if defined(ENABLE_THREADS)
NoaaMetarRealWxController::MetarLoadThread::MetarLoadThread( NoaaMetarRealWxController & controller ) :
  _controller(controller)
{
}

void NoaaMetarRealWxController::MetarLoadThread::requestMetar( const MetarLoadRequest & metarRequest )
{
    if( _requestQueue.size() > 10 ) {
        SG_LOG(SG_ALL,SG_ALERT,
            "NoaaMetarRealWxController::MetarLoadThread::requestMetar() more than 10 outstanding METAR requests, dropping " 
            << metarRequest._stationId );
        return;
    }

    _requestQueue.push( metarRequest );
}

void NoaaMetarRealWxController::MetarLoadThread::run()
{
    for( ;; ) {
        const MetarLoadRequest request = _requestQueue.pop();

        if( request._stationId.size() == 0 )
            break;

       SGSharedPtr<FGMetar> result = NULL;

        try {
            result = new FGMetar( request._stationId, request._proxyHost, request._proxyPort, request._proxyAuth );
        } catch (const sg_io_exception& e) {
            SG_LOG( SG_GENERAL, SG_WARN, "NoaaMetarRealWxController::fetchMetar(): can't get METAR for " 
                                        << request._stationId << ":" << e.getFormattedMessage().c_str() );
            continue;
        }

        if( result == NULL )
            continue;

        string reply = result->getData();
        std::replace(reply.begin(), reply.end(), '\n', ' ');
        string metar = simgear::strutils::strip( reply );
        if( metar.length() > 0 )
            _responseQueue.push( metar );
    }
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
