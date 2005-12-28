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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
    power_btn(NULL),
    nav_freq(NULL),
    nav_alt_freq(NULL),
    fmt_freq(NULL),
    fmt_alt_freq(NULL),
    nav_sel_radial(NULL),
    nav_vol_btn(NULL),
    nav_ident_btn(NULL),
    audio_btn(NULL),
    nav_heading(NULL),
    nav_radial(NULL),
    reciprocal_radial(NULL),
    nav_target_radial_true(NULL),
    nav_target_auto_hdg(NULL),
    nav_to_flag(NULL),
    nav_from_flag(NULL),
    nav_inrange(NULL),
    nav_cdi_deflection(NULL),
    nav_cdi_xtrack_error(NULL),
    nav_has_gs(NULL),
    nav_loc(NULL),
    nav_loc_dist(NULL),
    nav_gs_deflection(NULL),
    nav_gs_rate_of_climb(NULL),
    nav_gs_dist(NULL),
    nav_id(NULL),
    nav_id_c1(NULL),
    nav_id_c2(NULL),
    nav_id_c3(NULL),
    nav_id_c4(NULL),
    last_nav_id(""),
    last_nav_vor(false),
    nav_play_count(0),
    nav_last_time(0),
    nav_target_radial(0.0),
    horiz_vel(0.0),
    last_x(0.0),
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

    bus_power = 
	fgGetNode(("/systems/electrical/outputs/" + name).c_str(), true);

    // inputs
    power_btn = node->getChild("power-btn", 0, true);
    power_btn->setBoolValue( true );
    nav_vol_btn = node->getChild("volume", 0, true);
    nav_ident_btn = node->getChild("ident", 0, true);
    nav_ident_btn->setBoolValue( true );
    audio_btn = node->getChild("audio-btn", 0, true);
    audio_btn->setBoolValue( true );

    // frequencies
    SGPropertyNode *subnode = node->getChild("frequencies", 0, true);
    nav_freq = subnode->getChild("selected-mhz", 0, true);
    nav_alt_freq = subnode->getChild("standby-mhz", 0, true);
    fmt_freq = subnode->getChild("selected-mhz-fmt", 0, true);
    fmt_alt_freq = subnode->getChild("standby-mhz-fmt", 0, true);

    // radials
    subnode = node->getChild("radials", 0, true);
    nav_sel_radial = subnode->getChild("selected-deg", 0, true);
    nav_radial = subnode->getChild("actual-deg", 0, true);
    reciprocal_radial = subnode->getChild("reciprocal-radial-deg", 0, true);
    nav_target_radial_true = subnode->getChild("target-radial-deg", 0, true);
    nav_target_auto_hdg = subnode->getChild("target-auto-hdg-deg", 0, true);

    // outputs
    nav_heading = node->getChild("heading-deg", 0, true);
    nav_to_flag = node->getChild("to-flag", 0, true);
    nav_from_flag = node->getChild("from-flag", 0, true);
    nav_inrange = node->getChild("in-range", 0, true);
    nav_cdi_deflection = node->getChild("heading-needle-deflection", 0, true);
    nav_cdi_xtrack_error = node->getChild("crosstrack-error-m", 0, true);
    nav_has_gs = node->getChild("has-gs", 0, true);
    nav_loc = node->getChild("nav-loc", 0, true);
    nav_loc_dist = node->getChild("nav-distance", 0, true);
    nav_gs_deflection = node->getChild("gs-needle-deflection", 0, true);
    nav_gs_rate_of_climb = node->getChild("gs-rate-of-climb", 0, true);
    nav_gs_dist = node->getChild("gs-distance", 0, true);
    nav_id = node->getChild("nav-id", 0, true);
    nav_id_c1 = node->getChild("nav-id_asc1", 0, true);
    nav_id_c2 = node->getChild("nav-id_asc2", 0, true);
    nav_id_c3 = node->getChild("nav-id_asc3", 0, true);
    nav_id_c4 = node->getChild("nav-id_asc4", 0, true);

    nav_serviceable = node->getChild("serviceable", 0, true);
    cdi_serviceable = (node->getChild("cdi", 0, true))
	->getChild("serviceable", 0, true);
    gs_serviceable = (node->getChild("gs", 0, true))
	->getChild("serviceable");
    tofrom_serviceable = (node->getChild("to-from", 0, true))
	->getChild("serviceable", 0, true);
    nav_slaved_to_gps = node->getChild("slaved-to-gps", 0, true);

    gps_cdi_deflection = fgGetNode("/instrumentation/gps/cdi-deflection", true);
    gps_to_flag = fgGetNode("/instrumentation/gps/to-flag", true);
    gps_from_flag = fgGetNode("/instrumentation/gps/from-flag", true);
    
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


// Update the various nav values based on position and valid tuned in navs
void 
FGNavRadio::update(double dt) 
{
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    // SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );

    Point3D aircraft = sgGeodToCart( Point3D( lon, lat, elev ) );
    Point3D station;
    double az1, az2, s;

    // Create "formatted" versions of the nav frequencies for
    // consistant display output.
    char tmp[16];
    sprintf( tmp, "%.2f", nav_freq->getDoubleValue() );
    fmt_freq->setStringValue(tmp);
    sprintf( tmp, "%.2f", nav_alt_freq->getDoubleValue() );
    fmt_alt_freq->setStringValue(tmp);

    // On timeout, scan again
    _time_before_search_sec -= dt;
    if ( _time_before_search_sec < 0 ) {
	search();
    }

    ////////////////////////////////////////////////////////////////////////
    // Nav.
    ////////////////////////////////////////////////////////////////////////

    // cout << "nav_valid = " << nav_valid
    //      << " power_btn = " << power_btn->getBoolValue()
    //      << " bus_power = " << bus_power->getDoubleValue()
    //      << " nav_serviceable = " << nav_serviceable->getBoolValue()
    //      << endl;

    if ( nav_valid && power_btn->getBoolValue()
         && (bus_power->getDoubleValue() > 1.0)
         && nav_serviceable->getBoolValue() )
    {
	station = Point3D( nav_x, nav_y, nav_z );
	nav_loc_dist->setDoubleValue( aircraft.distance3D( station ) );
        // cout << "station = " << station << " dist = " 
        //      << nav_loc_dist->getDoubleValue() << endl;

	if ( nav_has_gs->getBoolValue() ) {
            // find closest distance to the gs base line
            sgdVec3 p;
            sgdSetVec3( p, aircraft.x(), aircraft.y(), aircraft.z() );
            sgdVec3 p0;
            sgdSetVec3( p0, nav_gs_x, nav_gs_y, nav_gs_z );
            double dist = sgdClosestPointToLineDistSquared( p, p0,
                                                            gs_base_vec );
            nav_gs_dist->setDoubleValue( sqrt( dist ) );
            // cout << "nav_gs_dist = " << nav_gs_dist->getDoubleValue()
            //      << endl;

            Point3D tmp( nav_gs_x, nav_gs_y, nav_gs_z );
            // cout << " (" << aircraft.distance3D( tmp ) << ")" << endl;

            // wgs84 heading to glide slope (to determine sign of distance)
            geo_inverse_wgs_84( elev,
                                lat * SGD_RADIANS_TO_DEGREES,
                                lon * SGD_RADIANS_TO_DEGREES, 
                                nav_gslat, nav_gslon,
                                &az1, &az2, &s );
            double r = az1 - nav_target_radial;
            while ( r >  180.0 ) { r -= 360.0;}
            while ( r < -180.0 ) { r += 360.0;}
            if ( r >= -90.0 && r <= 90.0 ) {
                nav_gs_dist_signed = nav_gs_dist->getDoubleValue();
            } else {
                nav_gs_dist_signed = -nav_gs_dist->getDoubleValue();
            }
            /* cout << "Target Radial = " << nav_target_radial 
                 << "  Bearing = " << az1
                 << "  dist (signed) = " << nav_gs_dist_signed
                 << endl; */
            
	} else {
	    nav_gs_dist->setDoubleValue( 0.0 );
	}
	
	// wgs84 heading to localizer
        double hdg;
	geo_inverse_wgs_84( elev,
                            lat * SGD_RADIANS_TO_DEGREES,
                            lon * SGD_RADIANS_TO_DEGREES, 
			    nav_loclat, nav_loclon,
			    &hdg, &az2, &s );
	// cout << "az1 = " << az1 << " magvar = " << nav_magvar << endl;
        nav_heading->setDoubleValue( hdg );
        double radial = az2 - nav_twist;
        double recip = radial + 180.0;
        if ( recip >= 360.0 ) { recip -= 360.0; }
	nav_radial->setDoubleValue( radial );
	reciprocal_radial->setDoubleValue( recip );
	// cout << " heading = " << nav_heading
	//      << " dist = " << nav_dist << endl;

	if ( nav_loc->getBoolValue() ) {
	    double offset = radial - nav_target_radial;
	    while ( offset < -180.0 ) { offset += 360.0; }
	    while ( offset > 180.0 ) { offset -= 360.0; }
	    // cout << "ils offset = " << offset << endl;
	    nav_effective_range
                = adjustILSRange( nav_elev, elev, offset,
                                  nav_loc_dist->getDoubleValue()
                                  * SG_METER_TO_NM );
	} else {
	    nav_effective_range = adjustNavRange( nav_elev, elev, nav_range );
	}
	// cout << "nav range = " << nav_effective_range
	//      << " (" << nav_range << ")" << endl;

	if ( nav_loc_dist->getDoubleValue()
             < nav_effective_range * SG_NM_TO_METER )
        {
	    nav_inrange->setBoolValue( true );
	} else if ( nav_loc_dist->getDoubleValue()
                    < 2 * nav_effective_range * SG_NM_TO_METER )
        {
	    nav_inrange->setBoolValue( sg_random() < 
		( 2 * nav_effective_range * SG_NM_TO_METER
                  - nav_loc_dist->getDoubleValue() ) /
		(nav_effective_range * SG_NM_TO_METER) );
	} else {
	    nav_inrange->setBoolValue( false );
	}

	if ( !nav_loc->getBoolValue() ) {
	    nav_target_radial = nav_sel_radial->getDoubleValue();
	}

        // Calculate some values for the nav/ils hold autopilot

        double cur_radial = recip;
        if ( nav_loc->getBoolValue() ) {
            // ILS localizers radials are already "true" in our
            // database
        } else {
            cur_radial += nav_twist;
        }
        if ( nav_from_flag->getBoolValue() ) {
            cur_radial += 180.0;
            while ( cur_radial >= 360.0 ) { cur_radial -= 360.0; }
        }
        
        // AUTOPILOT/FLIGHT-DIRECTOR HELPERS

        // determine the target radial in "true" heading
        double trtrue = 0.0;
        if ( nav_loc->getBoolValue() ) {
            // ILS localizers radials are already "true" in our
            // database
            trtrue = nav_target_radial;
        } else {
            // VOR radials need to have that vor's offset added in
            trtrue = nav_target_radial + nav_twist;
        }

        while ( trtrue < 0.0 ) { trtrue += 360.0; }
        while ( trtrue > 360.0 ) { trtrue -= 360.0; }
        nav_target_radial_true->setDoubleValue( trtrue );

        // determine the heading adjustment needed.
        // over 8km scale by 3.0 
        //    (3 is chosen because max deflection is 10
        //    and 30 is clamped angle to radial)
        // under 8km scale by 10.0
        //    because the overstated error helps drive it to the radial in a 
        //    moderate cross wind.
        double adjustment = 0.0;
        if (nav_loc_dist->getDoubleValue() > 8000) {
            adjustment = nav_cdi_deflection->getDoubleValue() * 3.0;
        } else {
            adjustment = nav_cdi_deflection->getDoubleValue() * 10.0;
        }
        SG_CLAMP_RANGE( adjustment, -30.0, 30.0 );
        
        // determine the target heading to fly to intercept the
        // tgt_radial
        double nta_hdg = trtrue + adjustment; 
        while ( nta_hdg <   0.0 ) { nta_hdg += 360.0; }
        while ( nta_hdg > 360.0 ) { nta_hdg -= 360.0; }
        nav_target_auto_hdg->setDoubleValue( nta_hdg );

        // cross track error
        // ????

        // Calculate desired rate of climb for intercepting the GS
        double x = nav_gs_dist->getDoubleValue();
        double y = (alt_node->getDoubleValue() - nav_elev)
            * SG_FEET_TO_METER;
        double current_angle = atan2( y, x ) * SGD_RADIANS_TO_DEGREES;

        double target_angle = nav_target_gs;
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

            nav_gs_rate_of_climb
                ->setDoubleValue( -sin( des_angle * SGD_DEGREES_TO_RADIANS )
                                  * horiz_vel * SG_METER_TO_FEET );
        }
    } else {
	nav_inrange->setBoolValue( false );
	// cout << "not picking up vor. :-(" << endl;
    }

    // compute to/from flag status
    double value = false;
    double offset = fabs(nav_radial->getDoubleValue() - nav_target_radial);
    if ( nav_slaved_to_gps->getBoolValue() ) {
        value = gps_to_flag->getBoolValue();
    } else if ( nav_inrange->getBoolValue()
                && nav_serviceable->getBoolValue()
                && tofrom_serviceable->getBoolValue() )
    {
        if ( nav_loc->getBoolValue() ) {
            value = true;
        } else {
            value = !(offset <= 90.0 || offset >= 270.0);
        }
    }
    nav_to_flag->setBoolValue( value );

    value = false;
    if ( nav_slaved_to_gps->getBoolValue() ) {
        value = gps_from_flag->getBoolValue();
    } else if ( nav_inrange->getBoolValue()
                && nav_serviceable->getBoolValue()
                && tofrom_serviceable->getBoolValue() )
    {
        if ( nav_loc->getBoolValue() ) {
            value = false;
        } else {
            value = !(offset > 90.0 && offset < 270.0);
        }
    }
    nav_from_flag->setBoolValue( value );

    // compute the deflection of the CDI needle, clamped to the range
    // of ( -10 , 10 )
    double r;
    if ( nav_slaved_to_gps->getBoolValue() ) {
	r = gps_cdi_deflection->getDoubleValue();
	// We want +- 5 dots deflection for the gps, so clamp to -12.5/12.5
	if ( r < -12.5 ) { r = -12.5; }
	if ( r >  12.5 ) { r =  12.5; }
    } else if ( nav_inrange->getBoolValue() && nav_serviceable->getBoolValue() 
                && cdi_serviceable->getBoolValue()
                && !nav_slaved_to_gps->getBoolValue() )
    {
        r = nav_radial->getDoubleValue() - nav_target_radial;
	// cout << "Target radial = " << nav_target_radial 
	//      << "  Actual radial = " << nav_radial->getDoubleValue() << endl;
    
	while ( r >  180.0 ) { r -= 360.0;}
	while ( r < -180.0 ) { r += 360.0;}
	if ( fabs(r) > 90.0 ) {
	    r = ( r<0.0 ? -r-180.0 : -r+180.0 );
        }

	// According to Robin Peel, the ILS is 4x more sensitive than a vor
        r = -r;                 // reverse, since radial is outbound
	if ( nav_loc->getBoolValue() ) { r *= 4.0; }
	if ( r < -10.0 ) { r = -10.0; }
	if ( r >  10.0 ) { r =  10.0; }
    } else {
	r = 0.0;
    }
    nav_cdi_deflection->setDoubleValue( r );

    // compute the amount of cross track distance error in meters
    double m;
    if ( nav_inrange->getBoolValue()
         && nav_serviceable->getBoolValue() && cdi_serviceable->getBoolValue() )
    {
        r = nav_radial->getDoubleValue() - nav_target_radial;
	// cout << "Target radial = " << nav_target_radial 
	//     << "  Actual radial = " << nav_radial->getDoubleValue()
        //     << "  r = " << r << endl;
    
	while ( r >  180.0 ) { r -= 360.0;}
	while ( r < -180.0 ) { r += 360.0;}
	if ( fabs(r) > 90.0 ) {
	    r = ( r<0.0 ? -r-180.0 : -r+180.0 );
        }

        r = -r;                 // reverse, since radial is outbound

        m = nav_loc_dist->getDoubleValue() * sin(r * SGD_DEGREES_TO_RADIANS);

    } else {
	m = 0.0;
    }
    nav_cdi_xtrack_error->setDoubleValue( m );

    // compute the amount of glide slope needle deflection (.i.e. the
    // number of degrees we are off the glide slope * 5.0
    if ( nav_inrange->getBoolValue() && nav_has_gs->getBoolValue()
         && nav_serviceable->getBoolValue()
         && gs_serviceable->getBoolValue()
         && !nav_slaved_to_gps->getBoolValue() )
    {
	double x = nav_gs_dist->getDoubleValue();
	double y = (fgGetDouble("/position/altitude-ft") - nav_elev)
            * SG_FEET_TO_METER;
        // cout << "dist = " << x << " height = " << y << endl;
	double angle = asin( y / x ) * SGD_RADIANS_TO_DEGREES;
	r = (nav_target_gs - angle) * 5.0;
    } else {
	r = 0.0;
    }
    nav_gs_deflection->setDoubleValue( r );

    // audio effects
    if ( nav_valid
         && nav_inrange->getBoolValue()
         && nav_serviceable->getBoolValue() )
    {
	// play station ident via audio system if on + ident,
	// otherwise turn it off
	if ( power_btn->getBoolValue() && (bus_power->getDoubleValue() > 1.0)
             && nav_ident_btn->getBoolValue() && audio_btn->getBoolValue() )
        {
	    SGSoundSample *sound;
	    sound = globals->get_soundmgr()->find( nav_fx_name );
            double vol = nav_vol_btn->getDoubleValue();
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
            // cout << "nav_last_time = " << nav_last_time << " ";
            // cout << "cur_time = "
            //      << globals->get_time_params()->get_cur_time();
	    if ( nav_last_time <
		 globals->get_time_params()->get_cur_time() - 30 ) {
		nav_last_time = globals->get_time_params()->get_cur_time();
		nav_play_count = 0;
	    }
            // cout << " nav_play_count = " << nav_play_count << endl;
            // cout << "playing = "
            //      << globals->get_soundmgr()->is_playing(nav_fx_name)
            //      << endl;
	    if ( nav_play_count < 4 ) {
		// play VOR ident
		if ( !globals->get_soundmgr()->is_playing(nav_fx_name) ) {
		    globals->get_soundmgr()->play_once( nav_fx_name );
		    ++nav_play_count;
                }
	    } else if ( nav_play_count < 5 && nav_has_dme ) {
		// play DME ident
		if ( !globals->get_soundmgr()->is_playing(nav_fx_name) &&
		     !globals->get_soundmgr()->is_playing(dme_fx_name) ) {
		    globals->get_soundmgr()->play_once( dme_fx_name );
		    ++nav_play_count;
		}
	    }
	} else {
	    globals->get_soundmgr()->stop( nav_fx_name );
	    globals->get_soundmgr()->stop( dme_fx_name );
	}
    }
}


// Update current nav/adf radio stations based on current postition
void FGNavRadio::search() 
{

    // reset search time
    _time_before_search_sec = 1.0;

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

    double freq = nav_freq->getDoubleValue();
    nav = globals->get_navlist()->findByFreq(freq, lon, lat, elev);
    dme = globals->get_dmelist()->findByFreq(freq, lon, lat, elev);
    if ( nav == NULL ) {
        loc = globals->get_loclist()->findByFreq(freq, lon, lat, elev);
        gs = globals->get_gslist()->findByFreq(freq, lon, lat, elev);
    }
        
    if ( loc != NULL ) {
	nav_id->setStringValue( loc->get_ident() );
        // cout << "localizer = " << nav_id->getStringValue() << endl;
	nav_valid = true;
	if ( last_nav_id != nav_id->getStringValue() || last_nav_vor ) {
	    nav_trans_ident = loc->get_trans_ident();
	    nav_target_radial = loc->get_multiuse();
	    while ( nav_target_radial <   0.0 ) { nav_target_radial += 360.0; }
	    while ( nav_target_radial > 360.0 ) { nav_target_radial -= 360.0; }
	    nav_loclon = loc->get_lon();
	    nav_loclat = loc->get_lat();
	    nav_x = loc->get_x();
	    nav_y = loc->get_y();
	    nav_z = loc->get_z();
	    last_nav_id = nav_id->getStringValue();
	    last_nav_vor = false;
	    nav_loc->setBoolValue( true );
	    nav_has_dme = (dme != NULL);
            nav_has_gs->setBoolValue( gs != NULL );
            if ( nav_has_gs->getBoolValue() ) {
                nav_gslon = gs->get_lon();
                nav_gslat = gs->get_lat();
                nav_elev = gs->get_elev_ft();
                int tmp = (int)(gs->get_multiuse() / 1000.0);
                nav_target_gs = (double)tmp / 100.0;
                nav_gs_x = gs->get_x();
                nav_gs_y = gs->get_y();
                nav_gs_z = gs->get_z();

                // derive GS baseline (perpendicular to the runay
                // along the ground)
                double tlon, tlat, taz;
                geo_direct_wgs_84 ( 0.0, nav_gslat, nav_gslon,
                                    nav_target_radial + 90,  
                                    100.0, &tlat, &tlon, &taz );
                // cout << "nav_target_radial = " << nav_target_radial << endl;
                // cout << "nav_loc = " << nav_loc->getBoolValue() << endl;
                // cout << nav_gslon << "," << nav_gslat << "  "
                //      << tlon << "," << tlat << "  (" << nav_elev << ")"
                //      << endl;
                Point3D p1 = sgGeodToCart( Point3D(tlon*SGD_DEGREES_TO_RADIANS,
                                                   tlat*SGD_DEGREES_TO_RADIANS,
                                                   nav_elev*SG_FEET_TO_METER)
                                           );
                // cout << nav_gs_x << "," << nav_gs_y << "," << nav_gs_z
                //      << endl;
                // cout << p1 << endl;
                sgdSetVec3( gs_base_vec,
                            p1.x()-nav_gs_x, p1.y()-nav_gs_y, p1.z()-nav_gs_z );
                // cout << gs_base_vec[0] << "," << gs_base_vec[1] << ","
                //      << gs_base_vec[2] << endl;
            } else {
                nav_elev = loc->get_elev_ft();
            }
	    nav_twist = 0;
	    nav_range = FG_LOC_DEFAULT_RANGE;
	    nav_effective_range = nav_range;

	    if ( globals->get_soundmgr()->exists( nav_fx_name ) ) {
		globals->get_soundmgr()->remove( nav_fx_name );
	    }
	    SGSoundSample *sound;
	    sound = morse.make_ident( nav_trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, nav_fx_name );

	    if ( globals->get_soundmgr()->exists( dme_fx_name ) ) {
		globals->get_soundmgr()->remove( dme_fx_name );
	    }
	    sound = morse.make_ident( nav_trans_ident, HI_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, dme_fx_name );

	    int offset = (int)(sg_random() * 30.0);
	    nav_play_count = offset / 4;
	    nav_last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << nav_play_count
	    //      << " nav_last_time = " << nav_last_time
	    //      << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;

	    // cout << "Found an loc station in range" << endl;
	    // cout << " id = " << loc->get_locident() << endl;
	}
    } else if ( nav != NULL ) {
	nav_id->setStringValue( nav->get_ident() );
        // cout << "nav = " << nav_id->getStringValue() << endl;
	nav_valid = true;
	if ( last_nav_id != nav_id->getStringValue() || !last_nav_vor ) {
	    last_nav_id = nav_id->getStringValue();
	    last_nav_vor = true;
	    nav_trans_ident = nav->get_trans_ident();
	    nav_loc->setBoolValue( false );
	    nav_has_dme = (dme != NULL);
	    nav_has_gs->setBoolValue( false );
	    nav_loclon = nav->get_lon();
	    nav_loclat = nav->get_lat();
	    nav_elev = nav->get_elev_ft();
	    nav_twist = nav->get_multiuse();
	    nav_range = nav->get_range();
	    nav_effective_range = adjustNavRange(nav_elev, elev, nav_range);
	    nav_target_gs = 0.0;
	    nav_target_radial = nav_sel_radial->getDoubleValue();
	    nav_x = nav->get_x();
	    nav_y = nav->get_y();
	    nav_z = nav->get_z();

	    if ( globals->get_soundmgr()->exists( nav_fx_name ) ) {
		globals->get_soundmgr()->remove( nav_fx_name );
	    }
	    SGSoundSample *sound;
	    sound = morse.make_ident( nav_trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    if ( globals->get_soundmgr()->add( sound, nav_fx_name ) ) {
                // cout << "Added nav-vor-ident sound" << endl;
            } else {
                SG_LOG(SG_COCKPIT, SG_WARN, "Failed to add v1-vor-ident sound");
            }

	    if ( globals->get_soundmgr()->exists( dme_fx_name ) ) {
		globals->get_soundmgr()->remove( dme_fx_name );
	    }
	    sound = morse.make_ident( nav_trans_ident, HI_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, dme_fx_name );

	    int offset = (int)(sg_random() * 30.0);
	    nav_play_count = offset / 4;
	    nav_last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << nav_play_count << " nav_last_time = "
	    //      << nav_last_time << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;

	    // cout << "Found a vor station in range" << endl;
	    // cout << " id = " << nav->get_ident() << endl;
	}
    } else {
	nav_valid = false;
	nav_id->setStringValue( "" );
	nav_target_radial = 0;
	nav_trans_ident = "";
	last_nav_id = "";
	if ( ! globals->get_soundmgr()->remove( nav_fx_name ) ) {
            SG_LOG(SG_COCKPIT, SG_WARN, "Failed to remove nav-vor-ident sound");
        }
	globals->get_soundmgr()->remove( dme_fx_name );
	// cout << "not picking up vor1. :-(" << endl;
    }

    char tmpid[5];
    strncpy( tmpid, nav_id->getStringValue(), 5 );
    nav_id_c1->setIntValue( (int)tmpid[0] );
    nav_id_c2->setIntValue( (int)tmpid[1] );
    nav_id_c3->setIntValue( (int)tmpid[2] );
    nav_id_c4->setIntValue( (int)tmpid[3] );
}
