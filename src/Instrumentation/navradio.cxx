// navradio.cxx -- class to manage a nav radio instance
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000 - 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sstream>

#include <simgear/sg_inlines.h>
#include <simgear/timing/sg_time.hxx>
#include <simgear/math/vector.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/math/interpolater.hxx>

#include "Navaids/navrecord.hxx"
#include <Navaids/navlist.hxx>
#include <Main/util.hxx>
#include "navradio.hxx"

using std::string;

// Constructor
FGNavRadio::FGNavRadio(SGPropertyNode *node) :
    lon_node(fgGetNode("/position/longitude-deg", true)),
    lat_node(fgGetNode("/position/latitude-deg", true)),
    alt_node(fgGetNode("/position/altitude-ft", true)),
    is_valid_node(NULL),
    power_btn_node(NULL),
    freq_node(NULL),
    alt_freq_node(NULL),
    sel_radial_node(NULL),
    vol_btn_node(NULL),
    ident_btn_node(NULL),
    audio_btn_node(NULL),
    backcourse_node(NULL),
    nav_serviceable_node(NULL),
    cdi_serviceable_node(NULL),
    gs_serviceable_node(NULL),
    tofrom_serviceable_node(NULL),
    fmt_freq_node(NULL),
    fmt_alt_freq_node(NULL),
    heading_node(NULL),
    radial_node(NULL),
    recip_radial_node(NULL),
    target_radial_true_node(NULL),
    target_auto_hdg_node(NULL),
    time_to_intercept(NULL),
    to_flag_node(NULL),
    from_flag_node(NULL),
    inrange_node(NULL),
    signal_quality_norm_node(NULL),
    cdi_deflection_node(NULL),
    cdi_deflection_norm_node(NULL),
    cdi_xtrack_error_node(NULL),
    cdi_xtrack_hdg_err_node(NULL),
    has_gs_node(NULL),
    loc_node(NULL),
    loc_dist_node(NULL),
    gs_deflection_node(NULL),
    gs_deflection_norm_node(NULL),
    gs_rate_of_climb_node(NULL),
    gs_dist_node(NULL),
    nav_id_node(NULL),
    id_c1_node(NULL),
    id_c2_node(NULL),
    id_c3_node(NULL),
    id_c4_node(NULL),
    nav_slaved_to_gps_node(NULL),
    gps_cdi_deflection_node(NULL),
    gps_to_flag_node(NULL),
    gps_from_flag_node(NULL),
    gps_has_gs_node(NULL),
    play_count(0),
    last_time(0),
    target_radial(0.0),
    horiz_vel(0.0),
    last_x(0.0),
    last_loc_dist(0.0),
    last_xtrack_error(0.0),
    _name(node->getStringValue("name", "nav")),
    _num(node->getIntValue("number", 0)),
    _time_before_search_sec(-1.0)
{
    SGPath path( globals->get_fg_root() );
    SGPath term = path;
    term.append( "Navaids/range.term" );
    SGPath low = path;
    low.append( "Navaids/range.low" );
    SGPath high = path;
    high.append( "Navaids/range.high" );

    term_tbl = new SGInterpTable( term.str() );
    low_tbl = new SGInterpTable( low.str() );
    high_tbl = new SGInterpTable( high.str() );
}


// Destructor
FGNavRadio::~FGNavRadio() 
{
    delete term_tbl;
    delete low_tbl;
    delete high_tbl;
}


void
FGNavRadio::init ()
{
    morse.init();

    string branch;
    branch = "/instrumentation/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );

    bus_power_node = 
	fgGetNode(("/systems/electrical/outputs/" + _name).c_str(), true);

    // inputs
    is_valid_node = node->getChild("data-is-valid", 0, true);
    power_btn_node = node->getChild("power-btn", 0, true);
    power_btn_node->setBoolValue( true );
    vol_btn_node = node->getChild("volume", 0, true);
    ident_btn_node = node->getChild("ident", 0, true);
    ident_btn_node->setBoolValue( true );
    audio_btn_node = node->getChild("audio-btn", 0, true);
    audio_btn_node->setBoolValue( true );
    backcourse_node = node->getChild("back-course-btn", 0, true);
    backcourse_node->setBoolValue( false );
    nav_serviceable_node = node->getChild("serviceable", 0, true);
    cdi_serviceable_node = (node->getChild("cdi", 0, true))
	->getChild("serviceable", 0, true);
    gs_serviceable_node = (node->getChild("gs", 0, true))
	->getChild("serviceable");
    tofrom_serviceable_node = (node->getChild("to-from", 0, true))
	->getChild("serviceable", 0, true);

    // frequencies
    SGPropertyNode *subnode = node->getChild("frequencies", 0, true);
    freq_node = subnode->getChild("selected-mhz", 0, true);
    alt_freq_node = subnode->getChild("standby-mhz", 0, true);
    fmt_freq_node = subnode->getChild("selected-mhz-fmt", 0, true);
    fmt_alt_freq_node = subnode->getChild("standby-mhz-fmt", 0, true);

    // radials
    subnode = node->getChild("radials", 0, true);
    sel_radial_node = subnode->getChild("selected-deg", 0, true);
    radial_node = subnode->getChild("actual-deg", 0, true);
    recip_radial_node = subnode->getChild("reciprocal-radial-deg", 0, true);
    target_radial_true_node = subnode->getChild("target-radial-deg", 0, true);
    target_auto_hdg_node = subnode->getChild("target-auto-hdg-deg", 0, true);

    // outputs
    heading_node = node->getChild("heading-deg", 0, true);
    time_to_intercept = node->getChild("time-to-intercept-sec", 0, true);
    to_flag_node = node->getChild("to-flag", 0, true);
    from_flag_node = node->getChild("from-flag", 0, true);
    inrange_node = node->getChild("in-range", 0, true);
    signal_quality_norm_node = node->getChild("signal-quality-norm", 0, true);
    cdi_deflection_node = node->getChild("heading-needle-deflection", 0, true);
    cdi_deflection_norm_node = node->getChild("heading-needle-deflection-norm", 0, true);
    cdi_xtrack_error_node = node->getChild("crosstrack-error-m", 0, true);
    cdi_xtrack_hdg_err_node
        = node->getChild("crosstrack-heading-error-deg", 0, true);
    has_gs_node = node->getChild("has-gs", 0, true);
    loc_node = node->getChild("nav-loc", 0, true);
    loc_dist_node = node->getChild("nav-distance", 0, true);
    gs_deflection_node = node->getChild("gs-needle-deflection", 0, true);
    gs_deflection_norm_node = node->getChild("gs-needle-deflection-norm", 0, true);
    gs_rate_of_climb_node = node->getChild("gs-rate-of-climb", 0, true);
    gs_dist_node = node->getChild("gs-distance", 0, true);
    nav_id_node = node->getChild("nav-id", 0, true);
    id_c1_node = node->getChild("nav-id_asc1", 0, true);
    id_c2_node = node->getChild("nav-id_asc2", 0, true);
    id_c3_node = node->getChild("nav-id_asc3", 0, true);
    id_c4_node = node->getChild("nav-id_asc4", 0, true);

    // gps slaving support
    nav_slaved_to_gps_node = node->getChild("slaved-to-gps", 0, true);
    gps_cdi_deflection_node = fgGetNode("/instrumentation/gps/cdi-deflection", true);
    gps_to_flag_node = fgGetNode("/instrumentation/gps/to-flag", true);
    gps_from_flag_node = fgGetNode("/instrumentation/gps/from-flag", true);
    gps_has_gs_node = fgGetNode("/instrumentation/gps/has-gs", true);
    
    std::ostringstream temp;
    temp << _name << "nav-ident" << _num;
    nav_fx_name = temp.str();
    temp << _name << "dme-ident" << _num;
    dme_fx_name = temp.str();
}

void
FGNavRadio::bind ()
{
    std::ostringstream temp;
    string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";
}


void
FGNavRadio::unbind ()
{
    std::ostringstream temp;
    string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";
}


// model standard VOR/DME/TACAN service volumes as per AIM 1-1-8
double FGNavRadio::adjustNavRange( double stationElev, double aircraftElev,
                                 double nominalRange )
{
    // extend out actual usable range to be 1.3x the published safe range
    const double usability_factor = 1.3;

    // assumptions we model the standard service volume, plus
    // ... rather than specifying a cylinder, we model a cone that
    // contains the cylinder.  Then we put an upside down cone on top
    // to model diminishing returns at too-high altitudes.

    // altitude difference
    double alt = ( aircraftElev * SG_METER_TO_FEET - stationElev );
    // cout << "aircraft elev = " << aircraftElev * SG_METER_TO_FEET
    //      << " station elev = " << stationElev << endl;

    if ( nominalRange < 25.0 + SG_EPSILON ) {
	// Standard Terminal Service Volume
	return term_tbl->interpolate( alt ) * usability_factor;
    } else if ( nominalRange < 50.0 + SG_EPSILON ) {
	// Standard Low Altitude Service Volume
	// table is based on range of 40, scale to actual range
	return low_tbl->interpolate( alt ) * nominalRange / 40.0
	    * usability_factor;
    } else {
	// Standard High Altitude Service Volume
	// table is based on range of 130, scale to actual range
	return high_tbl->interpolate( alt ) * nominalRange / 130.0
	    * usability_factor;
    }
}


// model standard ILS service volumes as per AIM 1-1-9
double FGNavRadio::adjustILSRange( double stationElev, double aircraftElev,
                                 double offsetDegrees, double distance )
{
    // assumptions we model the standard service volume, plus

    // altitude difference
    // double alt = ( aircraftElev * SG_METER_TO_FEET - stationElev );
//     double offset = fabs( offsetDegrees );

//     if ( offset < 10 ) {
// 	return FG_ILS_DEFAULT_RANGE;
//     } else if ( offset < 35 ) {
// 	return 10 + (35 - offset) * (FG_ILS_DEFAULT_RANGE - 10) / 25;
//     } else if ( offset < 45 ) {
// 	return (45 - offset);
//     } else if ( offset > 170 ) {
//         return FG_ILS_DEFAULT_RANGE;
//     } else if ( offset > 145 ) {
// 	return 10 + (offset - 145) * (FG_ILS_DEFAULT_RANGE - 10) / 25;
//     } else if ( offset > 135 ) {
//         return (offset - 135);
//     } else {
// 	return 0;
//     }
    return FG_LOC_DEFAULT_RANGE;
}


//////////////////////////////////////////////////////////////////////////
// Update the various nav values based on position and valid tuned in navs
//////////////////////////////////////////////////////////////////////////
void 
FGNavRadio::update(double dt) 
{
  if (dt <= 0.0) {
    return; // paused
  }
    
  // Create "formatted" versions of the nav frequencies for
  // instrument displays.
  char tmp[16];
  sprintf( tmp, "%.2f", freq_node->getDoubleValue() );
  fmt_freq_node->setStringValue(tmp);
  sprintf( tmp, "%.2f", alt_freq_node->getDoubleValue() );
  fmt_alt_freq_node->setStringValue(tmp);

  if (power_btn_node->getBoolValue() 
      && (bus_power_node->getDoubleValue() > 1.0)
      && nav_serviceable_node->getBoolValue() )
  {   
    if (nav_slaved_to_gps_node->getBoolValue()) {
      updateGPSSlaved();
    } else {
      updateReceiver(dt);
    }
    
    updateCDI(dt);
  } else {
    clearOutputs();
  }
  
  updateAudio();
}

void FGNavRadio::clearOutputs()
{
  inrange_node->setBoolValue( false );
  cdi_deflection_node->setDoubleValue( 0.0 );
  cdi_deflection_norm_node->setDoubleValue( 0.0 );
  cdi_xtrack_error_node->setDoubleValue( 0.0 );
  cdi_xtrack_hdg_err_node->setDoubleValue( 0.0 );
  time_to_intercept->setDoubleValue( 0.0 );
  gs_deflection_node->setDoubleValue( 0.0 );
  gs_deflection_norm_node->setDoubleValue(0.0);
  
  to_flag_node->setBoolValue( false );
  from_flag_node->setBoolValue( false );
}

void FGNavRadio::updateReceiver(double dt)
{
  // Do a nav station search only once a second to reduce
  // unnecessary work. (Also, make sure to do this before caching
  // any values!)
  _time_before_search_sec -= dt;
  if ( _time_before_search_sec < 0 ) {
   search();
  }

  if (!_navaid) {
    _cdiDeflection = 0.0;
    _cdiCrossTrackErrorM = 0.0;
    _toFlag = _fromFlag = false;
    _gsNeedleDeflection = 0.0;
    _gsNeedleDeflectionNorm = 0.0;
    inrange_node->setBoolValue(false);
    return;
  }

  SGGeod pos = SGGeod::fromDegFt(lon_node->getDoubleValue(),
                               lat_node->getDoubleValue(),
                               alt_node->getDoubleValue());
                               
  double nav_elev = _navaid->get_elev_ft();
  SGVec3d aircraft = SGVec3d::fromGeod(pos);
  double loc_dist = dist(aircraft, _navaid->cart());
  loc_dist_node->setDoubleValue( loc_dist );
  bool is_loc = loc_node->getBoolValue();
  double signal_quality_norm = signal_quality_norm_node->getDoubleValue();
  
  double az2, s;
  //////////////////////////////////////////////////////////
	// compute forward and reverse wgs84 headings to localizer
  //////////////////////////////////////////////////////////
  double hdg;
  SGGeodesy::inverse(pos, _navaid->geod(), hdg, az2, s);
  heading_node->setDoubleValue(hdg);
  double radial = az2 - twist;
  double recip = radial + 180.0;
  SG_NORMALIZE_RANGE(recip, 0.0, 360.0);
  radial_node->setDoubleValue( radial );
  recip_radial_node->setDoubleValue( recip );
  
  //////////////////////////////////////////////////////////
  // compute the target/selected radial in "true" heading
  //////////////////////////////////////////////////////////
  if (!is_loc) {
    target_radial = sel_radial_node->getDoubleValue();
  }
  
  // VORs need twist (mag-var) added; ILS/LOCs don't but we set twist to 0.0
  double trtrue = target_radial + twist;
  SG_NORMALIZE_RANGE(trtrue, 0.0, 360.0);
  target_radial_true_node->setDoubleValue( trtrue );

  //////////////////////////////////////////////////////////
  // adjust reception range for altitude
  // FIXME: make sure we are using the navdata range now that
  //        it is valid in the data file
  //////////////////////////////////////////////////////////
	if ( is_loc ) {
	    double offset = radial - target_radial;
      SG_NORMALIZE_RANGE(offset, -180.0, 180.0);
	    effective_range
                = adjustILSRange( nav_elev, pos.getElevationM(), offset,
                                  loc_dist * SG_METER_TO_NM );
	} else {
	    effective_range
                = adjustNavRange( nav_elev, pos.getElevationM(), _navaid->get_range() );
	}

  double effective_range_m = effective_range * SG_NM_TO_METER;

  //////////////////////////////////////////////////////////
  // compute signal quality
  // 100% within effective_range
  // decreases 1/x^2 further out
  //////////////////////////////////////////////////////////  
  double last_signal_quality_norm = signal_quality_norm;

  if ( loc_dist < effective_range_m ) {
    signal_quality_norm = 1.0;
  } else {
    double range_exceed_norm = loc_dist/effective_range_m;
    signal_quality_norm = 1/(range_exceed_norm*range_exceed_norm);
  }

  signal_quality_norm = fgGetLowPass( last_signal_quality_norm, 
           signal_quality_norm, dt );
  
  signal_quality_norm_node->setDoubleValue( signal_quality_norm );
  bool inrange = signal_quality_norm > 0.2;
  inrange_node->setBoolValue( inrange );
  
  //////////////////////////////////////////////////////////
  // compute to/from flag status
  //////////////////////////////////////////////////////////
  if (inrange) {
    if (is_loc) {
      _toFlag = true;
    } else {
      double offset = fabs(radial - target_radial);
      _toFlag = (offset > 90.0 && offset < 270.0);
    }
    _fromFlag = !_toFlag;
  } else {
    _toFlag = _fromFlag = false;
  }
  
  // CDI deflection
  double r = radial - target_radial;
  SG_NORMALIZE_RANGE(r, -180.0, 180.0);
  if ( fabs(r) > 90.0 ) {
    r = ( r<0.0 ? -r-180.0 : -r+180.0 );
  }
  
  r = -r; // reverse, since radial is outbound
  _cdiDeflection = r;
  if ( is_loc ) {
    // According to Robin Peel, the ILS is 4x more
    // sensitive than a vor
    // http://www.allstar.fiu.edu/aero/ILS.htm confirms both the 4x sensitvity
    // increase, and also the 'full-deflection is 10-degrees for a VOR' clamp
    _cdiDeflection *= 4.0;
  }
  SG_CLAMP_RANGE(_cdiDeflection, -10.0, 10.0 );
  _cdiDeflection *= signal_quality_norm;
  
  // cross-track error (in metres)
  _cdiCrossTrackErrorM = loc_dist * sin(r * SGD_DEGREES_TO_RADIANS);
  
  updateGlideSlope(dt, aircraft, signal_quality_norm);
  
  last_loc_dist = loc_dist;
}

void FGNavRadio::updateGlideSlope(double dt, const SGVec3d& aircraft, double signal_quality_norm)
{
  if (!_gs || !inrange_node->getBoolValue()) {
    gs_dist_node->setDoubleValue( 0.0 );
    return;
  }
  
  // find closest distance to the gs base line
  double dist = sgdClosestPointToLineDistSquared(aircraft.data(), _gs->cart().data(),
                                                 gs_base_vec.data());
  dist = sqrt(dist);
  gs_dist_node->setDoubleValue(dist);
  double heightAboveStationM = 
    (alt_node->getDoubleValue() - _gs->elevation()) * SG_FEET_TO_METER;
  
  //////////////////////////////////////////////////////////
  // compute the amount of glide slope needle deflection
  // (.i.e. the number of degrees we are off the glide slope * 5.0
  //
  // CLO - 13 Mar 2006: The glide slope needle should peg at
  // +/-0.7 degrees off the ideal glideslope.  I'm not sure why
  // we compute the factor the way we do (5*gs_error), but we
  // need to compensate for our 'odd' number in the glideslope
  // needle animation.  This means that the needle should peg
  // when this values is +/-3.5.
  //////////////////////////////////////////////////////////
  double angle = atan2(heightAboveStationM, dist) * SGD_RADIANS_TO_DEGREES;
  double deflectionAngle = target_gs - angle;
  //SG_CLAMP_RANGE(deflectionAngle, -0.7, 0.7);
  _gsNeedleDeflection = deflectionAngle * 5.0;
  _gsNeedleDeflection *= signal_quality_norm;
  _gsNeedleDeflectionNorm = (deflectionAngle / 0.7) * signal_quality_norm;
  SG_CLAMP_RANGE(_gsNeedleDeflectionNorm, -1.0, 1.0);
  
  //////////////////////////////////////////////////////////
  // Calculate desired rate of climb for intercepting the GS
  //////////////////////////////////////////////////////////
  double gs_diff = target_gs - angle;
  // convert desired vertical path angle into a climb rate
  double des_angle = angle - 10 * gs_diff;

  // estimate horizontal speed towards ILS in meters per minute
  double elapsedDistance = last_x - dist;
  last_x = dist;
      
  double new_vel = ( elapsedDistance / dt );
  horiz_vel = 0.75 * horiz_vel + 0.25 * new_vel;

  gs_rate_of_climb_node
      ->setDoubleValue( -sin( des_angle * SGD_DEGREES_TO_RADIANS )
                        * horiz_vel * SG_METER_TO_FEET );
}

void FGNavRadio::updateGPSSlaved()
{
  has_gs_node->setBoolValue(gps_has_gs_node->getBoolValue());
 
  _toFlag = gps_to_flag_node->getBoolValue();
  _fromFlag = gps_from_flag_node->getBoolValue();

  inrange_node->setBoolValue(_toFlag | _fromFlag);
  
  _cdiDeflection =  gps_cdi_deflection_node->getDoubleValue();
  // clmap to some range (+/- 10 degrees) as the regular deflection
  SG_CLAMP_RANGE(_cdiDeflection, -10.0, 10.0 );
  
  _cdiCrossTrackErrorM = 0.0; // FIXME, supply this
  _gsNeedleDeflection = 0.0; // FIXME, supply this
}

void FGNavRadio::updateCDI(double dt)
{
  bool cdi_serviceable = cdi_serviceable_node->getBoolValue();
  bool inrange = inrange_node->getBoolValue();
                               
  if (tofrom_serviceable_node->getBoolValue()) {
    to_flag_node->setBoolValue(_toFlag);
    from_flag_node->setBoolValue(_fromFlag);
  } else {
    to_flag_node->setBoolValue(false);
    from_flag_node->setBoolValue(false);
  }
  
  if (!cdi_serviceable) {
    _cdiDeflection = 0.0;
    _cdiCrossTrackErrorM = 0.0;
  }
  
  cdi_deflection_node->setDoubleValue(_cdiDeflection);
  cdi_deflection_norm_node->setDoubleValue(_cdiDeflection * 0.1);
  cdi_xtrack_error_node->setDoubleValue(_cdiCrossTrackErrorM);

  //////////////////////////////////////////////////////////
  // compute an approximate ground track heading error
  //////////////////////////////////////////////////////////
  double hdg_error = 0.0;
  if ( inrange && cdi_serviceable ) {
    double vn = fgGetDouble( "/velocities/speed-north-fps" );
    double ve = fgGetDouble( "/velocities/speed-east-fps" );
    double gnd_trk_true = atan2( ve, vn ) * SGD_RADIANS_TO_DEGREES;
    if ( gnd_trk_true < 0.0 ) { gnd_trk_true += 360.0; }

    SGPropertyNode *true_hdg
        = fgGetNode("/orientation/heading-deg", true);
    hdg_error = gnd_trk_true - true_hdg->getDoubleValue();

    // cout << "ground track = " << gnd_trk_true
    //      << " orientation = " << true_hdg->getDoubleValue() << endl;
  }
  cdi_xtrack_hdg_err_node->setDoubleValue( hdg_error );

  //////////////////////////////////////////////////////////
  // Calculate a suggested target heading to smoothly intercept
  // a nav/ils radial.
  //////////////////////////////////////////////////////////

  // Now that we have cross track heading adjustment built in,
  // we shouldn't need to overdrive the heading angle within 8km
  // of the station.
  //
  // The cdi deflection should be +/-10 for a full range of deflection
  // so multiplying this by 3 gives us +/- 30 degrees heading
  // compensation.
  double adjustment = _cdiDeflection * 3.0;
  SG_CLAMP_RANGE( adjustment, -30.0, 30.0 );

  // determine the target heading to fly to intercept the
  // tgt_radial = target radial (true) + cdi offset adjustmest -
  // xtrack heading error adjustment
  double nta_hdg;
  double trtrue = target_radial_true_node->getDoubleValue();
  if ( loc_node->getBoolValue() && backcourse_node->getBoolValue() ) {
      // tuned to a localizer and backcourse mode activated
      trtrue += 180.0;   // reverse the target localizer heading
      SG_NORMALIZE_RANGE(trtrue, 0.0, 360.0);
      nta_hdg = trtrue - adjustment - hdg_error;
  } else {
      nta_hdg = trtrue + adjustment - hdg_error;
  }

  SG_NORMALIZE_RANGE(nta_hdg, 0.0, 360.0);
  target_auto_hdg_node->setDoubleValue( nta_hdg );

  //////////////////////////////////////////////////////////
  // compute the time to intercept selected radial (based on
  // current and last cross track errors and dt
  //////////////////////////////////////////////////////////
  double t = 0.0;
  if ( inrange && cdi_serviceable ) {
    double xrate_ms = (last_xtrack_error - _cdiCrossTrackErrorM) / dt;
    if ( fabs(xrate_ms) > 0.00001 ) {
        t = _cdiCrossTrackErrorM / xrate_ms;
    } else {
        t = 9999.9;
    }
  }
  time_to_intercept->setDoubleValue( t );

  if (!gs_serviceable_node->getBoolValue() ) {
    _gsNeedleDeflection = 0.0;
    _gsNeedleDeflectionNorm = 0.0;
  }
  gs_deflection_node->setDoubleValue(_gsNeedleDeflection);
  gs_deflection_norm_node->setDoubleValue(_gsNeedleDeflectionNorm);
  
  last_xtrack_error = _cdiCrossTrackErrorM;
}

void FGNavRadio::updateAudio()
{
  if (!_navaid || !inrange_node->getBoolValue() || !nav_serviceable_node->getBoolValue()) {
    return;
  }
  
	// play station ident via audio system if on + ident,
	// otherwise turn it off
	if (!power_btn_node->getBoolValue()
      || !(bus_power_node->getDoubleValue() > 1.0)
      || !ident_btn_node->getBoolValue()
      || !audio_btn_node->getBoolValue() ) {
    globals->get_soundmgr()->stop( nav_fx_name );
    globals->get_soundmgr()->stop( dme_fx_name );
    return;
  }

  SGSoundSample *sound = globals->get_soundmgr()->find( nav_fx_name );
  double vol = vol_btn_node->getDoubleValue();
  SG_CLAMP_RANGE(vol, 0.0, 1.0);
  
  if ( sound != NULL ) {
    sound->set_volume( vol );
  } else {
    SG_LOG( SG_COCKPIT, SG_ALERT, "Can't find nav-vor-ident sound" );
  }
  
  sound = globals->get_soundmgr()->find( dme_fx_name );
  if ( sound != NULL ) {
    sound->set_volume( vol );
  } else {
    SG_LOG( SG_COCKPIT, SG_ALERT, "Can't find nav-dme-ident sound" );
  }

  if ( last_time < globals->get_time_params()->get_cur_time() - 30 ) {
		last_time = globals->get_time_params()->get_cur_time();
		play_count = 0;
  }
  
  if ( play_count < 4 ) {
		// play VOR ident
		if ( !globals->get_soundmgr()->is_playing(nav_fx_name) ) {
		    globals->get_soundmgr()->play_once( nav_fx_name );
		    ++play_count;
    }
  } else if ( play_count < 5 &&  has_dme) {
		// play DME ident
		if ( !globals->get_soundmgr()->is_playing(nav_fx_name) &&
		     !globals->get_soundmgr()->is_playing(dme_fx_name) ) {
		    globals->get_soundmgr()->play_once( dme_fx_name );
		    ++play_count;
		}
  }
}

FGNavRecord* FGNavRadio::findPrimaryNavaid(const SGGeod& aPos, double aFreqMHz)
{
  FGNavRecord* nav = globals->get_navlist()->findByFreq(aFreqMHz, aPos);
  if (nav) {
    return nav;
  }
  
  return globals->get_loclist()->findByFreq(aFreqMHz, aPos);
}

// Update current nav/adf radio stations based on current postition
void FGNavRadio::search() 
{
  _time_before_search_sec = 1.0;
  SGGeod pos = SGGeod::fromDegFt(lon_node->getDoubleValue(),
    lat_node->getDoubleValue(), alt_node->getDoubleValue());
  double freq = freq_node->getDoubleValue();
  
  FGNavRecord* nav = findPrimaryNavaid(pos, freq);
  if (nav == _navaid) {
    return; // found the same as last search, we're done
  }
  
  _navaid = nav;
  char identBuffer[5] = "    ";
  if (nav) {
    FGNavRecord* dme = globals->get_dmelist()->findByFreq(freq, pos);
    has_dme = (dme != NULL);
    
    nav_id_node->setStringValue(nav->get_ident());
    strncpy(identBuffer, nav->ident().c_str(), 5);
    
    effective_range = adjustNavRange(nav->get_elev_ft(), pos.getElevationM(), nav->get_range());
    loc_node->setBoolValue(nav->type() != FGPositioned::VOR);
    twist = nav->get_multiuse();

    if (nav->type() == FGPositioned::VOR) {
      target_radial = sel_radial_node->getDoubleValue();
      _gs = NULL;
    } else { // ILS or LOC
      _gs = globals->get_gslist()->findByFreq(freq, pos);
      has_gs_node->setBoolValue(_gs != NULL);
      twist = 0.0;
	    effective_range = FG_LOC_DEFAULT_RANGE;
      
      target_radial = nav->get_multiuse();
      SG_NORMALIZE_RANGE(target_radial, 0.0, 360.0);
      
      if (_gs) {
        int tmp = (int)(_gs->get_multiuse() / 1000.0);
        target_gs = (double)tmp / 100.0;
        
        SGGeod baseLine; 
        double dummy;
        SGGeodesy::direct(_gs->geod(), target_radial + 90.0, 100.0, baseLine, dummy);
        gs_base_vec = SGVec3d::fromGeod(baseLine) - _gs->cart();
      } // of have glideslope
    } // of found LOC or ILS
    
    audioNavidChanged();
  } else { // found nothing
    _gs = NULL;
    nav_id_node->setStringValue("");
    has_dme = false;
    globals->get_soundmgr()->remove( nav_fx_name );
    globals->get_soundmgr()->remove( dme_fx_name );
  }

  is_valid_node->setBoolValue(nav != NULL);
  id_c1_node->setIntValue( (int)identBuffer[0] );
  id_c2_node->setIntValue( (int)identBuffer[1] );
  id_c3_node->setIntValue( (int)identBuffer[2] );
  id_c4_node->setIntValue( (int)identBuffer[3] );
}

void FGNavRadio::audioNavidChanged()
{
  if ( globals->get_soundmgr()->exists(nav_fx_name)) {
		globals->get_soundmgr()->remove(nav_fx_name);
  }
  
  try {
    string trans_ident(_navaid->get_trans_ident());
    SGSoundSample* sound = morse.make_ident(trans_ident, LO_FREQUENCY);
    sound->set_volume( 0.3 );
    if (!globals->get_soundmgr()->add( sound, nav_fx_name )) {
      SG_LOG(SG_COCKPIT, SG_WARN, "Failed to add v1-vor-ident sound");
    }

	  if ( globals->get_soundmgr()->exists( dme_fx_name ) ) {
      globals->get_soundmgr()->remove( dme_fx_name );
    }
     
    sound = morse.make_ident( trans_ident, HI_FREQUENCY );
    sound->set_volume( 0.3 );
    globals->get_soundmgr()->add( sound, dme_fx_name );

	  int offset = (int)(sg_random() * 30.0);
	  play_count = offset / 4;
    last_time = globals->get_time_params()->get_cur_time() - offset;
  } catch (sg_io_exception& e) {
    SG_LOG(SG_GENERAL, SG_ALERT, e.getFormattedMessage());
  }
}
