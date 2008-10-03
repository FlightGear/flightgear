#include "Math.hpp"
#include "Surface.hpp"
namespace yasim {

Surface::Surface()
{
    // Start in a "sane" mode, so unset stuff doesn't freak us out
    _c0 = 1;
    _cx = _cy = _cz = 1;
    _cz0 = 0;
    _peaks[0] = _peaks[1] = 1;
    int i;
    for(i=0; i<4; i++) {
	_stalls[i] = 0;
        _widths[i] = 0.01; // half a degree
    }
    _orient[0] = 1; _orient[1] = 0; _orient[2] = 0;
    _orient[3] = 0; _orient[4] = 1; _orient[5] = 0;
    _orient[6] = 0; _orient[7] = 0; _orient[8] = 1;
    
    _chord = 0;
    _incidence = 0;
    _twist = 0;
    _slatPos = _spoilerPos = _flapPos = 0;
    _slatDrag = _spoilerDrag = _flapDrag = 1;

    _flapLift = 0;
    _flapEffectiveness = 1;
    _slatAlpha = 0;
    _spoilerLift = 1;
    _inducedDrag = 1;
}

void Surface::setPosition(float* p)
{
    int i;
    for(i=0; i<3; i++) _pos[i] = p[i];
}

void Surface::getPosition(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _pos[i];
}

void Surface::setChord(float chord)
{
    _chord = chord;
}

void Surface::setTotalDrag(float c0)
{
    _c0 = c0;
}

float Surface::getTotalDrag()
{
    return _c0;
}

void Surface::setXDrag(float cx)
{
    _cx = cx;
}

void Surface::setYDrag(float cy)
{
    _cy = cy;
}

void Surface::setZDrag(float cz)
{
    _cz = cz;
}

void Surface::setBaseZDrag(float cz0)
{
    _cz0 = cz0;
}

void Surface::setStallPeak(int i, float peak)
{
    _peaks[i] = peak;
}

void Surface::setStall(int i, float alpha)
{
    _stalls[i] = alpha;
}

void Surface::setStallWidth(int i, float width)
{
    _widths[i] = width;
}

void Surface::setOrientation(float* o)
{
    int i;
    for(i=0; i<9; i++)
        _orient[i] = o[i];
}

void Surface::setIncidence(float angle)
{
    _incidence = angle;
}

void Surface::setTwist(float angle)
{
    _twist = angle;
}

void Surface::setSlatParams(float stallDelta, float dragPenalty)
{
    _slatAlpha = stallDelta;
    _slatDrag = dragPenalty;
}

void Surface::setFlapParams(float liftAdd, float dragPenalty)
{
    _flapLift = liftAdd;
    _flapDrag = dragPenalty;
}

void Surface::setSpoilerParams(float liftPenalty, float dragPenalty)
{
    _spoilerLift = liftPenalty;
    _spoilerDrag = dragPenalty;
}

void Surface::setFlap(float pos)
{
    _flapPos = pos;
}

void Surface::setFlapEffectiveness(float effectiveness)
{
    _flapEffectiveness = effectiveness;
}

double Surface::getFlapEffectiveness()
{
    return _flapEffectiveness;
}


void Surface::setSlat(float pos)
{
    _slatPos = pos;
}

void Surface::setSpoiler(float pos)
{
    _spoilerPos = pos;
}

// Calculate the aerodynamic force given a wind vector v (in the
// aircraft's "local" coordinates) and an air density rho.  Returns a
// torque about the Y axis, too.
void Surface::calcForce(float* v, float rho, float* out, float* torque)
{
    // Split v into magnitude and direction:
    float vel = Math::mag3(v);

    // Handle the blowup condition.  Zero velocity means zero force by
    // definition.
    if(vel == 0) {
        int i;
	for(i=0; i<3; i++) out[i] = torque[i] = 0;
	return;
    }

    // special case this so the logic below doesn't produce a non-zero
    // force; should probably have a "no force" flag instead...
    if(_cx == 0. && _cy == 0. && _cz == 0.) {
        for(int i=0; i<3; i++) out[i] = torque[i] = 0.;
        return;
    }

    Math::mul3(1/vel, v, out);

    // Convert to the surface's coordinates
    Math::vmul33(_orient, out, out);

    // "Rotate" by the incidence angle.  Assume small angles, so we
    // need to diddle only the Z component, X is relatively unchanged
    // by small rotations.
    float incidence = _incidence + _twist;
    out[2] += incidence * out[0]; // z' = z + incidence * x

    // Hold onto the local wind vector so we can multiply the induced
    // drag at the end.
    float lwind[3];
    Math::set3(out, lwind);
    
    // Diddle the Z force according to our configuration
    float stallMul = stallFunc(out);
    stallMul *= 1 + _spoilerPos * (_spoilerLift - 1);
    float stallLift = (stallMul - 1) * _cz * out[2];
    float flaplift = flapLift(out[2]);

    out[2] *= _cz;       // scaling factor
    out[2] += _cz*_cz0;  // zero-alpha lift
    out[2] += stallLift;
    out[2] += flaplift;

    // Airfoil lift (pre-stall and zero-alpha) torques "up" (negative
    // torque) around the Y axis, while flap lift pushes down.  Both
    // forces are considered to act at one third chord from the
    // edge.  Convert to local (i.e. airplane) coordiantes and store
    // into "torque".
    torque[0] = 0;
    torque[1] = 0.1667f * _chord * (flaplift - (_cz*_cz0 + stallLift));
    torque[2] = 0;
    Math::tmul33(_orient, torque, torque);

    // The X (drag) force gets diddled for control deflection
    out[0] = controlDrag(out[2], _cx * out[0]);

    // Add in any specific Y (side force) coefficient.
    out[1] *= _cy;

    // Diddle the induced drag
    Math::mul3(-1*_inducedDrag*out[2]*lwind[2], lwind, lwind);
    Math::add3(lwind, out, out);

    // Reverse the incidence rotation to get back to surface
    // coordinates.
    out[2] -= incidence * out[0];

    // Convert back to external coordinates
    Math::tmul33(_orient, out, out);

    // Add in the units to make a real force:
    float scale = 0.5f*rho*vel*vel*_c0;
    Math::mul3(scale, out, out);
    Math::mul3(scale, torque, torque);
}

#if 0
void Surface::test()
{
    static const float DEG2RAD = 0.0174532925199;
    float v[3], force[3], torque[3];
    float rho = Atmosphere::getStdDensity(0);
    float spd = 30;

    setFlap(0);
    setSlat(0);
    setSpoiler(0);

    for(float angle = -90; angle<90; angle += 0.01) {
        float rad = angle * DEG2RAD;
        v[0] = spd * -Math::cos(rad);
        v[1] = 0;
        v[2] = spd * Math::sin(rad);
        calcForce(v, rho, force, torque);
        float lift = force[2] * Math::cos(rad) + force[0] * Math::sin(rad);
        //__builtin_printf("%f %f\n", angle, lift);
        __builtin_printf("%f %f\n", angle, torque[2]);
    }
}
#endif

// Returns a multiplier for the "plain" force equations that
// approximates an airfoil's lift/stall curve.
float Surface::stallFunc(float* v)
{
    // Sanity check to treat FPU psychopathology
    if(v[0] == 0) return 1;

    float alpha = Math::abs(v[2]/v[0]);

    // Wacky use of indexing, see setStall*() methods.
    int fwdBak = v[0] > 0; // set if this is "backward motion"
    int posNeg = v[2] < 0; // set if the airflow is toward -z
    int i = (fwdBak<<1) | posNeg;

    float stallAlpha = _stalls[i];
    if(stallAlpha == 0)
        return 1;

    if(i == 0)
	stallAlpha += _slatAlpha;

    // Beyond the stall
    if(alpha > stallAlpha+_widths[i])
	return 1;

    // (note mask: we want to use the "positive" stall angle here)
    float scale = 0.5f*_peaks[fwdBak]/_stalls[i&2];

    // Before the stall
    if(alpha <= stallAlpha)
	return scale;

    // Inside the stall.  Compute a cubic interpolation between the
    // pre-stall "scale" value and the post-stall unity.
    float frac = (alpha - stallAlpha) / _widths[i];
    frac = frac*frac*(3-2*frac);

    return scale*(1-frac) + frac;
}

// Similar to the above -- interpolates out the flap lift past the
// stall alpha
float Surface::flapLift(float alpha)
{
    float flapLift = _cz * _flapPos * (_flapLift-1) * _flapEffectiveness;

    if(_stalls[0] == 0)
        return 0;

    if(alpha < 0) alpha = -alpha;
    if(alpha < _stalls[0])
        return flapLift;
    else if(alpha > _stalls[0] + _widths[0])
        return 0;

    float frac = (alpha - _stalls[0]) / _widths[0];
    frac = frac*frac*(3-2*frac);
    return flapLift * (1-frac);
}

float Surface::controlDrag(float lift, float drag)
{
    // Negative flap deflections don't affect drag until their lift
    // multiplier exceeds the "camber" (cz0) of the surface.  Use a
    // synthesized "fp" number instead of the actual flap position.
    float fp = _flapPos;
    if(fp < 0) {
        fp = -fp;
        fp -= _cz0/(_flapLift-1);
        if(fp < 0) fp = 0;
    }
    
    // Calculate an "effective" drag -- this is the drag that would
    // have been produced by an unflapped surface at the same lift.
    float flapDragAoA = (_flapLift - 1 - _cz0) * _stalls[0];
    float fd = Math::abs(lift * flapDragAoA * fp);
    if(drag < 0) fd = -fd;
    drag += fd;

    // Now multiply by the various control factors
    drag *= 1 + fp * (_flapDrag - 1);
    drag *= 1 + _spoilerPos * (_spoilerDrag - 1);
    drag *= 1 + _slatPos * (_slatDrag - 1);

    return drag;
}

}; // namespace yasim
