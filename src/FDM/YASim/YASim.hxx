#ifndef _YASIM_HXX
#define _YASIM_HXX

#include <FDM/flight.hxx>
#include <vector>

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

    class GearProps
    {
    public:
        GearProps(SGPropertyNode_ptr gear_root);

        SGPropertyNode_ptr has_brake;
        SGPropertyNode_ptr wow;
        SGPropertyNode_ptr compression_norm;
        SGPropertyNode_ptr compression_m;
        SGPropertyNode_ptr caster_angle_deg;
        SGPropertyNode_ptr rollspeed_ms;
        SGPropertyNode_ptr ground_is_solid;
        SGPropertyNode_ptr ground_friction_factor;
    };

    SGPropertyNode_ptr _crashed;
    SGPropertyNode_ptr _pressure_inhg, _temp_degc, _density_slugft3;
    SGPropertyNode_ptr _gear_agl_m, _gear_agl_ft;
    SGPropertyNode_ptr _pilot_g, _speed_setprop;
    SGPropertyNode_ptr _catapult_launch_cmd, _tailhook_position_norm;
    SGPropertyNode_ptr _launchbar_position_norm, _launchbar_holdback_pos_norm;
    SGPropertyNode_ptr _launchbar_state, _launchbar_strop;
    vector<GearProps> _gearProps;
};

#endif // _YASIM_HXX
