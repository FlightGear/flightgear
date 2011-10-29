#ifndef _YASIM_HXX
#define _YASIM_HXX

#include <FDM/flight.hxx>

namespace yasim { class FGFDM; };

class YASim : public FGInterface {
public:
    YASim(double dt);
    ~YASim();

    // Load externally set stuff into the FDM
    virtual void init();
    virtual void bind();
    virtual void reinit();

    // Run an iteration
    virtual void update(double dt);

 private:

    void report();
    void copyFromYASim();
    void copyToYASim(bool copyState);

    yasim::FGFDM* _fdm;
    float _dt;
    double _simTime;
    enum {
        NED,
        UVW,
        KNOTS,
        MACH
    } _speed_set;

};

#endif // _YASIM_HXX
