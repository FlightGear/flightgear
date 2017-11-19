#include "yasim-common.hpp"
#include "Surface.hpp"
#include "Wing.hpp"

namespace yasim {
  
Wing::Wing(Version *ver, bool mirror, float* base, float chord, 
        float length, float taper, float sweep, float dihedral, float twist) :
    _version(ver),
    _mirror(mirror),
    _chord(chord),
    _length(length),
    _taper(taper),
    _sweepAngleCenterLine(sweep),
    _dihedral(dihedral),
    _twist(twist)
{
    Math::set3(base, _base);
    _meanChord = _chord*(_taper+1)*0.5f;
    calculateWingCoordinateSystem();
    calculateTip();
    calculateSpan();
    calculateMAC();
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

void Wing::setFlapParams(WingFlaps i, FlapParams fp)
{
    _flapParams[i] = fp;
}

void Wing::setFlapPos(WingFlaps f,float lval, float rval)
{
    float min {-1};
    if (f == WING_SPOILER || f == WING_SLAT) {
        min = 0;
    }
    lval = Math::clamp(lval, min, 1);
    rval = Math::clamp(rval, min, 1);
    for(int i=0; i<_flapSurfs[f].size(); i++) {
        switch (f) {
            case WING_FLAP0:
            case WING_FLAP1:
                ((Surface*)_flapSurfs[f].get(i))->setFlapPos(lval);
                if(_mirror) ((Surface*)_flapSurfs[f].get(++i))->setFlapPos(rval);
                break;
            case WING_SLAT:
                ((Surface*)_flapSurfs[f].get(i))->setSlatPos(lval);
                break;
            case WING_SPOILER:
                ((Surface*)_flapSurfs[f].get(i))->setSpoilerPos(lval);
                if(_mirror) ((Surface*)_flapSurfs[f].get(++i))->setSpoilerPos(rval);
                break;
        }
    }
}

void Wing::setFlapEffectiveness(WingFlaps f, float lval)
{
    lval = Math::clamp(lval, 1, 10);
    for(int i=0; i<_flapSurfs[f].size(); i++) {
        ((Surface*)_flapSurfs[f].get(i))->setFlapEffectiveness(lval);
    }
}

void Wing::calculateWingCoordinateSystem() {
  // prepare wing coordinate system, ignoring incidence and twist for now
  // (tail incidence is varied by the solver)
  // Generating a unit vector pointing out the left wing.
  float left[3];
  left[0] = -Math::tan(_sweepAngleCenterLine);
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
    _macX = _base[0]-Math::tan(_sweepAngleCenterLine) * _macRootDistance + _mac/2;
}

float Wing::calculateSweepAngleLeadingEdge()
{
    if (_length == 0) {
      return 0;
    }
    return Math::atan(
      (sin(_sweepAngleCenterLine)+(1-_taper)*_chord/(2*_length)) /
      cos(_sweepAngleCenterLine)
    );  
}


void Wing::compile()
{
    // Have we already been compiled?
    if(! _surfs.empty()) return;

    // Assemble the start/end coordinates of all control surfaces
    // and the wing itself into an array, sort them,
    // and remove duplicates.  This gives us the boundaries of our
    // segments.
    const int NUM_BOUNDS {10};
    float bounds[NUM_BOUNDS];
    bounds[0] = _flapParams[WING_FLAP0].start;   
    bounds[1] = _flapParams[WING_FLAP0].end;
    bounds[2] = _flapParams[WING_FLAP1].start;   
    bounds[3] = _flapParams[WING_FLAP1].end;
    bounds[4] = _flapParams[WING_SPOILER].start; 
    bounds[5] = _flapParams[WING_SPOILER].end;
    bounds[6] = _flapParams[WING_SLAT].start;    
    bounds[7] = _flapParams[WING_SLAT].end;
    //and don't forget the root and the tip of the wing itself
    bounds[8] = 0;             bounds[9] = 1;

    // Sort in increasing order
    int i;
    for(i=0; i<NUM_BOUNDS; i++) {
      int minIdx = i;
      float minVal = bounds[i];
      for(int j=i+1; j<NUM_BOUNDS; j++) {
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
    for(i=1; i<NUM_BOUNDS; i++) {
        if(bounds[i] != last)
            bounds[nbounds++] = bounds[i];
        last = bounds[i];
    }

    // Calculate a "nominal" segment length equal to an average chord,
    // normalized to lie within 0-1 over the length of the wing.
    float segLen = _meanChord / _length;

    // Now go through each boundary and make segments
    for(i=0; i<(nbounds-1); i++) {
        float start = bounds[i];
        float end = bounds[i+1];
        float mid = (start+end)/2;

        bool hasFlap0=0, hasFlap1=0, hasSlat=0, hasSpoiler=0;
        if(_flapParams[WING_FLAP0].start   < mid && mid < _flapParams[WING_FLAP0].end)   hasFlap0 = 1;
        if(_flapParams[WING_FLAP1].start   < mid && mid < _flapParams[WING_FLAP1].end)   hasFlap1 = 1;
        if(_flapParams[WING_SLAT].start    < mid && mid < _flapParams[WING_SLAT].end)    hasSlat = 1;
        if(_flapParams[WING_SPOILER].start < mid && mid < _flapParams[WING_SPOILER].end) hasSpoiler = 1;

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
            float weight = chord * segWid;
            float twist = _twist * frac;

            Surface *s = newSurface(pos, _orient, chord,
                                    hasFlap0, hasFlap1, hasSlat, hasSpoiler);

            addSurface(s, weight, twist);

            if(_mirror) {
                pos[1] = -pos[1];
                s = newSurface(pos, _rightOrient, chord,
                               hasFlap0, hasFlap1, hasSlat, hasSpoiler);
                addSurface(s, weight, twist);                
            }
        }
    }
    // Last of all, re-set the incidence in case setIncidence() was
    // called before we were compiled.
    setIncidence(_incidence);
    writeInfoToProptree();
}

void Wing::addSurface(Surface* s, float weight, float twist)
{
    SurfRec *sr = new SurfRec();
    sr->surface = s;
    sr->weight = weight;
    s->setDragCoefficient(sr->weight);
    s->setTwist(twist);
    _surfs.add(sr);
}

void Wing::setDragScale(float scale)
{
    _dragScale = scale;
    for(int i=0; i<_surfs.size(); i++) {
        SurfRec* s = (SurfRec*)_surfs.get(i);
        s->surface->setDragCoefficient(scale * s->weight);
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
    
    if(hasFlap0)   s->setFlapParams(_flapParams[WING_FLAP0].lift, _flapParams[WING_FLAP0].drag);
    if(hasFlap1)   s->setFlapParams(_flapParams[WING_FLAP1].lift, _flapParams[WING_FLAP1].drag);
    if(hasSlat)    s->setSlatParams(_flapParams[WING_SLAT].aoa, _flapParams[WING_SLAT].drag);
    if(hasSpoiler) s->setSpoilerParams(_flapParams[WING_SPOILER].lift, _flapParams[WING_SPOILER].drag);    

    if(hasFlap0)   _flapSurfs[WING_FLAP0].add(s);
    if(hasFlap1)   _flapSurfs[WING_FLAP1].add(s);
    if(hasSlat)    _flapSurfs[WING_SLAT].add(s);
    if(hasSpoiler) _flapSurfs[WING_SPOILER].add(s);

    s->setInducedDrag(_inducedDrag);

    return s;
}

void Wing::interp(const float* v1, const float* v2, const float frac, float* out)
{
    out[0] = v1[0] + frac*(v2[0]-v1[0]);
    out[1] = v1[1] + frac*(v2[1]-v1[1]);
    out[2] = v1[2] + frac*(v2[2]-v1[2]);
}

void Wing::writeInfoToProptree()
{
    if (_wingN == nullptr) 
        return;
    _wingN->getNode("tip-x", true)->setFloatValue(_tip[0]);
    _wingN->getNode("tip-y", true)->setFloatValue(_tip[1]);
    _wingN->getNode("tip-z", true)->setFloatValue(_tip[2]);
    _wingN->getNode("base-x", true)->setFloatValue(_base[0]);
    _wingN->getNode("base-y", true)->setFloatValue(_base[1]);
    _wingN->getNode("base-z", true)->setFloatValue(_base[2]);
    _wingN->getNode("wing-span", true)->setFloatValue(_wingspan);
    _wingN->getNode("wing-area", true)->setFloatValue(_wingspan*_meanChord);
    _wingN->getNode("aspect-ratio", true)->setFloatValue(_aspectRatio);
    _wingN->getNode("standard-mean-chord", true)->setFloatValue(_meanChord);
    _wingN->getNode("mac", true)->setFloatValue(_mac);
    _wingN->getNode("mac-x", true)->setFloatValue(_macX);
    _wingN->getNode("mac-y", true)->setFloatValue(_base[1]+_macRootDistance);

    float wgt = 0;
    float dragSum = 0;
    for(int surf=0; surf < numSurfaces(); surf++) {
        Surface* s = (Surface*)getSurface(surf);
        float td = s->getDragCoefficient();
        dragSum += td;

        float mass = getSurfaceWeight(surf);
        mass = mass * Math::sqrt(mass);
        wgt += mass;
    }
    _wingN->getNode("weight", true)->setFloatValue(wgt);
    _wingN->getNode("drag", true)->setFloatValue(dragSum);
}
}; // namespace yasim
