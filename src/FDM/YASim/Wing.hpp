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
    Wing(Version *ver, bool mirror, float* base, float chord, float length, 
        float taper = 1, float sweep = 0, float dihedral = 0, float twist = 0);
    ~Wing();

    // Do we mirror ourselves about the XZ plane?
    void setMirror(bool mirror) { _mirror = mirror; }
    const bool isMirrored() { return _mirror; };
    
    // Wing geometry in local coordinates:
    
    // base point of wing
    void getBase(float* base) const { Math::set3(_base, base); };
    // dist. ALONG wing (not span!)     
    float getLength() const { return _length; };
    // at base, measured along X axis
    float getChord() const { return _chord; };
    // fraction of chord at wing tip, 0..1
    float getTaper() const { return _taper; };
    // radians
    float getSweep() const { return _sweep; };
    // radians, positive is "up"
    void setDihedral(float dihedral) { _dihedral = dihedral; }
    float getDihedral() const { return _dihedral; };
    
    void setIncidence(float incidence);
    
    
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
    void getTip(float* tip) const { Math::set3(_tip, tip);};
    
    // valid only after Wing::compile() was called
    float getSpan() const { return _wingspan; };
    float getArea() const { return _wingspan*_meanChord; };
    float getAspectRatio() const { return _aspectRatio; };
    float getSMC() const { return _meanChord; };
    float getMAC() const { return _mac; }; // get length of MAC
    float getMACx() const { return _macX; }; // get x-coord of MAC leading edge 
    float getMACy() const { return _base[1]+_macRootDistance; }; // get y-coord of MAC leading edge 
    
    
    int numSurfaces() const { return _surfs.size(); }
    Surface* getSurface(int n) { return ((SurfRec*)_surfs.get(n))->surface; }
    float getSurfaceWeight(int n) const { return ((SurfRec*)_surfs.get(n))->weight; }

    // The overall drag coefficient for the wing as a whole.  Units are
    // arbitrary.
    void setDragScale(float scale);
    float getDragScale() const { return _dragScale; }

    // The ratio of force along the Z (lift) direction of each wing
    // segment to that along the X (drag) direction.
    void setLiftRatio(float ratio);
    float getLiftRatio() const { return _liftRatio; }

private:
    void interp(const float* v1, const float* v2, const float frac, float* out);
    Surface* newSurface(float* pos, float* orient, float chord,
                        bool hasFlap0, bool hasFlap1, bool hasSlat, bool hasSpoiler);
    void calculateWingCoordinateSystem();
    void calculateTip();
    void calculateSpan();
    void calculateMAC();
    
    struct SurfRec { Surface * surface; float weight; };

    Vector _surfs;
    Vector _flap0Surfs;
    Vector _flap1Surfs;
    Vector _slatSurfs;
    Vector _spoilerSurfs;

    Version * _version;
    bool _mirror {false};
    float _base[3] {0,0,0};
    float _chord {0};
    float _length {0};
    float _taper {1};
    float _sweep {0};
    float _dihedral {0};
    
    // calculated from above
    float _orient[9];
    float _rightOrient[9];
    float _tip[3] {0,0,0};
    float _meanChord {0}; // std. mean chord
    float _mac {0}; // mean aerodynamic chord length
    float _macRootDistance {0}; // y-distance of mac from root
    float _macX {0}; // x-coordinate of mac (leading edge)
    float _netSpan {0};
    float _wingspan {0};
    float _aspectRatio {1};

    float _stall {0};
    float _stallWidth {0};
    float _stallPeak {0};
    float _twist {0};
    float _camber {0};
    float _incidence {0};
    float _inducedDrag {1};

    float _dragScale {1};
    float _liftRatio {1};

    float _flap0Start {0};
    float _flap0End {0};
    float _flap0Lift {0};
    float _flap0Drag {0};

    float _flap1Start {0};
    float _flap1End {0};
    float _flap1Lift {0};
    float _flap1Drag {0};

    float _spoilerStart {0};
    float _spoilerEnd {0};
    float _spoilerLift {0};
    float _spoilerDrag {0};

    float _slatStart {0};
    float _slatEnd {0};
    float _slatAoA {0};
    float _slatDrag {0};
};

}; // namespace yasim
#endif // _WING_HPP
