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
    for(i=0; i<4; i++)
	_stalls[i] = _widths[i] = 0;
    _orient[0] = 1; _orient[1] = 0; _orient[2] = 0;
    _orient[3] = 0; _orient[4] = 1; _orient[5] = 0;
    _orient[6] = 0; _orient[7] = 0; _orient[8] = 1;
    
    _incidence = 0;
    _slatPos = _spoilerPos = _flapPos = 0;
    _slatDrag = _spoilerDrag = _flapDrag = 1;
    _flapLift = 0;
    _slatAlpha = 0;
    _spoilerLift = 1;
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

    Math::mul3(1/vel, v, out);

    // Convert to the surface's coordinates
    Math::vmul33(_orient, out, out);

    // "Rotate" by the incidence angle.  Assume small angles, so we
    // need to diddle only the Z component, X is relatively unchanged
    // by small rotations.
    out[2] += _incidence * out[0]; // z' = z + incidence * x

    // Diddle the Z force according to our configuration
    float stallMul = stallFunc(out);
    stallMul *= 1 + _spoilerPos * (_spoilerLift - 1);
    float stallLift = (stallMul - 1) * _cz * out[2];
    float flapLift = _cz * _flapPos * (_flapLift-1);

    out[2] *= _cz;       // scaling factor
    out[2] += _cz*_cz0;  // zero-alpha lift
    out[2] += stallLift;
    out[2] += flapLift;

    // Airfoil lift (pre-stall and zero-alpha) torques "up" (negative
    // torque) around the Y axis, while flap lift pushes down.  Both
    // forces are considered to act at one third chord from the
    // center.  Convert to local (i.e. airplane) coordiantes and store
    // into "torque".
    torque[0] = 0;
    torque[1] = 0.33 * _chord * (flapLift - (_cz*_cz0 + stallLift));
    torque[2] = 0;
    Math::tmul33(_orient, torque, torque);

    // Diddle X (drag) and Y (side force) in the same manner
    out[0] *= _cx * controlDrag();
    out[1] *= _cy;

    // Reverse the incidence rotation to get back to surface
    // coordinates.
    out[2] -= _incidence * out[0];

    // Convert back to external coordinates
    Math::tmul33(_orient, out, out);

    // Add in the units to make a real force:
    float scale = 0.5*rho*vel*vel*_c0;
    Math::mul3(scale, out, out);
    Math::mul3(scale, torque, torque);
}

// Returns a multiplier for the "plain" force equations that
// approximates an airfoil's lift/stall curve.
float Surface::stallFunc(float* v)
{
    // Sanity check to treat FPU psychopathology
    if(v[0] == 0) return 1;

    float alpha = Math::abs(v[2]/v[0]);

    // Wacky use of indexing, see setStall*() methods.
    int fwdBak = v[0] > 0; // set if this is "backward motion"
    int posNeg = v[2] < 0; // set if the lift is toward -z
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
    float scale = 0.5*_peaks[fwdBak]/_stalls[i&2];

    // Before the stall
    if(alpha <= stallAlpha)
	return scale;

    // Inside the stall.  Compute a cubic interpolation between the
    // pre-stall "scale" value and the post-stall unity.
    float frac = (alpha - stallAlpha) / _widths[i];
    frac = frac*frac*(3-2*frac);

    return scale*(1-frac) + frac;
}

float Surface::controlDrag()
{
    float d = 1;
    d *= 1 + _spoilerPos * (_spoilerDrag - 1);
    d *= 1 + _slatPos * (_slatDrag - 1);

    // Negative flap deflections don't affect drag until their lift
    // multiplier exceeds the "camber" (cz0) of the surface.
    float fp = _flapPos;
    if(fp < 0) {
        fp = -fp;
        fp -= _cz0/(_flapLift-1);
        if(fp < 0) fp = 0;
    }

    d *= 1 + fp * (_flapDrag - 1);

    return d;
}

}; // namespace yasim
