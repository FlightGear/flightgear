// steam.cxx - Steam Gauge Calculations
//
// Copyright (C) 2000  Alexander Perry - alex.perry@ieee.org
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

#if defined( FG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#else
#  include <iostream>
#endif

#include <simgear/constants.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/props.hxx>
#include <Aircraft/aircraft.hxx>
#include <Main/bfi.hxx>
#include <NetworkOLK/features.hxx>

FG_USING_NAMESPACE(std);

#include "radiostack.hxx"
#include "steam.hxx"

static bool isTied = false;



////////////////////////////////////////////////////////////////////////
// Declare the functions that read the variables
////////////////////////////////////////////////////////////////////////

// Anything that reads the BFI directly is not implemented at all!


double FGSteam::the_STATIC_inhg = 29.92;
double FGSteam::the_ALT_ft = 0.0;  // Indicated altitude
double FGSteam::get_ALT_ft() { _CatchUp(); return the_ALT_ft; }

double FGSteam::the_ALT_datum_mb = 1013.0;
double FGSteam::get_ALT_datum_mb() { return the_ALT_datum_mb; }

void FGSteam::set_ALT_datum_mb ( double datum_mb ) {
    the_ALT_datum_mb = datum_mb;
}

double FGSteam::get_ASI_kias() { return FGBFI::getAirspeed(); }

double FGSteam::the_VSI_case = 29.92;
double FGSteam::the_VSI_fps = 0.0;
double FGSteam::get_VSI_fps() { _CatchUp(); return the_VSI_fps; }

double FGSteam::the_VACUUM_inhg = 0.0;
double FGSteam::get_VACUUM_inhg() { _CatchUp(); return the_VACUUM_inhg; }

double FGSteam::the_MH_err   = 0.0;
double FGSteam::the_MH_deg   = 0.0;
double FGSteam::the_MH_degps = 0.0;
double FGSteam::get_MH_deg () { _CatchUp(); return the_MH_deg; }

double FGSteam::the_DG_err   = 0.0;
double FGSteam::the_DG_deg   = 0.0;
double FGSteam::the_DG_degps = 0.0;
double FGSteam::the_DG_inhg  = 0.0;
double FGSteam::get_DG_deg () { _CatchUp(); return the_DG_deg; }
double FGSteam::get_DG_err () { _CatchUp(); return the_DG_err; }

void FGSteam::set_DG_err ( double approx_magvar ) {
    the_DG_err = approx_magvar;
}

double FGSteam::the_TC_rad   = 0.0;
double FGSteam::the_TC_std   = 0.0;
double FGSteam::get_TC_rad () { _CatchUp(); return the_TC_rad; }
double FGSteam::get_TC_std () { _CatchUp(); return the_TC_std; }


////////////////////////////////////////////////////////////////////////
// Recording the current time
////////////////////////////////////////////////////////////////////////


int FGSteam::_UpdatesPending = 1000000;  /* Forces filter to reset */


				// FIXME: no need to use static
				// functions any longer.

void FGSteam::update ( int timesteps )
{
        if (!isTied) {
	  isTied = true;
	  fgTie("/steam/airspeed", FGSteam::get_ASI_kias);
	  fgTie("/steam/altitude", FGSteam::get_ALT_ft);
	  fgTie("/steam/altimeter-datum-mb",
		FGSteam::get_ALT_datum_mb, FGSteam::set_ALT_datum_mb,
		false);  /* don't modify the value */
	  fgTie("/steam/turn-rate", FGSteam::get_TC_std);
	  fgTie("/steam/slip-skid", FGSteam::get_TC_rad);
	  fgTie("/steam/vertical-speed", FGSteam::get_VSI_fps);
	  fgTie("/steam/gyro-compass", FGSteam::get_DG_deg);
	  fgTie("/steam/adf", FGSteam::get_HackADF_deg);
	  fgTie("/steam/gyro-compass-error",
		FGSteam::get_DG_err, FGSteam::set_DG_err,
		false);  /* don't modify the value */
	  fgTie("/steam/mag-compass", FGSteam::get_MH_deg);
	}
	_UpdatesPending += timesteps;
}


/* tc should be (elapsed_time_between_updates / desired_smoothing_time) */
void FGSteam::set_lowpass ( double *outthe, double inthe, double tc )
{
	if ( tc < 0.0 )
	{	if ( tc < -1.0 )
		{	/* time went backwards; kill the filter */
			(*outthe) = inthe;
		} else
		{	/* ignore mildly negative time */
		}
	} else
	if ( tc < 0.2 )
	{	/* Normal mode of operation; fast approximation to exp(-tc) */
		(*outthe) = (*outthe) * ( 1.0 - tc )
			  +    inthe  * tc;
	} else
	if ( tc > 5.0 )
	{	/* Huge time step; assume filter has settled */
		(*outthe) = inthe;
	} else
	{	/* Moderate time step; non linear response */
		double keep = exp ( -tc );
		// printf ( "ARP: Keep is %f\n", keep );
		(*outthe) = (*outthe) * keep
			  +    inthe  * ( 1.0 - keep );
	}
}


#define INHG_TO_MB 33.86388  /* Inches_of_mercury * INHG_TO_MB == millibars. */

// Convert air pressure to altitude by ICAO Standard Atmosphere
double pressInHgToAltFt(double p_inhg)
{
    // Ref. Aviation Formulary, Ed Williams, www.best.com/~williams/avform.htm
    const double P_0 = 29.92126;  // Std. MSL pressure, inHg. (=1013.25 mb)
    const double p_Tr = 0.2233609 * P_0;  // Pressure at tropopause, same units.
    const double h_Tr = 36089.24;  // Alt of tropopause, ft. (=11.0 km)

    if (p_inhg > p_Tr)  // 0.0 to 11.0 km
	return (1.0 - pow((p_inhg / P_0), 1.0 / 5.2558797)) / 6.8755856e-6;
    return h_Tr + log10(p_inhg / p_Tr) / -4.806346e-5;  // 11.0 to 20.0 km
    // We could put more code for higher altitudes here.
}


// Convert altitude to air pressure by ICAO Standard Atmosphere
double altFtToPressInHg(double alt_ft)
{
    // Ref. Aviation Formulary, Ed Williams, www.best.com/~williams/avform.htm
    const double P_0 = 29.92126;  // Std. MSL pressure, inHg. (=1013.25 mb)
    const double p_Tr = 0.2233609 * P_0;  // Pressure at tropopause, same units.
    const double h_Tr = 36089.24;  // Alt of tropopause, ft. (=11.0 km)

    if (alt_ft < h_Tr)  // 0.0 to 11.0 km
	return P_0 * pow(1.0 - 6.8755856e-6 * alt_ft, 5.2558797);
    return p_Tr * exp(-4.806346e-5 * (alt_ft - h_Tr));  // 11.0 to 20.0 km
    // We could put more code for higher altitudes here.
}



////////////////////////////////////////////////////////////////////////
// Here the fun really begins
////////////////////////////////////////////////////////////////////////


void FGSteam::_CatchUp()
{ if ( _UpdatesPending != 0 )
  {	double dt = _UpdatesPending * 1.0 / 
	    fgGetInt("/sim/model-hz"); // FIXME: inefficient
        double AccN, AccE, AccU;
	/* int i, j; */
	double d, the_ENGINE_rpm;

#if 0
        /**************************
	There is the possibility that this is the first call.
	If this is the case, we will emit the feature registrations
	just to be on the safe side.  Doing it more than once will
	waste CPU time but doesn't hurt anything really.
	*/
	if ( _UpdatesPending > 999999 )
	{ FGFeature::register_int (    "Avionics/NAV1/Localizer", &NAV1_LOC );
	  FGFeature::register_double ( "Avionics/NAV1/Latitude",  &NAV1_Lat );
	  FGFeature::register_double ( "Avionics/NAV1/Longitude", &NAV1_Lon );
	  FGFeature::register_double ( "Avionics/NAV1/Radial",    &NAV1_Rad );
	  FGFeature::register_double ( "Avionics/NAV1/Altitude",  &NAV1_Alt );
	  FGFeature::register_int (    "Avionics/NAV2/Localizer", &NAV2_LOC );
	  FGFeature::register_double ( "Avionics/NAV2/Latitude",  &NAV2_Lat );
	  FGFeature::register_double ( "Avionics/NAV2/Longitude", &NAV2_Lon );
	  FGFeature::register_double ( "Avionics/NAV2/Radial",    &NAV2_Rad );
	  FGFeature::register_double ( "Avionics/NAV2/Altitude",  &NAV2_Alt );
	  FGFeature::register_double ( "Avionics/ADF/Latitude",   &ADF_Lat );
	  FGFeature::register_double ( "Avionics/ADF/Longitude",  &ADF_Lon );
	}
#endif

	/**************************
	Someone has called our update function and
	it turns out that we are running somewhat behind.
	Here, we recalculate everything for a 'dt' second step.
	*/

	/**************************
	The ball responds to the acceleration vector in the body
	frame, only the components perpendicular to the longitudinal
	axis of the aircraft.  This is only related to actual
	side slip for a symmetrical aircraft which is not touching
	the ground and not changing its attitude.  Math simplifies
	by assuming (for small angles) arctan(x)=x in radians.
	Obvious failure mode is the absence of liquid in the
	tube, which is there to damp the motion, so that instead
	the ball will bounce around, hitting the tube ends.
	More subtle flaw is having it not move or a travel limit
	occasionally due to some dirt in the tube or on the ball.
	*/
	// the_TC_rad = - ( FGBFI::getSideSlip () ); /* incorrect */
	d = - current_aircraft.fdm_state->get_A_Z_pilot();
	if ( d < 1 ) d = 1;
	set_lowpass ( & the_TC_rad,
	        current_aircraft.fdm_state->get_A_Y_pilot () / d,
	        dt );

	/**************************
	The rate of turn indication is from an electric gyro.
	We should have it spin up with the master switch.
	It is mounted at a funny angle so that it detects
	both rate of bank (i.e. rolling into and out of turns)
	and the rate of turn (i.e. how fast heading is changing).
	*/
	set_lowpass ( & the_TC_std,
	        current_aircraft.fdm_state->get_Phi_dot ()
	                * RAD_TO_DEG / 20.0 +
	        current_aircraft.fdm_state->get_Psi_dot ()
	                * RAD_TO_DEG / 3.0  , dt );

	/**************************
	We want to know the pilot accelerations,
	to compute the magnetic compass errors.
	*/
	AccN = current_aircraft.fdm_state->get_V_dot_north();
	AccE = current_aircraft.fdm_state->get_V_dot_east();
	AccU = current_aircraft.fdm_state->get_V_dot_down()
	     - 9.81 / 0.3;
	if ( fabs(the_TC_rad) > 0.2 )
	{       /* Massive sideslip jams it; it stops turning */
	        the_MH_degps = 0.0;
	        the_MH_err   = FGBFI::getHeading () - the_MH_deg;
	} else
	{       double MagDip, MagVar, CosDip;
	        double FrcN, FrcE, FrcU, AccTot;
	        double EdgN, EdgE, EdgU;
	        double TrqN, TrqE, TrqU, Torque;
	        /* Find a force vector towards exact magnetic north */
	        MagVar = FGBFI::getMagVar() / RAD_TO_DEG;
	        MagDip = FGBFI::getMagDip() / RAD_TO_DEG;
	        CosDip = cos ( MagDip );
	        FrcN = CosDip * cos ( MagVar );
	        FrcE = CosDip * sin ( MagVar );
	        FrcU = sin ( MagDip );
	        /* Rotation occurs around acceleration axis,
	           but axis magnitude is irrelevant.  So compute it. */
	        AccTot = AccN*AccN + AccE*AccE + AccU*AccU;
	        if ( AccTot > 1.0 )  AccTot = sqrt ( AccTot );
	                        else AccTot = 1.0;
	        /* Force applies to north marking on compass card */
	        EdgN = cos ( the_MH_err / RAD_TO_DEG );
	        EdgE = sin ( the_MH_err / RAD_TO_DEG );
	        EdgU = 0.0;
	        /* Apply the force to the edge to get torques */
	        TrqN = EdgE * FrcU - EdgU * FrcE;
	        TrqE = EdgU * FrcN - EdgN * FrcU;
	        TrqU = EdgN * FrcE - EdgE * FrcN;
	        /* Select the component parallel to the axis */
	        Torque = ( TrqN * AccN + 
	                   TrqE * AccE + 
	                   TrqU * AccU ) * 5.0 / AccTot;
	        /* The magnetic compass has angular momentum,
	           so we apply a torque to it and wait */
	        if ( dt < 1.0 )
	        {       the_MH_degps= the_MH_degps * (1.0 - dt) - Torque;
	                the_MH_err += dt * the_MH_degps;
	        }
	        if ( the_MH_err >  180.0 ) the_MH_err -= 360.0; else
	        if ( the_MH_err < -180.0 ) the_MH_err += 360.0;
	        the_MH_deg  = FGBFI::getHeading () - the_MH_err;
	}

	/**************************
	This is not actually correct, but provides a
	scaling capability for the vacuum pump later on.
	When we have a real engine model, we can ask it.
	*/
	the_ENGINE_rpm = controls.get_throttle(0) * 26.0;

	/**************************
	First, we need to know what the static line is reporting,
	which is a whole simulation area in itself.  For now, we cheat.
	We filter the actual value by one second to
	account for the line impedance of the plumbing.
	*/
	double static_inhg = altFtToPressInHg(FGBFI::getAltitude());
	set_lowpass ( & the_STATIC_inhg, static_inhg, dt ); 

	/*
	NO alternate static source error (student feature), 
	NO possibility of blockage (instructor feature),
	NO slip-induced error, important for C172 for example.
	*/

	/**************************
	Altimeter.
	ICAO standard atmosphere MSL pressure is 1013.25 mb, and pressure
	gradient is about 28 ft per mb at MSL increasing to about 32 at
	5000 and 38 at 10000 ft.
	Standard altimeters apply the subscale offset to the output altitude,
	not to the input pressure; I don't know exactly what pressure gradient
	they assume for this.  I choose to make it accurate at low altitudes.
	Remember, we are trying to simulate a real altimeter, not an ideal one.
	*/
	set_lowpass ( & the_ALT_ft,
	    pressInHgToAltFt(the_STATIC_inhg) +
	    (the_ALT_datum_mb - 1013.25) * 28.0, /* accurate at low alt. */
	    dt * 10 ); /* smoothing time 0.1 s */

	/**************************
	The VSI case is a low-pass filter of the static line pressure.
	The instrument reports the difference, scaled to approx ft.
	NO option for student to break glass when static source fails.
	NO capability for a fixed non-zero reading when level.
	NO capability to have a scaling error of maybe a factor of two.
	*/
	the_VSI_fps = ( the_VSI_case - the_STATIC_inhg )
		    * 10000.0; /* manual scaling factor */	
	set_lowpass ( & the_VSI_case, the_STATIC_inhg, dt/6.0 );

	/**************************
	The engine driven vacuum pump is directly attached
	to the engine shaft, so each engine rotation pumps
	a fixed volume.  The amount of air in that volume
	is determined by the vacuum line's internal pressure.
	The instruments are essentially leaking air like
	a fixed source impedance from atmospheric pressure.
	The regulator provides a digital limit setting,
	which is open circuit unless the pressure drop is big.
	Thus, we can compute the vacuum line pressure directly.
	We assume that there is negligible reservoir space.
	NO failure of the pump supported (yet)
	*/
	the_VACUUM_inhg = the_STATIC_inhg *
		the_ENGINE_rpm / ( the_ENGINE_rpm + 10000.0 );
	if ( the_VACUUM_inhg > 5.0 )
	     the_VACUUM_inhg = 5.0;

/*
> I was merely going to do the engine rpm driven vacuum pump for both
> the AI and DG, have the gyros spin down down in power off descents, 
> have it tumble when you exceed the usual pitch or bank limits,
> put in those insidious turning errors ... for now anyway.
*/
 	if ( _UpdatesPending > 999999 )
	    the_DG_err = FGBFI::getMagVar();
 	the_DG_degps = 0.01; /* HACK! */
 	if (dt<1.0) the_DG_err += dt * the_DG_degps;
 	the_DG_deg = FGBFI::getHeading () - the_DG_err;

	/**************************
	Finished updates, now clear the timer 
	*/
	_UpdatesPending = 0;
  } else {
      // cout << "0 Updates pending" << endl;
  }
}


////////////////////////////////////////////////////////////////////////
// Everything below is a transient hack; expect it to disappear
////////////////////////////////////////////////////////////////////////


#if 0

double FGSteam::get_HackGS_deg () {
    if ( current_radiostack->get_nav1_inrange() && 
	 current_radiostack->get_nav1_has_gs() )
    {
	double x = current_radiostack->get_nav1_gs_dist();
	double y = (FGBFI::getAltitude() - current_radiostack->get_nav1_elev())
	    * FEET_TO_METER;
	double angle = atan2( y, x ) * RAD_TO_DEG;
	return (current_radiostack->get_nav1_target_gs() - angle) * 5.0;
    } else {
	return 0.0;
    }
}


double FGSteam::get_HackVOR1_deg () {
    double r;

    if ( current_radiostack->get_nav1_inrange() ) {
        r = current_radiostack->get_nav1_heading()
	    - current_radiostack->get_nav1_radial();
	// cout << "Radial = " << current_radiostack->get_nav1_radial() 
	//      << "  Bearing = " << current_radiostack->get_nav1_heading()
	//      << endl;
    
	if (r> 180.0) r-=360.0; else
	    if (r<-180.0) r+=360.0;
	if ( fabs(r) > 90.0 )
	    r = ( r<0.0 ? -r-180.0 : -r+180.0 );
	// According to Robin Peel, the ILS is 4x more sensitive than a vor
	if ( current_radiostack->get_nav1_loc() ) r *= 4.0;
    } else {
	r = 0.0;
    }

    return r;
}


double FGSteam::get_HackVOR2_deg () {
    double r;

    if ( current_radiostack->get_nav2_inrange() ) {
        r = current_radiostack->get_nav2_heading()
	    - current_radiostack->get_nav2_radial();
	// cout << "Radial = " << current_radiostack->get_nav1_radial() 
	// << "  Bearing = " << current_radiostack->get_nav1_heading() << endl;
    
	if (r> 180.0) r-=360.0; else
	    if (r<-180.0) r+=360.0;
	if ( fabs(r) > 90.0 )
	    r = ( r<0.0 ? -r-180.0 : -r+180.0 );
    } else {
	r = 0.0;
    }

    return r;
}
#endif


double FGSteam::get_HackOBS1_deg () {
    return current_radiostack->get_nav1_radial(); 
}


double FGSteam::get_HackOBS2_deg () {
    return current_radiostack->get_nav2_radial(); 
}


double FGSteam::get_HackADF_deg () {
    static double last_r = 0;
    double r;

    if ( current_radiostack->get_adf_inrange() ) {
	double r = current_radiostack->get_adf_heading() - FGBFI::getHeading();
	last_r = r;
	// cout << "Radial = " << current_radiostack->get_adf_heading() 
	//      << "  Heading = " << FGBFI::getHeading() << endl;
	return r;
    } else {
	return last_r;
    }
}


// end of steam.cxx
