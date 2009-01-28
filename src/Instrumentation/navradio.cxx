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
// $Id: navradio.cxx,v 1.1 2008/12/31 07:28:30 jsd Exp jsd $


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

#include <Navaids/navlist.hxx>
#include <Main/util.hxx>
#include "navradio.hxx"
#include <iostream>

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
    dme_serviceable_node(NULL),
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
    inrange_node(NULL),                 // azimuth inrange
    signal_quality_norm_node(NULL),
    heading_needle_deflection_node(NULL),
    heading_needle_deflection_norm(0.),
    cdi_xtrack_error_node(NULL),
    cdi_xtrack_hdg_err_node(NULL),
    has_gs_node(NULL),
    loc_node(NULL),
    loc_dist_node(NULL),
    gs_needle_deflection_node(NULL),
    gs_needle_deflection_norm(0.),
    gs_inrange(0),
    gs_rate_of_climb_node(NULL),
    gs_distance(0.),
    nav_id_node(NULL),
    id_c1_node(NULL),
    id_c2_node(NULL),
    id_c3_node(NULL),
    id_c4_node(NULL),
    nav_slaved_to_gps_node(NULL),
    gps_cdi_deflection_node(NULL),
    gps_to_flag_node(NULL),
    gps_from_flag_node(NULL),
    last_nav_id(""),
    last_nav_vor(false),
    play_count(0),
    last_time(0),
    radial(0.0),
    target_radial(0.0),
    horiz_vel(0.0),
    last_x(0.0),
    last_loc_dist(0.0),
    last_xtrack_error(0.0),
    _name(node->getStringValue("name", "nav")),
    _num(node->getIntValue("number", 0)),
    _time_before_search_sec(-1.0),
    loc_runway(NULL)
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

// Create a "serviceable" node with a default value of "true"
SGPropertyNode_ptr serve_up(SGPropertyNode *node, const char* svc){
  SGPropertyNode_ptr  rslt = (node->getChild(svc, 0, true))
	->getChild("serviceable", 0, true);
  SGPropertyNode::Type typ = rslt->getType();
  if (typ == SGPropertyNode::NONE 
        || typ == SGPropertyNode::UNSPECIFIED) {
    rslt->setBoolValue(true);
  }
  return rslt;  
}

void
FGNavRadio::init ()
{

    string branch = "/instrumentation/" + _name;
    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );

    bus_power_node = 
	fgGetNode(("/systems/electrical/outputs/" + _name).c_str(), true);

    string opt_branch = "/sim/instrument-options/" + _name;
    SGPropertyNode *options = fgGetNode(opt_branch.c_str(), _num, true);

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
    nav_serviceable_node = serve_up(node, "nav");
    cdi_serviceable_node = serve_up(node, "cdi");
    gs_serviceable_node = serve_up(node, "gs");
    dme_serviceable_node = serve_up(node, "dme");
    tofrom_serviceable_node = serve_up(node, "to-from");

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
    heading_needle_deflection_node = node->getChild("heading-needle-deflection", 0, true);
    cdi_xtrack_error_node = node->getChild("crosstrack-error-m", 0, true);
    cdi_xtrack_hdg_err_node
        = node->getChild("crosstrack-heading-error-deg", 0, true);
    has_gs_node = node->getChild("has-gs", 0, true);
    loc_node = node->getChild("nav-loc", 0, true);
    loc_dist_node = node->getChild("nav-distance", 0, true);
    gs_needle_deflection_node = node->getChild("gs-needle-deflection", 0, true);
    gs_rate_of_climb_node = node->getChild("gs-rate-of-climb", 0, true);
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
    
    std::ostringstream temp;
    temp << _name << "nav-ident" << _num;
    nav_fx_name = temp.str();
    temp << _name << "dme-ident" << _num;
    dme_fx_name = temp.str();

    morse.init();

//    SG_LOG( SG_COCKPIT, SG_ALERT, "navradio: init complete: "  
//        << dme_serviceable_node );
}

void FGNavRadio::bind ()
{
    std::ostringstream temp;
    temp << _num;
    string branch = "/instrumentation/"
        + _name + "[" + temp.str() + "]";
    string opt_branch = "/sim/instrument-options/" 
        + _name + "[" + temp.str() + "]";

    fgTie((branch + "/loc-width").c_str(), this,
          &FGNavRadio::get_loc_width,
          &FGNavRadio::set_loc_width);

    fgTie((branch + "/gs-inrange").c_str(), this,
          &FGNavRadio::get_gs_inrange,
          &FGNavRadio::set_gs_inrange);

    fgTie((branch + "/gs-distance").c_str(), this,
          &FGNavRadio::get_gs_distance,
          &FGNavRadio::set_gs_distance);

    fgTie((branch + "/heading-needle-deflection-norm").c_str(), this,
          &FGNavRadio::get_heading_needle_deflection_norm,
          &FGNavRadio::set_heading_needle_deflection_norm);

    fgTie((branch + "/gs-needle-deflection-norm").c_str(), this,
          &FGNavRadio::get_gs_needle_deflection_norm,
          &FGNavRadio::set_gs_needle_deflection_norm);

    fgTie((opt_branch + "/gs-park-norm").c_str(), this,
          &FGNavRadio::get_gs_park_norm,
          &FGNavRadio::set_gs_park_norm);
}


void
FGNavRadio::unbind ()
{
    std::ostringstream temp;
    temp << _num;
    string branch = "/instrumentation/"
        + _name + "[" + temp.str() + "]";
    string opt_branch = "/sim/instrument-options/" 
        + _name + "[" + temp.str() + "]";

    fgUntie((branch + "/loc-width").c_str());
    fgUntie((branch + "/gs-inrange").c_str());
    fgUntie((branch + "/heading-needle-deflection-norm").c_str());
    fgUntie((branch + "/gs-needle-deflection-norm").c_str());
    fgUntie((opt_branch + "/gs-park-norm").c_str());

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


// model standard localizer service volumes as per AIM 1-1-9
double FGNavRadio::adjustLocRange( double stationElev, double aircraftElev,
                                 double offsetDegrees, double range )
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
    return range;
}


// General-purpose sawtooth function.  Graph looks like this:
//         /\
//       \/
// Odd symmetry, inversion symmetry about the origin.
// Unit slope at the origin.
// Max 1, min -1, period 4.
// Two zero-crossings per period, one with + slope, one with - slope.
// Useful for false localizer courses.
inline double sawtooth(double xx){
   return 4.0 * fabs(xx/4.0 + 0.25 - floor(xx/4.0 + 0.75)) - 1.0;
}

//////////////////////////////////////////////////////////////////////////
// Update the various nav values based on position and valid tuned in navs
//////////////////////////////////////////////////////////////////////////
void 
FGNavRadio::update(double dt) 
{
    // Do a nav station search only once a second to reduce
    // unnecessary work. (Also, make sure to do this before caching
    // any values!)
    _time_before_search_sec -= dt;
    if ( _time_before_search_sec < 0 ) {
	search();
    }

    // cache a few strategic values locally for speed
    SGGeod pos = SGGeod::fromDegFt(lon_node->getDoubleValue(),
                                   lat_node->getDoubleValue(),
                                   alt_node->getDoubleValue());
    bool power_btn = power_btn_node->getBoolValue();
    bool nav_serviceable = nav_serviceable_node->getBoolValue();
    bool dme_serviceable = dme_serviceable_node->getBoolValue();
    bool cdi_serviceable = cdi_serviceable_node->getBoolValue();
    bool gs_serviceable = gs_serviceable_node->getBoolValue();
    bool tofrom_serviceable = tofrom_serviceable_node->getBoolValue();
    bool inrange = inrange_node->getBoolValue();
    bool has_gs = has_gs_node->getBoolValue();
    bool is_loc = loc_node->getBoolValue();
    double loc_dist = loc_dist_node->getDoubleValue();
    double az_eff_range_m;
    double signal_quality_norm = signal_quality_norm_node->getDoubleValue();

    double az1, az2, s;

    // Create "formatted" versions of the nav frequencies for
    // instrument displays.
    char tmp[16];
    sprintf( tmp, "%.2f", freq_node->getDoubleValue() );
    fmt_freq_node->setStringValue(tmp);
    sprintf( tmp, "%.2f", alt_freq_node->getDoubleValue() );
    fmt_alt_freq_node->setStringValue(tmp);

    // cout << "is_valid = " << is_valid
    //      << " power_btn = " << power_btn
    //      << " bus_power = " << bus_power_node->getDoubleValue()
    //      << " nav_serviceable = " << nav_serviceable
    //      << endl;

    if ( is_valid && power_btn && (bus_power_node->getDoubleValue() > 1.0)
         && nav_serviceable )
    {
        SGVec3d aircraft = SGVec3d::fromGeod(pos);
        loc_dist = dist(aircraft, az_xyz);
	loc_dist_node->setDoubleValue( loc_dist );
        // cout << "dt = " << dt << " dist = " << loc_dist << endl;

        //////////////////////////////////////////////////////////
	// compute forward and reverse wgs84 headings to localizer
        //////////////////////////////////////////////////////////
        double hdg;
        geo_inverse_wgs_84( pos, SGGeod::fromDeg(az_lon, az_lat),
			    &hdg, &az2, &s );
	// cout << "az1 = " << az1 << " magvar = " << nav_magvar << endl;
        heading_node->setDoubleValue( hdg );
        radial = az2 - twist;
        double recip = radial + 180.0;
        if ( recip >= 360.0 ) { recip -= 360.0; }
	radial_node->setDoubleValue( radial );
	recip_radial_node->setDoubleValue( recip );
	// cout << " heading = " << heading_node->getDoubleValue()
	//      << " dist = " << nav_dist << endl;

        //////////////////////////////////////////////////////////
        // compute the target/selected radial in "true" heading
        //////////////////////////////////////////////////////////
        double trtrue = 0.0;
        if ( is_loc ) {
            // ILS localizers radials are already "true" in our
            // database
            trtrue = target_radial;
        } else {
            // VOR radials need to have that vor's offset added in
            trtrue = target_radial + twist;
        }

        while ( trtrue < 0.0 ) { trtrue += 360.0; }
        while ( trtrue > 360.0 ) { trtrue -= 360.0; }
        target_radial_true_node->setDoubleValue( trtrue );

        //////////////////////////////////////////////////////////
	// adjust reception range for altitude
        // FIXME: make sure we are using the navdata range now that
        //        it is valid in the data file
        //////////////////////////////////////////////////////////
	if ( is_loc ) {
	    double offset = radial - target_radial;
	    while ( offset < -180.0 ) { offset += 360.0; }
	    while ( offset > 180.0 ) { offset -= 360.0; }
	    // cout << "ils offset = " << offset << endl;
            az_eff_range = adjustLocRange( az_elev, pos.getElevationM(), 
                  offset, loc_dist * SG_METER_TO_NM );
	} else {        // not a localizer
            az_eff_range
                = adjustNavRange( az_elev, pos.getElevationM(), az_range );
	}

        az_eff_range_m = az_eff_range * SG_NM_TO_METER;

	// cout << "nav range = " << az_eff_range
	//      << " (" << range << ")" << endl;

        //////////////////////////////////////////////////////////
        // compute signal quality
        // 100% if within effective range
        // decreases 1/x^2 further out
        //////////////////////////////////////////////////////////
        {
            double last_signal_quality_norm = signal_quality_norm;

            if ( loc_dist < az_eff_range_m ) {
              signal_quality_norm = 1.0;
            } else {
              double range_exceed_norm = loc_dist/az_eff_range_m;
              signal_quality_norm = 1/(range_exceed_norm*range_exceed_norm);
            }

            signal_quality_norm = fgGetLowPass( last_signal_quality_norm, 
                   signal_quality_norm, dt );
        }
        signal_quality_norm_node->setDoubleValue( signal_quality_norm );
        inrange = signal_quality_norm > 0.2;
        inrange_node->setBoolValue( inrange );

	if ( !is_loc ) {
	    target_radial = sel_radial_node->getDoubleValue();
	}

        //////////////////////////////////////////////////////////
        // compute to/from flag status
        //////////////////////////////////////////////////////////
        bool value = false;
        double offset = fabs(radial - target_radial);
        if ( tofrom_serviceable ) {
            if ( nav_slaved_to_gps_node->getBoolValue() ) {
                value = gps_to_flag_node->getBoolValue();
            } else if ( inrange ) {
                if ( is_loc ) {
                    value = true;
                } else {
                    value = offset > 91.0 && offset < 269.0;
                }
            }
        } else {
            value = false;
        }
        to_flag_node->setBoolValue( value );

        value = false;
        if ( tofrom_serviceable ) {
            if ( nav_slaved_to_gps_node->getBoolValue() ) {
                value = gps_from_flag_node->getBoolValue();
            } else if ( inrange ) {
                if ( is_loc ) {
                    value = false;
                } else {
                    value = offset < 89.0 || offset > 271.0;
                }
            }
        } else {
            value = false;
        }
        from_flag_node->setBoolValue( value );

        //////////////////////////////////////////////////////////
        // compute the deflection of the CDI needle
        //////////////////////////////////////////////////////////
        double r = 0.0;
        double rnorm = 0.0;

        if ( cdi_serviceable ) {
            if ( nav_slaved_to_gps_node->getBoolValue() ) {
                // This is normalized old-style: [-10 .. 10]
                r = gps_cdi_deflection_node->getDoubleValue();
                rnorm = r / 10.0;
            } else if ( inrange ) {
                r = target_radial - radial;     // in degrees
                // cout << "Target radial = " << target_radial 
                //      << "  Actual radial = " << radial << endl;
                
                while ( r >  180.0 ) { r -= 360.0;}
                while ( r < -180.0 ) { r += 360.0;}

                if ( is_loc ) {
                    // The factor of 30.0 gives a period of 120
                    // which gives us 3 cycles and six zeros
                    // i.e. six courses: 
                    // one front course, one back course,
                    // and four false courses.
                    // Three of the six are reverse sensing.
                   r = 30.0 * sawtooth(r / 30.0);
                   if (loc_width <= 0.) loc_width = 5.;
                   // loc_width is peg-to-peg,
                   // so we want half that excurson to each side:
                   rnorm = r / (0.5 * loc_width);
                } else {
                    // Handle the "TO" side of the VOR.
                  if ( fabs(r) > 90.0 ) {
                     r = ( r<0.0 ? -r-180.0 : -r+180.0 );
                  }
                  // here with r = VOR CDI normed to [-10 .. 10] degrees
                  rnorm = r / 10.0;    //  normed to [-1 .. 1] nominal "full"
                }

                r *= signal_quality_norm;
                rnorm *= signal_quality_norm;
            }
        }  else {
          // CDI not serviceable
        }
        // Here with r in degrees, which we pass on to xtrack;
        // Here with rnorm in [-1 .. 1] nominal "full" but not clamped 
        // ... so it may go outside the nominal interval.
        heading_needle_deflection_norm = rnorm;

        // Also normalize to [-10 .. 10] for historical reasons
        // which made sense for VOR (in degrees)
        // but not so much for LOC or RNAV
        heading_needle_deflection_node->setDoubleValue( rnorm * 10.0 );

        //////////////////////////////////////////////////////////
        // compute the amount of cross track distance error in meters
        //////////////////////////////////////////////////////////
        double xtrack_error = 0.0;
        if ( inrange && nav_serviceable && cdi_serviceable ) {
            r = radial - target_radial;
            // cout << "Target radial = " << target_radial 
            //     << "  Actual radial = " << radial
            //     << "  r = " << r << endl;
    
            while ( r >  180.0 ) { r -= 360.0;}
            while ( r < -180.0 ) { r += 360.0;}
            if ( fabs(r) > 90.0 ) {
                r = ( r<0.0 ? -r-180.0 : -r+180.0 );
            }

            r = -r;             // reverse, since radial is outbound

            xtrack_error = loc_dist * sin(r * SGD_DEGREES_TO_RADIANS);
        } else {
            xtrack_error = 0.0;
        }
        cdi_xtrack_error_node->setDoubleValue( xtrack_error );

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
        // compute the time to intercept selected radial 
        // (based on current and last cross track errors and dt)
        //////////////////////////////////////////////////////////
        double t = 0.0;
        if ( inrange && cdi_serviceable ) {
            double xrate_ms = (last_xtrack_error - xtrack_error) / dt;
            if ( fabs(xrate_ms) > 0.00001 ) {
                t = xtrack_error / xrate_ms;
            } else {
                t = 9999.9;
            }
        }
        time_to_intercept->setDoubleValue( t );

        //////////////////////////////////////////////////////////
        // Basic DME stuff
        //////////////////////////////////////////////////////////
        if (has_dme) {
          dme_inrange = 0;
          double dme_distance = dist(aircraft, dme_xyz);    // in meters
                                                // needed for range check
          if ( dme_distance < dme_range * SG_NM_TO_METER) {
            dme_inrange = 1;
          }
        }


        //////////////////////////////////////////////////////////
        // Compute the amount of glide slope needle deflection.
        // Normalization is discussed below.
        //////////////////////////////////////////////////////////
        int gsok(0);

        if ( has_gs && gs_serviceable ) {
            if ( nav_slaved_to_gps_node->getBoolValue() ) {
                //
                // FIXME/FINISHME, what should be set here?
                //
                // Current generation GPS uses the plain old
                // glideslope signal.  We assume it was properly
                // tuned up, above.
                //
            }

            gs_distance = dist(aircraft, gs_xyz);    // in meters
                                                // needed for range check

            if ( gs_distance < gs_range * SG_NM_TO_METER) {
                SGVec3d pos(aircraft);
                pos -= gs_xyz;         // relative vector from gs antenna to aircraft

// The positive GS axis points along the runway in the landing direction,
// toward the far end, not toward the approach area, so we need a - sign here:
                double dot_h = -dot(pos, gs_axis);
                double dot_v = dot(pos, gs_vertical);
                double angle = atan2(dot_v, dot_h) * SGD_RADIANS_TO_DEGREES;
                #if 0
                  cout << "h,v,angle: " << dot_h 
                        << ", " << dot_v 
                        << ", " << angle << endl;
                #endif

                r = target_gs - angle;          // positive ==> fly up.
// 90 degrees of fly-up means we are underground, below the transmitter
// which is a good place to put the branch cut:
                if (r > 90.0) r -= 360.0;

// Construct false glideslopes.  The scale factor of 1.5 
// in the sawtooth gives a period of 6 degrees.  
// There will be zeros at 3, 6r, 9, 12r et cetera
// where "r" indicates reverse sensing.
// This is is consistent with conventional pilot lore
// e.g. http://www.allstar.fiu.edu/aerojava/ILS.htm
// but inconsistent with
// http://www.freepatentsonline.com/3757338.html
//
// It may be that some transmitters of each type exist.
                if (r < 0) r = 1.5 * sawtooth( r / 1.5);
                else {} // never any false GS below the true GS.

                gsok = 1;
            } else {
              // GS out of range
            } 
        } else {                   // didn't have working GS
          gs_distance = 0.0;
          gsok = 0;
        }

        // Here with GS angle r in degrees.
        // rnorm is normed to std glideslope thickness, so that
        // rnorm lies in the interval [-1 .. 1] nominal "full" scale
        // but not clamped.
        rnorm = r / 0.7;     // GS sector is 1.4 degrees peg-to-peg

        if (!gsok) rnorm = gs_park_norm;
        gs_needle_deflection_norm = rnorm;
        // Magic factor of 3.5 for unfathomable historical reasons.
        // This node uses [-3.5 .. 3.5] nominal "full" range,
        // i.e. degrees * 5.0
        gs_needle_deflection_node->setDoubleValue( rnorm * 3.5);
        gs_inrange = gsok;              // tied to property tree

        //////////////////////////////////////////////////////////
        // Calculate desired rate of climb for intercepting the GS
        //////////////////////////////////////////////////////////
        double x = gs_distance;
        double y = (alt_node->getDoubleValue() - gs_elev)
            * SG_FEET_TO_METER;
        double current_angle = atan2( y, x ) * SGD_RADIANS_TO_DEGREES;

        double target_angle = target_gs;
        double gs_diff = target_angle - current_angle;

        // convert desired vertical path angle into a climb rate
        double des_angle = current_angle - 10 * gs_diff;

        // estimate horizontal speed towards ILS in meters per minute
        double dist = last_x - x;
        last_x = x;
        if ( dt > 0.0 ) {
            // avoid nan
            double new_vel = ( dist / dt );
 
            horiz_vel = 0.75 * horiz_vel + 0.25 * new_vel;
            // double horiz_vel = cur_fdm_state->get_V_ground_speed()
            //    * SG_FEET_TO_METER * 60.0;
            // double horiz_vel = airspeed_node->getFloatValue()
            //    * SG_FEET_TO_METER * 60.0;

            gs_rate_of_climb_node
                ->setDoubleValue( -sin( des_angle * SGD_DEGREES_TO_RADIANS )
                                  * horiz_vel * SG_METER_TO_FEET );
        }

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
        double adjustment = heading_needle_deflection_node->getDoubleValue() * 3.0;
        SG_CLAMP_RANGE( adjustment, -30.0, 30.0 );

        // determine the target heading to fly to intercept the
        // tgt_radial = target radial (true) + cdi offset adjustmest -
        // xtrack heading error adjustment
        double nta_hdg;
        if ( is_loc && backcourse_node->getBoolValue() ) {
            // tuned to a localizer and backcourse mode activated
            trtrue += 180.0;   // reverse the target localizer heading
            while ( trtrue > 360.0 ) { trtrue -= 360.0; }
            nta_hdg = trtrue - adjustment - hdg_error;
        } else {
            nta_hdg = trtrue + adjustment - hdg_error;
        }

        while ( nta_hdg <   0.0 ) { nta_hdg += 360.0; }
        while ( nta_hdg >= 360.0 ) { nta_hdg -= 360.0; }
        target_auto_hdg_node->setDoubleValue( nta_hdg );

        last_xtrack_error = xtrack_error;
   } else {
   // not a valid tuning at all.
	inrange_node->setBoolValue( false );
        heading_needle_deflection_node->setDoubleValue( 0.0 );
        cdi_xtrack_error_node->setDoubleValue( 0.0 );
        cdi_xtrack_hdg_err_node->setDoubleValue( 0.0 );
        time_to_intercept->setDoubleValue( 0.0 );
        gs_needle_deflection_norm = gs_park_norm;
        gs_needle_deflection_node->setDoubleValue( gs_park_norm * 3.5);
        gs_inrange = false;
        to_flag_node->setBoolValue( false );
        from_flag_node->setBoolValue( false );
	// cout << "Navradio: not picking up vor. " << endl;
    }

    // audio effects
    if ( is_valid ) {
	// play station ident via audio system if on + ident,
	// otherwise turn it off
	if ( power_btn
             && (bus_power_node->getDoubleValue() > 1.0)
             && ident_btn_node->getBoolValue()
             && audio_btn_node->getBoolValue() )
        {
	    SGSoundSample *sound;
	    sound = globals->get_soundmgr()->find( nav_fx_name );
            double vol = vol_btn_node->getDoubleValue();
            if ( vol < 0.0 ) { vol = 0.0; }
            if ( vol > 1.0 ) { vol = 1.0; }
            if ( sound != NULL ) {
                sound->set_volume( vol );
            } else {
                SG_LOG( SG_COCKPIT, SG_ALERT,
                        "Can't find nav-vor-ident sound" );
            }
	    sound = globals->get_soundmgr()->find( dme_fx_name );
            if ( sound != NULL ) {
                sound->set_volume( vol );
            } else {
                SG_LOG( SG_COCKPIT, SG_ALERT,
                        "Can't find nav-dme-ident sound" );
            }
            // cout << "last_time = " << last_time << " ";
            // cout << "cur_time = "
            //      << globals->get_time_params()->get_cur_time();
            const int nslots(5);
            const time_t slot_length(5);        // in seconds
            // There are N slots numbered 0 through (nslots-1) inclusive.
            // Each slot is 5 seconds long.
            // Slots 0 is for DME
            // the rest are for azimuth.
            time_t now = globals->get_time_params()->get_cur_time();
	    if ( now < last_time        // time runs backwards?
              || now > last_time + slot_length ) {
		last_time = now;
		play_count = ++play_count % nslots;
// Previous ident is out of time;  if still playing, cut it off:
                globals->get_soundmgr()->stop( nav_fx_name );
                globals->get_soundmgr()->stop( dme_fx_name );
              // cout << " play_count = " << play_count << endl;
              // cout << "playing = "
              //      << globals->get_soundmgr()->is_playing(nav_fx_name)
              //      << endl;
              if ( play_count == 0) {
                if (dme_inrange && dme_serviceable) {
                  // play DME ident
                  globals->get_soundmgr()->play_once( dme_fx_name );
                }
              } else {
                if (inrange && nav_serviceable) {
                  // play VOR ident
                  globals->get_soundmgr()->play_once( nav_fx_name );
                }
              }
	    }
	}
    }

    last_loc_dist = loc_dist;
}


////////
// Calculate a unit vector in the horizontal tangent plane
// starting at the given "tail" of the vector and going off 
// with the given heading.
// Lots of redundant args, 
// but it is efficient to precalculate them.
SGVec3d tangentVector(
          const double tail_lat, const double tail_lon, 
          const double tail_elev, const SGVec3d tail_xyz, 
          const double heading) {

    double head_lon, head_lat, headaz;    // head of vector
// The fudge factor here is presumably intended to improve
// numerical stability.  I don't know if it is necessary.
// It gets divided out later.
    double fudge(100.0);
    geo_direct_wgs_84 ( 0.0, tail_lat, tail_lon, heading,
                        fudge, &head_lat, &head_lon, &headaz );
    #if 0
      cout << "tangentVector: heading = " << heading << endl;
      cout << "tail: " << tail_lon << ", " << tail_lat << endl;
      cout << "head: " << head_lon << ", " << head_lat 
              <<  " (" << tail_elev << ")" << endl;
    #endif
    SGGeod head = SGGeod::fromDegFt(head_lon, head_lat, tail_elev);
    SGVec3d head_xyz = SGVec3d::fromGeod(head);

    #if 0
      cout << tail_xyz << endl;
      cout << head_xyz << endl;
    #endif
    return (head_xyz - tail_xyz) * (1.0/fudge);
}

// Update current nav/adf radio stations based on current postition
void FGNavRadio::search() 
{

    // reset search time
    _time_before_search_sec = 1.0;

    // cache values locally for speed
    SGGeod pos = SGGeod::fromDegFt(lon_node->getDoubleValue(),
      lat_node->getDoubleValue(), alt_node->getDoubleValue());
    FGNavRecord *nav = NULL;
    FGNavRecord *loc = NULL;
    FGNavRecord *dme = NULL;
    FGNavRecord *gs = NULL;

    ////////////////////////////////////////////////////////////////////////
    // Tune the nav.
    ////////////////////////////////////////////////////////////////////////

    double freq = freq_node->getDoubleValue();
    nav = globals->get_navlist()->findByFreq(freq, pos);
    if ( nav == NULL ) {
        loc = globals->get_loclist()->findByFreq(freq, pos);
        gs = globals->get_gslist()->findByFreq(freq, pos);
    }
    dme = globals->get_dmelist()->findByFreq(freq, pos);

    string nav_id = "";

    has_dme = (dme != NULL);
    if (has_dme) {
// FIXME:
// dme_range is not used (yet).
// It should be used!
        dme_range = dme->get_range();
        dme_xyz = dme->cart();
    }
    if ( loc != NULL ) {
        nav_id = loc->get_ident();
	nav_id_node->setStringValue( nav_id.c_str() );
        // cout << "localizer = " << nav_id_node->getStringValue() << endl;
	is_valid = true;
	if ( last_nav_id != nav_id || last_nav_vor ) {
	    trans_ident = loc->get_trans_ident();
	    target_radial = loc->get_multiuse();
	    while ( target_radial <   0.0 ) { target_radial += 360.0; }
	    while ( target_radial > 360.0 ) { target_radial -= 360.0; }
            az_lon = loc->get_lon();
            az_lat = loc->get_lat();
            az_range = loc->get_range();
            az_eff_range = az_range;
            twist = 0;          // localizers are never twisted

            az_xyz = loc->cart();
            last_nav_id = nav_id;
            last_nav_vor = false;
            loc_runway = loc->get_runway();
            if (loc_runway) {
// landing threshold (which may or may not be displaced relative
// to the beginning of the runway):
              loc_ldg_threshold = loc_runway->threshold();
              SGVec3d ldg_xyz = SGVec3d::fromGeod(loc_ldg_threshold);
              SGVec3d end_xyz = SGVec3d::fromGeod(loc_runway->end());
              double axis_length = dist(az_xyz, ldg_xyz);
// Reference: http://dcaa.slv.dk:8000/icaodocs/
// ICAO standard width at threshold is 210 m = 689 feet = approx 700 feet.
// ICAO 3.1.1 half course = DDM = 0.0775
// ICAO 3.1.3.7.1 Sensitivity 0.00145 DDM/m at threshold
//  implies peg-to-peg of 214 m ... we will stick with 210.
// ICAO 3.1.3.7.1 "Course sector angle shall not exceed 6 degrees."
              double ldg_len = dist(ldg_xyz, end_xyz);
// Very short runway:  less than 1200 m (4000 ft) landing length:
              if (ldg_len < 1200.0) {
              #if 0
                 cerr << "Very short rwy:" 
                      << "  ldg_len:  " << ldg_len
                      << "  prior axis: " << axis_length
                      << endl;
              #endif 
// ICAO fudges localizer sensitivity for very short runways.
// This produces a non-monotonic sensitivity-versus length relation.
                axis_length += 1050.0;
              }
// Main part of the calculation:
// Example: very short: San Diego   KMYF (Montgomery Field) ILS RWY 28R
// Example: short:      Tom's River KMJX (Robert J. Miller) ILS RWY 6
// Example: very long:  Denver      KDEN (Denver)           ILS RWY 16R
              double raw_width = 210.0 / axis_length * SGD_RADIANS_TO_DEGREES;
              loc_width = raw_width < 6.0? raw_width : 6.0;
              #if 0
		cerr  << nav_id 
		      << " localizer axis: " << axis_length << " m ==> "
		      << raw_width << " deg peg-to-peg ==> " 
		      << loc_width << endl;
              #endif
            } else {
                SG_LOG( SG_COCKPIT, SG_ALERT,
                  "Can't find a runway for localizer '" + nav_id + "'");
            }

            loc_node->setBoolValue( true );
            if ( gs != NULL ) {
// Here if we found a glideslope on the frequency.
// We look for a GS only when we found a localizer
// (not a VOR) on the frequency.
// We assume without checking that the GS and LOC
// serve the same airport and same runway.
                has_gs_node->setBoolValue( true );
                gs_elev = gs->get_elev_ft();
                gs_lat = gs->get_lat();
                gs_lon = gs->get_lon();
                gs_xyz = gs->cart();
                gs_range = gs->get_range();
                int tmp = (int)(gs->get_multiuse() / 1000.0);
                target_gs = (double)tmp / 100.0;

                // GS axis unit tangent vector
                // (along the runway)
                gs_axis = tangentVector(gs_lat, gs_lon, gs_elev,
                        gs_xyz, target_radial);

                // GS baseline unit tangent vector
                // (perpendicular to the runay along the ground)
                gs_baseline = tangentVector(gs_lat, gs_lon, gs_elev,
                        gs_xyz, target_radial + 90.0);

                gs_vertical = cross(gs_baseline, gs_axis);
                #if 0
                  cout << "axis: "     << gs_axis      << " @ " << norm(gs_axis)     << endl;
                  cout << "baseline: " << gs_baseline  << " @ " << norm(gs_baseline) << endl;
                  cout << "vertical: " << gs_vertical  << " @ " << norm(gs_vertical) << endl;
                #endif
            } else {
                has_gs_node->setBoolValue( false );
                az_elev = loc->get_elev_ft();
            }

	    if ( globals->get_soundmgr()->exists( nav_fx_name ) ) {
		globals->get_soundmgr()->remove( nav_fx_name );
	    }
	    SGSoundSample *sound;
	    sound = morse.make_ident( trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, nav_fx_name );

	    if ( globals->get_soundmgr()->exists( dme_fx_name ) ) {
		globals->get_soundmgr()->remove( dme_fx_name );
	    }
	    sound = morse.make_ident( trans_ident, HI_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, dme_fx_name );

	    int offset = (int)(sg_random() * 30.0);
	    play_count = offset / 4;
	    last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << play_count
	    //      << " last_time = " << last_time
	    //      << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;

	    // cout << "Found an loc station in range" << endl;
	    // cout << " id = " << loc->get_locident() << endl;
	}
    } else if ( nav != NULL ) {
// Here if we found a VOR (with or without DME) or TACAN
        nav_id = nav->get_ident();
	nav_id_node->setStringValue( nav_id.c_str() );
        // cout << "nav = " << nav_id << endl;
	is_valid = true;
	if ( last_nav_id != nav_id || !last_nav_vor ) {
	    last_nav_id = nav_id;
	    last_nav_vor = true;
	    trans_ident = nav->get_trans_ident();
	    loc_node->setBoolValue( false );
	    has_gs_node->setBoolValue( false );
            az_lon = nav->get_lon();
            az_lat = nav->get_lat();
            az_elev = nav->get_elev_ft();
            twist = nav->get_multiuse();
            az_range = nav->get_range();
            az_eff_range = adjustNavRange(az_elev, 
                        pos.getElevationM(), az_range);
	    target_gs = 0.0;
	    target_radial = sel_radial_node->getDoubleValue();
            az_xyz = nav->cart();

	    if ( globals->get_soundmgr()->exists( nav_fx_name ) ) {
		globals->get_soundmgr()->remove( nav_fx_name );
	    }
            try {
	        SGSoundSample *sound;
	        sound = morse.make_ident( trans_ident, LO_FREQUENCY );
	        sound->set_volume( 0.3 );
	        if ( globals->get_soundmgr()->add( sound, nav_fx_name ) ) {
                    // cout << "Added nav-vor-ident sound" << endl;
                } else {
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
	        // cout << "offset = " << offset << " play_count = "
	        //      << play_count << " last_time = "
	        //      << last_time << " current time = "
	        //      << globals->get_time_params()->get_cur_time() << endl;

	        // cout << "Found a vor station in range" << endl;
	        // cout << " id = " << nav->get_ident() << endl;
            } catch ( sg_io_exception &e ) {
                SG_LOG(SG_GENERAL, SG_ALERT, e.getFormattedMessage());
            }
	}
    } else {
	is_valid = false;
	nav_id_node->setStringValue( "" );
	target_radial = 0;
	trans_ident = "";
	last_nav_id = "";
	globals->get_soundmgr()->remove( nav_fx_name );
	globals->get_soundmgr()->remove( dme_fx_name );
    }

    is_valid_node->setBoolValue( is_valid );

    char tmpid[5];
    strncpy( tmpid, nav_id.c_str(), 5 );
    id_c1_node->setIntValue( (int)tmpid[0] );
    id_c2_node->setIntValue( (int)tmpid[1] );
    id_c3_node->setIntValue( (int)tmpid[2] );
    id_c4_node->setIntValue( (int)tmpid[3] );
}
