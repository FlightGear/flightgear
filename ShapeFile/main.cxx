// main.cxx -- process shapefiles and extract polygon outlines,
//             clipping against and sorting them into the revelant
//             tiles.
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)
 

// Include Geographic Foundation Classes library

// libgfc.a includes need this bit o' strangeness
#if defined ( linux )
#  define _LINUX_
#endif

#include <gfc/gadt_polygon.h>
#include <gfc/gdbf.h>
#include <gfc/gshapefile.h>

#undef E
#undef DEG_TO_RAD
#undef RAD_TO_DEG

// include Generic Polygon Clipping Library
extern "C" {
#include <gpc.h>
}

#include <Bucket/newbucket.hxx>
#include <Debug/logstream.hxx>


class point2d {
public:
    double x, y;
};


static void clip_and_write_poly( FGBucket b, int n_vertices, double *coords ) {
    point2d c, min, max;
    c.x = b.get_center_lon();
    c.y = b.get_center_lat();
    double span = bucket_span(c.y);

    // calculate bucket dimensions
    if ( (c.y >= -89.0) && (c.y < 89.0) ) {
	min.x = c.x - span / 2.0;
	max.x = c.x + span / 2.0;
	min.y = c.y - FG_HALF_BUCKET_SPAN;
	max.y = c.y + FG_HALF_BUCKET_SPAN;
    } else if ( c.y < -89.0) {
	min.x = -90.0;
	max.x = -89.0;
	min.y = -180.0;
	max.y = 180.0;
    } else if ( c.y >= 89.0) {
	min.x = 89.0;
	max.x = 90.0;
	min.y = -180.0;
	max.y = 180.0;
    } else {
	FG_LOG ( FG_GENERAL, FG_ALERT, 
		 "Out of range latitude in clip_and_write_poly() = " << c.y );
    }

    
}


int main( int argc, char **argv ) {
    point2d min, max;

    fglog().setLogLevels( FG_ALL, FG_DEBUG );

    if ( argc != 2 ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Usage: " << argv[0] << " <shapefile>" );
	exit(-1);
    }

    FG_LOG( FG_GENERAL, FG_DEBUG, "Opening " << argv[1] << " for reading." );

    GShapeFile * sf = new GShapeFile( argv[1] );
    GDBFile *names = new GDBFile( argv[1] );

    GPolygon shape;
    double  *coords; // in decimal degrees
    int	n_vertices;
    double lon, lat;

    FG_LOG( FG_GENERAL, FG_INFO, sf->numRecords() );

    GShapeFile::ShapeType t = sf->shapeType();
    if ( t != GShapeFile::av_Polygon ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Can't handle non-polygon shape files" );
	exit(-1);
    }

    for ( int i = 0; i < sf->numRecords(); i++ ) {
	//fetch i-th record (shape)
	FG_LOG( FG_GENERAL, FG_DEBUG, names->getRecord( i ) );

	sf->getShapeRec(i, &shape); 

	FG_LOG( FG_GENERAL, FG_DEBUG, "Record = " << i << "  rings = " 
		<< shape.numRings() );

	for ( int j = 0; j < shape.numRings(); j++ ) {
	    //return j-th branch's coords, # of vertices
	    n_vertices = shape.getRing(j, coords);

	    FG_LOG( FG_GENERAL, FG_DEBUG, "  ring " << j << " = " );
	    FG_LOG( FG_GENERAL, FG_INFO, n_vertices );

	    // find min/max of this polygon
	    min.x = min.y = 200.0;
	    max.x = max.y = -200.0;
	    for ( int k = 0; k < n_vertices; k++ ) {
		if ( coords[k*2+0] < min.x ) { min.x = coords[k*2+0]; }
		if ( coords[k*2+1] < min.y ) { min.y = coords[k*2+1]; }
		if ( coords[k*2+0] > max.x ) { max.x = coords[k*2+0]; }
		if ( coords[k*2+1] > max.y ) { max.y = coords[k*2+1]; }
	    }
	    FG_LOG( FG_GENERAL, FG_INFO, "min = " << min.x << "," << min.y
		    << " max = " << max.x << "," << max.y );

	    // find buckets for min, and max points of convex hull.
	    // note to self: self, you should think about checking for
	    // polygons that span the date line
	    FGBucket b_min(min.x, min.y);
	    FGBucket b_max(max.x, max.y);
	    cout << "Bucket min = " << b_min << endl;
	    cout << "Bucket max = " << b_max << endl;
	    
	    if ( b_min == b_max ) {
		clip_and_write_poly( b_min, n_vertices, coords );
	    } else {
		FGBucket b_cur;
		int dx, dy, i, j;

		fgBucketDiff(b_min, b_max, &dx, &dy);
		cout << "airport spans tile boundaries" << endl;
		cout << "  dx = " << dx << "  dy = " << dy << endl;

		if ( (dx > 1) || (dy > 1) ) {
		    cout << "somethings really wrong!!!!" << endl;
		    exit(-1);
		}

		for ( j = 0; j <= dy; j++ ) {
		    for ( i = 0; i <= dx; i++ ) {
			b_cur = fgBucketOffset(min.x, min.y, i, j);
			clip_and_write_poly( b_cur, n_vertices, coords );
		    }
		}
		// string answer; cin >> answer;
	    }

	    for ( int k = 0; k < n_vertices; k++ ) {
		lon = coords[k*2+0];
		lat = coords[k*2+1];
		FG_LOG( FG_GENERAL, FG_INFO, lon << " " << lat );
	    }
	}
    }

    return 0;
}

// $Log$
// Revision 1.2  1999/02/19 19:05:18  curt
// Working on clipping shapes and distributing into buckets.
//
// Revision 1.1  1999/02/15 19:10:23  curt
// Initial revision.
//
