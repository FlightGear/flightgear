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

#include <iostream>
#include <string>
#include <sstream>

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/sg_random.h>
#include <simgear/math/vector.hxx>

#include <Aircraft/aircraft.hxx>
#include <Navaids/navlist.hxx>

#include "navradio.hxx"

#include <string>
SG_USING_STD(string);


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
    cdi_deflection_node(NULL),
    cdi_xtrack_error_node(NULL),
    cdi_xtrack_hdg_err_node(NULL),
    has_gs_node(NULL),
    loc_node(NULL),
    loc_dist_node(NULL),
    gs_deflection_node(NULL),
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
    name("nav"),
    num(0),
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

    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_WARN, 
                    "Error in nav radio config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }

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
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );

    bus_power_node = 
	fgGetNode(("/systems/electrical/outputs/" + name).c_str(), true);

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
    cdi_deflection_node = node->getChild("heading-needle-deflection", 0, true);
    cdi_xtrack_error_node = node->getChild("crosstrack-error-m", 0, true);
    cdi_xtrack_hdg_err_node
        = node->getChild("crosstrack-heading-error-deg", 0, true);
    has_gs_node = node->getChild("has-gs", 0, true);
    loc_node = node->getChild("nav-loc", 0, true);
    loc_dist_node = node->getChild("nav-distance", 0, true);
    gs_deflection_node = node->getChild("gs-needle-deflection", 0, true);
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
    
    std::ostringstream temp;
    temp << name << "nav-ident" << num;
    nav_fx_name = temp.str();
    temp << name << "dme-ident" << num;
    dme_fx_name = temp.str();
}

void
FGNavRadio::bind ()
{
    std::ostringstream temp;
    string branch;
    temp << num;
    branch = "/instrumentation/" + name + "[" + temp.str() + "]";
}


void
FGNavRadio::unbind ()
{
    std::ostringstream temp;
    string branch;
    temp << num;
    branch = "/instrumentation/" + name + "[" + temp.str() + "]";
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
    bool cdi_serviceable = cdi_serviceable_node->getBoolValue();
    bool tofrom_serviceable = tofrom_serviceable_node->getBoolValue();
    bool inrange = inrange_node->getBoolValue();
    bool has_gs = has_gs_node->getBoolValue();
    bool is_loc = loc_node->getBoolValue();
    double loc_dist = loc_dist_node->getDoubleValue();

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
        loc_dist = dist(aircraft, nav_xyz);
	loc_dist_node->setDoubleValue( loc_dist );
        // cout << "dt = " << dt << " dist = " << loc_dist << endl;

	if ( has_gs ) {
            // find closest distance to the gs base line
            SGVec3d p = aircraft;
            double dist = sgdClosestPointToLineDistSquared(p.sg(), gs_xyz.sg(),
                                                           gs_base_vec.sg());
            gs_dist_node->setDoubleValue( sqrt( dist ) );
            // cout << "gs_dist = " << gs_dist_node->getDoubleValue()
            //      << endl;

            // wgs84 heading to glide slope (to determine sign of distance)
            geo_inverse_wgs_84( pos, SGGeod::fromDeg(gs_lon, gs_lat),
                                &az1, &az2, &s );
            double r = az1 - target_radial;
            while ( r >  180.0 ) { r -= 360.0;}
            while ( r < -180.0 ) { r += 360.0;}
            if ( r >= -90.0 && r <= 90.0 ) {
                gs_dist_signed = gs_dist_node->getDoubleValue();
            } else {
                gs_dist_signed = -gs_dist_node->getDoubleValue();
            }
            /* cout << "Target Radial = " << target_radial 
                 << "  Bearing = " << az1
                 << "  dist (signed) = " << gs_dist_signed
                 << endl; */
            
	} else {
	    gs_dist_node->setDoubleValue( 0.0 );
	}
	
        //////////////////////////////////////////////////////////
	// compute forward and reverse wgs84 headings to localizer
        //////////////////////////////////////////////////////////
        double hdg;
	geo_inverse_wgs_84( pos, SGGeod::fromDeg(loc_lon, loc_lat),
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
	    effective_range
                = adjustILSRange( nav_elev, pos.getElevationM(), offset,
                                  loc_dist * SG_METER_TO_NM );
	} else {
	    effective_range
                = adjustNavRange( nav_elev, pos.getElevationM(), range );
	}
	// cout << "nav range = " << effective_range
	//      << " (" << range << ")" << endl;

	if ( loc_dist < effective_range * SG_NM_TO_METER ) {
	    inrange = true;
	} else if ( loc_dist < 2 * effective_range * SG_NM_TO_METER ) {
	    inrange = sg_random() < ( 2 * effective_range * SG_NM_TO_METER
                                      - loc_dist )
                                      / (effective_range * SG_NM_TO_METER);
	} else {
	    inrange = false;
	}
        inrange_node->setBoolValue( inrange );

	if ( !is_loc ) {
	    target_radial = sel_radial_node->getDoubleValue();
	}

        //////////////////////////////////////////////////////////
        // compute to/from flag status
        //////////////////////////////////////////////////////////
        double value = false;
        double offset = fabs(radial - target_radial);
        if ( tofrom_serviceable ) {
            if ( nav_slaved_to_gps_node->getBoolValue() ) {
                value = gps_to_flag_node->getBoolValue();
            } else if ( inrange ) {
                if ( is_loc ) {
                    value = true;
                } else {
                    value = !(offset <= 90.0 || offset >= 270.0);
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
                    value = !(offset > 90.0 && offset < 270.0);
                }
            }
        } else {
            value = false;
        }
        from_flag_node->setBoolValue( value );

        //////////////////////////////////////////////////////////
        // compute the deflection of the CDI needle, clamped to the range
        // of ( -10 , 10 )
        //////////////////////////////////////////////////////////
        double r = 0.0;
        bool loc_backside = false; // an in-code flag indicating that we are
                                   // on a localizer backcourse.
        if ( cdi_serviceable ) {
            if ( nav_slaved_to_gps_node->getBoolValue() ) {
                r = gps_cdi_deflection_node->getDoubleValue();
                // We want +- 5 dots deflection for the gps, so clamp
                // to -12.5/12.5
                SG_CLAMP_RANGE( r, -12.5, 12.5 );
            } else if ( inrange ) {
                r = radial - target_radial;
                // cout << "Target radial = " << target_radial 
                //      << "  Actual radial = " << radial << endl;
                
                while ( r >  180.0 ) { r -= 360.0;}
                while ( r < -180.0 ) { r += 360.0;}
                if ( fabs(r) > 90.0 ) {
                    r = ( r<0.0 ? -r-180.0 : -r+180.0 );
                } else {
                    if ( is_loc ) {
                        loc_backside = true;
                    }
                }

                r = -r;         // reverse, since radial is outbound
                if ( is_loc ) {
                    // According to Robin Peel, the ILS is 4x more
                    // sensitive than a vor
                    r *= 4.0;
                }
                SG_CLAMP_RANGE( r, -10.0, 10.0 );
            }
        }
        cdi_deflection_node->setDoubleValue( r );

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
        // compute the time to intercept selected radial (based on
        // current and last cross track errors and dt
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
        r = 0.0;
        if ( has_gs && gs_serviceable_node->getBoolValue() ) {
            if ( nav_slaved_to_gps_node->getBoolValue() ) {
                // FIXME/FINISHME, what should be set here?
            } else if ( inrange ) {
                double x = gs_dist_node->getDoubleValue();
                double y = (fgGetDouble("/position/altitude-ft") - nav_elev)
                    * SG_FEET_TO_METER;
                // cout << "dist = " << x << " height = " << y << endl;
                double angle = asin( y / x ) * SGD_RADIANS_TO_DEGREES;
                r = (target_gs - angle) * 5.0;
            }
        }
        gs_deflection_node->setDoubleValue( r );

        //////////////////////////////////////////////////////////
        // Calculate desired rate of climb for intercepting the GS
        //////////////////////////////////////////////////////////
        double x = gs_dist_node->getDoubleValue();
        double y = (alt_node->getDoubleValue() - nav_elev)
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
        double adjustment = cdi_deflection_node->getDoubleValue() * 3.0;
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
	inrange_node->setBoolValue( false );
        cdi_deflection_node->setDoubleValue( 0.0 );
        cdi_xtrack_error_node->setDoubleValue( 0.0 );
        cdi_xtrack_hdg_err_node->setDoubleValue( 0.0 );
        time_to_intercept->setDoubleValue( 0.0 );
        gs_deflection_node->setDoubleValue( 0.0 );
        to_flag_node->setBoolValue( false );
        from_flag_node->setBoolValue( false );
	// cout << "not picking up vor. :-(" << endl;
    }

    // audio effects
    if ( is_valid && inrange && nav_serviceable ) {
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
	    if ( last_time <
		 globals->get_time_params()->get_cur_time() - 30 ) {
		last_time = globals->get_time_params()->get_cur_time();
		play_count = 0;
	    }
            // cout << " play_count = " << play_count << endl;
            // cout << "playing = "
            //      << globals->get_soundmgr()->is_playing(nav_fx_name)
            //      << endl;
	    if ( play_count < 4 ) {
		// play VOR ident
		if ( !globals->get_soundmgr()->is_playing(nav_fx_name) ) {
		    globals->get_soundmgr()->play_once( nav_fx_name );
		    ++play_count;
                }
	    } else if ( play_count < 5 && has_dme ) {
		// play DME ident
		if ( !globals->get_soundmgr()->is_playing(nav_fx_name) &&
		     !globals->get_soundmgr()->is_playing(dme_fx_name) ) {
		    globals->get_soundmgr()->play_once( dme_fx_name );
		    ++play_count;
		}
	    }
	} else {
	    globals->get_soundmgr()->stop( nav_fx_name );
	    globals->get_soundmgr()->stop( dme_fx_name );
	}
    }

    last_loc_dist = loc_dist;
}


// Update current nav/adf radio stations based on current postition
void FGNavRadio::search() 
{

    // reset search time
    _time_before_search_sec = 1.0;

    // cache values locally for speed
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    FGNavRecord *nav = NULL;
    FGNavRecord *loc = NULL;
    FGNavRecord *dme = NULL;
    FGNavRecord *gs = NULL;

    ////////////////////////////////////////////////////////////////////////
    // Nav.
    ////////////////////////////////////////////////////////////////////////

    double freq = freq_node->getDoubleValue();
    nav = globals->get_navlist()->findByFreq(freq, lon, lat, elev);
    dme = globals->get_dmelist()->findByFreq(freq, lon, lat, elev);
    if ( nav == NULL ) {
        loc = globals->get_loclist()->findByFreq(freq, lon, lat, elev);
        gs = globals->get_gslist()->findByFreq(freq, lon, lat, elev);
    }

    string nav_id = "";

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
	    loc_lon = loc->get_lon();
	    loc_lat = loc->get_lat();
	    nav_xyz = loc->get_cart();
	    last_nav_id = nav_id;
	    last_nav_vor = false;
	    loc_node->setBoolValue( true );
	    has_dme = (dme != NULL);
            if ( gs != NULL ) {
                has_gs_node->setBoolValue( true );
                gs_lon = gs->get_lon();
                gs_lat = gs->get_lat();
                nav_elev = gs->get_elev_ft();
                int tmp = (int)(gs->get_multiuse() / 1000.0);
                target_gs = (double)tmp / 100.0;
                gs_xyz = gs->get_cart();

                // derive GS baseline (perpendicular to the runay
                // along the ground)
                double tlon, tlat, taz;
                geo_direct_wgs_84 ( 0.0, gs_lat, gs_lon,
                                    target_radial + 90,
                                    100.0, &tlat, &tlon, &taz );
                // cout << "target_radial = " << target_radial << endl;
                // cout << "nav_loc = " << loc_node->getBoolValue() << endl;
                // cout << gs_lon << "," << gs_lat << "  "
                //      << tlon << "," << tlat << "  (" << nav_elev << ")"
                //      << endl;
                SGGeod tpos = SGGeod::fromDegFt(tlon, tlat, nav_elev);
                SGVec3d p1 = SGVec3d::fromGeod(tpos);

                // cout << gs_xyz << endl;
                // cout << p1 << endl;
                gs_base_vec = p1 - gs_xyz;
                // cout << gs_base_vec << endl;
            } else {
                has_gs_node->setBoolValue( false );
                nav_elev = loc->get_elev_ft();
            }
	    twist = 0;
	    range = FG_LOC_DEFAULT_RANGE;
	    effective_range = range;

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
        nav_id = nav->get_ident();
	nav_id_node->setStringValue( nav_id.c_str() );
        // cout << "nav = " << nav_id << endl;
	is_valid = true;
	if ( last_nav_id != nav_id || !last_nav_vor ) {
	    last_nav_id = nav_id;
	    last_nav_vor = true;
	    trans_ident = nav->get_trans_ident();
	    loc_node->setBoolValue( false );
	    has_dme = (dme != NULL);
	    has_gs_node->setBoolValue( false );
	    loc_lon = nav->get_lon();
	    loc_lat = nav->get_lat();
	    nav_elev = nav->get_elev_ft();
	    twist = nav->get_multiuse();
	    range = nav->get_range();
	    effective_range = adjustNavRange(nav_elev, elev, range);
	    target_gs = 0.0;
	    target_radial = sel_radial_node->getDoubleValue();
	    nav_xyz = nav->get_cart();

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
	if ( ! globals->get_soundmgr()->remove( nav_fx_name ) ) {
            SG_LOG(SG_COCKPIT, SG_WARN, "Failed to remove nav-vor-ident sound");
        }
	globals->get_soundmgr()->remove( dme_fx_name );
	// cout << "not picking up vor1. :-(" << endl;
    }

    is_valid_node->setBoolValue( is_valid );

    char tmpid[5];
    strncpy( tmpid, nav_id.c_str(), 5 );
    id_c1_node->setIntValue( (int)tmpid[0] );
    id_c2_node->setIntValue( (int)tmpid[1] );
    id_c3_node->setIntValue( (int)tmpid[2] );
    id_c4_node->setIntValue( (int)tmpid[3] );
}
