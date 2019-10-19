#ifndef _SURFACE_HPP
#define _SURFACE_HPP

#include <simgear/props/props.hxx>
#include "Version.hpp"
#include "Math.hpp"
#include "Wing.hpp"

namespace yasim {

// FIXME: need a "chord" member for calculating moments.  Generic
// forces act at the center, but "pre-stall" lift acts towards the
// front, and flaps act (in both lift and drag) toward the back.
class Surface
{
    static int s_idGenerator;
    int _id;        //index for property tree

public:
    Surface(Version * version, const float* pos, float c0);

    int getID() const { return _id; };
    static void resetIDgen() { s_idGenerator = 0; };

    // Position of this surface in local coords
    void setPosition(const float* p);
    void getPosition(float* out) const { Math::set3(_pos, out); }

    // Distance scale along the X axis used for torque (pitch) calculation
    void setChord(float chord);

    // Slats act to move the stall peak by the specified angle, and
    // increase drag by the multiplier specified.
    void setSlatParams(float stallDelta, float dragPenalty);

    // Flaps add to lift coefficient, and multiply drag.
    void setFlapParams(float liftAdd, float dragPenalty);

    // Spoilers reduce the pre-stall lift, and multiply drag.
    void setSpoilerParams(float liftPenalty, float dragPenalty);

    // Positions for the controls, in the range [0:1].  [-1:1] for
    // flaps, with positive meaning "force goes towards positive Z"
    void setFlapPos(float pos);
    void setSlatPos(float pos);
    void setSpoilerPos(float pos);

    // Modifier for flap lift coefficient, useful for simulating flap blowing etc.
    void setFlapEffectiveness(float effectiveness) { _flapEffectiveness = effectiveness; }
    double getFlapEffectiveness() const { return _flapEffectiveness; }

    // local -> Surface coords
    void setOrientation(const float* o);

    // For variable-incidence control surfaces.  The angle is a
    // negative rotation about the surface's Y axis, in radians, so
    // positive is "up" (i.e. "positive AoA")
    void setIncidence(float angle);

    // The offset from base incidence for this surface.
    void setTwist(float angle);

    void  setTotalForceCoefficient(float c0) { _c0 = c0; }
    void  mulTotalForceCoefficient(float factor) { _c0 *= factor; }
    float getTotalForceCoefficient() const { return _c0; }
    
    void  setDragCoefficient(float cx) { _cx = cx; }
    void  mulDragCoefficient(float factor) { _cx *= factor; }
    float getDragCoefficient() const { return _cx; }
    void  setYDrag(float cy) { _cy = cy; }
    void  setLiftCoefficient(float cz) { _cz = cz; }
    float getLiftCoefficient() const { return _cz; }

    // zero-alpha Z drag ("camber") specified as a fraction of cz
    void setZeroAlphaLift(float cz0) { _cz0 = cz0; }

    // i: 0 == forward, 1 == backwards
    void setStallPeak(int i, float peak) { _peaks[i] = peak; }

    // i: 0 == fwd/+z, 1 == fwd/-z, 2 == rev/+z, 3 == rev/-z
    void setStall(int i, float alpha) { _stalls[i] = alpha; }
    void setStallWidth(int i, float width) { _widths[i] = width; }

    // Induced drag multiplier
    void setInducedDrag(float mul) { _inducedDrag = mul; }

    void calcForce(const float* v, const float rho, float mach, float* out, float* torque);

    float getAlpha() const { return _alpha; };
    float getStallAlpha() const { return _stallAlpha; };
    
    void setFlowRegime(FlowRegime flow) { _flow = flow; };
    FlowRegime getFlowRegime() { return _flow; };
    
    void setCriticalMachNumber(float mach) { _Mcrit = mach; };
    float getCriticalMachNumber() const { return _Mcrit; };
    
private:
    SGPropertyNode_ptr _surfN;
    Version * _version;
    
    float stallFunc(float* v);
    float flapLift(float alpha);
    float controlDrag(float lift, float drag);

    float _chord {0};     // X-axis size
    float _c0 {1};        // total force coefficient
    float _cx {1};        // X-axis force coefficient
    float _cy {1};        // Y-axis force coefficient
    float _cz {1};        // Z-axis force coefficient
    float _cz0 {0};       // Z-axis force offset
    float _peaks[2] {1, 1};  // Stall peak coefficients (fwd, back)
    float _stalls[4] {0, 0, 0, 0}; // Stall angles (fwd/back, pos/neg)
    float _widths[4] {0.01f, 0.01f, 0.01f, 0.01f}; // Stall widths
    float _pos[3];    // position in local coords
    float _orient[9]; // local->surface orthonormal matrix

    float _slatAlpha {0};
    float _slatDrag {1};
    float _flapLift {0};
    float _flapDrag {1};
    float _flapEffectiveness {1};
    float _spoilerLift {1};
    float _spoilerDrag {1};

    float _slatPos {0};
    float _flapPos {0};
    float _spoilerPos {0};
    float _incidence {0};
    float _twist {0};
    float _inducedDrag {1};
    
    // used during calculations
    float _stallAlpha {0};
    float _alpha {0};

    FlowRegime _flow{FLOW_SUBSONIC};
    float _Mcrit {0.6f};

    std::vector<float> pg_coefficients {-1.7671f, 0.4495f, 10.3423f, -6.9237f};
    
    SGPropertyNode* _fxN;
    SGPropertyNode* _fyN;
    SGPropertyNode* _fzN;
    SGPropertyNode* _stallAlphaN;
    SGPropertyNode* _alphaN;
    SGPropertyNode* _flapN;
    SGPropertyNode* _slatN;
    SGPropertyNode* _spoilerN;
    SGPropertyNode* _fabsN;
    SGPropertyNode* _incidenceN;
    SGPropertyNode* _twistN;
    SGPropertyNode* _pgCorrectionN;
    SGPropertyNode* _dcdwaveN;
};

}; // namespace yasim
#endif // _SURFACE_HPP
