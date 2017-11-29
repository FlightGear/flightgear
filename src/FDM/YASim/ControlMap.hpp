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

    struct PropHandle { char* name; int handle; };
    
    // map control name to int (enum)
    Control parseControl(const char* name);
    // Adds a mapping to between input handle and a particular setting
    // on an output object.  The value of output MUST match the type
    // of object!
    void addMapping(int input, Control control, void* object, int options=0);

    // An additional form to specify a mapping range.  Input values
    // outside of [src0:src1] are clamped, and are then mapped to
    // [dst0:dst1] before being set on the object.
    void addMapping(int input, Control control, void* object, int options,
		    float src0, float src1, float dst0, float dst1);

    // Resets our accumulated input values.  Call before any
    // setInput() invokations.
    void reset();

    // Sets the specified input (as returned by propertyHandle) to the
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
    int getOutputHandle(void* obj, Control control);

    // Sets the transition time for the control output to swing
    // through its full range.
    void setTransitionTime(int handle, float time);

    // Retrieves the current value of the control output.  Controls
    // with OPT_SPLIT settable on inputs will have a separately
    // computed "right side" value.
    float getOutput(int handle);
    float getOutputR(int handle);

    // register property name, return handle
    int propertyHandle(const char* name);
    int numProperties() { return _properties.size(); }
    PropHandle* getProperty(const int i) { return ((PropHandle*)_properties.get(i)); }

private:
    struct OutRec { 
        Control control; 
        void* object; 
        Vector maps;
        float oldL {0};
        float oldR {0};
        float time {0};
    };
    struct MapRec  { 
        OutRec* out; 
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

    // helper
    char* dup(const char* s);
    bool eq(const char* a, const char* b);
};

}; // namespace yasim
#endif // _CONTROL_MAP_HPP
