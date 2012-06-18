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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sstream>
#include <cstring>

#include <simgear/sg_inlines.h>
#include <simgear/timing/sg_time.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/misc/strutils.hxx>

#include <Navaids/navrecord.hxx>
#include <Sound/audioident.hxx>
#include <Airports/runways.hxx>
#include <Navaids/navlist.hxx>
#include <Main/util.hxx>

#include "navradio.hxx"

using std::string;

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

// Calculate a Cartesian unit vector in the
// local horizontal plane, i.e. tangent to the 
// surface of the earth at the local ground zero.
// The tangent vector passes through the given  <midpoint> 
// and points forward along the given <heading>.
// The <heading> is given in degrees.
static SGVec3d tangentVector(const SGGeod& midpoint, const double heading)
{
// The size of the delta is presumably chosen to give
// numerical stability.  I don't know how the value was chosen.
// It probably doesn't matter much.  It gets divided out.
  double delta(100.0);          // in meters
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

// Create a "serviceable" node with a default value of "true"
SGPropertyNode_ptr createServiceableProp(SGPropertyNode* aParent, 
        const char* aName)
{
  SGPropertyNode_ptr n = 
     aParent->getChild(aName, 0, true)->getChild("serviceable", 0, true);
  simgear::props::Type typ = n->getType();
  if ((typ == simgear::props::NONE) || (typ == simgear::props::UNSPECIFIED)) {
    n->setBoolValue(true);
  }
  return n;  
}

// Constructor
FGNavRadio::FGNavRadio(SGPropertyNode *node) :
    term_tbl(NULL),
    low_tbl(NULL),
    high_tbl(NULL),
    _operable(false),
    play_count(0),
    _last_freq(0.0),
    target_radial(0.0),
    effective_range(0.0),
    target_gs(0.0),
    twist(0.0),
    horiz_vel(0.0),
    last_x(0.0),
    last_xtrack_error(0.0),
    xrate_ms(0.0),
    _localizerWidth(5.0),
    _name(node->getStringValue("name", "nav")),
    _num(node->getIntValue("number", 0)),
    _time_before_search_sec(-1.0),
    _gsCart(SGVec3d::zeros()),
    _gsAxis(SGVec3d::zeros()),
    _gsVertical(SGVec3d::zeros()),
    _toFlag(false),
    _fromFlag(false),
    _cdiDeflection(0.0),
    _cdiCrossTrackErrorM(0.0),
    _gsNeedleDeflection(0.0),
    _gsNeedleDeflectionNorm(0.0),
    _audioIdent(NULL)
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

    string branch("/instrumentation/" + _name);
    _radio_node = fgGetNode(branch.c_str(), _num, true);
}


// Destructor
FGNavRadio::~FGNavRadio() 
{
    if (gps_course_node) {
      gps_course_node->removeChangeListener(this);
    }
    
    if (nav_slaved_to_gps_node) {
      nav_slaved_to_gps_node->removeChangeListener(this);
    }
    
    delete term_tbl;
    delete low_tbl;
    delete high_tbl;

    delete _audioIdent;
}


void
FGNavRadio::init ()
{
    SGPropertyNode* node = _radio_node.get();
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
    cdi_serviceable_node = createServiceableProp(node, "cdi");
    gs_serviceable_node = createServiceableProp(node, "gs");
    tofrom_serviceable_node = createServiceableProp(node, "to-from");
    
    falseCoursesEnabledNode = 
      fgGetNode("/sim/realism/false-radio-courses-enabled");
    if (!falseCoursesEnabledNode) {
      falseCoursesEnabledNode = 
        fgGetNode("/sim/realism/false-radio-courses-enabled", true);
      falseCoursesEnabledNode->setBoolValue(true);
    }

    // frequencies
    SGPropertyNode *subnode = node->getChild("frequencies", 0, true);
    freq_node = subnode->getChild("selected-mhz", 0, true);
    alt_freq_node = subnode->getChild("standby-mhz", 0, true);
    fmt_freq_node = subnode->getChild("selected-mhz-fmt", 0, true);
    fmt_alt_freq_node = subnode->getChild("standby-mhz-fmt", 0, true);
    is_loc_freq_node = subnode->getChild("is-localizer-frequency", 0, true );

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
    gs_deflection_deg_node = node->getChild("gs-needle-deflection-deg", 0, true);
    gs_deflection_norm_node = node->getChild("gs-needle-deflection-norm", 0, true);
    gs_direct_node = node->getChild("gs-direct-deg", 0, true);
    gs_rate_of_climb_node = node->getChild("gs-rate-of-climb", 0, true);
    gs_rate_of_climb_fpm_node = node->getChild("gs-rate-of-climb-fpm", 0, true);
    gs_dist_node = node->getChild("gs-distance", 0, true);
    gs_inrange_node = node->getChild("gs-in-range", 0, true);
    
    nav_id_node = node->getChild("nav-id", 0, true);
    id_c1_node = node->getChild("nav-id_asc1", 0, true);
    id_c2_node = node->getChild("nav-id_asc2", 0, true);
    id_c3_node = node->getChild("nav-id_asc3", 0, true);
    id_c4_node = node->getChild("nav-id_asc4", 0, true);

    // gps slaving support
    nav_slaved_to_gps_node = node->getChild("slaved-to-gps", 0, true);
    nav_slaved_to_gps_node->addChangeListener(this);
    
    gps_cdi_deflection_node = fgGetNode("/instrumentation/gps/cdi-deflection", true);
    gps_to_flag_node = fgGetNode("/instrumentation/gps/to-flag", true);
    gps_from_flag_node = fgGetNode("/instrumentation/gps/from-flag", true);
    gps_has_gs_node = fgGetNode("/instrumentation/gps/has-gs", true);
    gps_course_node = fgGetNode("/instrumentation/gps/desired-course-deg", true);
    gps_course_node->addChangeListener(this);
    
    gps_xtrack_error_nm_node = fgGetNode("/instrumentation/gps/wp/wp[1]/course-error-nm", true);
    _magvarNode = fgGetNode("/environment/magnetic-variation-deg", true);
    
    std::ostringstream temp;
    temp << _name << "-ident-" << _num;
    if( NULL == _audioIdent ) 
        _audioIdent = new VORAudioIdent( temp.str() );
    _audioIdent->init();

    // dme-in-range is deprecated,
    // temporarily create dme-in-range alias for instrumentation/dme[0]/in-range
    // remove after flightgear 2.6.0
    node->getNode( "dme-in-range", true )->alias( fgGetNode("/instrumentation/dme[0]/in-range", true ) );
}

void
FGNavRadio::bind ()
{
    _radio_node->tie( "operable", SGRawValueMethods<FGNavRadio,bool>( *this, &FGNavRadio::isOperable ) );
}


void
FGNavRadio::unbind ()
{
    _radio_node->untie("operable");
}


// model standard VOR/DME/TACAN service volumes as per AIM 1-1-8
double FGNavRadio::adjustNavRange( double stationElev, double aircraftElev,
                                 double nominalRange )
{
    if (nominalRange <= 0.0) {
      nominalRange = FG_NAV_DEFAULT_RANGE;
    }
    
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

// Frequencies with odd 100kHz numbers in the range from 108.00 - 111.95
// are LOC/GS (ILS) frequency pairs
// (108.00, 108.05, 108.20, 108.25.. =VOR)
// (108.10, 108.15, 108.30, 108.35.. =ILS)
static inline bool IsLocalizerFrequency( double f )
{
  if( f < 108.0 || f >= 112.00 ) return false;
  return (((SGMiscd::roundToInt(f * 100.0) % 100)/10) % 2) != 0;
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
  is_loc_freq_node->setBoolValue( IsLocalizerFrequency( freq_node->getDoubleValue() ));

  if (power_btn_node->getBoolValue() 
      && (bus_power_node->getDoubleValue() > 1.0)
      && nav_serviceable_node->getBoolValue() )
  {
    _operable = true;
    updateReceiver(dt);
    updateCDI(dt);
  } else {
    clearOutputs();
  }
  
  updateAudio( dt );
}

void FGNavRadio::clearOutputs()
{
  inrange_node->setBoolValue( false );
  signal_quality_norm_node->setDoubleValue( 0.0 );
  cdi_deflection_node->setDoubleValue( 0.0 );
  cdi_deflection_norm_node->setDoubleValue( 0.0 );
  cdi_xtrack_error_node->setDoubleValue( 0.0 );
  cdi_xtrack_hdg_err_node->setDoubleValue( 0.0 );
  time_to_intercept->setDoubleValue( 0.0 );
  heading_node->setDoubleValue(0.0);
  gs_deflection_node->setDoubleValue( 0.0 );
  gs_deflection_deg_node->setDoubleValue(0.0);
  gs_deflection_norm_node->setDoubleValue(0.0);
  gs_direct_node->setDoubleValue(0.0);
  gs_inrange_node->setBoolValue( false );
  loc_node->setBoolValue( false );
  has_gs_node->setBoolValue(false);
  
  to_flag_node->setBoolValue( false );
  from_flag_node->setBoolValue( false );
  is_valid_node->setBoolValue(false);
  nav_id_node->setStringValue("");
  
  _operable = false;
  _navaid = NULL;
}

void FGNavRadio::updateReceiver(double dt)
{
  SGVec3d aircraft = SGVec3d::fromGeod(globals->get_aircraft_position());
  double loc_dist = 0;

  // Do a nav station search only once a second to reduce
  // unnecessary work. (Also, make sure to do this before caching
  // any values!)
  _time_before_search_sec -= dt;
  if ( _time_before_search_sec < 0 ) {
   search();
  }

  if (_navaid)
  {
      loc_dist = dist(aircraft, _navaid->cart());
      loc_dist_node->setDoubleValue( loc_dist );
  }

  if (nav_slaved_to_gps_node->getBoolValue()) {
    // when slaved to GPS: only allow stuff above: tune NAV station
    // All other data driven by GPS only.
    updateGPSSlaved();
    return;
  }

  if (!_navaid) {
    _cdiDeflection = 0.0;
    _cdiCrossTrackErrorM = 0.0;
    _toFlag = _fromFlag = false;
    _gsNeedleDeflection = 0.0;
    _gsNeedleDeflectionNorm = 0.0;
    heading_node->setDoubleValue(0.0);
    inrange_node->setBoolValue(false);
    signal_quality_norm_node->setDoubleValue(0.0);
    gs_dist_node->setDoubleValue( 0.0 );
    gs_inrange_node->setBoolValue(false);
    return;
  }

  double nav_elev = _navaid->get_elev_ft();

  bool is_loc = loc_node->getBoolValue();
  double signal_quality_norm = signal_quality_norm_node->getDoubleValue();
  
  double az2, s;
  //////////////////////////////////////////////////////////
	// compute forward and reverse wgs84 headings to localizer
  //////////////////////////////////////////////////////////
  double hdg;
  SGGeodesy::inverse(globals->get_aircraft_position(), _navaid->geod(), hdg, az2, s);
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
                = adjustILSRange( nav_elev, globals->get_aircraft_position().getElevationM(), offset,
                                  loc_dist * SG_METER_TO_NM );
	} else {
	    effective_range
                = adjustNavRange( nav_elev, globals->get_aircraft_position().getElevationM(), _navaid->get_range() );
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
  double r = target_radial - radial;
  SG_NORMALIZE_RANGE(r, -180.0, 180.0);
  
  if ( is_loc ) {
    if (falseCoursesEnabledNode->getBoolValue()) {
      // The factor of 30.0 gives a period of 120 which gives us 3 cycles and six 
      // zeros i.e. six courses: one front course, one back course, and four 
      // false courses. Three of the six are reverse sensing.
      _cdiDeflection = 30.0 * sawtooth(r / 30.0);
    } else {
      // no false courses, but we do need to create a back course
      if (fabs(r) > 90.0) { // front course
        _cdiDeflection = r - copysign(180.0, r);
      } else {
        _cdiDeflection = r; // back course
      }
      
      _cdiDeflection = -_cdiDeflection; // reverse for outbound radial
    } // of false courses disabled
    
    const double VOR_FULL_ARC = 20.0; // VOR is -10 .. 10 degree swing
    _cdiDeflection *= VOR_FULL_ARC / _localizerWidth; // increased localiser sensitivity
    
    if (backcourse_node->getBoolValue()) {
      _cdiDeflection = -_cdiDeflection;
    }
  } else {
    // handle the TO side of the VOR
    if (fabs(r) > 90.0) {
      r = ( r<0.0 ? -r-180.0 : -r+180.0 );
    }
    _cdiDeflection = r;
  } // of non-localiser case
  
  SG_CLAMP_RANGE(_cdiDeflection, -10.0, 10.0 );
  _cdiDeflection *= signal_quality_norm;
  
  // cross-track error (in meters)
  _cdiCrossTrackErrorM = loc_dist * sin(r * SGD_DEGREES_TO_RADIANS);
  
  updateGlideSlope(dt, aircraft, signal_quality_norm);
}

void FGNavRadio::updateGlideSlope(double dt, const SGVec3d& aircraft, double signal_quality_norm)
{
  bool gsInRange = (_gs && inrange_node->getBoolValue());
  double gsDist = 0;

  if (gsInRange)
  {
    gsDist = dist(aircraft, _gsCart);
    gsInRange = (gsDist < (_gs->get_range() * SG_NM_TO_METER));
  }

  gs_inrange_node->setBoolValue(gsInRange);
  gs_dist_node->setDoubleValue( gsDist );

  if (!gsInRange)
  {
    _gsNeedleDeflection = 0.0;
    _gsNeedleDeflectionNorm = 0.0;
    return;
  }
  
  SGVec3d pos = aircraft - _gsCart; // relative vector from gs antenna to aircraft
  // The positive GS axis points along the runway in the landing direction,
  // toward the far end, not toward the approach area, so we need a - sign here:
  double comp_h = -dot(pos, _gsAxis);      // component in horiz direction
  double comp_v = dot(pos, _gsVertical);   // component in vertical direction
  //double comp_b = dot(pos, _gsBaseline);   // component in baseline direction
  //if (comp_b) {}                           // ... (useful for debugging)

// _gsDirect represents the angle of elevation of the aircraft
// as seen by the GS transmitter.
  _gsDirect = atan2(comp_v, comp_h) * SGD_RADIANS_TO_DEGREES;
// At this point, if the aircraft is centered on the glide slope,
// _gsDirect will be a small positive number, e.g. 3.0 degrees

// Aim the branch cut straight down 
// into the ground below the GS transmitter:
  if (_gsDirect < -90.0) _gsDirect += 360.0;

  double deflectionAngle = target_gs - _gsDirect;
  
  if (falseCoursesEnabledNode->getBoolValue()) {
    // Construct false glideslopes.  The scale factor of 1.5 
    // in the sawtooth gives a period of 6 degrees.
    // There will be zeros at 3, 6r, 9, 12r et cetera
    // where "r" indicates reverse sensing.
    // This is is consistent with conventional pilot lore
    // e.g. http://www.allstar.fiu.edu/aerojava/ILS.htm
    // but inconsistent with
    // http://www.freepatentsonline.com/3757338.html
    //
    // It may be that some of each exist.
    if (deflectionAngle < 0) {
      deflectionAngle = 1.5 * sawtooth(deflectionAngle / 1.5);
    } else {
      // no false GS below the true GS
    }
  }
  
// GS is documented to be 1.4 degrees thick, 
// i.e. plus or minus 0.7 degrees from the midline:
  SG_CLAMP_RANGE(deflectionAngle, -0.7, 0.7);

// Many older instrument xml frontends depend on
// the un-normalized gs-needle-deflection.
// Apparently the interface standard is plus or minus 3.5 "volts"
// for a full-scale deflection:
  _gsNeedleDeflection = deflectionAngle * 5.0;
  _gsNeedleDeflection *= signal_quality_norm;
  
  _gsNeedleDeflectionNorm = (deflectionAngle / 0.7) * signal_quality_norm;
  
  //////////////////////////////////////////////////////////
  // Calculate desired rate of climb for intercepting the GS
  //////////////////////////////////////////////////////////
  double gs_diff = target_gs - _gsDirect;
  // convert desired vertical path angle into a climb rate
  double des_angle = _gsDirect - 10 * gs_diff;
  /* printf("target_gs=%.1f angle=%.1f gs_diff=%.1f des_angle=%.1f\n",
     target_gs, _gsDirect, gs_diff, des_angle); */

  // estimate horizontal speed towards ILS in meters per minute
  double elapsedDistance = last_x - gsDist;
  last_x = gsDist;
      
  double new_vel = ( elapsedDistance / dt );
  horiz_vel = 0.99 * horiz_vel + 0.01 * new_vel;
  /* printf("vel=%.1f (dist=%.1f dt=%.2f)\n", horiz_vel, elapsedDistance, dt);*/

  gs_rate_of_climb_node
      ->setDoubleValue( -sin( des_angle * SGD_DEGREES_TO_RADIANS )
                        * horiz_vel * SG_METER_TO_FEET );
  gs_rate_of_climb_fpm_node
      ->setDoubleValue( gs_rate_of_climb_node->getDoubleValue() * 60 );
}

void FGNavRadio::valueChanged (SGPropertyNode* prop)
{
  if (prop == gps_course_node) {
    if (!nav_slaved_to_gps_node->getBoolValue()) {
      return;
    }
  
    // GPS desired course has changed, sync up our selected-course
    double v = prop->getDoubleValue();
    if (v != sel_radial_node->getDoubleValue()) {
      sel_radial_node->setDoubleValue(v);
    }
  } else if (prop == nav_slaved_to_gps_node) {
    if (prop->getBoolValue()) {
      // slaved-to-GPS activated, clear obsolete NAV outputs and sync up selected course
      clearOutputs();
      sel_radial_node->setDoubleValue(gps_course_node->getDoubleValue());
    }
    // slave-to-GPS enabled/disabled, resync NAV station (update all outputs)
    _navaid = NULL;
    _time_before_search_sec = 0;
  }
}

void FGNavRadio::updateGPSSlaved()
{
  has_gs_node->setBoolValue(gps_has_gs_node->getBoolValue());
 
  _toFlag = gps_to_flag_node->getBoolValue();
  _fromFlag = gps_from_flag_node->getBoolValue();

  bool gpsValid = (_toFlag | _fromFlag);
  inrange_node->setBoolValue(gpsValid);
  if (!gpsValid) {
    signal_quality_norm_node->setDoubleValue(0.0);
    _cdiDeflection = 0.0;
    _cdiCrossTrackErrorM = 0.0;
    _gsNeedleDeflection = 0.0;
    _gsNeedleDeflectionNorm = 0.0;
    return;
  }
  
  // this is unfortunate, but panel instruments use this value to decide
  // if the navradio output is valid.
  signal_quality_norm_node->setDoubleValue(1.0);
  
  _cdiDeflection =  gps_cdi_deflection_node->getDoubleValue();
  // clmap to some range (+/- 10 degrees) as the regular deflection
  SG_CLAMP_RANGE(_cdiDeflection, -10.0, 10.0 );
  
  _cdiCrossTrackErrorM = gps_xtrack_error_nm_node->getDoubleValue() * SG_NM_TO_METER;
  _gsNeedleDeflection = 0.0; // FIXME, supply this
  
  double trtrue = gps_course_node->getDoubleValue() + _magvarNode->getDoubleValue();
  SG_NORMALIZE_RANGE(trtrue, 0.0, 360.0);
  target_radial_true_node->setDoubleValue( trtrue );
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
  // tgt_radial = target radial (true) + cdi offset adjustment -
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
  // current and last cross track errors and dt)
  //////////////////////////////////////////////////////////
  double t = 0.0;
  if ( inrange && cdi_serviceable ) {
    double cur_rate = (last_xtrack_error - _cdiCrossTrackErrorM) / dt;
    xrate_ms = 0.99 * xrate_ms + 0.01 * cur_rate;
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
  gs_deflection_deg_node->setDoubleValue(_gsNeedleDeflectionNorm * 0.7);
  gs_deflection_norm_node->setDoubleValue(_gsNeedleDeflectionNorm);
  gs_direct_node->setDoubleValue(_gsDirect);
  
  last_xtrack_error = _cdiCrossTrackErrorM;
}

void FGNavRadio::updateAudio( double dt )
{
  if (!_navaid || !inrange_node->getBoolValue() || !nav_serviceable_node->getBoolValue()) {
    _audioIdent->setIdent("", 0.0 );
    return;
  }
  
	// play station ident via audio system if on + ident,
	// otherwise turn it off
  if (!power_btn_node->getBoolValue()
      || !(bus_power_node->getDoubleValue() > 1.0)
      || !ident_btn_node->getBoolValue()
      || !audio_btn_node->getBoolValue() ) {
    _audioIdent->setIdent("", 0.0 );
    return;
  }

  _audioIdent->setIdent( _navaid->get_trans_ident(), vol_btn_node->getFloatValue() );

  _audioIdent->update( dt );
}

FGNavRecord* FGNavRadio::findPrimaryNavaid(const SGGeod& aPos, double aFreqMHz)
{
  FGNavRecord* nav = globals->get_navlist()->findByFreq(aFreqMHz, aPos);
  if (nav) {
    return nav;
  }
  
  return globals->get_loclist()->findByFreq(aFreqMHz, aPos);
}

// Update current nav/adf radio stations based on current position
void FGNavRadio::search() 
{
  // set delay for next search
  _time_before_search_sec = 1.0;

  double freq = freq_node->getDoubleValue();

  // immediate NAV search when frequency has changed (toggle between nav and g/s search otherwise)
  _nav_search |= (_last_freq != freq);

  // do we need to search a new NAV station in this iteration?
  if (_nav_search)
  {
      _last_freq = freq;
      FGNavRecord* nav = findPrimaryNavaid(globals->get_aircraft_position(), freq);
      if (nav == _navaid) {
        if (nav && (nav->type() != FGPositioned::VOR))
            _nav_search = false;  // search glideslope on next iteration
        return; // nav hasn't changed, we're done
      }
      // remember new navaid station
      _navaid = nav;
  }

  // search glideslope station
  if ((_navaid.valid()) && (_navaid->type() != FGPositioned::VOR))
  {
      FGNavRecord* gs = globals->get_gslist()->findByFreq(freq, globals->get_aircraft_position());
      if ((!_nav_search) && (gs == _gs))
      {
          _nav_search = true; // search NAV on next iteration
          return; // g/s hasn't changed, neither has nav - we're done
      }
      // remember new glideslope station
      _gs = gs;
  }

  _nav_search = true; // search NAV on next iteration

  // nav or gs station has changed
  updateNav();
}

// Update current nav/adf/glideslope outputs when station has changed
void FGNavRadio::updateNav()
{
  // update necessary, nav and/or gs has changed
  FGNavRecord* nav = _navaid;
  string identBuffer(4, ' ');
  if (nav) {
    nav_id_node->setStringValue(nav->get_ident());
    identBuffer =  simgear::strutils::rpad( nav->ident(), 4, ' ' );
    
    effective_range = adjustNavRange(nav->get_elev_ft(), globals->get_aircraft_position().getElevationM(), nav->get_range());
    loc_node->setBoolValue(nav->type() != FGPositioned::VOR);
    twist = nav->get_multiuse();

    if (nav->type() == FGPositioned::VOR) {
      target_radial = sel_radial_node->getDoubleValue();
      _gs = NULL;
    } else { // ILS or LOC
      _localizerWidth = nav->localizerWidth();
      twist = 0.0;
      effective_range = nav->get_range();
      
      target_radial = nav->get_multiuse();
      SG_NORMALIZE_RANGE(target_radial, 0.0, 360.0);

      if (_gs) {
        int tmp = (int)(_gs->get_multiuse() / 1000.0);
        target_gs = (double)tmp / 100.0;

        double gs_radial = fmod(_gs->get_multiuse(), 1000.0);
        SG_NORMALIZE_RANGE(gs_radial, 0.0, 360.0);
        _gsCart = _gs->cart();
                
        // GS axis unit tangent vector 
        // (along the runway):
        _gsAxis = tangentVector(_gs->geod(), gs_radial);

        // GS baseline unit tangent vector
        // (transverse to the runway along the ground)
        _gsBaseline = tangentVector(_gs->geod(), gs_radial + 90.0);
        _gsVertical = cross(_gsBaseline, _gsAxis);
      } // of have glideslope
    } // of found LOC or ILS
    
  } else { // found nothing
    _gs = NULL;
    nav_id_node->setStringValue("");
    loc_node->setBoolValue(false);
    _audioIdent->setIdent("", 0.0 );
  }

  has_gs_node->setBoolValue(_gs != NULL);
  is_valid_node->setBoolValue(nav != NULL);
  id_c1_node->setIntValue( (int)identBuffer[0] );
  id_c2_node->setIntValue( (int)identBuffer[1] );
  id_c3_node->setIntValue( (int)identBuffer[2] );
  id_c4_node->setIntValue( (int)identBuffer[3] );
}
