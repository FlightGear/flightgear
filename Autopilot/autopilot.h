/**************************************************************************
 * autopilot.h -- autopilot defines and prototypes (very alpha)
 *
 * Written by Jeff Goeke-Smith, started April 1998.
 *
 * Copyright (C) 1998 Jeff Goeke-Smith  - jgoeke@voyager.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 **************************************************************************/
                       
                       
#ifndef _AUTOPILOT_H
#define _AUTOPILOT_H
                       
#include <Aircraft/aircraft.h>
#include <Flight/flight.h>
#include <Controls/controls.h>
                       
                       

typedef struct {
		int Mode ; // the current mode the AP is operating in
		double Heading; // the heading the AP  should steer to.
		double MaxRoll ; // the max the plane can roll for the turn
		double RollOut;  // when the plane should roll out
				 // measured from Heading
		double MaxAileron; // how far to move the aleroin from center
		double RollOutSmooth; // deg to use for smoothing Aileron Control
		
		} fgAPData, *fgAPDataPtr ;
		

// prototypes
void fgAPInit( fgAIRCRAFT *current_aircraft );
int fgAPRun( void );
void fgAPSetMode( int mode);


#endif
		