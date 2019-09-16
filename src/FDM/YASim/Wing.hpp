#ifndef _WING_HPP
#define _WING_HPP

#include "yasim-common.hpp"
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
    float z {0};
    float length {0};
};

enum WingFlaps {
    WING_FLAP0,
    WING_FLAP1,
    WING_SPOILER,
    WING_SLAT,
};

enum FlowRegime {
    FLOW_SUBSONIC,
    FLOW_TRANSONIC
};

class Wing {
    SGPropertyNode_ptr _wingN {nullptr};
    
    struct SurfRec { 
        Surface* surface; 
        float weight; 
    };
    
    struct WingSection {
        int _id;
        /// length and midpoint (not leading edge!) of root chord
        Chord _rootChord;
        /// length is distance from base to tip, not wing span
        float _length {0};
        float _taper {1};
        /// sweep of center line, not leading edge!
        float _sweepAngleCenterLine {0}; 
        float _dihedral {0};
        float _twist {0};
        float _camber {0}; 
        float _inducedDrag {1};

        StallParams _stallParams;

        ///fixed incidence of section as given in config XML
        float _sectionIncidence {0};
        float _dragScale {1};
        float _liftRatio {1};

        FlapParams _flapParams[sizeof(WingFlaps)];

        // calculated from above
        float _orient[9];
        float _rightOrient[9];
        Chord _tipChord;
        /// std. mean chord
        float _meanChord {0}; 
        /// mean aerodynamic chord, (x,y) leading edge!
        Chord _mac; 
        float _sectionSpan {0};
        /// all SurfRec of this wing
        Vector _surfs;      
        /// surfaces having a certain type of flap (flap, slat, spoiler)
        Vector _flapSurfs[sizeof(WingFlaps)];
        
        void calculateGeometry();
        void calculateWingCoordinateSystem();
        void calculateTipChord();
        void calculateSpan();
        void calculateMAC();
        float calculateSweepAngleLeadingEdge();
        /// update surfaces of wing section to (incidence + _sectionIncidence)
        /// e.g. for rotating hstab 
        void setIncidence(float incidence);
        // parameters for stall curve
        void setStallParams(StallParams sp) { _stallParams = sp; }

        // valid only after Wing::compile() was called
        Chord getMAC() const { return _mac; };
        float getMACx() const { return _mac.x; };
        float getMACy() const { return _mac.y; };
        float getMACz() const { return _mac.z; };
        float getArea() const { return _sectionSpan*_meanChord; };
        void setDragCoefficient(float scale);
        void multiplyDragCoefficient(float factor);
        // The ratio of force along the Z (lift) direction of each wing
        // segment to that along the X (drag) direction.
        void setLiftRatio(float ratio);
        void multiplyLiftRatio(float factor);
        float getLiftRatio() const { return _liftRatio; };

        void newSurface(Version* _version, float* pos, float* orient, float chord,
            bool hasFlap0, bool hasFlap1, bool hasSlat, bool hasSpoiler, 
            float weight, float twist, FlowRegime flow, float mcrit);
        int numSurfaces() const { return _surfs.size(); }
        Surface* getSurface(int n) { return ((SurfRec*)_surfs.get(n))->surface; }
        float getSurfaceWeight(int n) const { return ((SurfRec*)_surfs.get(n))->weight; }
    }; //struct WingSection

    //-- wing member variables --    
    Version* _version;
    bool _mirror {false};
    Vector _sections;
    // midpoint (not leading edge!) of wing root chord
    float _base[3] {0,0,0};
    // midpoint (not leading edge!) of wing tip
    float _tip[3] {0,0,0};
    float _netSpan {0};
    float _wingSpan {0};
    float _area {0};
    float _aspectRatio {0};
    float _meanChord {0};
    Chord _mac;
    float _taper {1.0f};
    float _incidence {0};
    float _incidenceMin {INCIDENCE_MIN};
    float _incidenceMax {INCIDENCE_MAX};
    float _weight {0};
    float _sweepLEMin {0};
    float _sweepLEMax {0};
    FlowRegime _flow {FLOW_SUBSONIC};
    float _Mcrit {1.0f};
    

    //-- private methods
    //copy float[3] to chord x,y,z
    static Chord _float2chord(float* pos, float lenght = 0);
    //copy chord x,y,z to float[3]
    static void _chord2float(Chord c, float* pos);
    void _interp(const float* v1, const float* v2, const float frac, float* out);
    void _writeInfoToProptree();
    Chord _weightedMeanChord(Chord a, float wa, Chord b, float wb);
    
public:
    Wing(Version* ver, bool mirror);
    ~Wing();

    int addWingSection(float* base, float chord, float wingLength, 
        float taper = 1, float sweep = 0, float dihedral = 0, float twist = 0, 
        float camber = 0, float idrag = 1, float incidence = 0);

    void setFlapParams(int section, WingFlaps type, FlapParams fp);
    void setSectionDrag(int section, float pdrag);
    void setSectionStallParams(int section, StallParams sp);
    
    // Compile the thing into a bunch of Surface objects
    void compile();
    void multiplyLiftRatio(float factor);
    void multiplyDragCoefficient(float factor);
    // setIncidence used to rotate (trim) the hstab
    bool setIncidence(float incidence);
    // limits for setIncidence
    void setIncidenceMin(float min) { _incidenceMin = min; };
    void setIncidenceMax(float max) { _incidenceMax = max; };
    float getIncidenceMin() const { return _incidenceMin; };
    float getIncidenceMax() const { return _incidenceMax; };
    void setFlowRegime(FlowRegime flow) { _flow = flow; };
    void setCriticalMachNumber(float m) { _Mcrit = m; };
    // write mass (= _weight * scale) to property tree
    void weight2mass(float scale);
    
    bool isMirrored() const { return _mirror; };
    void getBase(float* base) const { Math::set3(_base, base); };
    void getTip(float* tip) const { Math::set3(_tip, tip); };
    float getSpan() const { return _wingSpan; };
    float getTaper() const { return _taper; };
    float getArea() const;
    float getAspectRatio() const { return _aspectRatio; };
    float getSMC() const { return _meanChord; };
    float getMACLength() const { return _mac.length; }; // get length of MAC
    float getMACx() const { return _mac.x; }; // get x-coord of MAC leading edge 
    float getMACy() const { return _mac.y; }; // get y-coord of MAC leading edge 
    float getMACz() const { return _mac.z; }; // get z-coord of MAC leading edge 
    float getSweepLEMin() const { return _sweepLEMin; }; //min sweep angle of leading edge
    float getSweepLEMax() const { return _sweepLEMax; }; //max sweep angle of leading edge
    FlowRegime getFlowRegime() const { return _flow; };
    
    float getCriticalMachNumber() const { return _Mcrit; };   
//-----------------------------    
    // propergate the control axes value for the sub-surfaces
    void setFlapPos(WingFlaps type, float lval, float rval = 0);
    void setFlapEffectiveness(WingFlaps f, float lval);
    /// base node for exporting data of this wing to property tree
    void setPropertyNode(SGPropertyNode_ptr n) { _wingN = n; };
    float updateModel(Model* model);
    /// for YASim CLI tool
    void  printSectionInfo();
};

}; // namespace yasim
#endif // _WING_HPP
