#ifndef _WING_HPP
#define _WING_HPP

#include "Vector.hpp"
#include "Version.hpp"
#include "Math.hpp"

namespace yasim {

class Surface;

// FIXME: need to handle "inverted" controls for mirrored wings.
class Wing {
public:
    Wing( Version * version );
    ~Wing();

    // Do we mirror ourselves about the XZ plane?
    void setMirror(bool mirror) { _mirror = mirror; }
    bool isMirrored() { return _mirror; };
    
    // Wing geometry in local coordinates:
    
    // base point of wing
    void setBase(const float* base) { Math::set3(base, _base); }
    void getBase(float* base) { Math::set3(_base, base); };
    // dist. ALONG wing (not span!)     
    void setLength(float length) { _length = length; }
    float getLength() { return _length; };
    // at base, measured along X axis
    void setChord(float chord) { _chord = chord; }
    float getChord() { return _chord; };
    // fraction of chord at wing tip, 0..1
    void setTaper(float taper) { _taper = taper; }
    float getTaper() { return _taper; };
    // radians
    void setSweep(float sweep) { _sweep = sweep; }
    float getSweep() { return _sweep; };
    // radians, positive is "up"
    void setDihedral(float dihedral) { _dihedral = dihedral; }
    float getDihedral() { return _dihedral; };
    
    void setIncidence(float incidence);
    void setTwist(float angle) { _twist = angle; }
    
    
    // parameters for stall curve
    void setStall(float aoa) { _stall = aoa; }
    void setStallWidth(float angle) { _stallWidth = angle; }
    void setStallPeak(float fraction) { _stallPeak = fraction; }
    void setCamber(float camber) { _camber = camber; }
    void setInducedDrag(float drag) { _inducedDrag = drag; }
    
    
    void setFlap0Params(float start, float end, float lift, float drag);
    void setFlap1Params(float start, float end, float lift, float drag);
    void setSpoilerParams(float start, float end, float lift, float drag);
    void setSlatParams(float start, float end, float aoa, float drag);

    // Set the control axes for the sub-surfaces
    void setFlap0Pos(float lval, float rval);
    void setFlap1Pos(float lval, float rval);
    void setSpoilerPos(float lval, float rval);
    void setSlatPos(float val);
    void setFlap0Effectiveness(float lval);
    void setFlap1Effectiveness(float lval);

    // Compile the thing into a bunch of Surface objects
    void compile();
    void getTip(float* tip) { Math::set3(_tip, tip);};
    
    // valid only after Wing::compile() was called
    float getSpan() { return _wingspan; };
    float getArea() { return _wingspan*_meanChord; };
    float getAspectRatio() { return _aspectRatio; };
    float getMAC() { return _meanChord; };
    
    int numSurfaces() { return _surfs.size(); }
    Surface* getSurface(int n) { return ((SurfRec*)_surfs.get(n))->surface; }
    float getSurfaceWeight(int n) { return ((SurfRec*)_surfs.get(n))->weight; }

    // The overall drag coefficient for the wing as a whole.  Units are
    // arbitrary.
    void setDragScale(float scale);
    float getDragScale() { return _dragScale; }

    // The ratio of force along the Z (lift) direction of each wing
    // segment to that along the X (drag) direction.
    void setLiftRatio(float ratio);
    float getLiftRatio() { return _liftRatio; }

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
    
    // calculated from above
    float _tip[3];
    float _meanChord;
    float _wingspan;
    float _aspectRatio;

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

    Version * _version;
};

}; // namespace yasim
#endif // _WING_HPP
