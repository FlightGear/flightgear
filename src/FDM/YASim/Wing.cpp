#include "Math.hpp"
#include "Surface.hpp"
#include "Wing.hpp"

namespace yasim {

Wing::Wing()
{
    _mirror = false;
    _base[0] = _base[1] = _base[2] = 0;
    _length = 0;
    _chord = 0;
    _taper = 0;
    _sweep = 0;
    _dihedral = 0;
    _stall = 0;
    _stallWidth = 0;
    _stallPeak = 0;
    _twist = 0;
    _camber = 0;
    _incidence = 0;
    _inducedDrag = 1;
    _dragScale = 1;
    _liftRatio = 1;
    _flap0Start = 0;
    _flap0End = 0;
    _flap0Lift = 0;
    _flap0Drag = 0;
    _flap1Start = 0;
    _flap1End = 0;
    _flap1Lift = 0;
    _flap1Drag = 0;
    _spoilerStart = 0;
    _spoilerEnd = 0;
    _spoilerLift = 0;
    _spoilerDrag = 0;
    _slatStart = 0;
    _slatEnd = 0;
    _slatAoA = 0;
    _slatDrag = 0;
}

Wing::~Wing()
{
    int i;
    for(i=0; i<_surfs.size(); i++) {
        SurfRec* s = (SurfRec*)_surfs.get(i);
        delete s->surface;
        delete s;
    }
}

int Wing::numSurfaces()
{
    return _surfs.size();
}

Surface* Wing::getSurface(int n)
{
    return ((SurfRec*)_surfs.get(n))->surface;
}

float Wing::getSurfaceWeight(int n)
{
    return ((SurfRec*)_surfs.get(n))->weight;
}

void Wing::setMirror(bool mirror)
{
    _mirror = mirror;
}

void Wing::setBase(float* base)
{
    int i;
    for(i=0; i<3; i++) _base[i] = base[i];
}

void Wing::setLength(float length)
{
    _length = length;
}

void Wing::setChord(float chord)
{
    _chord = chord;
}

void Wing::setTaper(float taper)
{
    _taper = taper;
}

void Wing::setSweep(float sweep)
{
    _sweep = sweep;
}

void Wing::setDihedral(float dihedral)
{
    _dihedral = dihedral;
}

void Wing::setStall(float aoa)
{
    _stall = aoa;
}

void Wing::setStallWidth(float angle)
{
    _stallWidth = angle;
}

void Wing::setStallPeak(float fraction)
{
    _stallPeak = fraction;
}

void Wing::setTwist(float angle)
{
    _twist = angle;
}

void Wing::setCamber(float camber)
{
    _camber = camber;
}

void Wing::setIncidence(float incidence)
{
    _incidence = incidence;
    int i;
    for(i=0; i<_surfs.size(); i++)
        ((SurfRec*)_surfs.get(i))->surface->setIncidence(incidence);
}

void Wing::setFlap0(float start, float end, float lift, float drag)
{
    _flap0Start = start;
    _flap0End = end;
    _flap0Lift = lift;
    _flap0Drag = drag;
}

void Wing::setFlap1(float start, float end, float lift, float drag)
{
    _flap1Start = start;
    _flap1End = end;
    _flap1Lift = lift;
    _flap1Drag = drag;
}

void Wing::setSlat(float start, float end, float aoa, float drag)
{
    _slatStart = start;
    _slatEnd = end;
    _slatAoA = aoa;
    _slatDrag = drag;
}

void Wing::setSpoiler(float start, float end, float lift, float drag)
{
    _spoilerStart = start;
    _spoilerEnd = end;
    _spoilerLift = lift;
    _spoilerDrag = drag;
}

void Wing::setFlap0(float lval, float rval)
{
    lval = Math::clamp(lval, -1, 1);
    rval = Math::clamp(rval, -1, 1);
    int i;
    for(i=0; i<_flap0Surfs.size(); i++) {
	((Surface*)_flap0Surfs.get(i))->setFlap(lval);
	if(_mirror) ((Surface*)_flap0Surfs.get(++i))->setFlap(rval);
    }
}

void Wing::setFlap0Effectiveness(float lval)
{
    lval = Math::clamp(lval, 1, 10);
    int i;
    for(i=0; i<_flap0Surfs.size(); i++) {
        ((Surface*)_flap0Surfs.get(i))->setFlapEffectiveness(lval);
//	if(_mirror) ((Surface*)_flap0Surfs.get(++i))->setFlapEffectiveness(rval);
    }
}

void Wing::setFlap1(float lval, float rval)
{
    lval = Math::clamp(lval, -1, 1);
    rval = Math::clamp(rval, -1, 1);
    int i;
    for(i=0; i<_flap1Surfs.size(); i++) {
	((Surface*)_flap1Surfs.get(i))->setFlap(lval);
	if(_mirror) ((Surface*)_flap1Surfs.get(++i))->setFlap(rval);
    }
}

void Wing::setFlap1Effectiveness(float lval)
{
    lval = Math::clamp(lval, 1, 10);
    int i;
    for(i=0; i<_flap1Surfs.size(); i++) {
        ((Surface*)_flap1Surfs.get(i))->setFlapEffectiveness(lval);
//	if(_mirror) ((Surface*)_flap1Surfs.get(++i))->setFlap(rval);
    }
}

void Wing::setSpoiler(float lval, float rval)
{
    lval = Math::clamp(lval, 0, 1);
    rval = Math::clamp(rval, 0, 1);
    int i;
    for(i=0; i<_spoilerSurfs.size(); i++) {
	((Surface*)_spoilerSurfs.get(i))->setSpoiler(lval);
	if(_mirror) ((Surface*)_spoilerSurfs.get(++i))->setSpoiler(rval);
    }
}

void Wing::setSlat(float val)
{
    val = Math::clamp(val, 0, 1);
    int i;
    for(i=0; i<_slatSurfs.size(); i++)
	((Surface*)_slatSurfs.get(i))->setSlat(val);
}

float Wing::getGroundEffect(float* posOut)
{
    int i;
    for(i=0; i<3; i++) posOut[i] = _base[i];
    float span = _length * Math::cos(_sweep) * Math::cos(_dihedral);
    span = 2*(span + Math::abs(_base[2]));
    return span;
}

void Wing::getTip(float* tip)
{
    tip[0] = -Math::tan(_sweep);
    tip[1] = Math::cos(_dihedral);
    tip[2] = Math::sin(_dihedral);
    Math::unit3(tip, tip);
    Math::mul3(_length, tip, tip);
    Math::add3(_base, tip, tip);
}

bool Wing::isMirrored()
{
    return _mirror;
}

void Wing::compile()
{
    // Have we already been compiled?
    if(_surfs.size() != 0) return;

    // Assemble the start/end coordinates of all control surfaces
    // and the wing itself into an array, sort them,
    // and remove duplicates.  This gives us the boundaries of our
    // segments.
    float bounds[10];
    bounds[0] = _flap0Start;   bounds[1] = _flap0End;
    bounds[2] = _flap1Start;   bounds[3] = _flap1End;
    bounds[4] = _spoilerStart; bounds[5] = _spoilerEnd;
    bounds[6] = _slatStart;    bounds[7] = _slatEnd;
    //and don't forget the root and the tip of the wing itself
    bounds[8] = 0;             bounds[9] = 1;

    // Sort in increasing order
    int i;
    for(i=0; i<10; i++) {
        int minIdx = i;
	float minVal = bounds[i];
        int j;
        for(j=i+1; j<10; j++) {
            if(bounds[j] < minVal) {
                minIdx = j;
		minVal = bounds[j];
	    }
	}
        float tmp = bounds[i];
        bounds[i] = minVal; bounds[minIdx] = tmp;
    }

    // Uniqify
    float last = bounds[0];
    int nbounds = 1;
    for(i=1; i<10; i++) {
        if(bounds[i] != last)
            bounds[nbounds++] = bounds[i];
        last = bounds[i];
    }

    // Calculate a "nominal" segment length equal to an average chord,
    // normalized to lie within 0-1 over the length of the wing.
    float segLen = _chord * (0.5f*(_taper+1)) / _length;

    // Generating a unit vector pointing out the left wing.
    float left[3];
    left[0] = -Math::tan(_sweep);
    left[1] = Math::cos(_dihedral);
    left[2] = Math::sin(_dihedral);
    Math::unit3(left, left);

    // Calculate coordinates for the root and tip of the wing
    float root[3], tip[3];
    Math::set3(_base, root);
    Math::set3(left, tip);
    Math::mul3(_length, tip, tip);
    Math::add3(root, tip, tip);

    // The wing's Y axis will be the "left" vector.  The Z axis will
    // be perpendicular to this and the local (!) X axis, because we
    // want motion along the local X axis to be zero AoA (i.e. in the
    // wing's XY plane) by definition.  Then the local X coordinate is
    // just Y cross Z.
    float orient[9], rightOrient[9];
    float *x = orient, *y = orient+3, *z = orient+6;
    x[0] = 1; x[1] = 0; x[2] = 0;
    Math::set3(left, y);
    Math::cross3(x, y, z);
    Math::unit3(z, z);
    Math::cross3(y, z, x);

    if(_mirror) {
	// Derive the right side orientation matrix from this one.
        int i;
        for(i=0; i<9; i++)  rightOrient[i] = orient[i];

	// Negate all Y coordinates, this gets us a valid basis, but
	// it's left handed!  So...
        for(i=1; i<9; i+=3) rightOrient[i] = -rightOrient[i];

	// Change the direction of the Y axis to get back to a
	// right-handed system.
	for(i=3; i<6; i++)  rightOrient[i] = -rightOrient[i];
    }

    // Now go through each boundary and make segments
    for(i=0; i<(nbounds-1); i++) {
        float start = bounds[i];
        float end = bounds[i+1];
        float mid = (start+end)/2;

        bool flap0=0, flap1=0, slat=0, spoiler=0;
        if(_flap0Start   < mid && mid < _flap0End)   flap0 = 1;
        if(_flap1Start   < mid && mid < _flap1End)   flap1 = 1;
        if(_slatStart    < mid && mid < _slatEnd)    slat = 1;
        if(_spoilerStart < mid && mid < _spoilerEnd) spoiler = 1;

        // FIXME: Should probably detect an error here if both flap0
        // and flap1 are set.  Right now flap1 overrides.

        int nSegs = (int)Math::ceil((end-start)/segLen);
        if (_twist != 0 && nSegs < 8) // more segments if twisted
            nSegs = 8;
        float segWid = _length * (end - start)/nSegs;

        int j;
        for(j=0; j<nSegs; j++) {
            float frac = start + (j+0.5f) * (end-start)/nSegs;
            float pos[3];
            interp(root, tip, frac, pos);

            float chord = _chord * (1 - (1-_taper)*frac);

            Surface *s = newSurface(pos, orient, chord,
                                    flap0, flap1, slat, spoiler);

            SurfRec *sr = new SurfRec();
            sr->surface = s;
            sr->weight = chord * segWid;
            s->setTotalDrag(sr->weight);
            s->setTwist(_twist * frac);
            _surfs.add(sr);

            if(_mirror) {
		pos[1] = -pos[1];
                s = newSurface(pos, rightOrient, chord,
                               flap0, flap1, slat, spoiler);
                sr = new SurfRec();
                sr->surface = s;
                sr->weight = chord * segWid;
                s->setTotalDrag(sr->weight);
                s->setTwist(_twist * frac);
                _surfs.add(sr);
            }
        }
    }

    // Last of all, re-set the incidence in case setIncidence() was
    // called before we were compiled.
    setIncidence(_incidence);
}

float Wing::getDragScale()
{
    return _dragScale;
}

void Wing::setDragScale(float scale)
{
    _dragScale = scale;
    int i;
    for(i=0; i<_surfs.size(); i++) {
        SurfRec* s = (SurfRec*)_surfs.get(i);
        s->surface->setTotalDrag(scale * s->weight);
    }
}

void Wing::setLiftRatio(float ratio)
{
    _liftRatio = ratio;
    int i;
    for(i=0; i<_surfs.size(); i++)
        ((SurfRec*)_surfs.get(i))->surface->setZDrag(ratio);
}

float Wing::getLiftRatio()
{
    return _liftRatio;
}

Surface* Wing::newSurface(float* pos, float* orient, float chord,
                          bool flap0, bool flap1, bool slat, bool spoiler)
{
    Surface* s = new Surface();

    s->setPosition(pos);
    s->setOrientation(orient);
    s->setChord(chord);

    // Camber is expressed as a fraction of stall peak, so convert.
    s->setBaseZDrag(_camber*_stallPeak);

    // The "main" (i.e. normal) stall angle
    float stallAoA = _stall - _stallWidth/4;
    s->setStall(0, stallAoA);
    s->setStallWidth(0, _stallWidth);
    s->setStallPeak(0, _stallPeak);

    // The negative AoA stall is the same if we're using an uncambered
    // airfoil, otherwise a "little badder".
    if(_camber > 0) {
        s->setStall(1, stallAoA * 0.8f);
        s->setStallWidth(1, _stallWidth * 0.5f);
    } else {
        s->setStall(1, stallAoA);
        s->setStall(1, _stallWidth);
    }

    // The "reverse" stalls are unmeasurable junk.  Just use 13deg and
    // "sharp".
    s->setStallPeak(1, 1);
    int i;
    for(i=2; i<4; i++) {
        s->setStall(i, 0.2267f);
        s->setStallWidth(i, 0.01);
    }
    
    if(flap0)   s->setFlapParams(_flap0Lift, _flap0Drag);
    if(flap1)   s->setFlapParams(_flap1Lift, _flap1Drag);
    if(slat)    s->setSlatParams(_slatAoA, _slatDrag);
    if(spoiler) s->setSpoilerParams(_spoilerLift, _spoilerDrag);    

    if(flap0)   _flap0Surfs.add(s);
    if(flap1)   _flap1Surfs.add(s);
    if(slat)    _slatSurfs.add(s);
    if(spoiler) _spoilerSurfs.add(s);

    s->setInducedDrag(_inducedDrag);

    return s;
}

void Wing::interp(float* v1, float* v2, float frac, float* out)
{
    out[0] = v1[0] + frac*(v2[0]-v1[0]);
    out[1] = v1[1] + frac*(v2[1]-v1[1]);
    out[2] = v1[2] + frac*(v2[2]-v1[2]);
}

}; // namespace yasim
