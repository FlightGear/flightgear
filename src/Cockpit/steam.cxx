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

#include "steam.hxx"



////////////////////////////////////////////////////////////////////////
// Declare the functions that read the variables
////////////////////////////////////////////////////////////////////////

// Anything that reads the BFI directly is not implemented at all!


double FGSteam::the_STATIC_inhg = 29.92;
double FGSteam::the_ALT_ft = 0.0;
double FGSteam::get_ALT_ft() { _CatchUp(); return the_ALT_ft; }

double FGSteam::get_ASI_kias() { return FGBFI::getAirspeed(); }

double FGSteam::the_VSI_case = 29.92;
double FGSteam::the_VSI_fps = 0.0;
double FGSteam::get_VSI_fps() { _CatchUp(); return the_VSI_fps; }

double FGSteam::get_MH_deg () { return FGBFI::getHeading (); }
double FGSteam::get_DG_deg () { return FGBFI::getHeading (); }

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
	if ( tc > 5 )
	{	/* Huge time step; assume filter has settled */
		(*outthe) = inthe;
	} else
	{	/* Moderate time step; non linear response */
		tc = exp ( -tc );
		(*outthe) = (*outthe) * ( 1.0 - tc )
			  +    inthe  * tc;
	}
}



////////////////////////////////////////////////////////////////////////
// Here the fun really begins
////////////////////////////////////////////////////////////////////////


void FGSteam::_CatchUp()
{ if ( _UpdatesPending != 0 )
  {	double dt = _UpdatesPending * 1.0 / current_options.get_model_hz();
	int i,j;
	double d;
	/*
	Someone has called our update function and we haven't
	incorporated this into our instrument modelling yet
	*/

	/**************************
	This is just temporary
	*/
	the_ALT_ft = FGBFI::getAltitude();

	/**************************
	First, we need to know what the static line is reporting,
	which is a whole simulation area in itself.  For now, we cheat.
	*/
	the_STATIC_inhg = 29.92; 
	i = (int) the_ALT_ft;
	while ( i > 18000 )
	{	the_STATIC_inhg /= 2;
		i -= 18000;
	}
	the_STATIC_inhg /= ( 1.0 + i / 18000.0 );

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
	set_lowpass ( & the_VSI_case, the_STATIC_inhg, dt/9.0 );
	the_VSI_fps = ( the_VSI_case - the_STATIC_inhg )
		    * 7000.0; /* manual scaling factor */	

	/**************************
	Finished updates, now clear the timer 
	*/
	_UpdatesPending = 0;
  }
}


// end of steam.cxx
