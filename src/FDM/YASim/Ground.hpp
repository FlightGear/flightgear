#ifndef _GROUND_HPP
#define _GROUND_HPP

class SGMaterial;
namespace yasim {

class Ground {
public:
    Ground();
    virtual ~Ground();

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
};

}; // namespace yasim
#endif // _GROUND_HPP
