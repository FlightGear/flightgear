// point3d.hxx -- a 3d point class.  
//
// Adapted from algebra3 by Jean-Francois Doue, started October 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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
// (Log is kept at end of this file)


#ifndef _POINT3D_HXX
#define _POINT3D_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <iostream>
#include <assert.h>
#if defined( __BORLANDC__ )
#  define exception c_exception
#elif defined( __FreeBSD__ )
#  include <math.h>
#endif

// -rp- assert.h is buggy under MWCWP3, as multiple #include undef assert !
#ifdef __MWERKS__
#  define assert(x)
#endif

const double fgPoint3_Epsilon = 0.0000001;

enum {PX, PY, PZ};		    // axes


///////////////////////////
//
// 3D Point
//
///////////////////////////

class Point3D {

protected:

    double n[3];

public:

    // Constructors

    Point3D();
    Point3D(const double x, const double y, const double z);
    explicit Point3D(const double d);
    Point3D(const Point3D &p);

    // Assignment operators

    Point3D& operator = ( const Point3D& p );	 // assignment of a Point3D
    Point3D& operator += ( const Point3D& p );	 // incrementation by a Point3D
    Point3D& operator -= ( const Point3D& p );	 // decrementation by a Point3D
    Point3D& operator *= ( const double d );     // multiplication by a constant
    Point3D& operator /= ( const double d );	 // division by a constant

    void setx(const double x);
    void sety(const double y);
    void setz(const double z);

    // Queries 

    double& operator [] ( int i);		 // indexing
    double operator[] (int i) const;		 // read-only indexing

    double x() const;      // cartesian x
    double y() const;      // cartesian y
    double z() const;      // cartesian z

    double lon() const;    // polar longitude
    double lat() const;    // polar latitude
    double radius() const; // polar radius
    double elev() const;   // geodetic elevation (if specifying a surface point)

    // friends
    friend Point3D operator - (const Point3D& p);	            // -p1
    friend bool operator == (const Point3D& a, const Point3D& b);  // p1 == p2?
    friend istream& operator>> ( istream&, Point3D& );
    friend ostream& operator<< ( ostream&, const Point3D& );

    // Special functions
    double distance3D(const Point3D& a) const; // distance between
};


// input from stream
inline istream&
operator >> ( istream& in, Point3D& p)
{
    char c;

    in >> p.n[PX];

    // read past optional comma
    while ( in.get(c) ) {
	if ( (c != ' ') && (c != ',') ) {
	    // push back on the stream
	    in.putback(c);
	    break;
	}
    }
	
    in >> p.n[PY];

    // read past optional comma
    while ( in.get(c) ) {
	if ( (c != ' ') && (c != ',') ) {
	    // push back on the stream
	    in.putback(c);
	    break;
	}
    }
	
    in >> p.n[PZ];

    return in;
}

inline ostream&
operator<< ( ostream& out, const Point3D& p )
{
    return out << p.n[PX] << ", " << p.n[PY] << ", " << p.n[PZ];
}

///////////////////////////
//
// Point3D Member functions
//
///////////////////////////

// CONSTRUCTORS

inline Point3D::Point3D() {}

inline Point3D::Point3D(const double x, const double y, const double z)
{
    n[PX] = x; n[PY] = y; n[PZ] = z;
}

inline Point3D::Point3D(const double d)
{
    n[PX] = n[PY] = n[PZ] = d;
}

inline Point3D::Point3D(const Point3D& p)
{
    n[PX] = p.n[PX]; n[PY] = p.n[PY]; n[PZ] = p.n[PZ];
}

// ASSIGNMENT OPERATORS

inline Point3D& Point3D::operator = (const Point3D& p)
{
    n[PX] = p.n[PX]; n[PY] = p.n[PY]; n[PZ] = p.n[PZ]; return *this;
}

inline Point3D& Point3D::operator += ( const Point3D& p )
{
    n[PX] += p.n[PX]; n[PY] += p.n[PY]; n[PZ] += p.n[PZ]; return *this;
}

inline Point3D& Point3D::operator -= ( const Point3D& p )
{
    n[PX] -= p.n[PX]; n[PY] -= p.n[PY]; n[PZ] -= p.n[PZ]; return *this;
}

inline Point3D& Point3D::operator *= ( const double d )
{
    n[PX] *= d; n[PY] *= d; n[PZ] *= d; return *this;
}

inline Point3D& Point3D::operator /= ( const double d )
{
    double d_inv = 1./d; n[PX] *= d_inv; n[PY] *= d_inv; n[PZ] *= d_inv;
    return *this;
}

inline void Point3D::setx(const double x) {
    n[PX] = x;
}

inline void Point3D::sety(const double y) {
    n[PY] = y;
}

inline void Point3D::setz(const double z) {
    n[PZ] = z;
}

// QUERIES

inline double& Point3D::operator [] ( int i)
{
    assert(! (i < PX || i > PZ));
    return n[i];
}

inline double Point3D::operator [] ( int i) const {
    assert(! (i < PX || i > PZ));
    return n[i];
}


inline double Point3D::x() const { return n[PX]; }

inline double Point3D::y() const { return n[PY]; }

inline double Point3D::z() const { return n[PZ]; }

inline double Point3D::lon() const { return n[PX]; }

inline double Point3D::lat() const { return n[PY]; }

inline double Point3D::radius() const { return n[PZ]; }

inline double Point3D::elev() const { return n[PZ]; }


// FRIENDS

inline Point3D operator - (const Point3D& a)
{
    return Point3D(-a.n[PX],-a.n[PY],-a.n[PZ]);
}

inline Point3D operator + (const Point3D& a, const Point3D& b)
{
    return Point3D(a) += b;
}

inline Point3D operator - (const Point3D& a, const Point3D& b)
{
    return Point3D(a) -= b;
}

inline Point3D operator * (const Point3D& a, const double d)
{
    return Point3D(a) *= d;
}

inline Point3D operator * (const double d, const Point3D& a)
{
    return a*d;
}

inline Point3D operator / (const Point3D& a, const double d)
{
    return Point3D(a) *= (1.0 / d );
}

inline bool operator == (const Point3D& a, const Point3D& b)
{
    return
	(a.n[PX] - b.n[PX]) < fgPoint3_Epsilon &&
	(a.n[PY] - b.n[PY]) < fgPoint3_Epsilon &&
	(a.n[PZ] - b.n[PZ]) < fgPoint3_Epsilon;
}

inline bool operator != (const Point3D& a, const Point3D& b)
{
    return !(a == b);
}

// Special functions

inline double
Point3D::distance3D(const Point3D& a ) const
{
    double x, y, z;

    x = n[PX] - a.n[PX];
    y = n[PY] - a.n[PY];
    z = n[PZ] - a.n[PZ];

    return sqrt(x*x + y*y + z*z);
}

#endif // _POINT3D_HXX


// $Log$
// Revision 1.7  1999/01/19 20:56:58  curt
// MacOS portability changes contributed by "Robert Puyol" <puyol@abvent.fr>
//
// Revision 1.6  1998/11/23 21:46:37  curt
// Borland portability tweaks.
//
// Revision 1.5  1998/11/20 01:00:38  curt
// Patch in fgGeoc2Geod() to avoid a floating explosion.
// point3d.hxx include math.h for FreeBSD
//
// Revision 1.4  1998/11/11 00:18:38  curt
// Check for domain error in fgGeoctoGeod()
//
// Revision 1.3  1998/10/20 18:21:49  curt
// Tweaks from Bernie Bright.
//
// Revision 1.2  1998/10/18 01:17:12  curt
// Point3D tweaks.
//
// Revision 1.1  1998/10/16 00:50:29  curt
// Added point3d.hxx to replace cheezy fgPoint3d struct.
//
//
