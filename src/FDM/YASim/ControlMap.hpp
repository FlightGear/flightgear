#ifndef _CONTROL_MAP_HPP
#define _CONTROL_MAP_HPP

#include "Vector.hpp"

namespace yasim {

class ControlMap {
public:
    ~ControlMap();

    enum OutputType { THROTTLE, MIXTURE, CONDLEVER, STARTER, MAGNETOS,
		      ADVANCE, REHEAT, PROP,
        BRAKE, STEER, EXTEND, HEXTEND, LEXTEND, LACCEL,
		      INCIDENCE, FLAP0, FLAP1, SLAT, SPOILER, VECTOR,
              FLAP0EFFECTIVENESS, FLAP1EFFECTIVENESS,
                      BOOST, CASTERING, PROPPITCH, PROPFEATHER,
                      COLLECTIVE, CYCLICAIL, CYCLICELE, ROTORENGINEON,
                      TILTYAW, TILTPITCH, TILTROLL,
                      ROTORBRAKE, ROTORENGINEMAXRELTORQUE, ROTORRELTARGET,
                      ROTORBALANCE, REVERSE_THRUST, WASTEGATE,
              WINCHRELSPEED, HITCHOPEN, PLACEWINCH, FINDAITOW
              };

    enum { OPT_SPLIT  = 0x01,
           OPT_INVERT = 0x02,
           OPT_SQUARE = 0x04 };

    // Returns a new, not-yet-used "input handle" for addMapping and
    // setInput.  This typically corresponds to one user axis.
    int newInput();

    // Adds a mapping to between input handle and a particular setting
    // on an output object.  The value of output MUST match the type
    // of object!
    void addMapping(int input, int output, void* object, int options=0);

    // An additional form to specify a mapping range.  Input values
    // outside of [src0:src1] are clamped, and are then mapped to
    // [dst0:dst1] before being set on the object.
    void addMapping(int input, int output, void* object, int options,
		    float src0, float src1, float dst0, float dst1);

    // Resets our accumulated input values.  Call before any
    // setInput() invokations.
    void reset();

    // Sets the specified input (as returned by newInput) to the
    // specified value.
    void setInput(int input, float value);

    // Calculates and applies the settings received since the last reset().
    void applyControls(float dt);

    // Returns the input/output range appropriate for the given
    // control.  Ailerons go from -1 to 1, while throttles are never
    // lower than zero, etc...
    static float rangeMin(int type);
    static float rangeMax(int type);

    // Each output record is identified by both an object/type tuple
    // and a numeric handle.
    int getOutputHandle(void* obj, int type);

    // Sets the transition time for the control output to swing
    // through its full range.
    void setTransitionTime(int handle, float time);

    // Retrieves the current value of the control output.  Controls
    // with OPT_SPLIT settable on inputs will have a separately
    // computed "right side" value.
    float getOutput(int handle);
    float getOutputR(int handle);

private:
    struct OutRec { int type; void* object; Vector maps;
                    float oldL, oldR, time; };
    struct MapRec  { OutRec* out; int idx; int opt; float val;
                     float src0; float src1; float dst0; float dst1; };

    // A list of (sub)Vectors containing a bunch of MapRec objects for
    // each input handle.
    Vector _inputs;

    // An unordered list of output settings.
    Vector _outputs;
};

}; // namespace yasim
#endif // _CONTROL_MAP_HPP
