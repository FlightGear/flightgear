#ifndef _FGFDM_HPP
#define _FGFDM_HPP

#include <simgear/xml/easyxml.hxx>
#include <simgear/props/props.hxx>

#include "Airplane.hpp"
#include "Vector.hpp"

namespace yasim {

class Wing;

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

private:
    struct AxisRec { char* name; int handle; };
    struct EngRec { char* prefix; Thruster* eng; };
    struct WeightRec { char* prop; float size; int handle; };
    struct PropOut { SGPropertyNode* prop; int handle, type; bool left;
                     float min, max; };

    void setOutputProperties();

    Rotor* parseRotor(XMLAttributes* a, const char* name);
    Wing* parseWing(XMLAttributes* a, const char* name);
    int parseAxis(const char* name);
    int parseOutput(const char* name);
    void parseWeight(XMLAttributes* a);
    void parseTurbineEngine(XMLAttributes* a);
    void parsePistonEngine(XMLAttributes* a);
    void parsePropeller(XMLAttributes* a);
    bool eq(const char* a, const char* b);
    bool caseeq(const char* a, const char* b);
    char* dup(const char* s);
    int attri(XMLAttributes* atts, char* attr);
    int attri(XMLAttributes* atts, char* attr, int def); 
    float attrf(XMLAttributes* atts, char* attr);
    float attrf(XMLAttributes* atts, char* attr, float def); 
    bool attrb(XMLAttributes* atts, char* attr);

    // The core Airplane object we manage.
    Airplane _airplane;

    // Aerodynamic turbulence model
    Turbulence* _turb;

    // The list of "axes" that we expect to find as input.  These are
    // typically property names.
    Vector _axes;

    // Settable weights
    Vector _weights;

    // Engine types.  Contains an EngRec structure.
    Vector _thrusters;

    // Output properties for the ControlMap
    Vector _controlProps;

    // Parsing temporaries
    void* _currObj;
    bool _cruiseCurr;
    int _nextEngine;
};

}; // namespace yasim
#endif // _FGFDM_HPP
