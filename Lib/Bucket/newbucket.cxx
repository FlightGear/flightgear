/**************************************************************************
 * newbucket.hxx -- new bucket routines for better world modeling
 *
 * Written by Curtis L. Olson, started February 1999.
 *
 * Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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
 * $Id$
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include <Misc/fgpath.hxx>

#include "newbucket.hxx"


// Build the path name for this bucket
string FGBucket::gen_base_path() const {
    // long int index;
    int top_lon, top_lat, main_lon, main_lat;
    char hem, pole;
    char raw_path[256];

    top_lon = lon / 10;
    main_lon = lon;
    if ( (lon < 0) && (top_lon * 10 != lon) ) {
	top_lon -= 1;
    }
    top_lon *= 10;
    if ( top_lon >= 0 ) {
	hem = 'e';
    } else {
	hem = 'w';
	top_lon *= -1;
    }
    if ( main_lon < 0 ) {
	main_lon *= -1;
    }
    
    top_lat = lat / 10;
    main_lat = lat;
    if ( (lat < 0) && (top_lat * 10 != lat) ) {
	top_lat -= 1;
    }
    top_lat *= 10;
    if ( top_lat >= 0 ) {
	pole = 'n';
    } else {
	pole = 's';
	top_lat *= -1;
    }
    if ( main_lat < 0 ) {
	main_lat *= -1;
    }

    sprintf(raw_path, "%c%03d%c%02d/%c%03d%c%02d", 
	    hem, top_lon, pole, top_lat, 
	    hem, main_lon, pole, main_lat);

    FGPath path( raw_path );

    return path.str();
}


// find the bucket which is offset by the specified tile units in the
// X & Y direction.  We need the current lon and lat to resolve
// ambiguities when going from a wider tile to a narrower one above or
// below.  This assumes that we are feeding in
FGBucket fgBucketOffset( double dlon, double dlat, int dx, int dy ) {
    FGBucket result( dlon, dlat );
    double clat = result.get_center_lat() + dy * FG_BUCKET_SPAN;

    // walk dy units in the lat direction
    result.set_bucket( dlon, clat );

    // find the lon span for the new latitude
    double span = bucket_span( clat );

    // walk dx units in the lon direction
    result.set_bucket( dlon + dx * span, clat );

    return result;
}


// calculate the offset between two buckets
void fgBucketDiff( const FGBucket& b1, const FGBucket& b2, int *dx, int *dy ) {

    // Latitude difference
    double c1_lat = b1.get_center_lat();
    double c2_lat = b2.get_center_lat();
    double diff_lat = c2_lat - c1_lat;

#ifdef HAVE_RINT
    *dy = (int)rint( diff_lat / FG_BUCKET_SPAN );
#else
    if ( diff_lat > 0 ) {
	*dy = (int)( diff_lat / FG_BUCKET_SPAN + 0.5 );
    } else {
	*dy = (int)( diff_lat / FG_BUCKET_SPAN - 0.5 );
    }
#endif

    // longitude difference
    double c1_lon = b1.get_center_lon();
    double c2_lon = b2.get_center_lon();
    double diff_lon = c2_lon - c1_lon;
    double span;
    if ( bucket_span(c1_lat) <= bucket_span(c2_lat) ) {
	span = bucket_span(c1_lat);
    } else {
	span = bucket_span(c2_lat);
    }

#ifdef HAVE_RINT
    *dx = (int)rint( diff_lon / span );
#else
    if ( diff_lon > 0 ) {
	*dx = (int)( diff_lon / span + 0.5 );
    } else {
	*dx = (int)( diff_lon / span - 0.5 );
    }
#endif
}


