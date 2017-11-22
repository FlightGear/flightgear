#ifndef _WING_HPP
#define _WING_HPP

#include "Vector.hpp"
#include "Version.hpp"
#include "Math.hpp"
#include <simgear/props/props.hxx>

namespace yasim {

class Surface;
class Model;

struct FlapParams {
    float start {0};
    float end {0};
    float lift {0}; 
    float drag {0};
    float aoa {0};
};

struct StallParams {
    float aoa {0};
    float width {0};
    float peak {0};
};

// position and length of a chord line
struct Chord {
    float x {0};
    float y {0};
    float length {0};
};

enum WingFlaps {
    WING_FLAP0,
    WING_FLAP1,
    WING_SPOILER,
    WING_SLAT,
};

// FIXME: need to handle "inverted" controls for mirrored wings.
class Wing {
    SGPropertyNode_ptr _wingN {nullptr};
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
    float getChord() const { return _rootChordLength; };
    // fraction of chord at wing tip, 0..1
    float getTaper() const { return _taper; };
    // radians
    float getSweep() const { return _sweepAngleCenterLine; };
    // radians, positive is "up"
    void setDihedral(float dihedral) { _dihedral = dihedral; }
    float getDihedral() const { return _dihedral; };
    
    void setIncidence(float incidence);
    
    
    // parameters for stall curve
    void setStallParams(StallParams sp) { _stallParams = sp; }
    void setCamber(float camber) { _camber = camber; }
    void setInducedDrag(float drag) { _inducedDrag = drag; }
    
    
    void setFlapParams(WingFlaps i, FlapParams fp);

    // propergate the control axes value for the sub-surfaces
    void setFlapPos(WingFlaps i, float lval, float rval = 0);
    void setFlapEffectiveness(WingFlaps f, float lval);

    // Compile the thing into a bunch of Surface objects
    void compile();
    void getTip(float* tip) const { Math::set3(_tip, tip);};
    
    // valid only after Wing::compile() was called
    float getSpan() const { return _wingspan; };
    float getArea() const { return _wingspan*_meanChord; };
    float getAspectRatio() const { return _aspectRatio; };
    float getSMC() const { return _meanChord; };
    Chord getMAC() const { return _mac; };
    float getMACLength() const { return _mac.length; }; // get length of MAC
    float getMACx() const { return _mac.x; }; // get x-coord of MAC 
    float getMACy() const { return _base[1]+_mac.y; }; // get y-coord of MAC  
    
    
    int numSurfaces() const { return _surfs.size(); }
    Surface* getSurface(int n) { return ((SurfRec*)_surfs.get(n))->surface; }
    float getSurfaceWeight(int n) const { return ((SurfRec*)_surfs.get(n))->weight; }

    // The overall drag coefficient for the wing as a whole.  Units are
    // arbitrary.
    void setDragCoefficient(float scale);
    void multiplyDragCoefficient(float factor);

    // The ratio of force along the Z (lift) direction of each wing
    // segment to that along the X (drag) direction.
    void setLiftRatio(float ratio);
    float getLiftRatio() const { return _liftRatio; }

    void setPropertyNode(SGPropertyNode_ptr n) { _wingN = n; };
    float updateModel(Model* model);
    
private:
    void interp(const float* v1, const float* v2, const float frac, float* out);
    Surface* newSurface(float* pos, float* orient, float chord,
                        bool hasFlap0, bool hasFlap1, bool hasSlat, bool hasSpoiler);
    void calculateWingCoordinateSystem();
    void calculateTip();
    void calculateSpan();
    void calculateMAC();
    static Chord calculateMAC(Chord root, Chord tip);
    float calculateSweepAngleLeadingEdge();

    void addSurface(Surface* s, float weight, float twist);
    void writeInfoToProptree();
    
    struct SurfRec { Surface * surface; float weight; };
    // all surfaces of this wing
    Vector _surfs;      
    // surfaces having a certain type of flap (flap, slat, spoiler)
    Vector _flapSurfs[sizeof(WingFlaps)]; 

    Version * _version;
    bool _mirror {false};
    float _base[3] {0,0,0};
    float _rootChordLength {0};
    // length is distance from base to tip, not wing span
    float _length {0};
    float _taper {1};
    // sweep of center line, not leading edge!
    float _sweepAngleCenterLine {0}; 
    float _dihedral {0};
    
    // calculated from above
    float _orient[9];
    float _rightOrient[9];
    float _tip[3] {0,0,0};
    float _meanChord {0}; // std. mean chord
    Chord _mac; // mean aerodynamic chord (x,y) leading edge
    float _netSpan {0};
    float _wingspan {0};
    float _aspectRatio {1};

    StallParams _stallParams;
    
    float _twist {0};
    float _camber {0};
    float _incidence {0};
    float _inducedDrag {1};

    float _dragScale {1};
    float _liftRatio {1};

    FlapParams _flapParams[sizeof(WingFlaps)];

};

}; // namespace yasim
#endif // _WING_HPP
