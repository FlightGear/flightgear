// polygon.cxx -- polygon (with holes) management class
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


// include Generic Polygon Clipping Library
//
//    http://www.cs.man.ac.uk/aig/staff/alan/software/
//
extern "C" {
#include <gpc.h>
}

#include <Include/fg_constants.h>
#include <Math/point3d.hxx>

#include "trisegs.hxx"
#include "polygon.hxx"


// Constructor 
FGPolygon::FGPolygon( void ) {
}


// Destructor
FGPolygon::~FGPolygon( void ) {
}


// Given a line segment specified by two endpoints p1 and p2, return
// the slope of the line.
static double slope( const Point3D& p0, const Point3D& p1 ) {
    if ( fabs(p0.x() - p1.x()) > FG_EPSILON ) {
	return (p0.y() - p1.y()) / (p0.x() - p1.x());
    } else {
	return 1.0e+999; // really big number
    }
}


// Given a line segment specified by two endpoints p1 and p2, return
// the y value of a point on the line that intersects with the
// verticle line through x.  Return true if an intersection is found,
// false otherwise.
static bool intersects( const Point3D& p0, const Point3D& p1, double x, 
			 Point3D *result ) {
    // equation of a line through (x0,y0) and (x1,y1):
    // 
    //     y = y1 + (x - x1) * (y0 - y1) / (x0 - x1)

    double y;

    if ( fabs(p0.x() - p1.x()) > FG_EPSILON ) {
	y = p1.y() + (x - p1.x()) * (p0.y() - p1.y()) / (p0.x() - p1.x());
    } else {
	return false;
    }
    result->setx(x);
    result->sety(y);

    if ( p0.y() <= p1.y() ) {
	if ( (p0.y() <= y) && (y <= p1.y()) ) {
	    return true;
	}
    } else {
 	if ( (p0.y() >= y) && (y >= p1.y()) ) {
	    return true;
	}
    }

    return false;
}


// calculate some "arbitrary" point inside the specified contour for
// assigning attribute areas
void FGPolygon::calc_point_inside( const int contour, 
				   const FGTriNodes& trinodes ) {
    Point3D tmp, min, ln, p1, p2, p3, m, result;
    int min_node_index = 0;
    int min_index = 0;
    int p1_index = 0;
    int p2_index = 0;
    int ln_index = 0;

    // 1. find a point on the specified contour, min, with smallest y

    // min.y() starts greater than the biggest possible lat (degrees)
    min.sety( 100.0 );

    point_list_iterator current, last;
    current = poly[contour].begin();
    last = poly[contour].end();

    for ( int i = 0; i < contour_size( contour ); ++i ) {
	tmp = get_pt( contour, i );
	if ( tmp.y() < min.y() ) {
	    min = tmp;
	    min_index = trinodes.find( min );
	    min_node_index = i;

	    // cout << "min index = " << *current 
	    //      << " value = " << min_y << endl;
	} else {
	    // cout << "  index = " << *current << endl;
	}
    }

    cout << "min node index = " << min_node_index << endl;
    cout << "min index = " << min_index
	 << " value = " << trinodes.get_node( min_index ) 
	 << " == " << min << endl;

    // 2. take midpoint, m, of min with neighbor having lowest
    // fabs(slope)

    if ( min_node_index == 0 ) {
	p1 = poly[contour][1];
	p2 = poly[contour][poly[contour].size() - 1];
    } else if ( min_node_index == (int)(poly[contour].size()) - 1 ) {
	p1 = poly[contour][0];
	p2 = poly[contour][poly[contour].size() - 2];
    } else {
	p1 = poly[contour][min_node_index - 1];
	p2 = poly[contour][min_node_index + 1];
    }
    p1_index = trinodes.find( p1 );
    p2_index = trinodes.find( p2 );

    double s1 = fabs( slope(min, p1) );
    double s2 = fabs( slope(min, p2) );
    if ( s1 < s2  ) {
	ln_index = p1_index;
	ln = p1;
    } else {
	ln_index = p2_index;
	ln = p2;
    }

    FGTriSeg base_leg( min_index, ln_index );

    m.setx( (min.x() + ln.x()) / 2.0 );
    m.sety( (min.y() + ln.y()) / 2.0 );
    cout << "low mid point = " << m << endl;

    // 3. intersect vertical line through m and all other segments of
    // all other contours of this polygon.  save point, p3, with
    // smallest y > m.y

    p3.sety(100);
    
    for ( int i = 0; i < (int)poly.size(); ++i ) {
	cout << "contour = " << i << " size = " << poly[i].size() << endl;
	for ( int j = 0; j < (int)(poly[i].size() - 1); ++j ) {
	    // cout << "  p1 = " << poly[i][j] << " p2 = " 
	    //      << poly[i][j+1] << endl;
	    p1 = poly[i][j];
	    p2 = poly[i][j+1];
	    p1_index = trinodes.find( p1 );
	    p2_index = trinodes.find( p2 );
	
	    if ( intersects(p1, p2, m.x(), &result) ) {
		// cout << "intersection = " << result << endl;
		if ( ( result.y() < p3.y() ) &&
		     ( result.y() > m.y() ) &&
		     ( base_leg != FGTriSeg(p1_index, p2_index) ) ) {
		    p3 = result;
		}
	    }
	}
	// cout << "  p1 = " << poly[i][0] << " p2 = " 
	//      << poly[i][poly[i].size() - 1] << endl;
	p1 = poly[i][0];
	p2 = poly[i][poly[i].size() - 1];
	p1_index = trinodes.find( p1 );
	p2_index = trinodes.find( p2 );
	if ( intersects(p1, p2, m.x(), &result) ) {
	    // cout << "intersection = " << result << endl;
	    if ( ( result.y() < p3.y() ) &&
		 ( result.y() > m.y() ) &&
		 ( base_leg != FGTriSeg(p1_index, p2_index) ) ) {
		p3 = result;
	    }
	}
    }
    if ( p3.y() < 100 ) {
	cout << "low intersection of other segment = " << p3 << endl;
	inside_list[contour].setx( (m.x() + p3.x()) / 2.0 );
	inside_list[contour].sety( (m.y() + p3.y()) / 2.0 );
    } else {
	cout << "Error:  Failed to find a point inside :-(" << endl;
	inside_list[contour] = p3;
	// exit(-1);
    }

    // 4. take midpoint of p2 && m as an arbitrary point inside polygon

    cout << "inside point = " << inside_list[contour] << endl;
}


//
// wrapper functions for gpc polygon clip routines
//

// Make a gpc_poly from an FGPolygon
void make_gpc_poly( const FGPolygon& in, gpc_polygon *out ) {
    gpc_vertex_list v_list;
    v_list.num_vertices = 0;
    v_list.vertex = new gpc_vertex[FG_MAX_VERTICES];

    cout << "making a gpc_poly" << endl;
    cout << "  input contours = " << in.contours() << endl;

    Point3D p;
    // build the gpc_polygon structures
    for ( int i = 0; i < in.contours(); ++i ) {
	cout << "    contour " << i << " = " << in.contour_size( i ) << endl;
	for ( int j = 0; j < in.contour_size( i ); ++j ) {
	    p = in.get_pt( i, j );
	    v_list.vertex[j].x = p.x();
	    v_list.vertex[j].y = p.y();
	}
	v_list.num_vertices = in.contour_size( i );
	gpc_add_contour( out, &v_list, in.get_hole_flag( i ) );
    }

    // free alocated memory
    delete v_list.vertex;
}


// Set operation type
typedef enum {
    POLY_DIFF,			// Difference
    POLY_INT,			// Intersection
    POLY_XOR,			// Exclusive or
    POLY_UNION			// Union
} clip_op;


// Generic clipping routine
FGPolygon polygon_clip( clip_op poly_op, const FGPolygon& subject, 
			const FGPolygon& clip )
{
    FGPolygon result;

    gpc_polygon *gpc_subject = new gpc_polygon;
    gpc_subject->num_contours = 0;
    gpc_subject->contour = NULL;
    gpc_subject->hole = NULL;
    make_gpc_poly( subject, gpc_subject );

    gpc_polygon *gpc_clip = new gpc_polygon;
    gpc_clip->num_contours = 0;
    gpc_clip->contour = NULL;
    gpc_clip->hole = NULL;
    make_gpc_poly( clip, gpc_clip );

    gpc_polygon *gpc_result = new gpc_polygon;
    gpc_result->num_contours = 0;
    gpc_result->contour = NULL;
    gpc_result->hole = NULL;

    gpc_op op;
    if ( poly_op == POLY_DIFF ) {
	op = GPC_DIFF;
    } else if ( poly_op == POLY_INT ) {
	op = GPC_INT;
    } else if ( poly_op == POLY_XOR ) {
	op = GPC_XOR;
    } else if ( poly_op == POLY_UNION ) {
	op = GPC_UNION;
    } else {
	cout << "Unknown polygon op, exiting." << endl;
	exit(-1);
    }

    gpc_polygon_clip( op, gpc_subject, gpc_clip, gpc_result );

    for ( int i = 0; i < gpc_result->num_contours; ++i ) {
	// cout << "  processing contour = " << i << ", nodes = " 
	//      << gpc_result->contour[i].num_vertices << ", hole = "
	//      << gpc_result->hole[i] << endl;
	
	// sprintf(junkn, "g.%d", junkc++);
	// junkfp = fopen(junkn, "w");

	for ( int j = 0; j < gpc_result->contour[i].num_vertices; j++ ) {
	    Point3D p( gpc_result->contour[i].vertex[j].x,
		       gpc_result->contour[i].vertex[j].y,
		       0 );
	    // junkp = in_nodes.get_node( index );
	    // fprintf(junkfp, "%.4f %.4f\n", junkp.x(), junkp.y());
	    result.add_node(i, p);
	    // cout << "  - " << index << endl;
	}
	// fprintf(junkfp, "%.4f %.4f\n", 
	//    gpc_result->contour[i].vertex[0].x, 
	//    gpc_result->contour[i].vertex[0].y);
	// fclose(junkfp);

	result.set_hole_flag( i, gpc_result->hole[i] );
    }

    // free allocated memory
    gpc_free_polygon( gpc_subject );
    gpc_free_polygon( gpc_clip );
    gpc_free_polygon( gpc_result );

    return result;
}


// Difference
FGPolygon polygon_diff(	const FGPolygon& subject, const FGPolygon& clip ) {
    return polygon_clip( POLY_DIFF, subject, clip );
}

// Intersection
FGPolygon polygon_int( const FGPolygon& subject, const FGPolygon& clip ) {
    return polygon_clip( POLY_INT, subject, clip );
}


// Exclusive or
FGPolygon polygon_xor( const FGPolygon& subject, const FGPolygon& clip ) {
    return polygon_clip( POLY_XOR, subject, clip );
}


// Union
FGPolygon polygon_union( const FGPolygon& subject, const FGPolygon& clip ) {
    return polygon_clip( POLY_UNION, subject, clip );
}



