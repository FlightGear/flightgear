// Utility functions for the ATC / AI subsytem declarations

#include <math.h>
#include <simgear/math/point3d.hxx>

// Given two positions, get the HORIZONTAL separation
double dclGetHorizontalSeparation(Point3D pos1, Point3D pos2);

// Given a position (lat/lon/elev), heading, vertical angle, and distance, calculate the new position.
// Assumes that the ground is not hit!!!  Expects heading and angle in degrees, distance in meters.
Point3D dclUpdatePosition(Point3D pos, double heading, double angle, double distance);
