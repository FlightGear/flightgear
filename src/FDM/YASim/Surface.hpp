#ifndef _SURFACE_HPP
#define _SURFACE_HPP

namespace yasim {

// FIXME: need a "chord" member for calculating moments.  Generic
// forces act at the center, but "pre-stall" lift acts towards the
// front, and flaps act (in both lift and drag) toward the back.
class Surface
{
public:
    Surface();

    // Position of this surface in local coords
    void setPosition(float* p);
    void getPosition(float* out);

    // Distance scale along the X axis
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
    void setFlap(float pos);
    void setSlat(float pos);
    void setSpoiler(float pos);

    // Modifier for flap lift coefficient, useful for simulating flap blowing etc.
    void setFlapEffectiveness(float effectiveness);
	double getFlapEffectiveness();

    // local -> Surface coords
    void setOrientation(float* o);

    // For variable-incidence control surfaces.  The angle is a
    // negative rotation about the surface's Y axis, in radians, so
    // positive is "up" (i.e. "positive AoA")
    void setIncidence(float angle);

    // The offset from base incidence for this surface.
    void setTwist(float angle);

    void setTotalDrag(float c0);
    float getTotalDrag();

    void setXDrag(float cx);
    void setYDrag(float cy);
    void setZDrag(float cz);

    // zero-alpha Z drag ("camber") specified as a fraction of cz
    void setBaseZDrag(float cz0);

    // i: 0 == forward, 1 == backwards
    void setStallPeak(int i, float peak);

    // i: 0 == fwd/+z, 1 == fwd/-z, 2 == rev/+z, 3 == rev/-z
    void setStall(int i, float alpha);
    void setStallWidth(int i, float width);

    // Induced drag multiplier
    void setInducedDrag(float mul) { _inducedDrag = mul; }

    void calcForce(float* v, float rho, float* forceOut, float* torqueOut);

private:
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
};

}; // namespace yasim
#endif // _SURFACE_HPP
