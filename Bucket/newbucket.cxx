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
 * (Log is kept at end of this file)
 **************************************************************************/


#include "newbucket.hxx"


// Build the path name for this bucket
string FGBucket::gen_base_path() {
    long int index;
    int top_lon, top_lat, main_lon, main_lat;
    char hem, pole;
    char path[256];

    index = gen_index();

    path[0] = '\0';
	
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

    sprintf(path, "%c%03d%c%02d/%c%03d%c%02d", 
	    hem, top_lon, pole, top_lat,
	    hem, main_lon, pole, main_lat);

    return path;
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


// $Log$
// Revision 1.1  1999/02/08 23:52:16  curt
// Added a new "newbucket.[ch]xx" FGBucket class to replace the old
// fgBUCKET struct and C routines.  This FGBucket class adjusts the tile
// width towards the poles to ensure the tiles are at least 8 miles wide.
//
