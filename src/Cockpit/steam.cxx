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
#include <simgear/math/fg_types.hxx>
#include <Main/options.hxx>
#include <Main/bfi.hxx>

FG_USING_NAMESPACE(std);

#include "radiostack.hxx"
#include "steam.hxx"



////////////////////////////////////////////////////////////////////////
// Declare the functions that read the variables
////////////////////////////////////////////////////////////////////////

// Anything that reads the BFI directly is not implemented at all!


#define VARY_E		(14)


double FGSteam::the_STATIC_inhg = 29.92;
double FGSteam::the_ALT_ft = 0.0;
double FGSteam::get_ALT_ft() { _CatchUp(); return the_ALT_ft; }

double FGSteam::get_ASI_kias() { return FGBFI::getAirspeed(); }

double FGSteam::the_VSI_case = 29.92;
double FGSteam::the_VSI_fps = 0.0;
double FGSteam::get_VSI_fps() { _CatchUp(); return the_VSI_fps; }

double FGSteam::the_VACUUM_inhg = 0.0;
double FGSteam::get_VACUUM_inhg() { _CatchUp(); return the_VACUUM_inhg; }

double FGSteam::get_MH_deg () {
    return FGBFI::getHeading () - FGBFI::getMagVar ();
}
double FGSteam::get_DG_deg () {
    return FGBFI::getHeading () - FGBFI::getMagVar ();
}

double FGSteam::get_TC_rad   () { return FGBFI::getSideSlip (); }
double FGSteam::get_TC_radps () { return FGBFI::getRoll (); }


////////////////////////////////////////////////////////////////////////
// Recording the current time
////////////////////////////////////////////////////////////////////////


int FGSteam::_UpdatesPending = 9999;  /* Forces filter to reset */


void FGSteam::update ( int timesteps )
{
	_UpdatesPending += timesteps;
}


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
	{	/* Normal mode of operation */
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



////////////////////////////////////////////////////////////////////////
// Here the fun really begins
////////////////////////////////////////////////////////////////////////


void FGSteam::_CatchUp()
{ if ( _UpdatesPending != 0 )
  {	double dt = _UpdatesPending * 1.0 / current_options.get_model_hz();
	int i,j;
	double d, the_ENGINE_rpm;
	/*
	Someone has called our update function and
	it turns out that we are running somewhat behind.
	Here, we recalculate everything for a 'dt' second step.
	*/

	/**************************
	This is not actually correct, but provides a
	scaling capability for the vacuum pump later on.
	When we have a real engine model, we can ask it.
	*/
	the_ENGINE_rpm = FGBFI::getThrottle() * 26.0;

	/**************************
	This is just temporary, until the static source works,
	so we just filter the actual value by one second to
	account for the line impedance of the plumbing.
	*/
	set_lowpass ( & the_ALT_ft, FGBFI::getAltitude(), dt );

	/**************************
	First, we need to know what the static line is reporting,
	which is a whole simulation area in itself.  For now, we cheat.
	*/
	the_STATIC_inhg = 29.92; 
	i = (int) the_ALT_ft;
	while ( i > 9000 )
	{	the_STATIC_inhg *= 0.707;
		i -= 9000;
	}
	the_STATIC_inhg *= ( 1.0 - 0.293 * i / 9000.0 );

	/*
	NO alternate static source error (student feature), 
	NO possibility of blockage (instructor feature),
	NO slip-induced error, important for C172 for example.
	*/

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

	/**************************
	Finished updates, now clear the timer 
	*/
	_UpdatesPending = 0;
  }
}


////////////////////////////////////////////////////////////////////////
// Everything below is a transient hack; expect it to disappear
////////////////////////////////////////////////////////////////////////

/* KMYF ILS */
#define NAV1_LOC	(1)
#define NAV1_Lat	(  32.0 + 48.94/60.0)
#define NAV1_Lon	(-117.0 - 08.37/60.0)
#define NAV1_Rad  	280.0
#define NAV1_Alt	423

/* MZB stepdown radial */
#define NAV2_Lat	(  32.0 + 46.93/60.0)
#define NAV2_Lon	(-117.0 - 13.53/60.0)
#define NAV2_Rad	068.0

/* HAILE intersection */
#define ADF_Lat		(  32.0 + 46.79/60.0)
#define ADF_Lon		(-117.0 - 02.70/60.0)



double FGSteam::get_HackGS_deg () {

    double x = current_radiostack->get_nav1_dist();
    double y = (FGBFI::getAltitude() - current_radiostack->get_nav1_elev())
	* FEET_TO_METER;
    double angle = atan2( y, x ) * RAD_TO_DEG;
    return current_radiostack->get_nav1_target_gs() - angle;

#if 0
    double x,y,dme;
    if (0==NAV1_LOC) return 0.0;
    y = 60.0 * ( NAV1_Lat - FGBFI::getLatitude () );
    x = 60.0 * ( NAV1_Lon - FGBFI::getLongitude() )
	                * cos ( FGBFI::getLatitude () / RAD_TO_DEG );
    dme = x*x + y*y;
    if ( dme > 0.1 ) x = sqrt ( dme ); else x = 0.3;
    y = FGBFI::getAltitude() - NAV1_Alt;
    return 3.0 - (y/x) * 60.0 / 6000.0;
#endif
}


double FGSteam::get_HackVOR1_deg () {
    double r;

#if 0
    double x,y;
    y = 60.0 * ( NAV1_Lat - FGBFI::getLatitude () );
    x = 60.0 * ( NAV1_Lon - FGBFI::getLongitude() )
	* cos ( FGBFI::getLatitude () / RAD_TO_DEG );
    r = atan2 ( x, y ) * RAD_TO_DEG - NAV1_Rad - FGBFI::getMagVar();
#endif

    r = current_radiostack->get_nav1_radial() - 
	current_radiostack->get_nav1_heading() + 180.0;
    // cout << "Radial = " << current_radiostack->get_nav1_radial() 
    //      << "  Bearing = " << current_radiostack->get_nav1_heading() << endl;
    
    if (r> 180.0) r-=360.0; else
	if (r<-180.0) r+=360.0;
    if ( fabs(r) > 90.0 )
	r = ( r<0.0 ? -r-180.0 : -r+180.0 );
    if (NAV1_LOC) r*=5.0;

    return r;
}


double FGSteam::get_HackVOR2_deg () {
    double r;

#if 0
    double x,y;
    y = 60.0 * ( NAV2_Lat - FGBFI::getLatitude () );
    x = 60.0 * ( NAV2_Lon - FGBFI::getLongitude() )
	* cos ( FGBFI::getLatitude () / RAD_TO_DEG );
    r = atan2 ( x, y ) * RAD_TO_DEG - NAV2_Rad - FGBFI::getMagVar();
#endif

    r = current_radiostack->get_nav2_radial() -
	current_radiostack->get_nav2_heading() + 180.0;
    // cout << "Radial = " << current_radiostack->get_nav1_radial() 
    // << "  Bearing = " << current_radiostack->get_nav1_heading() << endl;
    
    if (r> 180.0) r-=360.0; else
	if (r<-180.0) r+=360.0;
    if ( fabs(r) > 90.0 )
	r = ( r<0.0 ? -r-180.0 : -r+180.0 );
    return r;
}


double FGSteam::get_HackOBS1_deg ()
{	return  NAV1_Rad; 
}


double FGSteam::get_HackOBS2_deg ()
{	return  NAV2_Rad; 
}


double FGSteam::get_HackADF_deg ()
{	double r;
	double x,y;
	y = 60.0 * ( ADF_Lat - FGBFI::getLatitude () );
	x = 60.0 * ( ADF_Lon - FGBFI::getLongitude() )
	               * cos ( FGBFI::getLatitude () / RAD_TO_DEG );
	r = atan2 ( x, y ) * RAD_TO_DEG - FGBFI::getHeading ();
	return r;
}


// end of steam.cxx
