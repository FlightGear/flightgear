// shape.cxx -- shape/gpc utils
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


#include <Include/compiler.h>

#include STL_STRING

#include <Bucket/newbucket.hxx>
#include <Debug/logstream.hxx>

#include "names.hxx"
#include "shape.hxx"


#define FG_MAX_VERTICES 100000
static gpc_vertex_list v_list;


class point2d {
public:
    double x, y;
};


static void clip_and_write_poly( string root, AreaType area, 
				 FGBucket b, gpc_polygon *shape ) {
    point2d c, min, max;
    c.x = b.get_center_lon();
    c.y = b.get_center_lat();
    double span = bucket_span(c.y);
    gpc_polygon base, result;
    char tmp[256];

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

    FG_LOG( FG_GENERAL, FG_INFO, "  (" << min.x << "," << min.y << ") ("
	    << max.x << "," << max.y << ")" );

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

    base.num_contours = 0;
    base.contour = NULL;
    gpc_add_contour( &base, &v_list );

    // FG_LOG( FG_GENERAL, FG_DEBUG, "base = 4 vertices" );

    /*
    FILE *bfp= fopen("base", "w");
    gpc_write_polygon(bfp, &base);
    fclose(bfp);
    */

    gpc_polygon_clip(GPC_INT, &base, shape, &result);

    if ( result.num_contours > 0 ) {
	long int index = b.gen_index();
	string path = root + "/Scenery/" + b.gen_base_path();
	string command = "mkdir -p " + path;
	system( command.c_str() );

	sprintf(tmp, "%ld", index);
	string polyfile = path + "/" + tmp;

	if ( area == MarshArea ) {
	    polyfile += ".marsh";
	} else if ( area == OceanArea ) {
	    polyfile += ".ocean";
	} else if ( area == LakeArea ) {
	    polyfile += ".lake";
	} else if ( area == DryLakeArea ) {
	    polyfile += ".drylake";
	} else if ( area == IntLakeArea ) {
	    polyfile += ".intlake";
	} else if ( area == ReservoirArea ) {
	    polyfile += ".reservoir";
	} else if ( area == IntReservoirArea ) {
	    polyfile += ".intreservoir";
	} else if ( area == StreamArea ) {
	    polyfile += ".stream";
	} else if ( area == CanalArea ) {
	    polyfile += ".canal";
	} else if ( area == GlacierArea ) {
	    polyfile += ".glacier";
	} else {
	    cout << "unknown area type in clip_and_write_poly()!" << endl;
	    exit(-1);
	}
	
	FILE *rfp= fopen(polyfile.c_str(), "w");
	gpc_write_polygon(rfp, &result);
	fclose(rfp);
    }

    gpc_free_polygon(&base);
    gpc_free_polygon(&result);
}


// Initialize structure we use to create polygons for the gpc library
bool shape_utils_init() {
    v_list.num_vertices = 0;
    v_list.vertex = new gpc_vertex[FG_MAX_VERTICES];;

    return true;
}


// initialize a gpc_polygon
void init_shape(gpc_polygon *shape) {
    shape->num_contours = 0;
    shape->contour = NULL;
}


// make a gpc_polygon
void add_to_shape(int count, double *coords, gpc_polygon *shape) {
    
    for ( int i = 0; i < count; i++ ) {
	v_list.vertex[i].x = coords[i*2+0];
	v_list.vertex[i].y = coords[i*2+1];
    }

    v_list.num_vertices = count;
    gpc_add_contour( shape, &v_list );
}


// process shape (write polygon to all intersecting tiles)
void process_shape(string path, AreaType area, gpc_polygon *gpc_shape) {
    point2d min, max;
    int i, j;

    min.x = min.y = 200.0;
    max.x = max.y = -200.0;

    // find min/max of polygon
    for ( i = 0; i < gpc_shape->num_contours; i++ ) {
	for ( j = 0; j < gpc_shape->contour[i].num_vertices; j++ ) {
	    double x = gpc_shape->contour[i].vertex[j].x;
	    double y = gpc_shape->contour[i].vertex[j].y;

	    if ( x < min.x ) { min.x = x; }
	    if ( y < min.y ) { min.y = y; }
	    if ( x > max.x ) { max.x = x; }
	    if ( y > max.y ) { max.y = y; }
	}
    }

    /*
    FILE *sfp= fopen("shape", "w");
    gpc_write_polygon(sfp, gpc_shape);
    fclose(sfp);
    exit(-1);
    */
	
    FG_LOG( FG_GENERAL, FG_INFO, "  min = " << min.x << "," << min.y
	    << " max = " << max.x << "," << max.y );

    // find buckets for min, and max points of convex hull.
    // note to self: self, you should think about checking for
    // polygons that span the date line
    FGBucket b_min(min.x, min.y);
    FGBucket b_max(max.x, max.y);
    FG_LOG( FG_GENERAL, FG_INFO, "  Bucket min = " << b_min );
    FG_LOG( FG_GENERAL, FG_INFO, "  Bucket max = " << b_max );
	    
    if ( b_min == b_max ) {
	clip_and_write_poly( path, area, b_min, gpc_shape );
    } else {
	FGBucket b_cur;
	int dx, dy, i, j;
	    
	fgBucketDiff(b_min, b_max, &dx, &dy);
	FG_LOG( FG_GENERAL, FG_INFO, 
		"  polygon spans tile boundaries" );
	FG_LOG( FG_GENERAL, FG_INFO, "  dx = " << dx 
		<< "  dy = " << dy );

	if ( (dx > 100) || (dy > 100) ) {
	    FG_LOG( FG_GENERAL, FG_ALERT, 
		    "somethings really wrong!!!!" );
	    exit(-1);
	}

	for ( j = 0; j <= dy; j++ ) {
	    for ( i = 0; i <= dx; i++ ) {
		b_cur = fgBucketOffset(min.x, min.y, i, j);
		clip_and_write_poly( path, area, b_cur, gpc_shape );
	    }
	}
	// string answer; cin >> answer;
    }
}


// free a gpc_polygon
void free_shape(gpc_polygon *shape) {
    gpc_free_polygon(shape);
}


// $Log$
// Revision 1.1  1999/02/23 01:29:06  curt
// Additional progress.
//
