#ifndef _GLUE_HPP
#define _GLUE_HPP

#include "BodyEnvironment.hpp"

namespace yasim {

// The XYZ coordinate system has Z as the earth's axis, the Y axis
// pointing out the equator at zero longitude, and the X axis pointing
// out the middle of the western hemisphere.
class Glue {
public:
    static void calcAlphaBeta(State* s, float* alpha, float* beta);

    // Calculates the instantaneous rotation velocities about each
    // axis.
    static void calcEulerRates(State* s,
			       float* roll, float* pitch, float* hdg);

    static void xyz2geoc(double* xyz,
                         double* lat, double* lon, double* alt);
    static void geoc2xyz(double lat, double lon, double alt,
                         double* out);
    static void xyz2geod(double* xyz,
                         double* lat, double* lon, double* alt);
    static void geod2xyz(double lat, double lon, double alt,
                         double* out);

    static double geod2geocLat(double lat);
    static double geoc2geodLat(double lat);

    // Returns a global to "local" (north, east, down) matrix.  Note
    // that the latitude passed in is geoDETic.
    static void xyz2nedMat(double lat, double lon, float* out);

    // Conversion between a euler triplet and a matrix that transforms
    // "local" (north/east/down) coordinates to the aircraft frame.
    static void euler2orient(float roll, float pitch, float hdg,
                             float* out);
    static void orient2euler(float* o,
                             float* roll, float* pitch, float* hdg);

    // Returns a geodetic (i.e. gravitational, "level", etc...) "up"
    // vector for the specified xyz position.
    static void geodUp(double* pos, float* out);
};

}; // namespace yasim
#endif // _GLUE_HPP
