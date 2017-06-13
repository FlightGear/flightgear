#ifndef _CONTROL_MAP_HPP
#define _CONTROL_MAP_HPP

#include <simgear/props/props.hxx>
#include "Vector.hpp"

namespace yasim {

class ControlMap {
public:
    ~ControlMap();

    enum Control { 
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
        ROTORENGINEON,
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
        FINDAITOW
    };

    enum { 
        OPT_SPLIT  = 0x01,
        OPT_INVERT = 0x02,
        OPT_SQUARE = 0x04
    };

    struct PropHandle {
        char* name {nullptr};
        int handle {0};
    };
    struct ObjectID {
        void* object {nullptr};
        int subObj {0};
    };
    
    // map control name to int (enum)
    Control parseControl(const char* name);
    // create ID from object and optional sub index (e.g. for wing section)
    ObjectID getObjectID(void* object, int subObj = 0);
    
    // add input property for a control to an object
    void addMapping(const char* prop, Control control, ObjectID id, int options = 0);

    // same with limits. Input values are clamped to [src0:src1] and then mapped to
    // [dst0:dst1] before being set on the objects control.
    void addMapping(const char* prop, Control control, ObjectID id, int options, float src0, float src1, float dst0, float dst1);

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
    static float rangeMin(Control control);
    static float rangeMax(Control control);

    // Each output record is identified by both an object/type tuple
    // and a numeric handle.
    int getOutputHandle(ObjectID id, Control control);

    // Sets the transition time for the control output to swing
    // through its full range.
    void setTransitionTime(int handle, float time);

    // Retrieves the current value of the control output.  Controls
    // with OPT_SPLIT settable on inputs will have a separately
    // computed "right side" value.
    float getOutput(int handle);
    float getOutputR(int handle);

    // register property name, return handle
    int getPropertyHandle(const char* name);
    int numProperties() { return _properties.size(); }
    PropHandle* getProperty(const int i) { return ((PropHandle*)_properties.get(i)); }

private:
    struct OutRec { 
        Control control;
        ObjectID oid;
        Vector maps;
        float oldL {0};
        float oldR {0};
        float time {0};
    };
    struct MapRec  { 
        OutRec* out {nullptr}; 
        int idx {0};
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
    // control properties
    Vector _properties;
};

}; // namespace yasim
#endif // _CONTROL_MAP_HPP
