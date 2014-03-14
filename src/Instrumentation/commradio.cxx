// commradio.cxx -- class to manage a nav radio instance
//
// Written by Torsten Dreyer, February 2014
//
// Copyright (C) 2000 - 2011  Curtis L. Olson - http://www.flightgear.org/~curt
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
#  include <config.h>
#endif

#include "commradio.hxx"

#include <assert.h>
#include <boost/foreach.hpp>

#include <simgear/sg_inlines.h>
#include <simgear/props/propertyObject.hxx>
#include <simgear/misc/strutils.hxx>

#include <ATC/CommStation.hxx>
#include <ATC/MetarPropertiesATISInformationProvider.hxx>
#include <ATC/CurrentWeatherATISInformationProvider.hxx>
#include <Airports/airport.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/navlist.hxx>

#include "frequencyformatter.hxx"

namespace Instrumentation {

using simgear::PropertyObject;
using std::string;


SignalQualityComputer::~SignalQualityComputer()
{
}

class SimpleDistanceSquareSignalQualityComputer : public SignalQualityComputer
{
public:
  SimpleDistanceSquareSignalQualityComputer( double range ) : _rangeM(range), _rangeM2(range*range) {}
  virtual double computeSignalQuality( const SGGeod & sender, const SGGeod & receiver ) const;
  virtual double computeSignalQuality( const SGVec3d & sender, const SGVec3d & receiver ) const;
  virtual double computeSignalQuality( double slantDistanceM ) const;
private:
  double _rangeM;
  double _rangeM2;
};

double SimpleDistanceSquareSignalQualityComputer::computeSignalQuality( const SGVec3d & sender, const SGVec3d & receiver ) const
{
    return computeSignalQuality( dist( sender, receiver ) );
}

double SimpleDistanceSquareSignalQualityComputer::computeSignalQuality( const SGGeod & sender, const SGGeod & receiver ) const
{
    return computeSignalQuality( SGGeodesy::distanceM( sender, receiver ) );
}

double SimpleDistanceSquareSignalQualityComputer::computeSignalQuality( double distanceM ) const
{
    return distanceM < _rangeM ? 1.0 : ( _rangeM2 / (distanceM*distanceM) );
}

class OnExitHandler {
  public:
    virtual void onExit() = 0;
    virtual ~OnExitHandler() {}
};

class OnExit {
  public:
    OnExit( OnExitHandler * onExitHandler ) : _onExitHandler( onExitHandler ) {}
    ~OnExit() { _onExitHandler->onExit(); }
  private:
    OnExitHandler * _onExitHandler;
};


class OutputProperties : public OnExitHandler {
  public:
    OutputProperties( SGPropertyNode_ptr rootNode ) : 
      _rootNode(rootNode),
      _signalQuality_norm(0.0),
      _slantDistance_m(0.0),
      _trueBearingTo_deg(0.0),
      _trueBearingFrom_deg(0.0),
      _trackDistance_m(0.0),
      _heightAboveStation_ft(0.0),

      _PO_stationType( rootNode->getNode("station-type", true ) ),
      _PO_stationName( rootNode->getNode("station-name", true ) ),
      _PO_airportId( rootNode->getNode("airport-id", true ) ),
      _PO_signalQuality_norm( rootNode->getNode("signal-quality-norm",true) ),
      _PO_slantDistance_m( rootNode->getNode("slant-distance-m",true) ),
      _PO_trueBearingTo_deg( rootNode->getNode("true-bearing-to-deg",true) ),
      _PO_trueBearingFrom_deg( rootNode->getNode("true-bearing-from-deg",true) ),
      _PO_trackDistance_m( rootNode->getNode("track-distance-m",true) ),
      _PO_heightAboveStation_ft( rootNode->getNode("height-above-station-ft",true) )
      {}
    virtual ~OutputProperties() {}

protected:
    SGPropertyNode_ptr _rootNode;

    std::string  _stationType;
    std::string  _stationName;
    std::string  _airportId;
    double       _signalQuality_norm;
    double       _slantDistance_m;
    double       _trueBearingTo_deg;
    double       _trueBearingFrom_deg;
    double       _trackDistance_m;
    double       _heightAboveStation_ft;

private:
    PropertyObject<string> _PO_stationType;
    PropertyObject<string> _PO_stationName;
    PropertyObject<string> _PO_airportId;
    PropertyObject<double> _PO_signalQuality_norm;
    PropertyObject<double> _PO_slantDistance_m;
    PropertyObject<double> _PO_trueBearingTo_deg;
    PropertyObject<double> _PO_trueBearingFrom_deg;
    PropertyObject<double> _PO_trackDistance_m;
    PropertyObject<double> _PO_heightAboveStation_ft;

    virtual void onExit() {
        _PO_stationType = _stationType;
        _PO_stationName = _stationName;
        _PO_airportId = _airportId;
        _PO_signalQuality_norm = _signalQuality_norm;
        _PO_slantDistance_m = _slantDistance_m;
        _PO_trueBearingTo_deg = _trueBearingTo_deg;
        _PO_trueBearingFrom_deg = _trueBearingFrom_deg;
        _PO_trackDistance_m = _trackDistance_m;
        _PO_heightAboveStation_ft = _heightAboveStation_ft;
    }
};

/* ------------- The CommRadio implementation ---------------------- */

class MetarBridge : public SGReferenced, public SGPropertyChangeListener {
public:
  MetarBridge();
  ~MetarBridge();

  void bind();
  void unbind();
  void requestMetarForId( std::string & id );
  void clearMetar();
  void setMetarPropertiesRoot( SGPropertyNode_ptr n ) { _metarPropertiesNode = n; }
  void setAtisNode( SGPropertyNode * n ) { _atisNode = n; }

protected:
  virtual void valueChanged(SGPropertyNode * );

private:
  std::string _requestedId;
  SGPropertyNode_ptr _realWxEnabledNode;
  SGPropertyNode_ptr _metarPropertiesNode;
  SGPropertyNode * _atisNode;
  ATISEncoder _atisEncoder;
};
typedef SGSharedPtr<MetarBridge> MetarBridgeRef;

MetarBridge::MetarBridge() :
  _atisNode(0)
{
}

MetarBridge::~MetarBridge()
{
}

void MetarBridge::bind()
{
  _realWxEnabledNode = fgGetNode( "/environment/realwx/enabled", true );
  _metarPropertiesNode->getNode( "valid", true )->addChangeListener( this );
}

void MetarBridge::unbind()
{
  _metarPropertiesNode->getNode( "valid", true )->removeChangeListener( this );
}

void MetarBridge::requestMetarForId( std::string & id )
{
  std::string uppercaseId = simgear::strutils::uppercase( id );
  if( _requestedId == uppercaseId ) return;
  _requestedId = uppercaseId;

  if( _realWxEnabledNode->getBoolValue() ) {
    //  trigger a METAR request for the associated metarproperties
    _metarPropertiesNode->getNode( "station-id", true )->setStringValue( uppercaseId );
    _metarPropertiesNode->getNode( "valid", true )->setBoolValue( false );
    _metarPropertiesNode->getNode( "time-to-live", true )->setDoubleValue( 0.0 );
  } else  {
    // use the present weather to generate the ATIS. 
    if( NULL != _atisNode && false == _requestedId.empty() ) {
      CurrentWeatherATISInformationProvider provider( _requestedId );
      _atisNode->setStringValue( _atisEncoder.encodeATIS( &provider ) );
    }
  }
}

void MetarBridge::clearMetar()
{
  string empty;
  requestMetarForId( empty );
}

void MetarBridge::valueChanged(SGPropertyNode * node )
{
  // check for raising edge of valid flag
  if( NULL == node || false == node->getBoolValue() || false == _realWxEnabledNode->getBoolValue() )
    return;

  std::string responseId = simgear::strutils::uppercase(
     _metarPropertiesNode->getNode( "station-id", true )->getStringValue() );

  // unrequested metar!?
  if( responseId != _requestedId )
    return;

  if( NULL != _atisNode ) {
    MetarPropertiesATISInformationProvider provider( _metarPropertiesNode );
    _atisNode->setStringValue( _atisEncoder.encodeATIS( &provider ) );
  }
}

/* ------------- The CommRadio implementation ---------------------- */

class CommRadioImpl : public CommRadio, OutputProperties {

public:
  CommRadioImpl( SGPropertyNode_ptr node );
  virtual ~CommRadioImpl();

  virtual void update( double dt );
  virtual void init();
  void bind();
  void unbind();

private:
  int                _num;
  MetarBridgeRef     _metarBridge;
  FrequencyFormatter _useFrequencyFormatter;
  FrequencyFormatter _stbyFrequencyFormatter;
  const SignalQualityComputerRef _signalQualityComputer; 

  double _stationTTL;
  double _frequency;
  flightgear::CommStationRef _commStationForFrequency;

  PropertyObject<bool>   _serviceable;
  PropertyObject<bool>   _power_btn;
  PropertyObject<bool>   _power_good;
  PropertyObject<double> _volume_norm;
  PropertyObject<string> _atis;


};

CommRadioImpl::CommRadioImpl( SGPropertyNode_ptr node ) :
    OutputProperties( fgGetNode("/instrumentation",true)->getNode(
                        node->getStringValue("name", "comm"),
                        node->getIntValue("number", 0), true)),
    _num( node->getIntValue("number",0)),
    _metarBridge( new MetarBridge() ),
    _useFrequencyFormatter( _rootNode->getNode("frequencies/selected-mhz",true), 
                            _rootNode->getNode("frequencies/selected-mhz-fmt",true), 0.025, 118.0, 136.0 ),
    _stbyFrequencyFormatter( _rootNode->getNode("frequencies/standby-mhz",true), 
                            _rootNode->getNode("frequencies/standby-mhz-fmt",true), 0.025, 118.0, 136.0 ),
    _signalQualityComputer( new SimpleDistanceSquareSignalQualityComputer(10*SG_NM_TO_METER) ),

    _stationTTL(0.0),
    _frequency(-1.0),
    _commStationForFrequency(NULL),

    _serviceable( _rootNode->getNode("serviceable",true) ),
    _power_btn( _rootNode->getNode("power-btn",true) ),
    _power_good( _rootNode->getNode("power-good",true) ),
    _volume_norm( _rootNode->getNode("volume",true) ),
    _atis( _rootNode->getNode("atis",true) )
{
}

CommRadioImpl::~CommRadioImpl()
{
}

void CommRadioImpl::bind()
{
  _metarBridge->setAtisNode( _atis.node() );
   // link the metar node. /environment/metar[3] is comm1 and /environment[4] is comm2.
   // see FGDATA/Environment/environment.xml
  _metarBridge->setMetarPropertiesRoot( fgGetNode( "/environment",true)->getNode("metar", _num+3, true ) );
  _metarBridge->bind();
}

void CommRadioImpl::unbind()
{
  _metarBridge->unbind();
}

void CommRadioImpl::init()
{
}

void CommRadioImpl::update( double dt )
{
    if( dt < SGLimitsd::min() ) return;
    _stationTTL -= dt;

    // Ensure all output properties get written on exit of this method
    OnExit onExit(this);

    SGGeod position;
    try { position = globals->get_aircraft_position(); }
    catch( std::exception & ) { return; }

    if( false == (_power_btn )) {
        _stationTTL = 0.0;
        return;
    }


    if( _frequency != _useFrequencyFormatter.getFrequency() ) {
        _frequency = _useFrequencyFormatter.getFrequency();
        _stationTTL = 0.0;
    }

    if( _stationTTL <= 0.0 ) {
        _stationTTL = 30.0;

        // Note:  122.375 must be rounded DOWN to 122370
        // in order to be consistent with apt.dat et cetera.
        int freqKhz = 10 * static_cast<int>(_frequency * 100 + 0.25);
        _commStationForFrequency = flightgear::CommStation::findByFreq( freqKhz, position, NULL );

    }

    if( false == _commStationForFrequency.valid() ) return;

    _slantDistance_m = dist(_commStationForFrequency->cart(), SGVec3d::fromGeod(position));

    SGGeodesy::inverse(position, _commStationForFrequency->geod(), 
        _trueBearingTo_deg,
        _trueBearingFrom_deg,
        _trackDistance_m );

    _heightAboveStation_ft = 
         SGMiscd::max(0.0, position.getElevationFt() 
           - _commStationForFrequency->airport()->elevation());
    
    _signalQuality_norm = _signalQualityComputer->computeSignalQuality( _slantDistance_m );
    _stationType = _commStationForFrequency->nameForType( _commStationForFrequency->type() );
    _stationName = _commStationForFrequency->ident();
    _airportId = _commStationForFrequency->airport()->getId();

    switch( _commStationForFrequency->type() ) {
      case FGPositioned::FREQ_ATIS:
      case FGPositioned::FREQ_AWOS: {
        if( _signalQuality_norm > 0.01 ) {
          _metarBridge->requestMetarForId( _airportId );
        } else {
          _metarBridge->clearMetar();
          _atis = "";
        }
      }
      break;

      default:
        _metarBridge->clearMetar();
        _atis = "";
        break;
    }

}

SGSubsystem * CommRadio::createInstance( SGPropertyNode_ptr rootNode )
{
    return new CommRadioImpl( rootNode );
}

} // namespace Instrumentation

