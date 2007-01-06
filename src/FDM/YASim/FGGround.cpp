#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <FDM/flight.hxx>
#include <simgear/scene/material/mat.hxx>

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
    double loadCapacity, frictionFactor, agl;
    double cp[3], dvel[3];
    int type;
    _iface->get_agl_m(_toff, pos, cp, plane, dvel,
                      &type, &loadCapacity, &frictionFactor, &agl);

    // The plane below the actual contact point.
    plane[3] = plane[0]*cp[0] + plane[1]*cp[1] + plane[2]*cp[2];

    for(int i=0; i<3; i++) vel[i] = dvel[i];
}

void FGGround::getGroundPlane(const double pos[3],
                              double plane[4], float vel[3],
                              int *type,
                              double *frictionFactor, 
                              double *rollingFriction,
                              double *loadCapacity,
                              double *loadResistance,
                              double *bumpiness,
                              bool *isSolid)
{
    // Return values for the callback.
    double agl;
    double cp[3], dvel[3];
    const SGMaterial* material;
    _iface->get_agl_m(_toff, pos, cp, plane, dvel,
                      type, &material, &agl);
    if (material) {
      *loadCapacity = material->get_load_resistence();
      *frictionFactor = material->get_friction_factor();
      *rollingFriction = material->get_rolling_friction();
      *loadResistance = material->get_load_resistence();
      *bumpiness = material->get_bumpiness();
      *isSolid = material->get_solid();
    } else {
      if (*type == FGInterface::Solid) {
        *loadCapacity = DBL_MAX;
        *frictionFactor = 1.0;
        *rollingFriction = 0.02;
        *loadResistance = DBL_MAX;
        *bumpiness = 0.0;
        *isSolid=true;
      } else if (*type == FGInterface::Water) {
        *loadCapacity = DBL_MAX;
        *frictionFactor = 1.0;
        *rollingFriction = 2;
        *loadResistance = DBL_MAX;
        *bumpiness = 0.8;
        *isSolid=false;
      } else {
        *loadCapacity = DBL_MAX;
        *frictionFactor = 0.9;
        *rollingFriction = 0.1;
        *loadResistance = DBL_MAX;
        *bumpiness = 0.2;
        *isSolid=true;
      }

    }
  
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

