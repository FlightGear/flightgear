#ifndef _CONTROL_MAP_HPP
#define _CONTROL_MAP_HPP

#include "Vector.hpp"

namespace yasim {

class ControlMap {
public:
    ~ControlMap();

    enum OutputType { THROTTLE, MIXTURE, ADVANCE, REHEAT, PROP,
		      BRAKE, STEER, EXTEND,
		      INCIDENCE, FLAP0, FLAP1, SLAT, SPOILER };

    static const int OPT_SPLIT  = 0x01;
    static const int OPT_INVERT = 0x02;
    static const int OPT_SQUARE = 0x04;

    // Returns a new, not-yet-used "input handle" for addMapping and
    // setInput.  This typically corresponds to one user axis.
    int newInput();

    // Adds a mapping to between input handle and a particular setting
    // on an output object.  The value of output MUST match the type
    // of object!
    void addMapping(int input, int output, void* object, int options=0);

    // Resets our accumulated input values.  Call before any
    // setInput() invokations.
    void reset();

    // Sets the specified input (as returned by newInput) to the
    // specified value.
    void setInput(int input, float value);

    // Calculates and applies the settings received since the last reset().
    void applyControls();

private:
    struct OutRec { int type; void* object; int n;
	            float* values; Vector options; };
    struct MapRec  { OutRec* out; int idx; };

    // A list of (sub)Vectors containing a bunch of MapRec objects for
    // each input handle.
    Vector _inputs;

    // An unordered list of output settings.
    Vector _outputs;
};

}; // namespace yasim
#endif // _CONTROL_MAP_HPP
