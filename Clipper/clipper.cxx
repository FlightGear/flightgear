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
// (Log is kept at end of this file)
 


#include "clipper.hxx"

#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Misc/fgstream.hxx>
#include <Polygon/names.hxx>


#define EXTRA_SAFETY_CLIP

#define FG_MAX_VERTICES 100000

static gpc_vertex_list v_list;
// static gpc_polygon poly;
static FGPolyList polys_in, polys_out;


// Initialize Clipper (allocate and/or connect structures)
bool fgClipperInit() {
    v_list.num_vertices = 0;
    v_list.vertex = new gpc_vertex[FG_MAX_VERTICES];;

    return true;
}


// Load a polygon definition file
bool fgClipperLoadPolygons(const string& path) {
    string poly_name;
    AreaType poly_type;
    int contours, count, i, j;
    double startx, starty, x, y, lastx, lasty;

    FG_LOG( FG_CLIPPER, FG_INFO, "Loading " << path << " ..." );

    fg_gzifstream in( path );

    if ( !in ) {
        FG_LOG( FG_CLIPPER, FG_ALERT, "Cannot open file: " << path );
	exit(-1);
    }

    in >> skipcomment;
    while ( !in.eof() ) {
	in >> poly_name;
	cout << "poly name = " << poly_name << endl;
	poly_type = get_area_type( poly_name );
	cout << "poly type (int) = " << (int)poly_type << endl;
	in >> contours;
	cout << "num contours = " << contours << endl;

	for ( i = 0; i < contours; ++i ) {
	    in >> count;

	    if ( count < 3 ) {
		FG_LOG( FG_CLIPPER, FG_ALERT, 
			"Polygon with less than 3 data points." );
		exit(-1);
	    }

	    in >> startx;
	    in >> starty;
	    v_list.vertex[0].x = startx;
	    v_list.vertex[0].y = starty;
	    FG_LOG( FG_CLIPPER, FG_BULK, "0 = " 
		    << startx << ", " << starty );

	    for ( j = 1; j < count - 1; ++j ) {
		in >> x;
		in >> y;
		v_list.vertex[j].x = x;
		v_list.vertex[j].y = y;
		FG_LOG( FG_CLIPPER, FG_BULK, j << " = " << x << ", " << y );
	    }
	    v_list.num_vertices = count - 1;

	    in >> lastx;
	    in >> lasty;

	    if ( (fabs(startx - lastx) < FG_EPSILON) 
		 && (fabs(starty - lasty) < FG_EPSILON) ) {
		// last point same as first, discard
	    } else {
		v_list.vertex[count - 1].x = lastx;
		v_list.vertex[count - 1].y = lasty;
		++v_list.num_vertices;
		FG_LOG( FG_CLIPPER, FG_BULK, count - 1 << " = " 
			<< lastx << ", " << lasty );
	    }

	    gpc_polygon *poly = new gpc_polygon;
	    poly->num_contours = 0;
	    poly->contour = NULL;
	    gpc_add_contour( poly, &v_list );

	    int area = (int)poly_type;
	    if ( area < FG_MAX_AREAS ) {
		polys_in.polys[area].push_back(poly);
	    } else {
		FG_LOG( FG_CLIPPER, FG_ALERT, "Polygon type out of range = " 
			<< poly_type);
		exit(-1);
	    }
	}

	in >> skipcomment;
    }

    // FILE *ofp= fopen("outfile", "w");
    // gpc_write_polygon(ofp, &polys.landuse);

    return true;
}


// Do actually clipping work
bool fgClipperMaster(const point2d& min, const point2d& max) {
    gpc_polygon accum, result_diff, result_union, tmp;
    polylist_iterator current, last;

    FG_LOG( FG_CLIPPER, FG_INFO, "Running master clipper" );

    accum.num_contours = 0;

    cout << "  (" << min.x << "," << min.y << ") (" 
	 << max.x << "," << max.y << ")" << endl;

    // set up clipping tile
    v_list.vertex[0].x = min.x;
    v_list.vertex[0].y = min.y;

    v_list.vertex[1].x = max.x;
    v_list.vertex[1].y = min.y;

    v_list.vertex[2].x = max.x;
    v_list.vertex[2].y = max.y;

    v_list.vertex[3].x = min.x;
    v_list.vertex[3].y = max.y;

    v_list.num_vertices = 4;

    polys_in.safety_base.num_contours = 0;
    polys_in.safety_base.contour = NULL;
    gpc_add_contour( &polys_in.safety_base, &v_list );

    // process polygons in priority order
    for ( int i = 0; i < FG_MAX_AREAS; ++i ) {

	current = polys_in.polys[i].begin();
	last = polys_in.polys[i].end();
	for ( ; current != last; ++current ) {
	    FG_LOG( FG_CLIPPER, FG_DEBUG, get_area_name( (AreaType)i ) 
		    << " = " << (*current)->contour->num_vertices );

#ifdef EXTRA_SAFETY_CLIP
	    // clip to base tile
	    gpc_polygon_clip(GPC_INT, &polys_in.safety_base, *current, &tmp);
#else
	    &tmp = *current;
#endif

	    // clip current polygon against previous higher priority
	    // stuff
	    if ( accum.num_contours == 0 ) {
		result_diff = tmp;
		result_union = tmp;
	    } else {
		gpc_polygon_clip(GPC_DIFF, &accum, &tmp, &result_diff);
		gpc_polygon_clip(GPC_UNION, &accum, &tmp, &result_union);
	    }
	    
	    polys_out.polys[i].push_back(&result_diff);
	    accum = result_union;
	}
    }

    // finally, what ever is left over goes to base terrain

    // clip to accum against original base tile
    gpc_polygon_clip(GPC_DIFF, &polys_in.safety_base, &accum, 
		     &polys_out.safety_base);

    // tmp output accum
    FILE *ofp= fopen("accum", "w");
    gpc_write_polygon(ofp, &accum);

    // tmp output safety_base
    ofp= fopen("safety_base", "w");
    gpc_write_polygon(ofp, &polys_out.safety_base);

    return true;
}


// $Log$
// Revision 1.1  1999/03/01 15:39:39  curt
// Initial revision.
//
