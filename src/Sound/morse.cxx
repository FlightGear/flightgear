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


#include <simgear/constants.h>

#include "morse.hxx"


static const char alphabet[26][4] = {
    { DI, DAH, end, end },	/* A */ 
    { DA, DI, DI, DIT },	/* B */ 
    { DA, DI, DA, DIT },	/* C */ 
    { DA, DI, DIT, end },	/* D */ 
    { DIT, end, end, end },	/* E */ 
    { DI, DI, DA, DIT },	/* F */ 
    { DA, DA, DIT, end },	/* G */ 
    { DI, DI, DI, DIT },	/* H */ 
    { DI, DIT, end, end },	/* I */ 
    { DI, DA, DA, DAH },	/* J */ 
    { DA, DI, DAH, end },	/* K */ 
    { DI, DA, DI, DIT },	/* L */ 
    { DA, DAH, end, end },	/* M */ 
    { DA, DIT, end, end },	/* N */ 
    { DA, DA, DAH, end },	/* O */ 
    { DI, DA, DA, DIT },	/* P */ 
    { DA, DA, DI, DAH },	/* Q */ 
    { DI, DA, DIT, end },	/* R */ 
    { DI, DI, DIT, end },	/* S */ 
    { DAH, end, end, end },	/* T */ 
    { DI, DI, DAH, end },	/* U */ 
    { DI, DI, DI, DAH },	/* V */ 
    { DI, DA, DAH, end },	/* W */ 
    { DA, DI, DI, DAH },	/* X */ 
    { DA, DI, DA, DAH },	/* Y */ 
    { DA, DA, DI, DIT }		/* Z */ 
};


// constructor
FGMorse::FGMorse() {
}

// destructor
FGMorse::~FGMorse() {
}


// allocate and initialize sound samples
bool FGMorse::init() {
    int i, j;

    // Make Low DIT
    for ( i = 0; i < TRANSITION_BYTES; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
                             / (8000.0 / LO_FREQUENCY) ) )
	    * ((double)i / TRANSITION_BYTES)
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	lo_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = TRANSITION_BYTES;
	  i < DIT_SIZE - TRANSITION_BYTES - COUNT_SIZE; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / LO_FREQUENCY) ) )
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	lo_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = TRANSITION_BYTES;
    for ( i = DIT_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  i < DIT_SIZE - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / LO_FREQUENCY) ) )
	    * ((double)j / TRANSITION_BYTES) / 2.0 + 0.5;
	--j;

	/* Convert to unsigned byte */
	lo_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( i = DIT_SIZE - COUNT_SIZE; i < DIT_SIZE; ++i ) {
	lo_dit[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }

    // Make High DIT
    for ( i = 0; i < TRANSITION_BYTES; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / HI_FREQUENCY)) )
	    * ((double)i / TRANSITION_BYTES) / 2.0 + 0.5;

	/* Convert to unsigned byte */
	hi_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = TRANSITION_BYTES;
	  i < DIT_SIZE - TRANSITION_BYTES - COUNT_SIZE; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / HI_FREQUENCY) ) )
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	hi_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = TRANSITION_BYTES;
    for ( i = DIT_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  i < DIT_SIZE - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / HI_FREQUENCY) ) )
	    * ((double)j / TRANSITION_BYTES) / 2.0 + 0.5;
	--j;

	/* Convert to unsigned byte */
	hi_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( i = DIT_SIZE - COUNT_SIZE; i < DIT_SIZE; ++i ) {
	hi_dit[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }

    // Make Low DAH
    for ( i = 0; i < TRANSITION_BYTES; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / LO_FREQUENCY) ) )
	    * ((double)i / TRANSITION_BYTES) / 2.0 + 0.5;

	/* Convert to unsigned byte */
	lo_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = TRANSITION_BYTES;
	  i < DAH_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / LO_FREQUENCY) ) )
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	lo_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = TRANSITION_BYTES;
    for ( i = DAH_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  i < DAH_SIZE - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / LO_FREQUENCY) ) )
	    * ((double)j / TRANSITION_BYTES) / 2.0 + 0.5;
	--j;

	/* Convert to unsigned byte */
	lo_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( i = DAH_SIZE - COUNT_SIZE; i < DAH_SIZE; ++i ) {
	lo_dah[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }

    // Make High DAH
    for ( i = 0; i < TRANSITION_BYTES; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / HI_FREQUENCY) ) )
	    * ((double)i / TRANSITION_BYTES) / 2.0 + 0.5;

	/* Convert to unsigned byte */
	hi_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = TRANSITION_BYTES;
	  i < DAH_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / HI_FREQUENCY) ) )
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	hi_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = TRANSITION_BYTES;
    for ( i = DAH_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  i < DAH_SIZE - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI
			     / (8000.0 / HI_FREQUENCY) ) )
	    * ((double)j / TRANSITION_BYTES) / 2.0 + 0.5;
	--j;

	/* Convert to unsigned byte */
	hi_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( i = DAH_SIZE - COUNT_SIZE; i < DAH_SIZE; ++i ) {
	hi_dah[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }

    // Make SPACE
    for ( i = 0; i < SPACE_SIZE; ++i ) {
	space[ i ] = (unsigned char) ( 0.5 * 255 ) ;
    }

    return true;
}


// allocate and initialize sound samples
bool FGMorse::cust_init(const int freq ) {
    int i, j;

    // Make DIT
    for ( i = 0; i < TRANSITION_BYTES; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI / (8000.0 / freq)) )
	    * ((double)i / TRANSITION_BYTES) / 2.0 + 0.5;

	/* Convert to unsigned byte */
	cust_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = TRANSITION_BYTES;
	  i < DIT_SIZE - TRANSITION_BYTES - COUNT_SIZE; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI / (8000.0 / freq) ) )
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	cust_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = TRANSITION_BYTES;
    for ( i = DIT_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  i < DIT_SIZE - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI / (8000.0 / freq) ) )
	    * ((double)j / TRANSITION_BYTES) / 2.0 + 0.5;
	--j;

	/* Convert to unsigned byte */
	cust_dit[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( i = DIT_SIZE - COUNT_SIZE; i < DIT_SIZE; ++i ) {
	cust_dit[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }

    // Make DAH
    for ( i = 0; i < TRANSITION_BYTES; ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI / (8000.0 / freq) ) )
	    * ((double)i / TRANSITION_BYTES) / 2.0 + 0.5;

	/* Convert to unsigned byte */
	cust_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = TRANSITION_BYTES;
	  i < DAH_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI / (8000.0 / freq) ) )
	    / 2.0 + 0.5;

	/* Convert to unsigned byte */
	cust_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = TRANSITION_BYTES;
    for ( i = DAH_SIZE - TRANSITION_BYTES - COUNT_SIZE;
	  i < DAH_SIZE - COUNT_SIZE;
	  ++i ) {
	float level = ( sin( (double) i * 2.0 * FG_PI / (8000.0 / freq) ) )
	    * ((double)j / TRANSITION_BYTES) / 2.0 + 0.5;
	--j;

	/* Convert to unsigned byte */
	cust_dah[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( i = DAH_SIZE - COUNT_SIZE; i < DAH_SIZE; ++i ) {
	cust_dah[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }

    // Make SPACE
    for ( i = 0; i < SPACE_SIZE; ++i ) {
	space[ i ] = (unsigned char) ( 0.5 * 255 ) ;
    }

    return true;
}


// make a FGSimpleSound morse code transmission for the specified string
FGSimpleSound *FGMorse::make_ident( const string& id, const int freq ) {
    char *idptr = (char *)id.c_str();

    int length = 0;
    int i, j;

    // 0. Select the frequency.  If custom frequency, generate the
    // sound fragments we need on the fly.
    unsigned char *dit_ptr, *dah_ptr;

    if ( freq == LO_FREQUENCY ) {
	dit_ptr = lo_dit;
	dah_ptr = lo_dah;
    } else if ( freq == HI_FREQUENCY ) {
	dit_ptr = hi_dit;
	dah_ptr = hi_dah;
    } else {
	cust_init( freq );
	dit_ptr = cust_dit;
	dah_ptr = cust_dah;
    }

    // 1. Determine byte length of message
    for ( i = 0; i < (int)id.length(); ++i ) {
	if ( idptr[i] >= 'A' && idptr[i] <= 'Z' ) {
	    char c = idptr[i] - 'A';
	    for ( j = 0; j < 4 || alphabet[c][j] == end; ++j ) {
		if ( alphabet[c][j] == DIT ) {
		    length += DIT_SIZE;
		} else if ( alphabet[c][j] == DAH ) {
		    length += DAH_SIZE;
		}
	    }
	    length += SPACE_SIZE;
	} else {
	    // skip unknown character
	}
    }
    // add 2x more space to the end of the string
    length += 2 * SPACE_SIZE;

    // 2. Allocate space for the message
    unsigned char *buffer = new unsigned char[length];

    // 3. Assemble the message;
    unsigned char *buf_ptr = buffer;

    for ( i = 0; i < (int)id.length(); ++i ) {
	if ( idptr[i] >= 'A' && idptr[i] <= 'Z' ) {
	    char c = idptr[i] - 'A';
	    for ( j = 0; j < 4 || alphabet[c][j] == end; ++j ) {
		if ( alphabet[c][j] == DIT ) {
		    memcpy( buf_ptr, dit_ptr, DIT_SIZE );
		    buf_ptr += DIT_SIZE;
		} else if ( alphabet[c][j] == DAH ) {
		    memcpy( buf_ptr, dah_ptr, DAH_SIZE );
		    buf_ptr += DAH_SIZE;
		}
	    }
	    memcpy( buf_ptr, space, SPACE_SIZE );
	    buf_ptr += SPACE_SIZE;
	} else {
	    // skip unknown character
	}
    }
    memcpy( buf_ptr, space, SPACE_SIZE );
    buf_ptr += SPACE_SIZE;
    memcpy( buf_ptr, space, SPACE_SIZE );
    buf_ptr += SPACE_SIZE;

    // 4. create the simple sound and return
    FGSimpleSound *sample = new FGSimpleSound( buffer, length );

    return sample;
}
