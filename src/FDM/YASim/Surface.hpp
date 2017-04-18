#ifndef _SURFACE_HPP
#define _SURFACE_HPP

#include <simgear/props/props.hxx>
#include "Version.hpp"
#include "Math.hpp"

namespace yasim {

// FIXME: need a "chord" member for calculating moments.  Generic
// forces act at the center, but "pre-stall" lift acts towards the
// front, and flaps act (in both lift and drag) toward the back.
class Surface
{
    static int s_idGenerator;
    int _id;        //index for property tree

public:
    Surface( Version * version );

    int getID() const { return _id; };
    static void resetIDgen() { s_idGenerator = 0; };

    // Position of this surface in local coords
    void setPosition(const float* p);
    void getPosition(float* out) const { Math::set3(_pos, out); }

    // Distance scale along the X axis
    void setChord(float chord) { _chord = chord; }

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
    void setIncidence(float angle) { _incidence = angle; }

    // The offset from base incidence for this surface.
    void setTwist(float angle) { _twist = angle; }

    void setTotalDrag(float c0) { _c0 = c0; }
    float getTotalDrag() const { return _c0; }
    
    void setXDrag(float cx) { _cx = cx; }
    void setYDrag(float cy) { _cy = cy; }
    void setZDrag(float cz) { _cz = cz; }
    float getXDrag() const { return _cx; }

    // zero-alpha Z drag ("camber") specified as a fraction of cz
    void setBaseZDrag(float cz0) { _cz0 = cz0; }

    // i: 0 == forward, 1 == backwards
    void setStallPeak(int i, float peak) { _peaks[i] = peak; }

    // i: 0 == fwd/+z, 1 == fwd/-z, 2 == rev/+z, 3 == rev/-z
    void setStall(int i, float alpha) { _stalls[i] = alpha; }
    void setStallWidth(int i, float width) { _widths[i] = width; }

    // Induced drag multiplier
    void setInducedDrag(float mul) { _inducedDrag = mul; }

    void calcForce(const float* v, const float rho, float* out, float* torque);

    float getAlpha() const { return _alpha; };
    float getStallAlpha() const { return _stallAlpha; };
    
private:
    SGPropertyNode_ptr _surfN;
    
    float stallFunc(float* v);
    float flapLift(float alpha);
    float controlDrag(float lift, float drag);

    float _chord;     // X-axis size
    float _c0;        // total force coefficient
    float _cx;        // X-axis force coefficient
    float _cy;        // Y-axis force coefficient
    float _cz;        // Z-axis force coefficient
    float _cz0;       // Z-axis force offset
    float _peaks[2];  // Stall peak coefficients (fwd, back)
    float _stalls[4]; // Stall angles (fwd/back, pos/neg)
    float _widths[4]; // Stall widths  " "
    float _pos[3];    // position in local coords
    float _orient[9]; // local->surface orthonormal matrix

    float _slatAlpha;
    float _slatDrag;
    float _flapLift;
    float _flapDrag;
    float _flapEffectiveness;
    float _spoilerLift;
    float _spoilerDrag;

    float _slatPos;
    float _flapPos;
    float _spoilerPos;
    float _incidence;
    float _twist;
    float _inducedDrag;
    
    // used during calculations
    float _stallAlpha;
    float _alpha;

    Version * _version;
    SGPropertyNode* _fxN;
    SGPropertyNode* _fyN;
    SGPropertyNode* _fzN;
    SGPropertyNode* _stallAlphaN;
    SGPropertyNode* _alphaN;
    SGPropertyNode* _flapN;
    SGPropertyNode* _slatN;
    SGPropertyNode* _spoilerN;
    SGPropertyNode* _fabsN;
};

}; // namespace yasim
#endif // _SURFACE_HPP
