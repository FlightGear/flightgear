// morse.cxx -- Morse code generation class
//
// Written by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - curt@flightgear.org
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


#include "morse.hxx"


// constructor
FGMorse::FGMorse() {
}

// destructor
FGMorse::~FGMorse() {
}


// allocate and initialize sound samples
bool FGMorse::init() {
    int i, j;

    // Make DIT
    unsigned char dit[ DIT_SIZE ] ;
    for ( i = 0; i < TRANSITION_BYTES; ++i ) {
	float level = ( sin( (double) i * 2.0 * M_PI / (8000.0 / FREQUENCY)) )
	    * ((double)i / TRANSITION_BYTES) / 2.0 + 0.5;

	/* Convert to unsigned byte */
	dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = TRANSITION_BYTES;
	  i < DIT_SIZE - TRANSITION_BYTES - COUNT_SIZE; ++i ) {
	float level = ( sin( (double) i * 2.0 * M_PI / (8000.0 / FREQUENCY) ) )
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = TRANSITION_BYTES;
    for ( i = DIT_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  i < DIT_SIZE - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * M_PI / (8000.0 / FREQUENCY) ) )
	    * ((double)j / TRANSITION_BYTES) / 2.0 + 0.5;
	--j;

	/* Convert to unsigned byte */
	dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( i = DIT_SIZE - COUNT_SIZE; i < DIT_SIZE; ++i ) {
	dit[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }

    // Make DAH
    unsigned char dah[ DAH_SIZE ] ;
    for ( i = 0; i < TRANSITION_BYTES; ++i ) {
	float level = ( sin( (double) i * 2.0 * M_PI / (8000.0 / FREQUENCY) ) )
	    * ((double)i / TRANSITION_BYTES) / 2.0 + 0.5;

	/* Convert to unsigned byte */
	dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = TRANSITION_BYTES;
	  i < DAH_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * M_PI / (8000.0 / FREQUENCY) ) )
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = TRANSITION_BYTES;
    for ( int i = DAH_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  i < DAH_SIZE - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * M_PI / (8000.0 / FREQUENCY) ) )
	    * ((double)j / TRANSITION_BYTES) / 2.0 + 0.5;
	--j;

	/* Convert to unsigned byte */
	dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( int i = DAH_SIZE - COUNT_SIZE; i < DAH_SIZE; ++i ) {
	dah[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }

    // Make SPACE
    for ( int i = 0; i < SPACE_SIZE; ++i ) {
	space[ i ] = (unsigned char) ( 0.5 * 255 ) ;
    }

    return true;
}
