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


#include <Include/fg_constants.h>
#include <Math/point3d.hxx>

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

    // 1. find a point on the specified contour, min, with smallest y

    // min.y() starts greater than the biggest possible lat (degrees)
    min.sety( 100.0 );
    // int min_index;
    int min_node_index = 0;

    int_list_iterator current, last;
    current = poly[contour].begin();
    last = poly[contour].end();

    int counter = 0;
    for ( ; current != last; ++current ) {
	tmp = trinodes.get_node( *current );
	if ( tmp.y() < min.y() ) {
	    min = tmp;
	    // min_index = *current;
	    min_node_index = counter;

	    // cout << "min index = " << *current 
	    //      << " value = " << min_y << endl;
	} else {
	    // cout << "  index = " << *current << endl;
	}
	++counter;
    }
    cout << "min node index = " << min_node_index << endl;
    cout << "min index = " << poly[contour][min_node_index] 
	 << " value = " << trinodes.get_node( poly[contour][min_node_index] ) 
	 << " == " << min << endl;

    // 2. take midpoint, m, of min with neighbor having lowest
    // fabs(slope)

    if ( min_node_index == 0 ) {
	p1 = trinodes.get_node( poly[contour][1] );
	p2 = trinodes.get_node( poly[contour][poly[contour].size() - 1] );
    } else if ( min_node_index == (int)(poly[contour].size()) - 1 ) {
	p1 = trinodes.get_node( poly[contour][0] );
	p2 = trinodes.get_node( poly[contour][poly[contour].size() - 2] );
    } else {
	p1 = trinodes.get_node( poly[contour][min_node_index - 1] );
	p2 = trinodes.get_node( poly[contour][min_node_index + 1] );
    }
    double s1 = fabs( slope(min, p1) );
    double s2 = fabs( slope(min, p2) );
    if ( s1 < s2  ) {
	ln = p1;
    } else {
	ln = p2;
    }

    m.setx( (min.x() + ln.x()) / 2.0 );
    m.sety( (min.y() + ln.y()) / 2.0 );
    cout << "low mid point = " << m << endl;

    // 3. intersect vertical line through m and all other segments of
    // all other contours of this polygon.  save point, p3, with
    // smallest y > m.y

    p3.sety(100);
    
    // for ( int i = 0; i < (int)poly.size(); ++i ) {
    int i = contour;
    cout << "contour = " << i << " size = " << poly[i].size() << endl;
    for ( int j = 0; j < (int)(poly[i].size() - 1); ++j ) {
	// cout << "  p1 = " << poly[i][j] << " p2 = " 
	//      << poly[i][j+1] << endl;
	p1 = trinodes.get_node( poly[i][j] );
	p2 = trinodes.get_node( poly[i][j+1] );
	
	if ( intersects(p1, p2, m.x(), &result) ) {
	    // cout << "intersection = " << result << endl;
	    if ( ( result.y() < p3.y() ) &&
		 ( result.y() > m.y() + FG_EPSILON ) ) {
		p3 = result;
	    }
	}
    }
    // cout << "  p1 = " << poly[i][0] << " p2 = " 
    //      << poly[i][poly[i].size() - 1] << endl;
    p1 = trinodes.get_node( poly[i][0] );
    p2 = trinodes.get_node( poly[i][poly[i].size() - 1] );
    if ( intersects(p1, p2, m.x(), &result) ) {
	// cout << "intersection = " << result << endl;
	if ( ( result.y() < p3.y() ) &&
	     ( result.y() > m.y() + FG_EPSILON ) ) {
	    p3 = result;
	}
    }
    // }
    cout << "low intersection of other segment = " << p3 << endl;

    // 4. take midpoint of p2 && m as an arbitrary point inside polygon

    inside_list[contour].setx( (m.x() + p3.x()) / 2.0 );
    inside_list[contour].sety( (m.y() + p3.y()) / 2.0 );
    cout << "inside point = " << inside_list[contour] << endl;
}


