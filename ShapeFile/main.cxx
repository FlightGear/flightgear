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

#include <Include/compiler.h>

#include STL_STRING

#include <Debug/logstream.hxx>

#include <Polygon/index.hxx>
#include <Polygon/names.hxx>
#include "shape.hxx"


int main( int argc, char **argv ) {
    gpc_polygon gpc_shape;
    int i, j;

    fglog().setLogLevels( FG_ALL, FG_DEBUG );

    if ( argc != 3 ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Usage: " << argv[0] 
		<< " <shape_file> <work_dir>" );
	exit(-1);
    }

    FG_LOG( FG_GENERAL, FG_DEBUG, "Opening " << argv[1] << " for reading." );

    // make work directory
    string work_dir = argv[2];
    string command = "mkdir -p " + work_dir;
    system( command.c_str() );

    // initialize persistant polygon counter
    string counter_file = work_dir + "/../work.counter";
    poly_index_init( counter_file );

    // initialize structure for building gpc polygons
    shape_utils_init();

    GShapeFile * sf = new GShapeFile( argv[1] );
    GDBFile *dbf = new GDBFile( argv[1] );
    string path = argv[2];

    GPolygon shape;
    double  *coords; // in decimal degrees
    int	n_vertices;

    FG_LOG( FG_GENERAL, FG_INFO, "shape file records = " << sf->numRecords() );

    GShapeFile::ShapeType t = sf->shapeType();
    if ( t != GShapeFile::av_Polygon ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Can't handle non-polygon shape files" );
	exit(-1);
    }

    for ( i = 0; i < sf->numRecords(); i++ ) {
	//fetch i-th record (shape)
	sf->getShapeRec(i, &shape); 
	FG_LOG( FG_GENERAL, FG_DEBUG, "Record = " << i << "  rings = " 
		<< shape.numRings() );

	AreaType area = get_shapefile_type(dbf, i);
	FG_LOG( FG_GENERAL, FG_DEBUG, "area type = " << get_area_name(area) 
		<< " (" << (int)area << ")" );

	FG_LOG( FG_GENERAL, FG_INFO, "  record = " << i 
		<< " ring = " << 0 );

	if ( area == MarshArea ) {
	    // interior of polygon is marsh, holes are water

	    // do main outline first
	    init_shape(&gpc_shape);
	    n_vertices = shape.getRing(0, coords);
	    add_to_shape(n_vertices, coords, &gpc_shape);
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);

	    // do lakes (individually) next
	    for (  j = 1; j < shape.numRings(); j++ ) {
		FG_LOG( FG_GENERAL, FG_INFO, "  record = " << i 
			<< " ring = " << j );
		init_shape(&gpc_shape);
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
		process_shape(path, LakeArea, &gpc_shape);
		free_shape(&gpc_shape);
	    }
	} else if ( area == OceanArea ) {
	    // interior of polygon is ocean, holes are islands

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == LakeArea ) {
	    // interior of polygon is lake, holes are islands

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == DryLakeArea ) {
	    // interior of polygon is dry lake, holes are islands

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == IntLakeArea ) {
	    // interior of polygon is intermittent lake, holes are islands

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == ReservoirArea ) {
	    // interior of polygon is reservoir, holes are islands

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == IntReservoirArea ) {
	    // interior of polygon is intermittent reservoir, holes are islands

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == StreamArea ) {
	    // interior of polygon is stream, holes are islands

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == CanalArea ) {
	    // interior of polygon is canal, holes are islands

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == GlacierArea ) {
	    // interior of polygon is glacier, holes are dry land

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == VoidArea ) {
	    // interior is ????

	    // skip for now
	    FG_LOG(  FG_GENERAL, FG_ALERT, "Void area ... SKIPPING!" );

	    if ( shape.numRings() > 1 ) {
		FG_LOG(  FG_GENERAL, FG_ALERT, "  Void area with holes!" );
		// exit(-1);
	    }

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    // process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else if ( area == NullArea ) {
	    // interior is ????

	    // skip for now
	    FG_LOG(  FG_GENERAL, FG_ALERT, "Null area ... SKIPPING!" );

	    if ( shape.numRings() > 1 ) {
		FG_LOG(  FG_GENERAL, FG_ALERT, "  Null area with holes!" );
		// exit(-1);
	    }

	    init_shape(&gpc_shape);
	    for (  j = 0; j < shape.numRings(); j++ ) {
		n_vertices = shape.getRing(j, coords);
		add_to_shape(n_vertices, coords, &gpc_shape);
	    }
	    // process_shape(path, area, &gpc_shape);
	    free_shape(&gpc_shape);
	} else {
	    FG_LOG(  FG_GENERAL, FG_ALERT, "Uknown area!" );
	    exit(-1);
	}
    }

    return 0;
}


// $Log$
// Revision 1.7  1999/03/17 23:51:29  curt
// Changed polygon index counter file.
//
// Revision 1.6  1999/03/02 01:04:28  curt
// Don't crash when work directory doesn't exist ... create it.
//
// Revision 1.5  1999/03/01 15:36:28  curt
// Tweaked a function call name in "names.hxx".
//
// Revision 1.4  1999/02/25 21:31:05  curt
// First working version???
//
// Revision 1.3  1999/02/23 01:29:04  curt
// Additional progress.
//
// Revision 1.2  1999/02/19 19:05:18  curt
// Working on clipping shapes and distributing into buckets.
//
// Revision 1.1  1999/02/15 19:10:23  curt
// Initial revision.
//
