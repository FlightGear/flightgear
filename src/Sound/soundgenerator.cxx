// soundgenerator.cxx -- simple sound generation 
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

#include "soundgenerator.hxx"
#include <simgear/constants.h>

FGSoundGenerator::~FGSoundGenerator()
{
}

// Make a tone of specified freq and total_len with trans_len ramp in
// and out and only the first len bytes with sound, the rest with
// silence
void FGSoundGenerator::make_tone( unsigned char *buf, int freq, 
		int len, int total_len, int trans_len )
{
    int i, j;

    for ( i = 0; i < trans_len; ++i ) {
        float level = ( sin( (double) i * SGD_2PI / (BYTES_PER_SECOND / freq) ) )
            * ((double)i / trans_len) / 2.0 + 0.5;

        /* Convert to unsigned byte */
        buf[ i ] = (unsigned char) ( level * 255.0 ) ;
    }

    for ( i = trans_len; i < len - trans_len; ++i ) {
        float level = ( sin( (double) i * SGD_2PI / (BYTES_PER_SECOND / freq) ) )
            / 2.0 + 0.5;

        /* Convert to unsigned byte */
        buf[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    j = trans_len;
    for ( i = len - trans_len; i < len; ++i ) {
        float level = ( sin( (double) i * SGD_2PI / (BYTES_PER_SECOND / freq) ) )
            * ((double)j / trans_len) / 2.0 + 0.5;
        --j;

        /* Convert to unsigned byte */
        buf[ i ] = (unsigned char) ( level * 255.0 ) ;
    }
    for ( i = len; i < total_len; ++i ) {
        buf[ i ] = (unsigned char) ( 0.5 * 255.0 ) ;
    }
}
