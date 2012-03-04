// FGAIThermal - FGAIBase-derived class creates an AI thermal
//
// Copyright (C) 2004  David P. Culp - davidculp2@comcast.net
//
// An attempt to refine the thermal shape and behaviour by WooT 2009
//
// Copyright (C) 2009 Patrice Poly ( WooT )
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <string>
#include <math.h>

using std::string;

#include "AIThermal.hxx"


FGAIThermal::FGAIThermal() :
   FGAIBase(otThermal, false)
{
   max_strength = 6.0;
   diameter = 0.5;
   strength = factor = 0.0;
   cycle_timer = 60*(rand()%31); // some random in the birth time
   ground_elev_ft = 0.0;
   dt_count=0.9;
   alt=0.0;
}

FGAIThermal::~FGAIThermal() {
}

void FGAIThermal::readFromScenario(SGPropertyNode* scFileNode) {
  if (!scFileNode)
    return;

  FGAIBase::readFromScenario(scFileNode);

  setMaxStrength(scFileNode->getDoubleValue("strength-fps", 8.0)); 
  setDiameter(scFileNode->getDoubleValue("diameter-ft", 0.0)/6076.11549); 
  setHeight(scFileNode->getDoubleValue("height-msl", 5000.0));  
}

bool FGAIThermal::init(bool search_in_AI_path) {
   factor = 8.0 * max_strength / (diameter * diameter * diameter);
   setAltitude( height );
   _surface_wind_from_deg_node =
   	    fgGetNode("/environment/config/boundary/entry[0]/wind-from-heading-deg", true);
   _surface_wind_speed_node =
            fgGetNode("/environment/config/boundary/entry[0]/wind-speed-kt", true);
   _aloft_wind_from_deg_node =
   	    fgGetNode("/environment/config/aloft/entry[2]/wind-from-heading-deg", true);
   _aloft_wind_speed_node =
            fgGetNode("/environment/config/aloft/entry[2]/wind-speed-kt", true);
    do_agl_calc = 1;
   return FGAIBase::init(search_in_AI_path);
}

void FGAIThermal::bind() {
    FGAIBase::bind();
    tie("position/altitude-agl-ft", // for debug and tweak
                SGRawValuePointer<double>(&altitude_agl_ft));
    tie("alt-rel", // for debug and tweak
                SGRawValuePointer<double>(&alt_rel));
    tie("time", // for debug and tweak
                SGRawValuePointer<double>(&time));
    tie("xx", // for debug and tweak
                SGRawValuePointer<double>(&xx));
    tie("is-forming", // for debug abd tweak
                SGRawValuePointer<bool>(&is_forming));
    tie("is-formed", // for debug abd tweak
                SGRawValuePointer<bool>(&is_formed));
    tie("is-dying", // for debug abd tweak
                SGRawValuePointer<bool>(&is_dying));
    tie("is-dead", // for debug abd tweak
                SGRawValuePointer<bool>(&is_dead));
}

void FGAIThermal::update(double dt) {
   FGAIBase::update(dt);
   Run(dt);
   Transform();
}



//the formula to get the available portion of VUpMax depending on altitude
//returns a double between 0 and 1
double FGAIThermal::get_strength_fac(double alt_frac) {

double PI = 4.0 * atan(1.0);
double fac = 0.0;
if ( alt_frac <=0.0 ) { // do submarines get thermals ?
	fac = 0.0;
	}
else if ( ( alt_frac>0.0 ) && (alt_frac<=0.1) ) { // ground layer
	fac = ( 0.1*( pow( (10.0*alt_frac),10.0) ) );
	}
else if ( ( alt_frac>0.1 ) && (alt_frac<=1.0) ) {   // main body of the thermal
	fac = 0.4175 - 0.5825* ( cos ( PI*  (1.0-sqrt(alt_frac) ) +PI) ) ;
	}
else if ( ( alt_frac >1.0 ) && (alt_frac < 1.1 ) ) {  //above the ceiling, but not above the cloud
	fac = (0.5 * ( 1.0 + cos ( PI*( (-2.0*alt_frac)*5.0 ) ) ) );
	}
else if ( alt_frac >= 1.1 ) {  //above the cloud
	fac = 0.0;
	}
return fac;
}


void FGAIThermal::Run(double dt) {

// *****************************************
// the thermal characteristics and variables
// *****************************************

cycle_timer += dt ;

// time

// the time needed for the thermal to be completely formed
double tmin1 = 5.0 ;
// the time when the thermal begins to die
double tmin2 = 20.0 ;
// the time when the thermal is completely dead
double tmin3 = 25.0;
double alive_cycle_time = tmin3*60.0;
//the time of the complete cycle, including a period of inactivity
double tmin4 = 30.0;
// some times expressed in a fraction of tmin3;
double t1 = tmin1/tmin3 ;
double t2 = tmin2/tmin3 ;
double t3 = 1.0 ;
double t4 = tmin4/tmin3;
// the time elapsed since the thermal was born, in a 0-1 fraction of tmin3

time = cycle_timer/alive_cycle_time;
//comment above and
//uncomment below to freeze the time cycle
 time=0.5;

if ( time >= t4) { 
	cycle_timer = 60*(rand()%31);
	}


//the position of the thermal 'top'
double thermal_foot_lat = (pos.getLatitudeDeg());
double thermal_foot_lon = (pos.getLongitudeDeg());

//the max updraft one can expect in this thermal
double MaxUpdraft=max_strength;
//the max sink one can expect in this thermal, this is a negative number
double MinUpdraft=-max_strength*0.25;
//the fraction of MaxUpdraft one can expect at our height and time
double maxstrengthavail;
//max updraft at the user altitude and time
double v_up_max;
//min updraft at the user altitude and time, this is a negative number
double v_up_min;
double wind_speed;


//max radius of the the thermal, including the sink area
double Rmax = diameter/2.0;
// 'shaping' of the thermal, the higher, the more conical the thermal- between 0 and 1
double shaping=0.8;
//the radius of the thermal at our altitude in FT, including sink
double Rsink;
//the relative radius of the thermal where we have updraft, between 0 an 1
double r_up_frac=0.9;
//radius of the thermal where we have updraft, in FT
double Rup;
//how far are we from the thermal center at our altitude in FEET
double dist_center;

//the position of the center of the thermal slice at our altitude
double slice_center_lon;
double slice_center_lat;



// **************************************
// various variables relative to the user
// **************************************

double user_latitude  = manager->get_user_latitude();
double user_longitude = manager->get_user_longitude();
double user_altitude  = manager->get_user_altitude(); // MSL

//we need to know the thermal foot AGL altitude


//we could do this only once, as thermal don't move
//but then agl info is lost on user reset
//so we only do this every 10 seconds to save cpu
dt_count += dt;
if (dt_count >= 10.0 ) {
	//double alt;
	if (getGroundElevationM(SGGeod::fromGeodM(pos, 20000), alt, 0)) {
	ground_elev_ft =  alt * SG_METER_TO_FEET;
	do_agl_calc = 0;
	altitude_agl_ft = height - ground_elev_ft ;
	dt_count = 0.0;
	}
}

//user altitude relative to the thermal height, seen AGL from the thermal foot
if ( user_altitude < 1.0 ) { user_altitude = 1.0 ;}; // an ugly way to avoid NaNs for users at alt 0
double user_altitude_agl= ( user_altitude - ground_elev_ft ) ;
alt_rel = user_altitude_agl / altitude_agl_ft;



//the updraft user feels !
double Vup;

// *********************
// environment variables
// *********************

// the  wind heading at the user alt
double wind_heading_rad;

// the "ambient" sink outside thermals
double global_sink = -1.0;

// **************
// some constants
// **************

double PI = 4.0 * atan(1.0);


// ******************
// thermal main cycle
// ******************

//we get the max strenght proportion we can expect at the time and altitude, formuled between 0 and 1
//double xx;
if (time <= t1) {
	xx= ( time / t1 );
	maxstrengthavail = xx* get_strength_fac ( alt_rel / xx );

	is_forming=1;is_formed=0;is_dying=0;is_dead=0;

	}
else if ( (time > t1) && (time <= t2) ) {
	maxstrengthavail = get_strength_fac ( (alt_rel) );

	is_forming=0;is_formed=1;is_dying=0;is_dead=0;

	}
else if ( (time > t2) && (time <= t3) ) {
	xx= ( ( time - t2) / (1.0 - t2) ) ;
	maxstrengthavail = get_strength_fac ( alt_rel - xx );

	is_forming=0;is_formed=0;is_dying=1;is_dead=0;

	}
else {
	maxstrengthavail = 0.0;
	is_forming=0;is_formed=0;is_dying=0;is_dead=1;

	}

//we get the diameter of the thermal slice at the user altitude
//the thermal has a slight conic shape

if ( (alt_rel >= 0.0) && (alt_rel < 1.0 ) ) {
	Rsink = ( shaping*Rmax ) + ( (  (1.0-shaping)*Rmax*alt_rel ) / altitude_agl_ft );  // in the main thermal body
	}
else if ( (alt_rel >=1.0) && (alt_rel < 1.1) ) {
	Rsink = (Rmax/2.0) * ( 1.0+ cos ( (10.0*PI*alt_rel)-(2.0*PI) ) ); // above the ceiling
	}
else {
	Rsink = 0.0; // above the cloud
	}

//we get the portion of the diameter that produces lift
Rup = r_up_frac * Rsink ;

//we now determine v_up_max and VupMin depending on our altitude

v_up_max = maxstrengthavail * MaxUpdraft;
v_up_min = maxstrengthavail * MinUpdraft;

// Now we know, for current t and alt, v_up_max, VupMin, Rup, Rsink.

// We still need to know how far we are from the thermal center

// To determine the thermal inclinaison in the wind, we use a ugly approximation,
// in which we say the thermal bends 20° (0.34906 rad ) for 10 kts wind.
// We move the thermal foot upwind, to keep the cloud model over the "center" at ceiling level.
// the displacement distance of the center of the thermal slice, at user altitude,
// and relative to a hipothetical vertical thermal,  would be:

// get surface and 9000 ft wind

double ground_wind_from_deg = _surface_wind_from_deg_node->getDoubleValue();
double ground_wind_speed_kts  = _surface_wind_speed_node->getDoubleValue();
double aloft_wind_from_deg = _aloft_wind_from_deg_node->getDoubleValue();
double aloft_wind_speed_kts  = _aloft_wind_speed_node->getDoubleValue();

double ground_wind_from_rad = (PI/2.0) - PI*( ground_wind_from_deg/180.0);
double aloft_wind_from_rad = (PI/2.0) - PI*( aloft_wind_from_deg/180.0);

wind_heading_rad= PI+ 0.5*( ground_wind_from_rad + aloft_wind_from_rad );

wind_speed = ground_wind_speed_kts + user_altitude * ( (aloft_wind_speed_kts -ground_wind_speed_kts ) /	9000.0 );

double dt_center_alt = -(tan (0.034906*wind_speed)) * ( altitude_agl_ft-user_altitude_agl );

// now, lets find how far we are from this shifted slice

double dt_slice_lon_FT = ( dt_center_alt * cos ( wind_heading_rad ));
double dt_slice_lat_FT = ( dt_center_alt * sin ( wind_heading_rad ));

double dt_slice_lon = dt_slice_lon_FT / ft_per_deg_lon;
double dt_slice_lat = dt_slice_lat_FT / ft_per_deg_lat;

slice_center_lon = thermal_foot_lon + dt_slice_lon;
slice_center_lat = thermal_foot_lat + dt_slice_lat;

double dist_center_lon = fabs(slice_center_lon - user_longitude)* ft_per_deg_lon;
double dist_center_lat = fabs(slice_center_lat - user_latitude)* ft_per_deg_lat;

double dist_center_FT = sqrt ( dist_center_lon*dist_center_lon + dist_center_lat*dist_center_lat ); // feet

dist_center = dist_center_FT/ 6076.11549; //nautic miles


// Now we can calculate Vup

if ( max_strength >=0.0 ) { // this is a thermal

	if ( ( dist_center >= 0.0 ) && ( dist_center < Rup ) ) {  //user is in the updraft area
		Vup = v_up_max * cos ( dist_center* PI/(2.0*Rup) );
		}
	else if ( ( dist_center > Rup ) && ( dist_center <= ((Rup+Rsink)/2.0) ) ) { //user in the 1st half of the sink area
		Vup = v_up_min * cos (( dist_center - ( Rup+Rsink)/2.0 ) * PI / ( 2.0* (  ( Rup+Rsink)/2.0 -Rup )));
		}
	else if ( ( dist_center > ((Rup+Rsink)/2.0) ) && dist_center <= Rsink ) {   // user in the 2nd half of the sink area
		Vup = ( global_sink + v_up_min )/2.0 + ( global_sink - v_up_min )/2.0 *cos ( (dist_center-Rsink) *PI/ ( (Rsink-Rup )/2.0) );
		}
	else {  // outside the thermal
		Vup = global_sink;
		} 
	}

else { // this is a sink, we don't want updraft on the sides, nor do we want to feel sink near or above ceiling and ground
	if ( alt_rel <=1.1 ) {
		double fac =  ( 1.0 - ( 1.0 - 1.815*alt_rel)*( 1.0 - 1.815*alt_rel) );
		Vup = fac * (global_sink + ( ( v_up_max - global_sink )/2.0 ) * ( 1.0+cos ( dist_center* PI / Rmax ) )) ;
		}
	else { Vup = global_sink; }
}

//correct for no global sink above clouds and outside thermals
if ( ( (alt_rel > 1.0) && (alt_rel <1.1)) && ( dist_center > Rsink ) ) {
	Vup = global_sink * ( 11.0 -10.0 * alt_rel );
	}
if ( alt_rel >= 1.1 ) { 
	Vup = 0.0;
	}

strength = Vup;
range = dist_center;

}



