#ifndef _FG_ATC_PROJECTION_HXX
#define _FG_ATC_PROJECTION_HXX

#include <simgear/math/point3d.hxx>

// FGATCProjection - a class to project an area local to an airport onto an orthogonal co-ordinate system
class FGATCProjection {

public:
    FGATCProjection();
    ~FGATCProjection();

    void Init(Point3D centre);

    // Convert a lat/lon co-ordinate to the local projection
    Point3D ConvertToLocal(Point3D pt);

    // Convert a local projection co-ordinate to lat/lon
    Point3D ConvertFromLocal(Point3D pt);

private:
    Point3D origin;	// lat/lon of local area origin
    double correction_factor;	// Reduction in surface distance per degree of longitude due to latitude.  Saves having to do a cos() every call.

};


// FGATCAlignedProjection - a class to project an area local to a runway onto an orthogonal co-ordinate system
// with the origin at the threshold and the runway aligned with the y axis.
class FGATCAlignedProjection {

public:
    FGATCAlignedProjection();
    ~FGATCAlignedProjection();

    void Init(Point3D centre, double heading);

    // Convert a lat/lon co-ordinate to the local projection
    Point3D ConvertToLocal(Point3D pt);

    // Convert a local projection co-ordinate to lat/lon
    Point3D ConvertFromLocal(Point3D pt);

private:
    Point3D origin;	// lat/lon of local area origin (the threshold)
    double theta;	// the rotation angle for alignment in radians
    double correction_factor;	// Reduction in surface distance per degree of longitude due to latitude.  Saves having to do a cos() every call.

};

#endif	// _FG_ATC_PROJECTION_HXX
