// clipper.cxx -- top level routines to take a series of arbitrary areas and
//                produce a tight fitting puzzle pieces that combine to make a
//                tile
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
 


#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Misc/fgstream.hxx>
#include <Polygon/names.hxx>

#include "clipper.hxx"


// Constructor
FGClipper::FGClipper( void ) {
}


// Destructor
FGClipper::~FGClipper( void ) {
}


// Initialize Clipper (allocate and/or connect structures)
bool FGClipper::init() {
    // v_list.num_vertices = 0;
    // v_list.vertex = new gpc_vertex[FG_MAX_VERTICES];;

    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	polys_in.polys[i].clear();
    }

    return true;
}


// Load a polygon definition file
bool FGClipper::load_polys(const string& path) {
    string poly_name;
    AreaType poly_type = DefaultArea;
    int contours, count, i, j;
    int hole_flag;
    double startx, starty, x, y, lastx, lasty;

    FG_LOG( FG_CLIPPER, FG_INFO, "Loading " << path << " ..." );

    fg_gzifstream in( path );

    if ( !in ) {
        FG_LOG( FG_CLIPPER, FG_ALERT, "Cannot open file: " << path );
	exit(-1);
    }

    // gpc_polygon *poly = new gpc_polygon;
    // poly->num_contours = 0;
    // poly->contour = NULL;
    FGPolygon poly;

    Point3D p;
    in >> skipcomment;
    while ( !in.eof() ) {
	in >> poly_name;
	cout << "poly name = " << poly_name << endl;
	poly_type = get_area_type( poly_name );
	cout << "poly type (int) = " << (int)poly_type << endl;
	in >> contours;
	cout << "num contours = " << contours << endl;

	poly.erase();

	for ( i = 0; i < contours; ++i ) {
	    in >> count;

	    if ( count < 3 ) {
		FG_LOG( FG_CLIPPER, FG_ALERT, 
			"Polygon with less than 3 data points." );
		exit(-1);
	    }

	    in >> hole_flag;

	    in >> startx;
	    in >> starty;
	    p = Point3D(startx, starty, 0.0);
	    poly.add_node( i, p );
	    FG_LOG( FG_CLIPPER, FG_BULK, "0 = " 
		    << startx << ", " << starty );

	    for ( j = 1; j < count - 1; ++j ) {
		in >> x;
		in >> y;
		p = Point3D( x, y, 0.0 );
		poly.add_node( i, p );
		FG_LOG( FG_CLIPPER, FG_BULK, j << " = " << x << ", " << y );
	    }

	    in >> lastx;
	    in >> lasty;

	    if ( (fabs(startx - lastx) < FG_EPSILON) 
		 && (fabs(starty - lasty) < FG_EPSILON) ) {
		// last point same as first, discard
	    } else {
		p = Point3D( lastx, lasty, 0.0 );
		poly.add_node( i, p );
		FG_LOG( FG_CLIPPER, FG_BULK, count - 1 << " = " 
			<< lastx << ", " << lasty );
	    }

	    // gpc_add_contour( poly, &v_list, hole_flag );
	}

	in >> skipcomment;
    }

    int area = (int)poly_type;
    if ( area == OceanArea ) {
	// TEST - Ignore
    } else if ( area < FG_MAX_AREA_TYPES ) {
	polys_in.polys[area].push_back(poly);
    } else {
	FG_LOG( FG_CLIPPER, FG_ALERT, "Polygon type out of range = " 
		<< (int)poly_type);
	exit(-1);
    }

    // FILE *ofp= fopen("outfile", "w");
    // gpc_write_polygon(ofp, &polys.landuse);

    return true;
}


// merge any slivers in the specified polygon with larger
// neighboring polygons of higher priorigy
void FGClipper::merge_slivers(FGPolygon& poly) {
    cout << "Begin merge slivers" << endl;
    // traverse each contour of the polygon and attempt to identify
    // likely slivers
    for ( int i = 0; i < poly.contours(); ++i ) {
	cout << "contour " << i << endl;
	for (int j = 0; j < poly.contour_size( i ); ++j ) {
	    // cout << poly->contour[i].vertex[j].x << ","
	    //      << poly->contour[i].vertex[j].y << endl;
	}
    }
}


// Do actually clipping work
bool FGClipper::clip_all(const point2d& min, const point2d& max) {
    FGPolygon accum, result_union, tmp;
    FGPolygon result_diff, remains;
    // gpcpoly_iterator current, last;

    FG_LOG( FG_CLIPPER, FG_INFO, "Running master clipper" );

    accum.erase();

    cout << "  (" << min.x << "," << min.y << ") (" 
	 << max.x << "," << max.y << ")" << endl;

    // set up clipping tile
    polys_in.safety_base.erase();
    polys_in.safety_base.add_node( 0, Point3D(min.x, min.y, 0.0) );
    polys_in.safety_base.add_node( 0, Point3D(max.x, min.y, 0.0) );
    polys_in.safety_base.add_node( 0, Point3D(max.x, max.y, 0.0) );
    polys_in.safety_base.add_node( 0, Point3D(min.x, max.y, 0.0) );

    // int count = 0;
    // process polygons in priority order
    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	cout << "num polys of type (" << i << ") = " 
	     << polys_in.polys[i].size() << endl;
	// current = polys_in.polys[i].begin();
	// last = polys_in.polys[i].end();
	// for ( ; current != last; ++current ) {
	for( int j = 0; j < (int)polys_in.polys[i].size(); ++j ) {
	    FGPolygon current = polys_in.polys[i][j];
	    FG_LOG( FG_CLIPPER, FG_DEBUG, get_area_name( (AreaType)i ) 
		    << " = " << current.contours() );

#ifdef EXTRA_SAFETY_CLIP
	    // clip to base tile
	    tmp = polygon_int( current, polys_in.safety_base );
#else
	    tmp = current;
#endif

	    // clip current polygon against previous higher priority
	    // stuff

	    // result_diff = new gpc_polygon;
	    // result_diff->num_contours = 0;
	    // result_diff->contour = NULL;

	    if ( accum.contours() == 0 ) {
		result_diff = tmp;
		result_union = tmp;
	    } else {
   		// cout << "DIFF: tmp.num_contours = " << tmp.num_contours
		//      << " accum.num_contours = " << accum.num_contours
		//      << endl;
		// tmp output accum

		// FILE *ofp= fopen("tmp-debug", "w");
		// gpc_write_polygon(ofp, 1, &tmp);
		// fclose(ofp);

		// ofp= fopen("accum-debug", "w");
		// gpc_write_polygon(ofp, 1, &accum);
		// fclose(ofp);

		result_diff = polygon_diff( tmp, accum);
		result_union = polygon_union( tmp, accum);
	    }

	    /*
	      cout << "original contours = " << tmp.num_contours << endl;

	      for ( int j = 0; j < tmp.num_contours; j++ ) {
	      for (int k = 0;k < tmp.contour[j].num_vertices;k++ ) {
	      cout << tmp.contour[j].vertex[k].x << ","
	      << tmp.contour[j].vertex[k].y << endl;
	      }
	      }

	      cout << "clipped contours = " << result_diff->num_contours << endl;

	      for ( int j = 0; j < result_diff->num_contours; j++ ) {
	      for (int k = 0;k < result_diff->contour[j].num_vertices;k++ ) {
	      cout << result_diff->contour[j].vertex[k].x << ","
	      << result_diff->contour[j].vertex[k].y << endl;
	      }
	      }
	      */

	    // only add to output list if the clip left us with a polygon
	    if ( result_diff.contours() > 0 ) {
		// merge any slivers with larger neighboring polygons
		merge_slivers(result_diff);

		polys_clipped.polys[i].push_back(result_diff);

		// char filename[256];
		// sprintf(filename, "next-result-%02d", count++);
		// FILE *tmpfp= fopen(filename, "w");
		// gpc_write_polygon(tmpfp, 1, result_diff);
		// fclose(tmpfp);
	    }
	    accum = result_union;
	}
    }

    // finally, what ever is left over goes to ocean

    // clip to accum against original base tile
    // remains = new gpc_polygon;
    // remains->num_contours = 0;
    // remains->contour = NULL;
    remains = polygon_diff( polys_in.safety_base, accum );

    if ( remains.contours() > 0 ) {
	polys_clipped.polys[(int)OceanArea].push_back(remains);
    }

#if 0
    FILE *ofp;

    // tmp output accum
    if ( accum.num_contours ) {
	ofp = fopen("accum", "w");
	gpc_write_polygon(ofp, 1, &accum);
	fclose(ofp);
    }

    // tmp output safety_base
    if ( remains->num_contours ) {
	ofp= fopen("remains", "w");
	gpc_write_polygon(ofp, 1, remains);
	fclose(ofp);
    }
#endif

    return true;
}


