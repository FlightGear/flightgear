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

// include Generic Polygon Clipping Library

extern "C" {
#include <gpc.h>
}

#include <Debug/logstream.hxx>


int main( int argc, char **argv ) {
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
// Revision 1.1  1999/02/15 19:10:23  curt
// Initial revision.
//
