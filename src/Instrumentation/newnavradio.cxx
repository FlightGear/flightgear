// navradio.cxx -- class to manage a nav radio instance
//
// Written by Curtis Olson, started April 2000.
// Rewritten by Torsten Dreyer, August 2011
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

#include "newnavradio.hxx"

#include <assert.h>
#include <boost/foreach.hpp>

#include <simgear/math/interpolater.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/props/propertyObject.hxx>
#include <simgear/misc/strutils.hxx>

#include <Main/fg_props.hxx>
#include <Navaids/navlist.hxx>
#include <Sound/audioident.hxx>

#include "navradio.hxx"


namespace Instrumentation {

using simgear::PropertyObject;

/* --------------The Navigation Indicator ----------------------------- */

class NavIndicator {
public:
    NavIndicator( SGPropertyNode * rootNode ) :
      _cdi( rootNode->getNode("heading-needle-deflection", true ) ),
      _cdiNorm( rootNode->getNode("heading-needle-deflection-norm", true ) ),
      _course( rootNode->getNode("radials/selected-deg", true ) ),
      _toFlag( rootNode->getNode("to-flag", true ) ),
      _fromFlag( rootNode->getNode("from-flag", true ) ),
      _signalQuality( rootNode->getNode("signal-quality-norm", true ) ),
      _hasGS( rootNode->getNode("has-gs", true ) ),
      _gsDeflection(rootNode->getNode("gs-needle-deflection", true )),
      _gsDeflectionDeg(rootNode->getNode("gs-needle-deflection-deg", true )),
      _gsDeflectionNorm(rootNode->getNode("gs-needle-deflection-norm", true ))
  {
  }

  virtual ~NavIndicator() {}

  /**
   * set the normalized CDI deflection
   * @param norm the cdi deflection normalized [-1..1]
   */
  void setCDI( double norm )
  {
      _cdi = norm * 10.0;
      _cdiNorm = norm;
  }

  /**
   * set the normalized GS deflection
   * @param norm the gs deflection normalized to [-1..1]
   */
  void setGS( double norm )
  {
      _gsDeflectionNorm = norm;
      _gsDeflectionDeg = norm * 0.7;
      _gsDeflection = norm * 3.5;
  }

  void setGS( bool enabled )
  {
      _hasGS = enabled;
      if( !enabled ) {
        setGS( 0.0 );
      }
  }

  void showFrom( bool on )
  {
      _fromFlag = on;
  }

  void showTo( bool on )
  {
      _toFlag = on;
  }
      
  void setSelectedCourse( double course )
  {
      _course = course;
  }

  double getSelectedCourse() const
  {
      return SGMiscd::normalizePeriodic(0.0, 360.0, _course );
  }

  void setSignalQuality( double signalQuality )
  {
      _signalQuality = signalQuality;
  }

private:
  PropertyObject<double> _cdi;
  PropertyObject<double> _cdiNorm;
  PropertyObject<double> _course;
  PropertyObject<double> _toFlag;
  PropertyObject<double> _fromFlag;
  PropertyObject<double> _signalQuality;
  PropertyObject<double> _hasGS;
  PropertyObject<double> _gsDeflection;
  PropertyObject<double> _gsDeflectionDeg;
  PropertyObject<double> _gsDeflectionNorm;
};

/* ---------------------------------------------------------------- */

class NavRadioComponent {
public:
  NavRadioComponent( const std::string & name, SGPropertyNode_ptr rootNode );
  virtual ~NavRadioComponent();

  virtual void   update( double dt, const SGGeod & aircraftPosition );
  virtual void   search( double frequency, const SGGeod & aircraftPosition );
  virtual double getRange_nm( const SGGeod & aircraftPosition );
  virtual void   display( NavIndicator & navIndicator ) = 0;
  virtual bool   valid() const { return NULL != _navRecord && true == _serviceable; }
  virtual const std::string getIdent() const { return _ident; }

protected:
  virtual double computeSignalQuality_norm( const SGGeod & aircraftPosition );
  virtual FGNavList * getNavaidList() = 0;

  // General-purpose sawtooth function.  Graph looks like this:
  //         /\                                    .
  //       \/
  // Odd symmetry, inversion symmetry about the origin.
  // Unit slope at the origin.
  // Max 1, min -1, period 4.
  // Two zero-crossings per period, one with + slope, one with - slope.
  // Useful for false localizer courses.
  static double sawtooth(double xx)
  {
    return 4.0 * fabs(xx/4.0 + 0.25 - floor(xx/4.0 + 0.75)) - 1.0;
  }

  SGPropertyNode_ptr _rootNode;
  const std::string _name;
  FGNavRecord * _navRecord;
  PropertyObject<bool>   _serviceable;
  PropertyObject<double> _signalQuality_norm;
  PropertyObject<double> _trueBearingTo_deg;
  PropertyObject<double> _trueBearingFrom_deg;
  PropertyObject<double> _trackDistance_m;
  PropertyObject<double> _slantDistance_m;
  PropertyObject<double> _heightAboveStation_ft;
  PropertyObject<string> _ident;
  PropertyObject<bool>   _inRange;
  PropertyObject<double> _range_nm;
};

class NavRadioComponentWithIdent : public NavRadioComponent {
public:
  NavRadioComponentWithIdent( const std::string & name, SGPropertyNode_ptr rootNode, AudioIdent * audioIdent );
  virtual ~NavRadioComponentWithIdent();
  void update( double dt, const SGGeod & aircraftPosition );
protected:
  static std::string getIdentString( const std::string & name, int index );
private:
  AudioIdent * _audioIdent;
  PropertyObject<double> _identVolume;
  PropertyObject<bool>   _identEnabled;
};

std::string NavRadioComponentWithIdent::getIdentString( const std::string & name, int index )
{
  std::ostringstream temp;
  temp << name << "-ident-" << index;
  return temp.str();
}

NavRadioComponentWithIdent::NavRadioComponentWithIdent( const std::string & name, SGPropertyNode_ptr rootNode, AudioIdent * audioIdent ) :
  NavRadioComponent( name, rootNode ),
  _audioIdent( audioIdent ),
  _identVolume( rootNode->getNode(name,true)->getNode("ident-volume",true) ),
  _identEnabled( rootNode->getNode(name,true)->getNode("ident-enabled",true) )
{
  _audioIdent->init();

}
NavRadioComponentWithIdent::~NavRadioComponentWithIdent()
{
  delete _audioIdent;
}

void NavRadioComponentWithIdent::update( double dt, const SGGeod & aircraftPosition )
{
  NavRadioComponent::update( dt, aircraftPosition );
  _audioIdent->update( dt );

  if( false == ( valid() && _identEnabled && _signalQuality_norm > 0.1 ) ) {
      _audioIdent->setIdent("", 0.0 );
      return;
  }
  _audioIdent->setIdent( _ident, SGMiscd::clip(_identVolume, 0.0, 1.0) );
}

NavRadioComponent::NavRadioComponent( const std::string & name, SGPropertyNode_ptr rootNode ) :
  _rootNode(rootNode),
  _name(name),
  _navRecord(NULL),
  _serviceable( rootNode->getNode(name,true)->getNode("serviceable",true) ),
  _signalQuality_norm( rootNode->getNode(name,true)->getNode("signal-quality-norm",true) ),
  _trueBearingTo_deg( rootNode->getNode(name,true)->getNode("true-bearing-to-deg",true) ),
  _trueBearingFrom_deg( rootNode->getNode(name,true)->getNode("true-bearing-from-deg",true) ),
  _trackDistance_m( rootNode->getNode(name,true)->getNode("track-distance-m",true) ),
  _slantDistance_m( rootNode->getNode(name,true)->getNode("slant-distance-m",true) ),
  _heightAboveStation_ft( rootNode->getNode(name,true)->getNode("height-above-station-ft",true) ),
  _ident( rootNode->getNode(name,true)->getNode("ident",true) ),
  _inRange( rootNode->getNode(name,true)->getNode("in-range",true) ),
  _range_nm( rootNode->getNode(_name,true)->getNode("range-nm",true) )
{
  simgear::props::Type typ = _serviceable.node()->getType();
  if ((typ == simgear::props::NONE) || (typ == simgear::props::UNSPECIFIED))
    _serviceable = true;
}

NavRadioComponent::~NavRadioComponent()
{
}

double NavRadioComponent::getRange_nm( const SGGeod & aircraftPosition )
{ 
  if( _navRecord == NULL ) return 0.0; // no station: no range
  double d = _navRecord->get_range();
  if( d <= SGLimitsd::min() ) return 25.0; // no configured range: arbitrary number
  return d; // configured range
}

void NavRadioComponent::search( double frequency, const SGGeod & aircraftPosition )
{
  if( NULL == (_navRecord = getNavaidList()->findByFreq(frequency, aircraftPosition )) ) {
    SG_LOG(SG_INSTR,SG_ALERT, "No " << _name << " available at " << frequency );
    _ident = "";
    return;
  }
  SG_LOG(SG_INSTR,SG_ALERT, "Using " << _name << "'" << _navRecord->get_ident() << "' for " << frequency );
  _ident = _navRecord->ident();
}

double NavRadioComponent::computeSignalQuality_norm( const SGGeod & aircraftPosition )
{
  if( false == valid() ) return 0.0;

  double distance_nm = _slantDistance_m * SG_METER_TO_NM;
  double range_nm = _range_nm;

  // assume signal quality is 100% up to the published range and 
  // decay with the distance squared further out
  if ( distance_nm <= range_nm ) return 1.0;
  return range_nm*range_nm/(distance_nm*distance_nm);
}

void NavRadioComponent::update( double dt, const SGGeod & aircraftPosition )
{
    if( false == valid() ) {
      _signalQuality_norm = 0.0;
      _trueBearingTo_deg = 0.0;
      _trueBearingFrom_deg = 0.0;
      _trackDistance_m = 0.0;
      _slantDistance_m = 0.0;
      return;
    } 

    _slantDistance_m = dist(_navRecord->cart(), SGVec3d::fromGeod(aircraftPosition));

    double az1 = 0.0, az2 = 0.0, dist = 0.0;
    SGGeodesy::inverse(aircraftPosition, _navRecord->geod(), az1, az2, dist );
    _trueBearingTo_deg = az1; _trueBearingFrom_deg = az2; _trackDistance_m = dist;
    _heightAboveStation_ft = SGMiscd::max(0.0, aircraftPosition.getElevationFt() - _navRecord->get_elev_ft());

    _range_nm = getRange_nm(aircraftPosition);
    _signalQuality_norm = computeSignalQuality_norm( aircraftPosition );
    _inRange = _signalQuality_norm > 0.2;
}

/* ---------------------------------------------------------------- */

static std::string VORTablePath( const char * name )
{
    SGPath path( globals->get_fg_root() );
    path.append( "Navaids" );
    path.append(name);
    return path.str();
}

class VOR : public NavRadioComponentWithIdent {
public:
  VOR( SGPropertyNode_ptr rootNode);
  virtual ~VOR();
  virtual void update( double dt, const SGGeod & aircraftPosition );
  virtual void display( NavIndicator & navIndicator );
  virtual double getRange_nm(const SGGeod & aircraftPosition);
protected:
  virtual double computeSignalQuality_norm( const SGGeod & aircraftPosition );
  virtual FGNavList * getNavaidList();

private:
  double _totalTime;
  class ServiceVolume {
  public:
    ServiceVolume() :
      term_tbl(VORTablePath("range.term")),
      low_tbl(VORTablePath("range.low")),
      high_tbl(VORTablePath("range.high")) {
    }
    double adjustRange( double height_ft, double nominalRange_nm );

  private:
    SGInterpTable term_tbl;
    SGInterpTable low_tbl;
    SGInterpTable high_tbl;
  } _serviceVolume;

  PropertyObject<double> _radial;
  PropertyObject<double> _radialInbound;
};

// model standard VOR/DME/TACAN service volumes as per AIM 1-1-8
double VOR::ServiceVolume::adjustRange( double height_ft, double nominalRange_nm )
{
    if (nominalRange_nm < SGLimitsd::min() )
      nominalRange_nm = FG_NAV_DEFAULT_RANGE;
    
    // extend out actual usable range to be 1.3x the published safe range
    const double usability_factor = 1.3;

    // assumptions we model the standard service volume, plus
    // ... rather than specifying a cylinder, we model a cone that
    // contains the cylinder.  Then we put an upside down cone on top
    // to model diminishing returns at too-high altitudes.

    if ( nominalRange_nm < 25.0 + SG_EPSILON ) {
	// Standard Terminal Service Volume
	return term_tbl.interpolate( height_ft ) * usability_factor;
    } else if ( nominalRange_nm < 50.0 + SG_EPSILON ) {
	// Standard Low Altitude Service Volume
	// table is based on range of 40, scale to actual range
	return low_tbl.interpolate( height_ft ) * nominalRange_nm / 40.0
	    * usability_factor;
    } else {
	// Standard High Altitude Service Volume
	// table is based on range of 130, scale to actual range
	return high_tbl.interpolate( height_ft ) * nominalRange_nm / 130.0
	    * usability_factor;
    }
}

VOR::VOR( SGPropertyNode_ptr rootNode) :
  NavRadioComponentWithIdent("vor", rootNode, new VORAudioIdent(getIdentString(string("vor"), rootNode->getIndex()))),
  _totalTime(0.0),
  _radial( rootNode->getNode(_name,true)->getNode("radial",true) ),
  _radialInbound( rootNode->getNode(_name,true)->getNode("radial-inbound",true) )
{
}

VOR::~VOR()
{
}

double VOR::getRange_nm( const SGGeod & aircraftPosition )
{
  return _serviceVolume.adjustRange( _heightAboveStation_ft, _navRecord->get_range() );
}

FGNavList * VOR::getNavaidList()
{
  return globals->get_navlist();
}

double VOR::computeSignalQuality_norm( const SGGeod & aircraftPosition )
{
  // apply cone of confusion. Some sources say it's opening angle is 53deg, others estimate
  // a diameter of 1NM per 6000ft (approx. 45deg). ICAO Annex 10 says minimum 40deg.
  // We use 1NM@6000ft and a distance-squared
  // function to make signal-quality=100% 0.5NM@6000ft from the center and zero overhead
  double cone_of_confusion_width = 0.5 * _heightAboveStation_ft / 6000.0 * SG_NM_TO_METER;
  if( _trackDistance_m < cone_of_confusion_width ) {
    double d = cone_of_confusion_width <= SGLimitsd::min() ? 1 : 
              (1 - _trackDistance_m/cone_of_confusion_width);
    return 1-d*d;
  } 

  // use default decay function outside the cone of confusion
  return NavRadioComponentWithIdent::computeSignalQuality_norm( aircraftPosition );
}

void VOR::update( double dt, const SGGeod & aircraftPosition )
{
  _totalTime += dt;
  NavRadioComponentWithIdent::update( dt, aircraftPosition );

  if( false == valid() ) {
      _radial = 0.0;
      return;
  }

  // an arbitrary error function
  double error = 0.5*(sin(_totalTime/11.0) + sin(_totalTime/23.0));

  // add 1% error at 100% signal-quality
  // add 50% error at  0% signal-quality
  // of full deflection (+/-10deg)
  double e = 10.0 * ( 0.01 + (1-_signalQuality_norm) * 0.49 ) * error;

  // compute magnetic bearing from the station (aka current radial)
  double r = SGMiscd::normalizePeriodic(0.0, 360.0, _trueBearingFrom_deg - _navRecord->get_multiuse() + e );

  _radial = r;
  _radialInbound = SGMiscd::normalizePeriodic(0.0,360.0, 180.0 + _radial);
}

void VOR::display( NavIndicator & navIndicator )
{
  if( false == valid() ) return;

  double offset = SGMiscd::normalizePeriodic(-180.0,180.0,_radial - navIndicator.getSelectedCourse());
  bool to = fabs(offset) >= 90.0;

  if( to ) offset = -offset + copysign(180.0,offset);

  navIndicator.showTo( to );
  navIndicator.showFrom( !to );
  // normalize to +/- 1.0 for +/- 10deg, decrease deflection with decreasing signal
  navIndicator.setCDI( SGMiscd::clip( -offset/10.0, -1.0, 1.0 ) * _signalQuality_norm );
  navIndicator.setSignalQuality( _signalQuality_norm );
}

/* ---------------------------------------------------------------- */
class LOC : public NavRadioComponentWithIdent {
public:
  LOC( SGPropertyNode_ptr rootNode );
  virtual ~LOC();
  virtual void update( double dt, const SGGeod & aircraftPosition );
  virtual void search( double frequency, const SGGeod & aircraftPosition );
  virtual void display( NavIndicator & navIndicator );
  virtual double getRange_nm(const SGGeod & aircraftPosition);

protected:
  virtual double computeSignalQuality_norm( const SGGeod & aircraftPosition );
  virtual FGNavList * getNavaidList();

private:
  class ServiceVolume {
  public:
      ServiceVolume();
      double adjustRange( double azimuthAngle_deg, double elevationAngle_deg );
  private:
      SGInterpTable _azimuthTable;
      SGInterpTable _elevationTable;
  } _serviceVolume;
  PropertyObject<double> _localizerOffset_norm;
  PropertyObject<double> _localizerWidth_deg;
};

LOC::ServiceVolume::ServiceVolume()
{
// maybe this: http://www.tpub.com/content/aviation2/P-1244/P-12440125.htm
  // ICAO Annex 10 - 3.1.3.2.2: The emission from the localizer
  // shall be horizontally polarized
  // very rough abstraction of a 5-element yagi antenna's
  // E-plane radiation diagram
  _azimuthTable.addEntry(   0.0, 1.0 );
  _azimuthTable.addEntry(  10.0, 1.0 );
  _azimuthTable.addEntry(  30.0, 0.75 );
  _azimuthTable.addEntry(  40.0, 0.50 );
  _azimuthTable.addEntry(  50.0, 0.20 );
  _azimuthTable.addEntry(  60.0, 0.10 );
  _azimuthTable.addEntry(  70.0, 0.20 );
  _azimuthTable.addEntry(  80.0, 0.10 );
  _azimuthTable.addEntry(  90.0, 0.05 );
  _azimuthTable.addEntry( 105.0, 0.10 );
  _azimuthTable.addEntry( 130.0, 0.05 );
  _azimuthTable.addEntry( 150.0, 0.30 );
  _azimuthTable.addEntry( 160.0, 0.40 );
  _azimuthTable.addEntry( 170.0, 0.50 );
  _azimuthTable.addEntry( 180.0, 0.50 );

  _elevationTable.addEntry(   0.0, 0.1 );
  _elevationTable.addEntry(  1.05, 1.0 );
  _elevationTable.addEntry(  7.00, 1.0 );
  _elevationTable.addEntry(  45.0, 0.3 );
  _elevationTable.addEntry(  90.0, 0.1 );
  _elevationTable.addEntry( 180.0, 0.01 );
}

double LOC::ServiceVolume::adjustRange( double azimuthAngle_deg, double elevationAngle_deg )
{
    return _azimuthTable.interpolate( fabs(azimuthAngle_deg) ) * 
        _elevationTable.interpolate( fabs(elevationAngle_deg) );
}

LOC::LOC( SGPropertyNode_ptr rootNode) :
  NavRadioComponentWithIdent("loc", rootNode, new LOCAudioIdent(getIdentString(string("loc"), rootNode->getIndex()))),
  _serviceVolume(),
  _localizerOffset_norm( rootNode->getNode(_name,true)->getNode("offset-norm",true) ),
  _localizerWidth_deg( rootNode->getNode(_name,true)->getNode("width-deg",true) )
{
}

LOC::~LOC()
{
}

FGNavList * LOC::getNavaidList()
{
  return globals->get_loclist();
}

void LOC::search( double frequency, const SGGeod & aircraftPosition )
{
  NavRadioComponentWithIdent::search( frequency, aircraftPosition );
  if( false == valid() ) {
      _localizerWidth_deg = 0.0;
      return;
  }

  // cache slightly expensive value, 
  // sanitized in FGNavRecord::localizerWidth() to  never become zero
  _localizerWidth_deg = _navRecord->localizerWidth();
}

/* Localizer coverage (ICAO Annex 10 Volume I 3.1.3.3 
  25NM within +/-10 deg from the front course line
  17NM between 10 and 35deg from the front course line
  10NM outside of +/- 35deg  if coverage is provided
  at and above a height of 2000ft above threshold or
  1000ft above the highest point within intermediate
  and final approach areas. Upper limit is a surface
  extending outward from the localizer and inclined at
  7 degrees above the horizontal
 */
double LOC::getRange_nm(const SGGeod & aircraftPosition)
{
  double elevationAngle = ::atan2(_heightAboveStation_ft*SG_FEET_TO_METER, _trackDistance_m)*SG_RADIANS_TO_DEGREES;
  double azimuthAngle = SGMiscd::normalizePeriodic( -180.0, 180.0, _trueBearingFrom_deg + 180.0 - _navRecord->get_multiuse() );

  // looks like our navrecord declared range is based on 10NM?
  return  _navRecord->get_range() * _serviceVolume.adjustRange( azimuthAngle, elevationAngle );
}

double LOC::computeSignalQuality_norm( const SGGeod & aircraftPosition )
{
  return NavRadioComponentWithIdent::computeSignalQuality_norm( aircraftPosition );
}

void LOC::update( double dt, const SGGeod & aircraftPosition )
{
  NavRadioComponentWithIdent::update( dt, aircraftPosition );

  if( false == valid() ) {
    _localizerOffset_norm = 0.0;
    return;
  }

  double offset = SGMiscd::normalizePeriodic( -180.0, 180.0, _trueBearingFrom_deg + 180.0 - _navRecord->get_multiuse() );

  // The factor of 30.0 gives a period of 120 which gives us 3 cycles and six 
  // zeros i.e. six courses: one front course, one back course, and four 
  // false courses. Three of the six are reverse sensing.
  offset = 30.0 * sawtooth(offset / 30.0);

  // normalize offset to the localizer width, scale and clip to [-1..1]
  offset = SGMiscd::clip( 2.0 * offset / _localizerWidth_deg, -1.0, 1.0 );
  
  _localizerOffset_norm = offset;
}

void LOC::display( NavIndicator & navIndicator )
{
  if( false == valid() ) 
    return;

  navIndicator.showTo( true );
  navIndicator.showFrom( false );

  navIndicator.setCDI( _localizerOffset_norm * _signalQuality_norm );
  navIndicator.setSignalQuality( _signalQuality_norm );
}

class GS : public NavRadioComponent {
public:
  GS( SGPropertyNode_ptr rootNode);
  virtual ~GS();
  virtual void update( double dt, const SGGeod & aircraftPosition );
  virtual void search( double frequency, const SGGeod & aircraftPosition );
  virtual void display( NavIndicator & navIndicator );

  virtual double getRange_nm(const SGGeod & aircraftPosition);
protected:
  virtual FGNavList * getNavaidList();

private:
  class ServiceVolume {
  public:
      ServiceVolume();
      double adjustRange( double azimuthAngle_deg, double elevationAngle_deg );
  private:
      SGInterpTable _azimuthTable;
      SGInterpTable _elevationTable;
  } _serviceVolume;
  static SGVec3d tangentVector(const SGGeod& midpoint, const double heading);

  PropertyObject<double>  _targetGlideslope_deg;
  PropertyObject<double>  _glideslopeOffset_norm;
  SGVec3d _gsAxis;
  SGVec3d _gsVertical;
};

GS::ServiceVolume::ServiceVolume()
{
// maybe this: http://www.tpub.com/content/aviation2/P-1244/P-12440125.htm
  // ICAO Annex 10 - 3.1.5.2.2: The emission from the glide path equipment
  // shall be horizontally polarized
  // very rough abstraction of a 5-element yagi antenna's
  // E-plane radiation diagram
  _azimuthTable.addEntry(   0.0, 1.0 );
  _azimuthTable.addEntry(  10.0, 1.0 );
  _azimuthTable.addEntry(  30.0, 0.75 );
  _azimuthTable.addEntry(  40.0, 0.50 );
  _azimuthTable.addEntry(  50.0, 0.20 );
  _azimuthTable.addEntry(  60.0, 0.10 );
  _azimuthTable.addEntry(  70.0, 0.20 );
  _azimuthTable.addEntry(  80.0, 0.10 );
  _azimuthTable.addEntry(  90.0, 0.05 );
  _azimuthTable.addEntry( 105.0, 0.10 );
  _azimuthTable.addEntry( 130.0, 0.05 );
  _azimuthTable.addEntry( 150.0, 0.30 );
  _azimuthTable.addEntry( 160.0, 0.40 );
  _azimuthTable.addEntry( 170.0, 0.50 );
  _azimuthTable.addEntry( 180.0, 0.50 );

  _elevationTable.addEntry(   0.0, 0.1 );
  _elevationTable.addEntry(  1.05, 1.0 );
  _elevationTable.addEntry(  7.00, 1.0 );
  _elevationTable.addEntry(  45.0, 0.3 );
  _elevationTable.addEntry(  90.0, 0.1 );
  _elevationTable.addEntry( 180.0, 0.01 );
}

double GS::ServiceVolume::adjustRange( double azimuthAngle_deg, double elevationAngle_deg )
{
    return _azimuthTable.interpolate( fabs(azimuthAngle_deg) ) * 
        _elevationTable.interpolate( fabs(elevationAngle_deg) );
}

GS::GS( SGPropertyNode_ptr rootNode) :
  NavRadioComponent("gs", rootNode ),
  _targetGlideslope_deg( rootNode->getNode(_name,true)->getNode("slope",true) ),
  _glideslopeOffset_norm( rootNode->getNode(_name,true)->getNode("offset-norm",true) ),
  _gsAxis(SGVec3d::zeros()),
  _gsVertical(SGVec3d::zeros())
{
}

GS::~GS()
{
}

FGNavList * GS::getNavaidList()
{
  return globals->get_gslist();
}

double GS::getRange_nm(const SGGeod & aircraftPosition)
{
  double elevationAngle = ::atan2(_heightAboveStation_ft*SG_FEET_TO_METER, _trackDistance_m)*SG_RADIANS_TO_DEGREES;
  double azimuthAngle = SGMiscd::normalizePeriodic( -180.0, 180.0, _trueBearingFrom_deg + 180.0 - fmod(_navRecord->get_multiuse(), 1000.0) );
  return  _navRecord->get_range() * _serviceVolume.adjustRange( azimuthAngle, elevationAngle );
}

// Calculate a Cartesian unit vector in the
// local horizontal plane, i.e. tangent to the 
// surface of the earth at the local ground zero.
// The tangent vector passes through the given  <midpoint> 
// and points forward along the given <heading>.
// The <heading> is given in degrees.
SGVec3d GS::tangentVector(const SGGeod& midpoint, const double heading)
{
  // move 100m away from the midpoint - arbitrary number
  const double delta(100.0);
  SGGeod head, tail;
  double az2;                   // ignored
  SGGeodesy::direct(midpoint, heading,     delta, head, az2);
  SGGeodesy::direct(midpoint, 180+heading, delta, tail, az2);
  head.setElevationM(midpoint.getElevationM());
  tail.setElevationM(midpoint.getElevationM());
  SGVec3d head_xyz = SGVec3d::fromGeod(head);
  SGVec3d tail_xyz = SGVec3d::fromGeod(tail);
// Awkward formula here, needed because vector-by-scalar
// multiplication is defined, but not vector-by-scalar division.
  return (head_xyz - tail_xyz) * (0.5/delta);
}

void GS::search( double frequency, const SGGeod & aircraftPosition )
{
  NavRadioComponent::search( frequency, aircraftPosition );
  if( false == valid() ) {
      _gsAxis = SGVec3d::zeros();
      _gsVertical = SGVec3d::zeros();
      _targetGlideslope_deg = 3.0;
      return;
  }
  
  double gs_radial = SGMiscd::normalizePeriodic(0.0, 360.0, fmod(_navRecord->get_multiuse(), 1000.0) );

  _gsAxis = tangentVector(_navRecord->geod(), gs_radial);
  SGVec3d gsBaseline = tangentVector(_navRecord->geod(), gs_radial + 90.0);
  _gsVertical = cross(gsBaseline, _gsAxis);

  int tmp = (int)(_navRecord->get_multiuse() / 1000.0);
  // catch unconfigured glideslopes here, they will cause nan later
  _targetGlideslope_deg = SGMiscd::max( 1.0, (double)tmp / 100.0 );
}

void GS::update( double dt, const SGGeod & aircraftPosition )
{
  NavRadioComponent::update( dt, aircraftPosition );
  if( false == valid() ) {
      _glideslopeOffset_norm = 0.0;
      return;
  }
  
  SGVec3d pos = SGVec3d::fromGeod(aircraftPosition) - _navRecord->cart(); // relative vector from gs antenna to aircraft
  // The positive GS axis points along the runway in the landing direction,
  // toward the far end, not toward the approach area, so we need a - sign here:
  double comp_h = -dot(pos, _gsAxis);      // component in horiz direction
  double comp_v = dot(pos, _gsVertical);   // component in vertical direction
  //double comp_b = dot(pos, _gsBaseline);   // component in baseline direction
  //if (comp_b) {}                           // ... (useful for debugging)

// _gsDirect represents the angle of elevation of the aircraft
// as seen by the GS transmitter.
  double gsDirect = atan2(comp_v, comp_h) * SGD_RADIANS_TO_DEGREES;
// At this point, if the aircraft is centered on the glide slope,
// _gsDirect will be a small positive number, e.g. 3.0 degrees

// Aim the branch cut straight down 
// into the ground below the GS transmitter:
  if (gsDirect < -90.0) gsDirect += 360.0;

  double offset = _targetGlideslope_deg - gsDirect;
  if( offset < 0.0 )
    offset = _targetGlideslope_deg/2 * sawtooth(2.0*offset/_targetGlideslope_deg);
  assert( false == isnan(offset) );
// GS is documented to be 1.4 degrees thick, 
// i.e. plus or minus 0.7 degrees from the midline:
  _glideslopeOffset_norm = SGMiscd::clip(offset/0.7, -1.0, 1.0);
}

void GS::display( NavIndicator & navIndicator )
{
  if( false == valid() ) {
    navIndicator.setGS( false );
    return;
  }
  navIndicator.setGS( true );
  navIndicator.setGS( _glideslopeOffset_norm );
}

/* ------------- A NAV/COMM Frequency formatter ---------------------- */

class FrequencyFormatter : public SGPropertyChangeListener {
public:
  FrequencyFormatter( SGPropertyNode_ptr freqNode, SGPropertyNode_ptr fmtFreqNode, double channelSpacing ) :
    _freqNode( freqNode ),
    _fmtFreqNode( fmtFreqNode ),
    _channelSpacing(channelSpacing)
  {
    _freqNode->addChangeListener( this );
    valueChanged(_freqNode);
  }
  ~FrequencyFormatter()
  {
    _freqNode->removeChangeListener( this );
  }

  void valueChanged (SGPropertyNode * prop)
  {
    // format as fixed decimal "nnn.nn"
    std::ostringstream buf;
    buf << std::fixed 
        << std::setw(5) 
        << std::setfill('0') 
        << std::setprecision(2)
        << getFrequency();
    _fmtFreqNode->setStringValue( buf.str() );
  }

  double getFrequency() const 
  {
    double d = SGMiscd::roundToInt(_freqNode->getDoubleValue() / _channelSpacing) * _channelSpacing;
    // strip last digit, do not round
    return ((int)(d*100))/100.0;
  }

private:
  SGPropertyNode_ptr _freqNode;
  SGPropertyNode_ptr _fmtFreqNode;
  double _channelSpacing;
};


/* ------------- The NavRadio implementation ---------------------- */

class NavRadioImpl : public NavRadio {
public:
  NavRadioImpl( SGPropertyNode_ptr node );
  virtual ~NavRadioImpl();

  virtual void update( double dt );
  virtual void init();
private:
  void search();

  class Legacy {
  public:
      Legacy( NavRadioImpl * navRadioImpl ) : _navRadioImpl( navRadioImpl ) {}

      void init();
      void update( double dt );
  private:
      NavRadioImpl * _navRadioImpl;
      SGPropertyNode_ptr is_valid_node;
      SGPropertyNode_ptr nav_serviceable_node;
      SGPropertyNode_ptr nav_id_node;
      SGPropertyNode_ptr id_c1_node;
      SGPropertyNode_ptr id_c2_node;
      SGPropertyNode_ptr id_c3_node;
      SGPropertyNode_ptr id_c4_node;
  } _legacy;

  const static int VOR_COMPONENT = 0;
  const static int LOC_COMPONENT = 1;
  const static int GS_COMPONENT  = 2;

  std::string _name;
  int         _num;
  SGPropertyNode_ptr _rootNode;
  FrequencyFormatter _useFrequencyFormatter;
  FrequencyFormatter _stbyFrequencyFormatter;
  std::vector<NavRadioComponent*> _components;
  NavIndicator _navIndicator;
  double _stationTTL;
  double _frequency;
  PropertyObject<bool> _cdiDisconnected;
};

NavRadioImpl::NavRadioImpl( SGPropertyNode_ptr node ) :
  _legacy( this ),
  _name(node->getStringValue("name", "nav")),
  _num(node->getIntValue("number", 0)),
  _rootNode(fgGetNode( string("/instrumentation/") + _name, _num, true)),
  _useFrequencyFormatter( _rootNode->getNode("frequencies/selected-mhz",true), _rootNode->getNode("frequencies/selected-mhz-fmt",true), 0.05 ),
  _stbyFrequencyFormatter( _rootNode->getNode("frequencies/standby-mhz",true), _rootNode->getNode("frequencies/standby-mhz-fmt",true), 0.05 ),
  _navIndicator(_rootNode),
  _stationTTL(0.0),
  _frequency(-1.0),
  _cdiDisconnected(_rootNode->getNode("cdi-disconnected",true))
{
}

NavRadioImpl::~NavRadioImpl()
{
  BOOST_FOREACH( NavRadioComponent * p, _components ) {
    delete p;
  }
}

void NavRadioImpl::init()
{
  if( 0 < _components.size() )
    return;

  _components.push_back( new VOR(_rootNode) );
  _components.push_back( new LOC(_rootNode) );
  _components.push_back( new GS(_rootNode) );

  _legacy.init();
}

void NavRadioImpl::search()
{
}

void NavRadioImpl::update( double dt )
{
  if( dt < SGLimitsd::min() ) return;

  SGGeod position;

  try {
    position = globals->get_aircraft_position();
  }
  catch( std::exception & ) {
    return;
  }

  _stationTTL -= dt;
  if( _frequency != _useFrequencyFormatter.getFrequency() ) {
      _frequency = _useFrequencyFormatter.getFrequency();
      _stationTTL = 0.0;
  }

  BOOST_FOREACH( NavRadioComponent * p, _components ) {
      if( _stationTTL <= 0.0 )
          p->search( _frequency, position );
      p->update( dt, position );

      if( false == _cdiDisconnected )
          p->display( _navIndicator );
  }

  if( _stationTTL <= 0.0 )
      _stationTTL = 30.0;

  _legacy.update( dt );
}

void NavRadioImpl::Legacy::init()
{
    is_valid_node = _navRadioImpl->_rootNode->getChild("data-is-valid", 0, true);
    nav_serviceable_node = _navRadioImpl->_rootNode->getChild("serviceable", 0, true);

    nav_id_node = _navRadioImpl->_rootNode->getChild("nav-id", 0, true );
    id_c1_node = _navRadioImpl->_rootNode->getChild("nav-id_asc1", 0, true );
    id_c2_node = _navRadioImpl->_rootNode->getChild("nav-id_asc2", 0, true );
    id_c3_node = _navRadioImpl->_rootNode->getChild("nav-id_asc3", 0, true );
    id_c4_node = _navRadioImpl->_rootNode->getChild("nav-id_asc4", 0, true );

}

void NavRadioImpl::Legacy::update( double dt )
{
    is_valid_node->setBoolValue( 
        _navRadioImpl->_components[VOR_COMPONENT]->valid() || _navRadioImpl->_components[LOC_COMPONENT]->valid()  
        );

    string ident = _navRadioImpl->_components[VOR_COMPONENT]->getIdent();
    if( ident.empty() )
        ident = _navRadioImpl->_components[LOC_COMPONENT]->getIdent();

    nav_id_node->setStringValue( ident );

    ident = simgear::strutils::rpad( ident, 4, ' ' );
    id_c1_node->setIntValue( (int)ident[0] );
    id_c2_node->setIntValue( (int)ident[1] );
    id_c3_node->setIntValue( (int)ident[2] );
    id_c4_node->setIntValue( (int)ident[3] );
}


SGSubsystem * NavRadio::createInstance( SGPropertyNode_ptr rootNode )
{
    // use old navradio code by default
    if( fgGetBool( "/instrumentation/use-new-navradio", false ) )
        return new NavRadioImpl( rootNode );

    return new FGNavRadio( rootNode );
}

} // namespace Instrumentation

