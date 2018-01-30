#ifndef _CONTROL_MAP_HPP
#define _CONTROL_MAP_HPP

#include <simgear/props/props.hxx>
#include "yasim-common.hpp"
#include "Vector.hpp"

namespace yasim {



    
class ControlMap {
public:
    ~ControlMap();
    
    //! keep this enum in sync with the static vector ControlNames in ControlMap.cpp !
    enum ControlType { 
        THROTTLE, 
        MIXTURE, 
        CONDLEVER, 
        STARTER, 
        MAGNETOS,
        ADVANCE, 
        REHEAT, 
        PROP,
        BRAKE,
        STEER,
        EXTEND,
        HEXTEND,
        LEXTEND,
        LACCEL,
        INCIDENCE,
        FLAP0,
        FLAP1,
        SLAT,
        SPOILER,
        VECTOR,
        FLAP0EFFECTIVENESS,
        FLAP1EFFECTIVENESS,
        BOOST,
        CASTERING,
        PROPPITCH,
        PROPFEATHER,
        COLLECTIVE, 
        CYCLICAIL, 
        CYCLICELE, 
        ROTORGEARENGINEON,
        TILTYAW, 
        TILTPITCH, 
        TILTROLL,
        ROTORBRAKE, 
        ROTORENGINEMAXRELTORQUE, 
        ROTORRELTARGET,
        ROTORBALANCE,
        REVERSE_THRUST,
        WASTEGATE,
        WINCHRELSPEED,
        HITCHOPEN,
        PLACEWINCH,
        FINDAITOW,
    }; //! keep this enum in sync with the static vector ControlNames in ControlMap.cpp !

        
    enum { 
        OPT_SPLIT  = 0x01,
        OPT_INVERT = 0x02,
        OPT_SQUARE = 0x04
    };

    struct PropHandle {
        char* name {nullptr};
        int handle {0};
    };
    // to identify controls per wing section we need wing object + section id 
    struct ObjectID {
        void* object {nullptr};
        int subObj {0};
    };
    
    // map control name to int (enum)
    static ControlType parseControl(const char* name);
    static ControlType getControlByName(const std::string& name);
    static std::string getControlName(ControlType c);
    // create ID from object and optional sub index (e.g. for wing section)
    static ObjectID getObjectID(void* object, int subObj = 0);
    
    // add input property for a control to an object

    // same with limits. Input values are clamped to [src0:src1] and then mapped to
    // [dst0:dst1] before being set on the objects control.
    void addMapping(const char* inputProp, ControlType control, ObjectID id, int options, float src0, float src1, float dst0, float dst1);

    // Resets our accumulated input values.  Call before any
    // setInput() invokations.
    void reset();

    // Sets the specified input (as returned by getPropertyHandle()) to the
    // specified value.
    void setInput(int propHandle, float value);

    /// Calculates and applies the settings received since the last reset(). 
    /// dt defaults to a large value used at solve time.
    void applyControls(float dt=1e6);

    // Returns the input/output range appropriate for the given
    // control.  Ailerons go from -1 to 1, while throttles are never
    // lower than zero, etc...
    static float rangeMin(ControlType control);
    static float rangeMax(ControlType control);

    // Each output record is identified by both an object/type tuple
    // and a numeric handle.
    int getOutputHandle(ObjectID id, ControlType control);
    
    // Sets the transition time for the control output to swing
    // through its full range.
    void setTransitionTime(int handle, float time);

    // Retrieves the current value of the control output.  Controls
    // with OPT_SPLIT settable on inputs will have a separately
    // computed "right side" value.
    float getOutput(int handle);
    float getOutputR(int handle);

    // register property name, return handle
    int getInputPropertyHandle(const char* name);
    int numProperties() { return _properties.size(); }
    PropHandle* getProperty(const int i);

private:
    //output data for a control of an object
    struct OutRec {
        int id {0};
        ControlType control;
        ObjectID oid;
        Vector maps;
        float transitionTime {0};
        float oldValueLeft {0};
        float oldValueRight {0};
    };
    struct MapRec  { 
        int id {0};
        int opt {0};
        float val {0};
        float src0 {0}; 
        float src1 {0}; 
        float dst0 {0}; 
        float dst1 {0};
    };

    // A list of (sub)Vectors containing a bunch of MapRec objects for
    // each input handle.
    Vector _inputs;

    // An unordered list of output settings.
    Vector _outputs;
    
    Vector _properties; // list of PropHandle*

    OutRec* getOutRec(ObjectID id, ControlType control);
};

}; // namespace yasim
#endif // _CONTROL_MAP_HPP
