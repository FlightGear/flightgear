#include "ATCProjection.hxx"
#include <math.h>
#include <simgear/constants.h>

#define DCL_PI  3.1415926535f
//#define SG_PI  ((SGfloat) M_PI)
#define DCL_DEGREES_TO_RADIANS  (DCL_PI/180.0)
#define DCL_RADIANS_TO_DEGREES  (180.0/DCL_PI)

FGATCProjection::FGATCProjection() {
    origin.setlat(0.0);
    origin.setlon(0.0);
    origin.setelev(0.0);
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

FGATCProjection::~FGATCProjection() {
}

void FGATCProjection::Init(Point3D centre) {
    origin = centre;
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

Point3D FGATCProjection::ConvertToLocal(Point3D pt) {
    double delta_lat = pt.lat() - origin.lat();
    double delta_lon = pt.lon() - origin.lon();

    double y = sin(delta_lat * DCL_DEGREES_TO_RADIANS) * SG_EQUATORIAL_RADIUS_M;
    double x = sin(delta_lon * DCL_DEGREES_TO_RADIANS) * SG_EQUATORIAL_RADIUS_M * correction_factor;

    return(Point3D(x,y,0.0));
}

Point3D FGATCProjection::ConvertFromLocal(Point3D pt) {
    return(Point3D(0,0,0));
}

/**********************************************************************************/

FGATCAlignedProjection::FGATCAlignedProjection() {
    origin.setlat(0.0);
    origin.setlon(0.0);
    origin.setelev(0.0);
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

FGATCAlignedProjection::~FGATCAlignedProjection() {
}

void FGATCAlignedProjection::Init(Point3D centre, double heading) {
    origin = centre;
    theta = heading * DCL_DEGREES_TO_RADIANS;
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

Point3D FGATCAlignedProjection::ConvertToLocal(Point3D pt) {
    // convert from lat/lon to orthogonal
    double delta_lat = pt.lat() - origin.lat();
    double delta_lon = pt.lon() - origin.lon();
    double y = sin(delta_lat * DCL_DEGREES_TO_RADIANS) * SG_EQUATORIAL_RADIUS_M;
    double x = sin(delta_lon * DCL_DEGREES_TO_RADIANS) * SG_EQUATORIAL_RADIUS_M * correction_factor;
    //cout << "Before alignment, x = " << x << " y = " << y << '\n';

    // Align
    double xbar = x;
    x = x*cos(theta) - y*sin(theta);
    y = (xbar*sin(theta)) + (y*cos(theta));
    //cout << "After alignment, x = " << x << " y = " << y << '\n';

    return(Point3D(x,y,0.0));
}

Point3D FGATCAlignedProjection::ConvertFromLocal(Point3D pt) {
    return(Point3D(0,0,0));
}
