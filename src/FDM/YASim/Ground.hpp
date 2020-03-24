#ifndef _GROUND_HPP
#define _GROUND_HPP

namespace simgear {
class BVHMaterial;
}
namespace yasim {

class Ground {
public:
    virtual ~Ground() = default;

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
};

}; // namespace yasim
#endif // _GROUND_HPP
