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


#ifndef _NEWBUCKET_HXX
#define _NEWBUCKET_HXX

#include <Include/compiler.h>

#include STL_STRING

#ifdef FG_HAVE_STD_INCLUDES
#  include <cstdio> // sprintf()
#  include <iostream>
#else
#  include <stdio.h> // sprintf()
#  include <iostream.h>
#endif

FG_USING_STD(string);
FG_USING_STD(ostream);

#include <Include/fg_constants.h>


#define FG_BUCKET_SPAN      0.125   // 1/8 of a degree
#define FG_HALF_BUCKET_SPAN 0.0625  // 1/2 of 1/8 of a degree = 1/16 = 0.0625

class FGBucket;
ostream& operator<< ( ostream&, const FGBucket& );
bool operator== ( const FGBucket&, const FGBucket& );

class FGBucket {

private:
    double cx, cy;  // centerpoint (lon, lat) in degrees of bucket
    int lon;        // longitude index (-180 to 179)
    int lat;        // latitude index (-90 to 89)
    int x;          // x subdivision (0 to 7)
    int y;          // y subdivision (0 to 7)

public:
    
    // default constructor
    FGBucket();

    // create a bucket which would contain the specified lon/lat
    FGBucket(const double lon, const double lat);

    // create a bucket based on "long int" index
    FGBucket(const long int bindex);

    // create an impossible bucket if false
    FGBucket(const bool is_good);

    ~FGBucket();

    // Set the bucket params for the specified lat and lon
    void set_bucket( double dlon, double dlat );

    // Generate the unique scenery tile index for this bucket
    long int gen_index();
    string gen_index_str() const;

    // Build the path name for this bucket
    string gen_base_path() const;

    // return the center lon of a tile
    double get_center_lon() const;

    // return width of the tile
    double get_width() const;

    // return the center lat of a tile
    double get_center_lat() const;

    // return height of the tile
    double get_height() const;

    // Informational methods
    inline int get_lon() const { return lon; }
    inline int get_lat() const { return lat; }
    inline int get_x() const { return x; }
    inline int get_y() const { return y; }

    // friends
    friend ostream& operator<< ( ostream&, const FGBucket& );
    friend bool operator== ( const FGBucket&, const FGBucket& );
};


// return the horizontal tile span factor based on latitude
inline double bucket_span( double l ) {
    if ( l >= 89.0 ) {
	return 0.0;
    } else if ( l >= 88.0 ) {
	return 8.0;
    } else if ( l >= 86.0 ) {
	return 4.0;
    } else if ( l >= 83.0 ) {
	return 2.0;
    } else if ( l >= 76.0 ) {
	return 1.0;
    } else if ( l >= 62.0 ) {
	return 0.5;
    } else if ( l >= 22.0 ) {
	return 0.25;
    } else if ( l >= -22.0 ) {
	return 0.125;
    } else if ( l >= -62.0 ) {
	return 0.25;
    } else if ( l >= -76.0 ) {
	return 0.5;
    } else if ( l >= -83.0 ) {
	return 1.0;
    } else if ( l >= -86.0 ) {
	return 2.0;
    } else if ( l >= -88.0 ) {
	return 4.0;
    } else if ( l >= -89.0 ) {
	return 8.0;
    } else {
	return 0.0;
    }
}


// Set the bucket params for the specified lat and lon
inline void FGBucket::set_bucket( double dlon, double dlat ) {
    //
    // latitude first
    //
    double span = bucket_span( dlat );
    double diff = dlon - (double)(int)dlon;

    // cout << "diff = " << diff << "  span = " << span << endl;

    if ( (dlon >= 0) || (fabs(diff) < FG_EPSILON) ) {
	lon = (int)dlon;
    } else {
	lon = (int)dlon - 1;
    }

    // find subdivision or super lon if needed
    if ( span < FG_EPSILON ) {
	// polar cap
	lon = 0;
	x = 0;
    } else if ( span <= 1.0 ) {
	x = (int)((dlon - lon) / span);
    } else {
	if ( (dlon >= 0) || (fabs(diff) < FG_EPSILON) ) {
	    lon = (int)( (int)(lon / span) * span);
	} else {
	    // cout << " lon = " << lon 
	    //  << "  tmp = " << (int)((lon-1) / span) << endl;
	    lon = (int)( (int)((lon + 1) / span) * span - span);
	    if ( lon < -180 ) {
		lon = -180;
	    }
	}
	x = 0;
    }

    //
    // then latitude
    //
    diff = dlat - (double)(int)dlat;

    if ( (dlat >= 0) || (fabs(diff) < FG_EPSILON) ) {
	lat = (int)dlat;
    } else {
	lat = (int)dlat - 1;
    }
    y = (int)((dlat - lat) * 8);
}


// default constructor
inline FGBucket::FGBucket() {}


// constructor for specified location
inline FGBucket::FGBucket(const double dlon, const double dlat) {
    set_bucket(dlon, dlat);
}

// create an impossible bucket if false
inline FGBucket::FGBucket(const bool is_good) {
    set_bucket(0.0, 0.0);
    if ( !is_good ) {
	lon = -1000;
    }
}
    

// Parse a unique scenery tile index and find the lon, lat, x, and y
inline FGBucket::FGBucket(const long int bindex) {
    long int index = bindex;

    lon = index >> 14;
    index -= lon << 14;
    lon -= 180;
    
    lat = index >> 6;
    index -= lat << 6;
    lat -= 90;
    
    y = index >> 3;
    index -= y << 3;

    x = index;
}


// default destructor
inline FGBucket::~FGBucket() {}


// Generate the unique scenery tile index for this bucket
// 
// The index is constructed as follows:
// 
// 9 bits - to represent 360 degrees of longitude (-180 to 179)
// 8 bits - to represent 180 degrees of latitude (-90 to 89)
//
// Each 1 degree by 1 degree tile is further broken down into an 8x8
// grid.  So we also need:
//
// 3 bits - to represent x (0 to 7)
// 3 bits - to represent y (0 to 7)

inline long int FGBucket::gen_index() {
    return ((lon + 180) << 14) + ((lat + 90) << 6) + (y << 3) + x;
}

inline string FGBucket::gen_index_str() const {
    char tmp[20];
    sprintf(tmp, "%ld", 
	    (((long)lon + 180) << 14) + ((lat + 90) << 6) + (y << 3) + x);
    return (string)tmp;
}


// return the center lon of a tile
inline double FGBucket::get_center_lon() const {
    double span = bucket_span( lat + y / 8.0 + FG_HALF_BUCKET_SPAN );

    if ( span >= 1.0 ) {
	return lon + span / 2.0;
    } else {
	return lon + x * span + span / 2.0;
    }
}


// return width of the tile
inline double FGBucket::get_width() const {
    return bucket_span( get_center_lat() );
}


// return the center lat of a tile
inline double FGBucket::get_center_lat() const {
    return lat + y / 8.0 + FG_HALF_BUCKET_SPAN;
}


// return height of the tile
inline double FGBucket::get_height() const {
    return FG_BUCKET_SPAN;
}


// offset a bucket struct by the specified tile units in the X & Y
// direction
FGBucket fgBucketOffset( double dlon, double dlat, int x, int y );


// calculate the offset between two buckets
void fgBucketDiff( const FGBucket& b1, const FGBucket& b2, int *dx, int *dy );


/*
// Given a lat/lon, fill in the local tile index array
void fgBucketGenIdxArray(fgBUCKET *p1, fgBUCKET *tiles, int width, int height);
*/


inline ostream&
operator<< ( ostream& out, const FGBucket& b )
{
    return out << b.lon << ":" << b.x << ", " << b.lat << ":" << b.y;
}


inline bool
operator== ( const FGBucket& b1, const FGBucket& b2 )
{
    return ( b1.lon == b2.lon &&
	     b1.lat == b2.lat &&
	     b1.x == b2.x &&
	     b1.y == b2.y );
}


#endif // _NEWBUCKET_HXX


// $Log$
// Revision 1.1  1999/04/05 21:32:34  curt
// Initial revision
//
// Revision 1.8  1999/03/27 05:34:06  curt
// Elimitated some const warnings from the compiler.
//
// Revision 1.7  1999/03/25 19:01:51  curt
// Jettisoned old bucketutils.[ch] for newbucket.[ch]xx
//
// Revision 1.6  1999/03/15 17:58:41  curt
// MSVC++ portability tweaks contributed by Bernie Bright.
//   Added using std::ostream declaration.
//   Added forward declarations to work around a MSVC bug.
//
// Revision 1.5  1999/03/12 22:51:18  curt
// Added some informational methods.
//
// Revision 1.4  1999/03/02 01:01:43  curt
// Tweaks for compiling with native SGI compilers.
//
// Revision 1.3  1999/02/26 22:07:55  curt
// Added initial support for native SGI compilers.
//
// Revision 1.2  1999/02/11 01:09:34  curt
// Added a routine to calculate the offset in bucket units between two buckets.
//
// Revision 1.1  1999/02/08 23:52:16  curt
// Added a new "newbucket.[ch]xx" FGBucket class to replace the old
// fgBUCKET struct and C routines.  This FGBucket class adjusts the tile
// width towards the poles to ensure the tiles are at least 8 miles wide.
//
