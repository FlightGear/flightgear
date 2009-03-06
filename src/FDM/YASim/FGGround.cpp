#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <simgear/scene/material/mat.hxx>

#include <FDM/flight.hxx>

#include "Glue.hpp"
#include "Ground.hpp"

#include "FGGround.hpp"
namespace yasim {

FGGround::FGGround(FGInterface *iface) : _iface(iface)
{
    _toff = 0.0;
}

FGGround::~FGGround()
{
}

void FGGround::getGroundPlane(const double pos[3],
                              double plane[4], float vel[3])
{
    // Return values for the callback.
    double cp[3], dvel[3], dangvel[3];
    const SGMaterial* material;
    simgear::BVHNode::Id id;
    _iface->get_agl_m(_toff, pos, 2, cp, plane, dvel, dangvel, material, id);

    // The plane below the actual contact point.
    plane[3] = plane[0]*cp[0] + plane[1]*cp[1] + plane[2]*cp[2];

    for(int i=0; i<3; i++) vel[i] = dvel[i];
}

void FGGround::getGroundPlane(const double pos[3],
                              double plane[4], float vel[3],
                              const SGMaterial **material)
{
    // Return values for the callback.
    double cp[3], dvel[3], dangvel[3];
    simgear::BVHNode::Id id;
    _iface->get_agl_m(_toff, pos, 2, cp, plane, dvel, dangvel, *material, id);

    // The plane below the actual contact point.
    plane[3] = plane[0]*cp[0] + plane[1]*cp[1] + plane[2]*cp[2];

    for(int i=0; i<3; i++) vel[i] = dvel[i];
}

bool FGGround::caughtWire(const double pos[4][3])
{
    return _iface->caught_wire_m(_toff, pos);
}

bool FGGround::getWire(double end[2][3], float vel[2][3])
{
    double dvel[2][3];
    bool ret = _iface->get_wire_ends_m(_toff, end, dvel);
    for (int i=0; i<2; ++i)
      for (int j=0; j<3; ++j)
        vel[i][j] = dvel[i][j];
    return ret;
}

void FGGround::releaseWire(void)
{
    _iface->release_wire();
}

float FGGround::getCatapult(const double pos[3], double end[2][3],
                            float vel[2][3])
{
    double dvel[2][3];
    float dist = _iface->get_cat_m(_toff, pos, end, dvel);
    for (int i=0; i<2; ++i)
      for (int j=0; j<3; ++j)
        vel[i][j] = dvel[i][j];
    return dist;
}

void FGGround::setTimeOffset(double toff)
{
    _toff = toff;
}


}; // namespace yasim

