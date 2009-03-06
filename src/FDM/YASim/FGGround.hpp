#ifndef _FGGROUND_HPP
#define _FGGROUND_HPP

#include "Ground.hpp"

class FGInterface;
class SGMaterial;

namespace yasim {

// The XYZ coordinate system has Z as the earth's axis, the Y axis
// pointing out the equator at zero longitude, and the X axis pointing
// out the middle of the western hemisphere.
class FGGround : public Ground {
public:
    FGGround(FGInterface *iface);
    virtual ~FGGround();

    virtual void getGroundPlane(const double pos[3],
                                double plane[4], float vel[3]);

    virtual void getGroundPlane(const double pos[3],
                                double plane[4], float vel[3],
                                const SGMaterial **material);

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
