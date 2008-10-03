#ifndef _WING_HPP
#define _WING_HPP

#include "Vector.hpp"

namespace yasim {

class Surface;

// FIXME: need to handle "inverted" controls for mirrored wings.
class Wing {
public:
    Wing();
    ~Wing();

    // Do we mirror ourselves about the XZ plane?
    void setMirror(bool mirror);

    // Wing geometry:
    void setBase(float* base);        // in local coordinates
    void setLength(float length);     // dist. ALONG wing (not span!)     
    void setChord(float chord);       // at base, measured along X axis
    void setTaper(float taper);       // fraction, 0-1
    void setSweep(float sweep);       // radians
    void setDihedral(float dihedral); // radians, positive is "up"

    void setStall(float aoa);
    void setStallWidth(float angle);
    void setStallPeak(float fraction);
    void setTwist(float angle);
    void setCamber(float camber);
    void setIncidence(float incidence);
    void setInducedDrag(float drag) { _inducedDrag = drag; }
    
    void setFlap0(float start, float end, float lift, float drag);
    void setFlap1(float start, float end, float lift, float drag);
    void setSpoiler(float start, float end, float lift, float drag);
    void setSlat(float start, float end, float aoa, float drag);

    // Set the control axes for the sub-surfaces
    void setFlap0(float lval, float rval);
    void setFlap1(float lval, float rval);
    void setSpoiler(float lval, float rval);
    void setSlat(float val);
    void setFlap0Effectiveness(float lval);
    void setFlap1Effectiveness(float lval);

    // Compile the thing into a bunch of Surface objects
    void compile();

    void getTip(float* tip);

    bool isMirrored();

    // Ground effect information
    float getGroundEffect(float* posOut);
    
    // Query the list of Surface objects
    int numSurfaces();
    Surface* getSurface(int n);
    float getSurfaceWeight(int n);

    // The overall drag coefficient for the wing as a whole.  Units are
    // arbitrary.
    void setDragScale(float scale);
    float getDragScale();

    // The ratio of force along the Z (lift) direction of each wing
    // segment to that along the X (drag) direction.
    void setLiftRatio(float ratio);
    float getLiftRatio();

private:
    void interp(float* v1, float* v2, float frac, float* out);
    Surface* newSurface(float* pos, float* orient, float chord,
                        bool flap0, bool flap1, bool slat, bool spoiler);

    struct SurfRec { Surface * surface; float weight; };

    Vector _surfs;
    Vector _flap0Surfs;
    Vector _flap1Surfs;
    Vector _slatSurfs;
    Vector _spoilerSurfs;

    bool _mirror;

    float _base[3];
    float _length;
    float _chord;
    float _taper;
    float _sweep;
    float _dihedral;

    float _stall;
    float _stallWidth;
    float _stallPeak;
    float _twist;
    float _camber;
    float _incidence;
    float _inducedDrag;

    float _dragScale;
    float _liftRatio;

    float _flap0Start;
    float _flap0End;
    float _flap0Lift;
    float _flap0Drag;

    float _flap1Start;
    float _flap1End;
    float _flap1Lift;
    float _flap1Drag;

    float _spoilerStart;
    float _spoilerEnd;
    float _spoilerLift;
    float _spoilerDrag;

    float _slatStart;
    float _slatEnd;
    float _slatAoA;
    float _slatDrag;
};

}; // namespace yasim
#endif // _WING_HPP
