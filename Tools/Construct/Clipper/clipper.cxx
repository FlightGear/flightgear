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


// remove any slivers from in polygon and move them to out polygon.
void FGClipper::move_slivers( FGPolygon& in, FGPolygon& out ) {
    cout << "Begin move slivers" << endl;
    // traverse each contour of the polygon and attempt to identify
    // likely slivers

    out.erase();

    double angle_cutoff = 10.0 * DEG_TO_RAD;
    double area_cutoff = 0.00001;
    double min_angle;
    double area;

    point_list contour;
    int hole_flag;

    // process contours in reverse order so deleting a contour doesn't
    // foul up our sequence
    for ( int i = in.contours() - 1; i >= 0; --i ) {
	cout << "contour " << i << endl;

	min_angle = in.minangle_contour( i );
	area = in.area_contour( i );

	cout << "  min_angle (rad) = " 
	     << min_angle << endl;
	cout << "  min_angle (deg) = " 
	     << min_angle * 180.0 / FG_PI << endl;
	cout << "  area = " << area << endl;

	if ( (min_angle < angle_cutoff) && (area < area_cutoff)  ) {
	    cout << "      WE THINK IT'S A SLIVER!" << endl;

	    // check if this is a hole
	    hole_flag = in.get_hole_flag( i );

	    if ( hole_flag ) {
		// just delete/eliminate/remove sliver holes
		in.delete_contour( i );
	    } else {
		// move sliver contour to out polygon
		contour = in.get_contour( i );
		in.delete_contour( i );
		out.add_contour( contour, hole_flag );
	    }
	}
    }
}


// for each sliver contour, see if a union with another polygon yields
// a polygon with no increased contours (i.e. the sliver is adjacent
// and can be merged.)  If so, replace the clipped polygon with the
// new polygon that has the sliver merged in.
void FGClipper::merge_slivers( FGPolyList& clipped, FGPolygon& slivers ) {
    FGPolygon poly, result, sliver;
    point_list contour;
    int original_contours, result_contours;
    bool done;

    for ( int i = 0; i < slivers.contours(); ++i ) {
	cout << "Merging sliver = " << i << endl;

	// make the sliver polygon
	contour = slivers.get_contour( i );
	sliver.erase();
	sliver.add_contour( contour, 0 );
	done = false;

	for ( int area = 0; area < FG_MAX_AREA_TYPES && !done; ++area ) {
	    cout << "  testing area = " << area << " with " 
		 << clipped.polys[area].size() << " polys" << endl;
	    for ( int j = 0; 
		  j < (int)clipped.polys[area].size() && !done;
		  ++j )
	    {
		cout << "  polygon = " << j << endl;

		poly = clipped.polys[area][j];
		original_contours = poly.contours();
		result = polygon_union( poly, sliver );
		result_contours = result.contours();

		if ( original_contours == result_contours ) {
		    cout << "    FOUND a poly to merge the sliver with" << endl;
		    clipped.polys[area][j] = result;
		    done = true;
		    // poly.write("orig");
		    // sliver.write("sliver");
		    // result.write("result");
		    // cout << "press return: ";
		    // string input;
		    // cin >> input;
		} else {
		    cout << "    poly not a match" << endl;
		    cout << "    original = " << original_contours
			 << " result = " << result_contours << endl;
		    cout << "    sliver = " << endl;
		    for ( int k = 0; k < (int)contour.size(); ++k ) {
			cout << "      " << contour[k].x() << ", "
			     << contour[k].y() << endl;
		    }
		}
	    }
	}
    }
}


// Do actually clipping work
bool FGClipper::clip_all(const point2d& min, const point2d& max) {
    FGPolygon accum, result_union, tmp;
    FGPolygon result_diff, slivers, remains;
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
		// move slivers from result_diff polygon to slivers polygon
		move_slivers(result_diff, slivers);
		cout << "  After sliver move:" << endl;
		cout << "    result_diff = " << result_diff.contours() << endl;
		cout << "    slivers = " << slivers.contours() << endl;

		// merge any slivers with previously clipped
		// neighboring polygons
		if ( slivers.contours() > 0 ) {
		    merge_slivers(polys_clipped, slivers);
		}

		// add the sliverless result polygon (from after the
		// move_slivers) to the clipped polys list
		if ( result_diff.contours() > 0  ) {
		    polys_clipped.polys[i].push_back(result_diff);
		}

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
	cout << "remains contours = " << remains.contours() << endl;
	// move slivers from remains polygon to slivers polygon
	move_slivers(remains, slivers);
	cout << "  After sliver move:" << endl;
	cout << "    remains = " << remains.contours() << endl;
	cout << "    slivers = " << slivers.contours() << endl;

	// merge any slivers with previously clipped
	// neighboring polygons
	if ( slivers.contours() > 0 ) {
	    merge_slivers(polys_clipped, slivers);
	}

	if ( remains.contours() > 0 ) {
	    polys_clipped.polys[(int)OceanArea].push_back(remains);
	}
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


