#ifndef _FGFDM_HPP
#define _FGFDM_HPP

#include <simgear/xml/easyxml.hxx>
#include <simgear/props/props.hxx>

#include "Airplane.hpp"
#include "Vector.hpp"

namespace yasim {

class Wing;
class Version;

// This class forms the "glue" to the FlightGear codebase.  It handles
// parsing of XML airplane files, interfacing to the properties
// system, and providing data for the use of the FGInterface object.
class FGFDM : public XMLVisitor {
public:
    FGFDM();
    ~FGFDM();
    void init();
    void iterate(float dt);
    void getExternalInput(float dt=1e6);

    Airplane* getAirplane();

    // XML parsing callback from XMLVisitor
    virtual void startElement(const char* name, const XMLAttributes &atts);

    float getVehicleRadius(void) const { return _vehicle_radius; }

private:
    struct EngRec {
        std::string prefix;
        Thruster* eng {nullptr};
    };
    struct WeightRec {
        std::string prop;
        float size {0};
        int handle {0};
    };
    struct ControlOutput {
        int handle {0};
        SGPropertyNode* prop {nullptr};
        ControlMap::ControlType control;
        bool left {false};
        float min {0};
        float max {0};
    };

    void parseAirplane(const XMLAttributes* a);
    void parseApproachCruise(const XMLAttributes* a, const char* name);
    void parseSolveWeight(const XMLAttributes* a);
    void parseCockpit(const XMLAttributes* a);


    void setOutputProperties(float dt);

    void parseRotor(const XMLAttributes* a, const char* name);
    void parseRotorGear(const XMLAttributes* a);
    void parseWing(const XMLAttributes* a, const char* name, Airplane* airplane);
    int parseOutput(const char* name);
    void parseWeight(const XMLAttributes* a);
    void parseStall(const XMLAttributes* a);
    void parseFlap(const XMLAttributes* a, const char* name);

    void parseTurbineEngine(const XMLAttributes* a);
    void parsePistonEngine(const XMLAttributes* a);
    void parseElectricEngine(const XMLAttributes* a);
    void parsePropeller(const XMLAttributes* a);
    void parseThruster(const XMLAttributes* a);
    void parseJet(const XMLAttributes* a);
    void parseHitch(const XMLAttributes* a);
    void parseTow(const XMLAttributes* a);
    void parseWinch(const XMLAttributes* a);
    void parseGear(const XMLAttributes* a);
    void parseHook(const XMLAttributes* a);
    void parseLaunchbar(const XMLAttributes* a);
    void parseFuselage(const XMLAttributes* a);
    void parseTank(const XMLAttributes* a);
    void parseBallast(const XMLAttributes* a);
    void parseControlSetting(const XMLAttributes* a);
    void parseControlIn(const XMLAttributes* a);
    void parseControlOut(const XMLAttributes* a);
    void parseControlSpeed(const XMLAttributes* a);


    int attri(const XMLAttributes* atts, const char* attr);
    int attri(const XMLAttributes* atts, const char* attr, int def);
    float attrf(const XMLAttributes* atts, const char* attr);
    float attrf(const XMLAttributes* atts, const char* attr, float def);
    void attrf_xyz(const XMLAttributes* atts, float* out);
    double attrd(const XMLAttributes* atts, const char* attr);
    double attrd(const XMLAttributes* atts, const char* attr, double def);
    bool attrb(const XMLAttributes* atts, const char* attr);

    // The core Airplane object we manage.
    Airplane _airplane;

    // Aerodynamic turbulence model
    Turbulence* _turb;

    // Settable weights
    Vector _weights;

    // Engine types.  Contains an EngRec structure.
    Vector _thrusters;

    // Output properties for the ControlMap
    Vector _controlOutputs;

    // Radius of the vehicle, for intersection testing.
    float _vehicle_radius {0};

    // Parsing temporaries
    void* _currObj {nullptr};
    Airplane::Configuration _airplaneCfg;
    int _nextEngine {0};
    int _wingSection {0};

    class FuelProps
    {
    public:
        SGPropertyNode_ptr _out_of_fuel;
        SGPropertyNode_ptr _fuel_consumed_lbs;
    };

    class ThrusterProps
    {
    public:
        SGPropertyNode_ptr _running, _cranking;
        SGPropertyNode_ptr _prop_thrust, _thrust_lbs, _fuel_flow_gph;
        SGPropertyNode_ptr _rpm, _torque_ftlb, _mp_osi, _mp_inhg;
        SGPropertyNode_ptr _oil_temperature_degf, _boost_gauge_inhg;
        SGPropertyNode_ptr _n1, _n2, _epr, _egt_degf;
    };

    SGPropertyNode_ptr _turb_magnitude_norm, _turb_rate_hz;
    SGPropertyNode_ptr _gross_weight_lbs;
    SGPropertyNode_ptr _cg_x;
    SGPropertyNode_ptr _cg_y;
    SGPropertyNode_ptr _cg_z;
    SGPropertyNode_ptr _yasimN;

    std::vector<SGPropertyNode_ptr> _tank_level_lbs;
    std::vector<ThrusterProps> _thrust_props;
    std::vector<FuelProps> _fuel_props;
    SGPropertyNode_ptr _vxN;
    SGPropertyNode_ptr _vyN;
    SGPropertyNode_ptr _vzN;
    SGPropertyNode_ptr _vrxN;
    SGPropertyNode_ptr _vryN;
    SGPropertyNode_ptr _vrzN;
    SGPropertyNode_ptr _axN;
    SGPropertyNode_ptr _ayN;
    SGPropertyNode_ptr _azN;
    SGPropertyNode_ptr _arxN;
    SGPropertyNode_ptr _aryN;
    SGPropertyNode_ptr _arzN;
    SGPropertyNode_ptr _cg_xmacN;
};

}; // namespace yasim
#endif // _FGFDM_HPP
