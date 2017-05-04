#include "yasim-common.hpp"
#include "Surface.hpp"
#include "Wing.hpp"

namespace yasim {
  
Wing::Wing( Version * version ) :
  _version(version)
{

}

Wing::~Wing()
{
    for(int i=0; i<_surfs.size(); i++) {
        SurfRec* s = (SurfRec*)_surfs.get(i);
        delete s->surface;
        delete s;
    }
}

void Wing::setIncidence(float incidence)
{
    _incidence = incidence;
    for(int i=0; i<_surfs.size(); i++)
        ((SurfRec*)_surfs.get(i))->surface->setIncidence(incidence);
}

void Wing::setFlap0Params(float start, float end, float lift, float drag)
{
    _flap0Start = start;
    _flap0End = end;
    _flap0Lift = lift;
    _flap0Drag = drag;
}

void Wing::setFlap1Params(float start, float end, float lift, float drag)
{
    _flap1Start = start;
    _flap1End = end;
    _flap1Lift = lift;
    _flap1Drag = drag;
}

void Wing::setSlatParams(float start, float end, float aoa, float drag)
{
    _slatStart = start;
    _slatEnd = end;
    _slatAoA = aoa;
    _slatDrag = drag;
}

void Wing::setSpoilerParams(float start, float end, float lift, float drag)
{
    _spoilerStart = start;
    _spoilerEnd = end;
    _spoilerLift = lift;
    _spoilerDrag = drag;
}

void Wing::setFlap0Pos(float lval, float rval)
{
    lval = Math::clamp(lval, -1, 1);
    rval = Math::clamp(rval, -1, 1);
    for(int i=0; i<_flap0Surfs.size(); i++) {
      ((Surface*)_flap0Surfs.get(i))->setFlapPos(lval);
      if(_mirror) ((Surface*)_flap0Surfs.get(++i))->setFlapPos(rval);
    }
}

void Wing::setFlap0Effectiveness(float lval)
{
    lval = Math::clamp(lval, 1, 10);
    for(int i=0; i<_flap0Surfs.size(); i++) {
        ((Surface*)_flap0Surfs.get(i))->setFlapEffectiveness(lval);
    }
}

void Wing::setFlap1Pos(float lval, float rval)
{
    lval = Math::clamp(lval, -1, 1);
    rval = Math::clamp(rval, -1, 1);
    for(int i=0; i<_flap1Surfs.size(); i++) {
      ((Surface*)_flap1Surfs.get(i))->setFlapPos(lval);
      if(_mirror) ((Surface*)_flap1Surfs.get(++i))->setFlapPos(rval);
    }
}

void Wing::setFlap1Effectiveness(float lval)
{
    lval = Math::clamp(lval, 1, 10);
    for(int i=0; i<_flap1Surfs.size(); i++) {
        ((Surface*)_flap1Surfs.get(i))->setFlapEffectiveness(lval);
    }
}

void Wing::setSpoilerPos(float lval, float rval)
{
    lval = Math::clamp(lval, 0, 1);
    rval = Math::clamp(rval, 0, 1);
    for(int i=0; i<_spoilerSurfs.size(); i++) {
      ((Surface*)_spoilerSurfs.get(i))->setSpoilerPos(lval);
      if(_mirror) ((Surface*)_spoilerSurfs.get(++i))->setSpoilerPos(rval);
    }
}

void Wing::setSlatPos(float val)
{
    val = Math::clamp(val, 0, 1);
    for(int i=0; i<_slatSurfs.size(); i++)
      ((Surface*)_slatSurfs.get(i))->setSlatPos(val);
}

void Wing::calculateWingCoordinateSystem() {
  // prepare wing coordinate system, ignoring incidence and twist for now
  // (tail incidence is varied by the solver)
  // Generating a unit vector pointing out the left wing.
  float left[3];
  left[0] = -Math::tan(_sweep);
  left[1] = Math::cos(_dihedral);
  left[2] = Math::sin(_dihedral);
  Math::unit3(left, left);
  // The wing's Y axis will be the "left" vector.  The Z axis will
  // be perpendicular to this and the local (!) X axis, because we
  // want motion along the local X axis to be zero AoA (i.e. in the
  // wing's XY plane) by definition.  Then the local X coordinate is
  // just Y cross Z.
  float *x = _orient, *y = _orient+3, *z = _orient+6;
  x[0] = 1; x[1] = 0; x[2] = 0;
  Math::set3(left, y);
  Math::cross3(x, y, z);
  Math::unit3(z, z);
  Math::cross3(y, z, x);    
  // Derive the right side orientation matrix from this one.
  int i;
  for(i=0; i<9; i++)  _rightOrient[i] = _orient[i];
  // Negate all Y coordinates, this gets us a valid basis, but
  // it's left handed!  So...
  for(i=1; i<9; i+=3) _rightOrient[i] = -_rightOrient[i];
  // Change the direction of the Y axis to get back to a
  // right-handed system.
  for(i=3; i<6; i++)  _rightOrient[i] = -_rightOrient[i];
}

void Wing::calculateTip() {
    float *y = _orient+3;
    Math::mul3(_length, y, _tip);
    Math::add3(_base, _tip, _tip);
}

void Wing::calculateSpan()
{
    // wingspan in y-direction (not for vstab)
    _wingspan = Math::abs(2*_tip[1]);
    _netSpan = Math::abs(2*(_tip[1]-_base[1]));
    _aspectRatio = _wingspan / _meanChord;  
}

void Wing::calculateMAC()
{
    // http://www.nasascale.org/p2/wp-content/uploads/mac-calculator.htm
    const float commonFactor = _chord*(0.5+_taper)/(3*_chord*(1+_taper));
    _mac = _chord-(2*_chord*(1-_taper)*commonFactor);
    _macRootDistance = _netSpan*commonFactor;
    _macX = _base[0]-Math::tan(_sweep) * _macRootDistance + _mac/2;
}

void Wing::compile()
{
    // Have we already been compiled?
    if(! _surfs.empty()) return;

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

    calculateWingCoordinateSystem();
    calculateTip();
    _meanChord = _chord*(_taper+1)*0.5f;
    
    calculateSpan();
    calculateMAC();

    // Calculate a "nominal" segment length equal to an average chord,
    // normalized to lie within 0-1 over the length of the wing.
    float segLen = _meanChord / _length;

    // Now go through each boundary and make segments
    for(i=0; i<(nbounds-1); i++) {
        float start = bounds[i];
        float end = bounds[i+1];
        float mid = (start+end)/2;

        bool hasFlap0=0, hasFlap1=0, hasSlat=0, hasSpoiler=0;
        if(_flap0Start   < mid && mid < _flap0End)   hasFlap0 = 1;
        if(_flap1Start   < mid && mid < _flap1End)   hasFlap1 = 1;
        if(_slatStart    < mid && mid < _slatEnd)    hasSlat = 1;
        if(_spoilerStart < mid && mid < _spoilerEnd) hasSpoiler = 1;

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
            interp(_base, _tip, frac, pos);

            float chord = _chord * (1 - (1-_taper)*frac);

            Surface *s = newSurface(pos, _orient, chord,
                                    hasFlap0, hasFlap1, hasSlat, hasSpoiler);

            SurfRec *sr = new SurfRec();
            sr->surface = s;
            sr->weight = chord * segWid;
            s->setTotalDrag(sr->weight);
            s->setTwist(_twist * frac);
            _surfs.add(sr);

            if(_mirror) {
                pos[1] = -pos[1];
                s = newSurface(pos, _rightOrient, chord,
                               hasFlap0, hasFlap1, hasSlat, hasSpoiler);
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


void Wing::setDragScale(float scale)
{
    _dragScale = scale;
    for(int i=0; i<_surfs.size(); i++) {
        SurfRec* s = (SurfRec*)_surfs.get(i);
        s->surface->setTotalDrag(scale * s->weight);
    }
}

void Wing::setLiftRatio(float ratio)
{
    _liftRatio = ratio;
    for(int i=0; i<_surfs.size(); i++)
        ((SurfRec*)_surfs.get(i))->surface->setZDrag(ratio);
}

Surface* Wing::newSurface(float* pos, float* orient, float chord,
                          bool hasFlap0, bool hasFlap1, bool hasSlat, bool hasSpoiler)
{
    Surface* s = new Surface(_version);

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

    // The negative AoA stall is the same if we're using an symmetric
    // airfoil, otherwise a "little worse".
    if(_camber > 0) {
        s->setStall(1, stallAoA * 0.8f);
        s->setStallWidth(1, _stallWidth * 0.5f);
    } else {
      s->setStall(1, stallAoA);
      if( _version->isVersionOrNewer( Version::YASIM_VERSION_2017_2 )) {
        // what was presumably meant
        s->setStallWidth(1, _stallWidth);
      } else {
        // old code; presumably a copy&paste error
        s->setStall(1, _stallWidth);
      }
    }

    // The "reverse" stalls are unmeasurable junk.  Just use 13deg and
    // "sharp".
    s->setStallPeak(1, 1);
    int i;
    for(i=2; i<4; i++) {
        s->setStall(i, 0.2267f);
        s->setStallWidth(i, 0.01);
    }
    
    if(hasFlap0)   s->setFlapParams(_flap0Lift, _flap0Drag);
    if(hasFlap1)   s->setFlapParams(_flap1Lift, _flap1Drag);
    if(hasSlat)    s->setSlatParams(_slatAoA, _slatDrag);
    if(hasSpoiler) s->setSpoilerParams(_spoilerLift, _spoilerDrag);    

    if(hasFlap0)   _flap0Surfs.add(s);
    if(hasFlap1)   _flap1Surfs.add(s);
    if(hasSlat)    _slatSurfs.add(s);
    if(hasSpoiler) _spoilerSurfs.add(s);

    s->setInducedDrag(_inducedDrag);

    return s;
}

void Wing::interp(const float* v1, const float* v2, const float frac, float* out)
{
    out[0] = v1[0] + frac*(v2[0]-v1[0]);
    out[1] = v1[1] + frac*(v2[1]-v1[1]);
    out[2] = v1[2] + frac*(v2[2]-v1[2]);
}

}; // namespace yasim
