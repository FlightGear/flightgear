#ifndef _FGGROUND_HPP
#define _FGGROUND_HPP

#include "Ground.hpp"

class FGInterface;
namespace simgear {
class BVHMaterial;
}

namespace yasim {

// The XYZ coordinate system has Z as the earth's axis, the Y axis
// pointing out the equator at zero longitude, and the X axis pointing
// out the middle of the western hemisphere.
class FGGround : public Ground {
public:
    FGGround(FGInterface *iface);
    virtual ~FGGround();

    virtual void getGroundPlane(const double pos[3],
                                double plane[4], float vel[3],
                                unsigned int &body);

    virtual void getGroundPlane(const double pos[3],
                                double plane[4], float vel[3],
                                const simgear::BVHMaterial **material,
                                unsigned int &body);

    virtual bool getBody(double t, double bodyToWorld[16], double linearVel[3],
                         double angularVel[3], unsigned int &id);

    virtual bool caughtWire(const double pos[4][3]);

    virtual bool getWire(double end[2][3], float vel[2][3]);

    virtual void releaseWire(void);

    virtual float getCatapult(const double pos[3],
                              double end[2][3], float vel[2][3]);

    void setTimeOffset(double toff);

private:
    FGInterface *_iface;
    double _toff;
};

}; // namespace yasim
#endif // _FGGROUND_HPP
