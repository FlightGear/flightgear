#ifndef _YASIM_HXX
#define _YASIM_HXX

#include <FDM/flight.hxx>

namespace yasim { class FGFDM; };

class YASim : public FGInterface {
public:
    YASim(double dt);

    // Load externally set stuff into the FDM
    virtual void init();

    // Run an iteration
    virtual bool update(int iterations);

 private:
    void report();
    void copyFromYASim();
    void copyToYASim(bool copyState);

    void printDEBUG();

    yasim::FGFDM* _fdm;
    int _updateCount;
    float _dt;
};

#endif // _YASIM_HXX
