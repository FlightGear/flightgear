// morse.cxx -- Morse code generation class
//
// Written by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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


#include <simgear/constants.h>

#include "morse.hxx"

#include <cstring>

static const char DI = '1';
static const char DIT = '1';
static const char DA = '2';
static const char DAH = '2';
static const char END = '0';

static const char alphabet[26][4] = {
    { DI, DAH, END, END },	/* A */ 
    { DA, DI, DI, DIT },	/* B */ 
    { DA, DI, DA, DIT },	/* C */ 
    { DA, DI, DIT, END },	/* D */ 
    { DIT, END, END, END },	/* E */ 
    { DI, DI, DA, DIT },	/* F */ 
    { DA, DA, DIT, END },	/* G */ 
    { DI, DI, DI, DIT },	/* H */ 
    { DI, DIT, END, END },	/* I */ 
    { DI, DA, DA, DAH },	/* J */ 
    { DA, DI, DAH, END },	/* K */ 
    { DI, DA, DI, DIT },	/* L */ 
    { DA, DAH, END, END },	/* M */ 
    { DA, DIT, END, END },	/* N */ 
    { DA, DA, DAH, END },	/* O */ 
    { DI, DA, DA, DIT },	/* P */ 
    { DA, DA, DI, DAH },	/* Q */ 
    { DI, DA, DIT, END },	/* R */ 
    { DI, DI, DIT, END },	/* S */ 
    { DAH, END, END, END },	/* T */ 
    { DI, DI, DAH, END },	/* U */ 
    { DI, DI, DI, DAH },	/* V */ 
    { DI, DA, DAH, END },	/* W */ 
    { DA, DI, DI, DAH },	/* X */ 
    { DA, DI, DA, DAH },	/* Y */ 
    { DA, DA, DI, DIT }		/* Z */ 
};

static const char numerals[10][5] = {
    { DA, DA, DA, DA, DAH },    // 0
    { DI, DA, DA, DA, DAH },    // 1
    { DI, DI, DA, DA, DAH },    // 2
    { DI, DI, DI, DA, DAH },    // 3
    { DI, DI, DI, DI, DAH },    // 4
    { DI, DI, DI, DI, DIT },    // 5
    { DA, DI, DI, DI, DIT },    // 6
    { DA, DA, DI, DI, DIT },    // 7
    { DA, DA, DA, DI, DIT },    // 8
    { DA, DA, DA, DA, DIT }     // 9
};


// constructor
FGMorse::FGMorse() {
}

// destructor
FGMorse::~FGMorse() {
}


// allocate and initialize sound samples
bool FGMorse::init() {
    // Make Low DIT
    make_tone( lo_dit, LO_FREQUENCY, DIT_SIZE - COUNT_SIZE, DIT_SIZE,
	       TRANSITION_BYTES );

    // Make High DIT
    make_tone( hi_dit, HI_FREQUENCY, DIT_SIZE - COUNT_SIZE, DIT_SIZE,
	       TRANSITION_BYTES );

    // Make Low DAH
    make_tone( lo_dah, LO_FREQUENCY, DAH_SIZE - COUNT_SIZE, DAH_SIZE,
	       TRANSITION_BYTES );

    // Make High DAH
    make_tone( hi_dah, HI_FREQUENCY, DAH_SIZE - COUNT_SIZE, DAH_SIZE,
	       TRANSITION_BYTES );

    // Make SPACE
    int i;
    for ( i = 0; i < SPACE_SIZE; ++i ) {
	space[ i ] = (unsigned char) ( 0.5 * 255 ) ;
    }

    return true;
}


// allocate and initialize sound samples
bool FGMorse::cust_init(const int freq ) {
    int i;

    // Make DIT
    make_tone( cust_dit, freq, DIT_SIZE - COUNT_SIZE, DIT_SIZE,
	       TRANSITION_BYTES );

    // Make DAH
    make_tone( cust_dah, freq, DAH_SIZE - COUNT_SIZE, DAH_SIZE,
	       TRANSITION_BYTES );

    // Make SPACE
    for ( i = 0; i < SPACE_SIZE; ++i ) {
	space[ i ] = (unsigned char) ( 0.5 * 255 ) ;
    }

    return true;
}


// make a SGSoundSample morse code transmission for the specified string
SGSoundSample *FGMorse::make_ident( const string& id, const int freq ) {

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
	    int c = (int)(idptr[i] - 'A');
	    for ( j = 0; j < 4 && alphabet[c][j] != END; ++j ) {
		if ( alphabet[c][j] == DIT ) {
		    length += DIT_SIZE;
		} else if ( alphabet[c][j] == DAH ) {
		    length += DAH_SIZE;
		}
	    }
	    length += SPACE_SIZE;
        } else if ( idptr[i] >= '0' && idptr[i] <= '9' ) {
            int c = (int)(idptr[i] - '0');
            for ( j = 0; j < 5; ++j) {
                if ( numerals[c][j] == DIT ) {
                    length += DIT_SIZE;
                } else if ( numerals[c][j] == DAH ) {
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
    const unsigned char* buffer = (const unsigned char *)malloc(length);

    // 3. Assemble the message;
    unsigned char *buf_ptr = (unsigned char*)buffer;

    for ( i = 0; i < (int)id.length(); ++i ) {
	if ( idptr[i] >= 'A' && idptr[i] <= 'Z' ) {
	    int c = (int)(idptr[i] - 'A');
	    for ( j = 0; j < 4 && alphabet[c][j] != END; ++j ) {
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
        } else if ( idptr[i] >= '0' && idptr[i] <= '9' ) {
            int c = (int)(idptr[i] - '0');
            for ( j = 0; j < 5; ++j ) {
                if ( numerals[c][j] == DIT ) {
                    memcpy( buf_ptr, dit_ptr, DIT_SIZE );
                    buf_ptr += DIT_SIZE;
                } else if ( numerals[c][j] == DAH ) {
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
    SGSoundSample *sample = new SGSoundSample( &buffer, length,
                                               BYTES_PER_SECOND );

    sample->set_reference_dist( 10.0 );
    sample->set_max_dist( 20.0 );

    return sample;
}

FGMorse * FGMorse::_instance = NULL;

FGMorse * FGMorse::instance()
{
    if( _instance == NULL ) {
        _instance = new FGMorse();
        _instance->init();
    }
    return _instance;
}
