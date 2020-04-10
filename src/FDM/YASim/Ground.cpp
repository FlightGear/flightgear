#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "Glue.hpp"

#include <simgear/bvh/BVHMaterial.hxx>
#include "Ground.hpp"
namespace yasim {

void Ground::getGroundPlane(const double pos[3],
                            double plane[4], float vel[3],
                            unsigned int &body)
{
    // ground.  Calculate a cartesian coordinate for the ground under
    // us, find the (geodetic) up vector normal to the ground, then
    // use that to find the final (radius) term of the plane equation.
    float up[3];
    Glue::geodUp((double*)pos, up);
    int i;
    for(i=0; i<3; i++) plane[i] = up[i];
    plane[3] = plane[0]*pos[0] + plane[1]*pos[1] + plane[2]*pos[2];

    vel[0] = 0.0;
    vel[1] = 0.0;
    vel[2] = 0.0;

    body = 0;
}

void Ground::getGroundPlane(const double pos[3],
                            double plane[4], float vel[3],
                            const simgear::BVHMaterial **material,
                            unsigned int &body)
{
    getGroundPlane(pos,plane,vel,body);
}

bool Ground::getBody(double t, double bodyToWorld[16], double linearVel[3],
                     double angularVel[3], unsigned int &body)
{
    return false;
}

bool Ground::caughtWire(const double pos[4][3])
{
    return false;
}

bool Ground::getWire(double end[2][3], float vel[2][3])
{
    return false;
}

void Ground::releaseWire(void)
{
}

float Ground::getCatapult(const double pos[3], double end[2][3],
                          float vel[2][3])
{
    return 1e10;
}

}; // namespace yasim

