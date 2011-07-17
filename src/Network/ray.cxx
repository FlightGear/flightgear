// ray.cxx -- "RayWoodworth" motion chair support
//
// Written by Alexander Perry, started May 2000
//
// Copyright (C) 2000, Alexander Perry, alex.perry@ieee.org
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

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>

#include <FDM/flightProperties.hxx>
#include <Environment/gravity.hxx>
#include "ray.hxx"


FGRAY::FGRAY() {
	chair_rising = 0.0;
	chair_height = 0.0;
	chair_heading = 0.0;
	chair_vertical[0] = 0.0;
	chair_vertical[1] = 0.0;
//	chair_FILE = stderr;
	chair_FILE = 0;
}


FGRAY::~FGRAY() {
}


// Ray Woodworth (949) 262-9118 has a three axis motion chair.
//
// It expects +/- 5V signals for full scale.  In channel order, axes are:
//	roll, pitch, yaw, sway, surge, heave
// The drivers are capable of generating (in the same order)
//	+/- 30deg, 30deg, 30deg, 12in, 12in, 12in
// The signs of the motion are such that positive volts gives
//	head right, head back, feet right, body right, body back, body up
//
// In this code implementation, the voltage outputs are generated
// using a ComputerBoards DDA06/Jr card and the associated Linux driver.
// Data is written to the device /dev/dda06jr-A as byte triplets;
// The first byte is the channel number (0-5 respectively) and
// the remaining two bytes are an unsigned short for the signal.


bool FGRAY::gen_message() {
    // cout << "generating RayWoodworth message" << endl;
    FlightProperties f;
    
    int axis, subaxis;
    const double fullscale[6] = { -0.5, -0.5, -0.5, /* radians */
				  -0.3, -0.3, -0.15  /* meters */ };

    /* Figure out how big our timesteps are */
    double dt = 0.05; /* seconds */

    /* get basic information about gravity */
    double grav_acc = -Environment::Gravity::instance()->getGravity( f.getPosition() );
    double vert_acc = f.get_A_Z_pilot() * 0.3;
    if ( -3.0 < vert_acc )
	vert_acc = -3.0;

    for ( axis = 0; axis < 3; axis++ )
    {	/* Compute each angular axis together with the linear
	   axis which is coupled by smooth coordinated flight
	*/
	double ang_pos;
	double lin_pos, lin_acc;

	/* Retrieve the desired components */
	switch ( axis ) {
	case 0: ang_pos = f.get_Phi();
		lin_acc = f.get_A_Y_pilot() * 0.3;
		break;
	case 1: ang_pos = f.get_Theta();
		lin_acc = f.get_A_X_pilot() * 0.3;
		break;
	case 2: ang_pos = f.get_Psi();
		lin_acc = grav_acc - vert_acc;
		break;
	default:
		ang_pos = 0.0;
		lin_acc = 0.0;
		break;
	}

	/* Make sure the angles are reasonable onscale */
 	/* We use an asymmetric mapping so that the chair behaves
 	   reasonably when upside down.  Otherwise it oscillates. */
 	while ( ang_pos < -SGD_2PI/3 ) {
		ang_pos += SGD_2PI;
	}
 	while ( ang_pos >  2*SGD_2PI/3 ) {
		ang_pos -= SGD_2PI;
	}

	/* Tell interested parties what the situation is */
	if (chair_FILE) {
	    fprintf ( chair_FILE, "RAY %s, %8.3f rad %8.3f m/s/s  =>",
		      ((axis==0)?"Roll ":((axis==1)?"Pitch":"Yaw  ")),
		      ang_pos, lin_acc );
	}

	/* The upward direction and axis are special cases */
	if ( axis == 2 )
	{
	/* heave */
		/* Integrate vertical acceleration into velocity,
		   diluted by 50% and with a 0.2 second high pass */
		chair_rising += ( lin_acc - chair_rising ) * dt * 0.5;
		/* Integrate velocity into position, 0.2 sec high pass */
		chair_height += ( chair_rising - chair_height ) * dt * 0.5;
		lin_pos = chair_height;

	/* yaw */
		/* Make sure that we walk through North cleanly */
		if ( fabs ( ang_pos - chair_heading ) > SGD_PI )
		{	/* Need to swing chair by 360 degrees */
			if ( ang_pos < chair_heading )
				chair_heading -= SGD_2PI;
			else	chair_heading +=  SGD_2PI;
		}
		/* Remove the chair heading from the true heading */
		ang_pos -= chair_heading;
		/* Wash out the error at 5 sec timeconstant because
		   a standard rate turn is 3 deg/sec and the chair
 		   can just about represent 30 degrees full scale.  */
		chair_heading += ang_pos * dt * 0.2;
		/* If they turn fast, at 90 deg error subtract 30 deg */
		if ( fabs(ang_pos) > SGD_PI_2 )
			chair_heading += ang_pos / 3;

	} else
	{	/* 3 second low pass to find attitude and gravity vector */
		chair_vertical[axis] += ( dt / 3 ) *
			( lin_acc / vert_acc + ang_pos 
				- chair_vertical[axis] );
		/* find out how much linear acceleration is left */
		lin_acc -= chair_vertical[axis] * vert_acc;
		/* reposition the pilot tilt relative to the chair */
		ang_pos -= chair_vertical[axis];
		/* integrate linear acceleration into a position */
		lin_pos = lin_acc; /* HACK */
	}

	/* Tell interested parties what we'll do */
	if ( chair_FILE ) {
	    fprintf ( chair_FILE, "  %8.3f deg %8.3f cm.\n",
		      ang_pos * 60.0, lin_pos * 100.0 );
	}

	/* Write the resulting numbers to the command buffer */
	/* The first pass number is linear, second pass is angle */
	for ( subaxis = axis; subaxis < 6; subaxis += 3 )
	{	unsigned short *dac;
		/* Select the DAC in the command buffer */
		buf [ 3*subaxis ] = subaxis;
		dac = (unsigned short *) ( buf + 1 + 3*subaxis );
		/* Select the relevant number to put there */
		double propose = ( subaxis < 3 ) ? ang_pos : lin_pos;
		/* Scale to the hardware's full scale range */
		propose /= fullscale [ subaxis ];
		/* Use a sine shaped washout on all axes */
		if ( propose < -SGD_PI_2 ) *dac = 0x0000; else
		if ( propose >  SGD_PI_2 ) *dac = 0xFFFF; else
		   *dac = (unsigned short) ( 32767 * 
				( 1.0 + sin ( propose ) ) );
	}

	/* That concludes the per-axis calculations */
    }

    /* Tell the caller what we did */
    length = 18;

    return true;
}


// parse RUL message
bool FGRAY::parse_message() {
    SG_LOG( SG_IO, SG_ALERT, "RAY input not supported" );

    return false;
}


// process work for this port
bool FGRAY::process() {
    SGIOChannel *io = get_io_channel();

    if ( get_direction() == SG_IO_OUT ) {
	gen_message();
	if ( ! io->write( buf, length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	SG_LOG( SG_IO, SG_ALERT, "in direction not supported for RAY." );
	return false;
    }

    return true;
}
