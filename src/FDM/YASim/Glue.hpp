#ifndef _GLUE_HPP
#define _GLUE_HPP

#include "BodyEnvironment.hpp"

namespace yasim {

// The XYZ coordinate system has Z as the earth's axis, the Y axis
// pointing out the equator at zero longitude, and the X axis pointing
// out the middle of the western hemisphere.
class Glue {
public:
    static void calcAlphaBeta(State* s, float* wind, float* alpha, float* beta);

    // Calculates the instantaneous rotation velocities about each
    // axis.
    static void calcEulerRates(State* s,
			       float* roll, float* pitch, float* hdg);

    // Returns a global to "local" (north, east, down) matrix.  Note
    // that the latitude passed in is geoDETic.
    static void xyz2nedMat(double lat, double lon, float* out);

    // Conversion between a euler triplet and a matrix that transforms
    // "local" (north/east/down) coordinates to the aircraft frame.
    static void euler2orient(float roll, float pitch, float hdg,
                             float* out);
    static void orient2euler(float* o,
                             float* roll, float* pitch, float* hdg);

    static void geodUp(double lat, double lon, float* up);
    static void geodUp(double* pos, float* up);
};

}; // namespace yasim
#endif // _GLUE_HPP
