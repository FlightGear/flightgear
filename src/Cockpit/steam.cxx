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

#include <simgear/compiler.h>

#include STL_IOSTREAM

#include <simgear/constants.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/props.hxx>

#include <Main/fg_props.hxx>
#include <Aircraft/aircraft.hxx>
#ifdef FG_WEATHERCM
# include <WeatherCM/FGLocalWeatherDatabase.h>
#else
# include <Environment/environment_mgr.hxx>
# include <Environment/environment.hxx>
#endif

SG_USING_NAMESPACE(std);

#include "radiostack.hxx"
#include "steam.hxx"

static bool isTied = false;



////////////////////////////////////////////////////////////////////////
// Constructor and destructor.
////////////////////////////////////////////////////////////////////////

FGSteam::FGSteam ()
  : 
    the_ALT_ft(0.0),
    the_ALT_datum_mb(1013.0),
    the_TC_rad(0.0),
    the_TC_std(0.0),
    the_STATIC_inhg(29.92),
    the_VACUUM_inhg(0.0),
    the_VSI_fps(0.0),
    the_VSI_case(29.92),
    the_MH_deg(0.0),
    the_MH_degps(0.0),
    the_MH_err(0.0),
    the_DG_deg(0.0),
    the_DG_degps(0.0),
    the_DG_inhg(0.0),
    the_DG_err(0.0),
    _UpdateTimePending(1000000)
{
}

FGSteam::~FGSteam ()
{
}

void
FGSteam::init ()
{
  _heading_deg_node = fgGetNode("/orientation/heading-deg", true);
  _mag_var_deg_node = fgGetNode("/environment/magnetic-variation-deg", true);
  _mag_dip_deg_node = fgGetNode("/environment/magnetic-dip-deg", true);
  _engine_0_rpm_node = fgGetNode("/engines/engine[0]/rpm", true);
  _pressure_inhg_node = fgGetNode("environment/pressure-inhg", true);
}

void
FGSteam::update (double dt_sec)
{
  _UpdateTimePending += dt_sec;
  _CatchUp();
}

void
FGSteam::bind ()
{
  fgTie("/steam/airspeed-kt", this, &FGSteam::get_ASI_kias);
  fgSetArchivable("/steam/airspeed-kt");
  fgTie("/steam/altitude-ft", this, &FGSteam::get_ALT_ft);
  fgSetArchivable("/steam/altitude-ft");
  fgTie("/steam/altimeter-datum-mb", this,
	&FGSteam::get_ALT_datum_mb, &FGSteam::set_ALT_datum_mb,
	false);  /* don't modify the value */
  fgSetArchivable("/steam/altimeter-datum-mb");
  fgTie("/steam/turn-rate", this, &FGSteam::get_TC_std);
  fgSetArchivable("/steam/turn-rate");
  fgTie("/steam/slip-skid",this, &FGSteam::get_TC_rad);
  fgSetArchivable("/steam/slip-skid");
  fgTie("/steam/vertical-speed-fps", this, &FGSteam::get_VSI_fps);
  fgSetArchivable("/steam/vertical-speed-fps");
  fgTie("/steam/gyro-compass-deg", this, &FGSteam::get_DG_deg);
  fgSetArchivable("/steam/gyro-compass-deg");
  // fgTie("/steam/adf-deg", FGSteam::get_HackADF_deg);
  // fgSetArchivable("/steam/adf-deg");
  fgTie("/steam/gyro-compass-error-deg", this,
	&FGSteam::get_DG_err, &FGSteam::set_DG_err,
  	false);  /* don't modify the value */
  fgSetArchivable("/steam/gyro-compass-error-deg");
  fgTie("/steam/mag-compass-deg", this, &FGSteam::get_MH_deg);
  fgSetArchivable("/steam/mag-compass-deg");
}

void
FGSteam::unbind ()
{
  fgUntie("/steam/airspeed-kt");
  fgUntie("/steam/altitude-ft");
  fgUntie("/steam/altimeter-datum-mb");
  fgUntie("/steam/turn-rate");
  fgUntie("/steam/slip-skid");
  fgUntie("/steam/vertical-speed-fps");
  fgUntie("/steam/gyro-compass-deg");
  fgUntie("/steam/gyro-compass-error-deg");
  fgUntie("/steam/mag-compass-deg");
}



////////////////////////////////////////////////////////////////////////
// Declare the functions that read the variables
////////////////////////////////////////////////////////////////////////

double
FGSteam::get_ALT_ft () const
{
  return the_ALT_ft;
}

double
FGSteam::get_ALT_datum_mb () const
{
  return the_ALT_datum_mb;
}

void
FGSteam::set_ALT_datum_mb (double datum_mb)
{
    the_ALT_datum_mb = datum_mb;
}

double
FGSteam::get_ASI_kias () const
{
  return fgGetDouble("/velocities/airspeed-kt");
}

double
FGSteam::get_VSI_fps () const
{
  return the_VSI_fps;
}

double
FGSteam::get_VACUUM_inhg () const
{
  return the_VACUUM_inhg;
}

double
FGSteam::get_MH_deg () const
{
  return the_MH_deg;
}

double
FGSteam::get_DG_deg () const
{
  return the_DG_deg;
}

double
FGSteam::get_DG_err () const
{
  return the_DG_err;
}

void
FGSteam::set_DG_err (double approx_magvar)
{
  the_DG_err = approx_magvar;
}

double
FGSteam::get_TC_rad () const
{
  return the_TC_rad;
}

double
FGSteam::get_TC_std () const
{
  return the_TC_std;
}



////////////////////////////////////////////////////////////////////////
// Recording the current time
////////////////////////////////////////////////////////////////////////


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
{
  if ( _UpdateTimePending != 0 )
  {
        double dt = _UpdateTimePending;
        double AccN, AccE, AccU;
	/* int i, j; */
	double d, the_ENGINE_rpm;

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
        // cout << current_aircraft.fdm_state->get_A_Z_pilot() << endl;
	d = -current_aircraft.fdm_state->get_A_Z_pilot();
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
	                * SGD_RADIANS_TO_DEGREES / 20.0 +
	        current_aircraft.fdm_state->get_Psi_dot ()
	                * SGD_RADIANS_TO_DEGREES / 3.0  , dt );

	/**************************
	We want to know the pilot accelerations,
	to compute the magnetic compass errors.
	*/
	AccN = current_aircraft.fdm_state->get_V_dot_north();
	AccE = current_aircraft.fdm_state->get_V_dot_east();
	AccU = current_aircraft.fdm_state->get_V_dot_down()
	     - 9.81 * SG_METER_TO_FEET;
	if ( fabs(the_TC_rad) > 0.2 /* 2.0 */ )
	{       /* Massive sideslip jams it; it stops turning */
	        the_MH_degps = 0.0;
	        the_MH_err   = _heading_deg_node->getDoubleValue() - the_MH_deg;
	} else
	{       double MagDip, MagVar, CosDip;
	        double FrcN, FrcE, FrcU, AccTot;
	        double EdgN, EdgE, EdgU;
	        double TrqN, TrqE, TrqU, Torque;
	        /* Find a force vector towards exact magnetic north */
	        MagVar = _mag_var_deg_node->getDoubleValue() 
                    / SGD_RADIANS_TO_DEGREES;
	        MagDip = _mag_var_deg_node->getDoubleValue()
                    / SGD_RADIANS_TO_DEGREES;
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
	        EdgN = cos ( the_MH_err / SGD_RADIANS_TO_DEGREES );
	        EdgE = sin ( the_MH_err / SGD_RADIANS_TO_DEGREES );
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
	        the_MH_deg  = _heading_deg_node->getDoubleValue() - the_MH_err;
	}

	/**************************
	This is not actually correct, but provides a
	scaling capability for the vacuum pump later on.
	When we have a real engine model, we can ask it.
	*/
	the_ENGINE_rpm = _engine_0_rpm_node->getDoubleValue();

	/**************************
	First, we need to know what the static line is reporting,
	which is a whole simulation area in itself.
	We filter the actual value by one second to
	account for the line impedance of the plumbing.
	*/
#ifdef FG_WEATHERCM
	sgVec3 plane_pos = { cur_fdm_state->get_Latitude(),
			     cur_fdm_state->get_Longitude(),
			     cur_fdm_state->get_Altitude() * SG_FEET_TO_METER };
	double static_inhg = WeatherDatabase->get(plane_pos).AirPressure *
	    (0.01 / INHG_TO_MB);
#else
	double static_inhg = _pressure_inhg_node->getDoubleValue();
#endif

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
	if ( _UpdateTimePending > 999999 )
	    the_DG_err = fgGetDouble("/environment/magnetic-variation-deg");
 	the_DG_degps = 0.01; /* HACK! */
 	if (dt<1.0) the_DG_err += dt * the_DG_degps;
 	the_DG_deg = _heading_deg_node->getDoubleValue() - the_DG_err;

	/**************************
	Finished updates, now clear the timer 
	*/
	_UpdateTimePending = 0;
  } else {
      // cout << "0 Updates pending" << endl;
  }
}


////////////////////////////////////////////////////////////////////////
// Everything below is a transient hack; expect it to disappear
////////////////////////////////////////////////////////////////////////

double FGSteam::get_HackOBS1_deg () const
{
    return current_radiostack->get_navcom1()->get_nav_radial(); 
}

double FGSteam::get_HackOBS2_deg () const
{
    return current_radiostack->get_navcom2()->get_nav_radial(); 
}


// end of steam.cxx
