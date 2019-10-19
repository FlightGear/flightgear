#include <Main/fg_props.hxx>
#include "yasim-common.hpp"
#include "Math.hpp"
#include "Surface.hpp"

namespace yasim {
int Surface::s_idGenerator = 0;

Surface::Surface(Version* version, const float* pos, float c0 = 1 ) :
    _version(version),
    _c0(c0)
{
    _id = s_idGenerator++;

    _orient[0] = 1; _orient[1] = 0; _orient[2] = 0;
    _orient[3] = 0; _orient[4] = 1; _orient[5] = 0;
    _orient[6] = 0; _orient[7] = 0; _orient[8] = 1;
    
    Math::set3(pos, _pos);
    
    _surfN = fgGetNode("/fdm/yasim/debug/surfaces", true);
    if (_surfN != 0) {
        _surfN = _surfN->getChild("surface", _id, true);
        _fxN = _surfN->getNode("f-x", true);
        _fyN = _surfN->getNode("f-y", true);
        _fzN = _surfN->getNode("f-z", true);
        _fabsN = _surfN->getNode("f-abs", true);
        _alphaN = _surfN->getNode("alpha", true);
        _stallAlphaN = _surfN->getNode("stall-alpha", true);
        _flapN = _surfN->getNode("flap-pos", true);
        _slatN = _surfN->getNode("slat-pos", true);
        _spoilerN = _surfN->getNode("spoiler-pos", true);
        _incidenceN = _surfN->getNode("incidence-deg", true);
        _incidenceN->setFloatValue(0);
        _twistN = _surfN->getNode("twist-deg", true);
        _twistN->setFloatValue(0);
        _pgCorrectionN = _surfN->getNode("pg-correction", true);
        _pgCorrectionN->setFloatValue(1);
        _dcdwaveN = _surfN->getNode("wavedrag", true);
        _dcdwaveN->setFloatValue(1);        
        _surfN->getNode("pos-x", true)->setFloatValue(pos[0]);
        _surfN->getNode("pos-y", true)->setFloatValue(pos[1]);
        _surfN->getNode("pos-z", true)->setFloatValue(pos[2]);
        _surfN->getNode("chord",true)->setFloatValue(0);
        _surfN->getNode("axis-x", true)->setFloatValue(0);
        _surfN->getNode("axis-y", true)->setFloatValue(0);
        _surfN->getNode("axis-z", true)->setFloatValue(0);
    }
}


void Surface::setPosition(const float* pos)
{
    Math::set3(pos, _pos);
    if (_surfN != 0) {
        _surfN->getNode("pos-x", true)->setFloatValue(pos[0]);
        _surfN->getNode("pos-y", true)->setFloatValue(pos[1]);
        _surfN->getNode("pos-z", true)->setFloatValue(pos[2]);
    }
}

void Surface::setChord(float chord) 
{
    _chord = chord;
    if (_surfN != 0) {
        _surfN->getNode("chord",true)->setFloatValue(_chord);
    }
}

void Surface::setOrientation(const float* o)
{
    for(int i=0; i<9; i++) _orient[i] = o[i];
    if (_surfN) {
        // export the chord line (transformed into aircraft coordiantes)
        float xaxis[3] {1,0,0};
        Math::tmul33(_orient,xaxis, xaxis);
        _surfN->getNode("axis-x", true)->setFloatValue(xaxis[0]);
        _surfN->getNode("axis-y", true)->setFloatValue(xaxis[1]);
        _surfN->getNode("axis-z", true)->setFloatValue(xaxis[2]);
    }
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

void Surface::setFlapPos(float pos)
{
  if (_flapPos != pos) {
    _flapPos = pos;
    if (_surfN != 0) {
        _flapN->setFloatValue(pos);
    }
  }
}

void Surface::setSlatPos(float pos)
{
  if (_slatPos != pos) {
    _slatPos = pos;
    if (_surfN != 0) {
        _slatN->setFloatValue(pos);
    }
  }
}

void Surface::setSpoilerPos(float pos)
{
  if (_spoilerPos != pos) {
    _spoilerPos = pos;
    if (_surfN != 0) _spoilerN->setFloatValue(pos);
  }
}


// Calculate the aerodynamic force given a wind vector v (in the
// aircraft's "local" coordinates), an air density rho and the freestream 
// mach number (for compressibility correction).  Returns a torque about 
// the Y axis ("pitch"), too.
void Surface::calcForce(const float* v, const float rho, float mach, float* out, float* torque)
{
    // initialize outputs to zero
    Math::zero3(out);
    Math::zero3(torque);
    
    // Split v into magnitude and direction:
    float vel = Math::mag3(v);

    // Zero velocity means zero force by definition (also prevents div0).
    if(vel == 0) {
        return;
    }

    // special case this so the logic below doesn't produce a non-zero
    // force; should probably have a "no force" flag instead...
    if(_cx == 0. && _cy == 0. && _cz == 0.) {
        return;
    }
    
    // Normalize wind and convert to the surface's coordinates
    Math::mul3(1/vel, v, out);
    Math::vmul33(_orient, out, out);

    // "Rotate" by the incidence angle.  Assume small angles, so we
    // need to diddle only the Z component, X is relatively unchanged
    // by small rotations. sin(a) ~ a, cos(a) ~ 1 for small a
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

    //compute Prandtl/Glauert compressibility factor
    float pg_correction {1};
    float wavedrag {0};
    if (_flow == FLOW_TRANSONIC) {
        if (mach < 0.8f) {
            pg_correction = 1.0f/sqrt(1.0f-(mach*mach));
        }
        if ((mach >= 0.8f) && (mach < 1.2f)) {
            pg_correction = Math::polynomial(pg_coefficients, mach);
        }
        if (mach >= 1.2f) {
            pg_correction = 2.0f/(((mach*mach)-1.0f)*YASIM_PI);
        }
        out[2] *= pg_correction;

        // Add mach dependent wave drag (Perkins and Hage)
        if (mach > _Mcrit) {
            wavedrag = 9.5f * Math::pow((mach > 1.0f ? 1.0f : mach)-_Mcrit, 2.8f) + 0.00193f;
            out[0] += wavedrag;
        } 
    }


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
    
    // Add mach dependent wave drag
    

    // Reverse the incidence rotation to get back to surface
    // coordinates. Since out[] is now the force vector and is
    // roughly parallel with Z, the small-angle approximation
    // must change its X component.
    if( _version->isVersionOrNewer( Version::YASIM_VERSION_32 )) {
        out[0] += incidence * out[2];
    } else {
        out[2] -= incidence * out[0];
    }

    // Convert back to external coordinates
    Math::tmul33(_orient, out, out);

    // Add in the units to make a real force:
    float scale = 0.5f*rho*vel*vel*_c0;
    Math::mul3(scale, out, out);
    Math::mul3(scale, torque, torque);
    // if we have a property tree, export info
    if (_surfN != 0) {
      _fabsN->setFloatValue(Math::mag3(out));
      _fxN->setFloatValue(out[0]);
      _fyN->setFloatValue(out[1]);
      _fzN->setFloatValue(out[2]);
      _alphaN->setFloatValue(_alpha);
      _stallAlphaN->setFloatValue(_stallAlpha);      
      _pgCorrectionN->setFloatValue(pg_correction);
      _dcdwaveN->setFloatValue(wavedrag);
    }
}

#if 0
void Surface::test()
{
    static const float DEG2RAD = 0.0174532925199;
    float v[3], force[3], torque[3];
    float rho = Atmosphere::getStdDensity(0);
    float spd = 30;

    setFlapPos(0);
    setSlatPos(0);
    setSpoilerPos(0);

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

    _alpha = Math::abs(v[2]/v[0]);

    // Wacky use of indexing, see setStall*() methods.
    int fwdBak = v[0] > 0; // set if this is "backward motion"
    int posNeg = v[2] < 0; // set if the airflow is toward -z
    int i = (fwdBak<<1) | posNeg;

    _stallAlpha = _stalls[i];
    if(_stallAlpha == 0)
        return 1;

    // consider slat position, moves the stall aoa some degrees
    if(i == 0) {
        if( _version->isVersionOrNewer( Version::YASIM_VERSION_32 )) {
            _stallAlpha += _slatPos * _slatAlpha;
        } else {
            _stallAlpha += _slatAlpha;
        }
    }

    // Beyond the stall
    if(_alpha > _stallAlpha+_widths[i])
	return 1;

    // (note mask: we want to use the "positive" stall angle here)
    float scale = 0.5f*_peaks[fwdBak]/_stalls[i&2];

    // Before the stall
    if(_alpha <= _stallAlpha)
	return scale;

    // Inside the stall.  Compute a cubic interpolation between the
    // pre-stall "scale" value and the post-stall unity.
    float frac = (_alpha - _stallAlpha) / _widths[i];
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


void Surface::setIncidence(float angle) {
    _incidence = angle * -1;
    if (_surfN != 0) {
        _incidenceN->setFloatValue(angle * RAD2DEG);
    }
}


void Surface::setTwist(float angle) {
    _twist = angle * -1;
    if (_surfN != 0) {
        _twistN->setFloatValue(angle * RAD2DEG);
    }
}

}; // namespace yasim
