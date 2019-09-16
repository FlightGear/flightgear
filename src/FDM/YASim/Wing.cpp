#include "yasim-common.hpp"
#include "Model.hpp"
#include "Surface.hpp"
#include "Wing.hpp"

namespace yasim {
  
Wing::Wing(Version *ver, bool mirror) :
    _version(ver),
    _mirror(mirror)
{
}

Wing::~Wing()
{
    WingSection* ws;
    for (int s=0; s < _sections.size(); s++){
        ws = (WingSection*)_sections.get(s);
        for(int i=0; i<ws->_surfs.size(); i++) {
            SurfRec* s = (SurfRec*)ws->_surfs.get(i);
            delete s->surface;
            delete s;
        }
    }
}

int Wing::addWingSection(float* base, float chord, float wingLength, float taper, 
                         float sweep, float dihedral, float twist, float camber, 
                         float idrag, float incidence)
{
    WingSection* ws = new WingSection;
    if (_sections.size() == 0) {
        // first section
        Math::set3(base, _base);
        ws->_rootChord = _float2chord(base, chord);
        ws->_sectionIncidence = incidence;
    } else { 
        WingSection* prev = (WingSection*)_sections.get(_sections.size()-1);
        //use old wing tip instead of base argument
        ws->_rootChord = prev->_tipChord;
        ws->_sectionIncidence = prev->_sectionIncidence + prev->_twist;
    }
    ws->_length = wingLength;
    ws->_taper = taper; 
    ws->_sweepAngleCenterLine = sweep;    
    ws->_dihedral = dihedral;
    ws->_twist = twist;
    ws->_camber = camber;
    ws->_inducedDrag = idrag;
    ws->_id = _sections.add(ws);
    ws->calculateGeometry();
    // first / only section
    if (ws->_id == 0) {
        _mac = ws->getMAC();
        _netSpan = ws->_sectionSpan;
        _area = ws->getArea();
        _meanChord = ws->_meanChord;
        _sweepLEMin = _sweepLEMax = ws->calculateSweepAngleLeadingEdge();
    }
    // append section: Calculate wing MAC from MACs of section and prev wing
    else {
        _mac = _weightedMeanChord(_mac, _area, ws->getMAC(), ws->getArea());
        _netSpan += ws->_sectionSpan;
        _area += ws->getArea();
        _meanChord = _meanChord * ws->_meanChord * 0.5f;
        float s = ws->calculateSweepAngleLeadingEdge();
        if (_sweepLEMax < s) _sweepLEMax = s;
        if (_sweepLEMin > s) _sweepLEMin = s;
    }
    _chord2float(ws->_tipChord, _tip);    
    _wingSpan = 2 * _tip[1];
    if (_area > 0.0f) {
        _aspectRatio = _wingSpan*_wingSpan/_area;
    }    
    _taper = ws->_tipChord.length / ((WingSection*)_sections.get(0))->_rootChord.length;
    return ws->_id;
}

Chord Wing::_float2chord(float* pos, float lenght)
{
    Chord c;
    c.x = pos[0];
    c.y = pos[1];
    c.z = pos[2];
    c.length = lenght;
    return c;
}

void Wing::_chord2float(Chord c, float* pos)
{
    pos[0] = c.x;
    pos[1] = c.y;
    pos[2] = c.z;
}

Chord Wing::_weightedMeanChord(Chord a, float wa, Chord b, float wb)
{
    Chord c;
    c.length = Math::weightedMean(a.length, wa, b.length, wb);
    c.x = Math::weightedMean(a.x, wa, b.x, wb);
    c.y = Math::weightedMean(a.y, wa, b.y, wb);
    c.z = Math::weightedMean(a.z, wa, b.z, wb);
    return c;
}

void Wing::WingSection::calculateGeometry()
{
    _meanChord = _rootChord.length*(_taper+1)*0.5f;
    calculateWingCoordinateSystem();
    calculateTipChord();
    calculateSpan();
    calculateMAC();    
}

void Wing::WingSection::calculateWingCoordinateSystem() {
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

void Wing::WingSection::calculateTipChord() {
    float *y = _orient+3;
    _tipChord.x = _rootChord.x + _length * y[0];
    _tipChord.y = _rootChord.y + _length * y[1];
    _tipChord.z = _rootChord.z + _length * y[2];
    _tipChord.length = _rootChord.length * _taper;
}

void Wing::WingSection::calculateSpan()
{
    // wingspan in y-direction (not for vstab)
    _sectionSpan = Math::abs(_rootChord.y - _tipChord.y);
}

void Wing::WingSection::calculateMAC()
{    
    // http://www.nasascale.org/p2/wp-content/uploads/mac-calculator.htm
    //const float commonFactor = _rootChord.length*0.5(1+2*_taper)/(3*_rootChord.length*(1+_taper));
    
    const float commonFactor = (0.5+_taper)/(3*(1+_taper));
    _mac.length = _rootChord.length-(2*_rootChord.length*(1-_taper)*commonFactor);
    // y distance to root chord
    _mac.y = Math::abs(2*(_tipChord.y-_rootChord.y))*commonFactor;
    // MAC leading edge x = midpoint + half MAC length
    _mac.x = -Math::tan(_sweepAngleCenterLine)*_mac.y + _mac.length/2 + _rootChord.x;
    _mac.z = Math::tan(_dihedral)*_mac.y + _rootChord.z;
    //add root y to get aircraft coordinates
    _mac.y += _rootChord.y;
}

float Wing::WingSection::calculateSweepAngleLeadingEdge()
{
    if (_length == 0) {
      return 0;
    }
    return Math::atan(
      (sin(_sweepAngleCenterLine)+(1-_taper)*_rootChord.length/(2*_length)) /
      cos(_sweepAngleCenterLine)
    );  
}

void Wing::WingSection::setIncidence(float incidence)
{
    //update surface
    for(int i=0; i<_surfs.size(); i++)
        ((SurfRec*)_surfs.get(i))->surface->setIncidence(incidence + _sectionIncidence);
}

void Wing::setFlapParams(int section, WingFlaps type, FlapParams fp)
{
    ((WingSection*)_sections.get(section))->_flapParams[type] = fp;
}

void Wing::setSectionDrag(int section, float pdrag)
{
    ((WingSection*)_sections.get(section))->_dragScale = pdrag;
}

void Wing::setSectionStallParams(int section, StallParams sp)
{
    ((WingSection*)_sections.get(section))->_stallParams = sp;
}
    
void Wing::setFlapPos(WingFlaps type,float lval, float rval)
{
    float min {-1.0f};
    if (type == WING_SPOILER || type == WING_SLAT) {
        min = 0.0f;
    }
    lval = Math::clamp(lval, min, 1.0f);
    rval = Math::clamp(rval, min, 1.0f);
    WingSection* ws;
    for (int section=0; section < _sections.size(); section++) 
    {        
        ws = (WingSection*)_sections.get(section);
        for(int i=0; i < ws->_flapSurfs[type].size(); i++) {
            switch (type) {
                case WING_FLAP0:
                case WING_FLAP1:
                    ((Surface*)ws->_flapSurfs[type].get(i))->setFlapPos(lval);
                    if(_mirror) ((Surface*)ws->_flapSurfs[type].get(++i))->setFlapPos(rval);
                    break;
                case WING_SLAT:
                    ((Surface*)ws->_flapSurfs[type].get(i))->setSlatPos(lval);
                    break;
                case WING_SPOILER:
                    ((Surface*)ws->_flapSurfs[type].get(i))->setSpoilerPos(lval);
                    if(_mirror) ((Surface*)ws->_flapSurfs[type].get(++i))->setSpoilerPos(rval);
                    break;
            }
        }
    }
}

void Wing::setFlapEffectiveness(WingFlaps f, float lval)
{
    lval = Math::clamp(lval, 1, 10);
    WingSection* ws;
    for (int section=0; section < _sections.size(); section++) 
    {        
        ws = (WingSection*)_sections.get(section);        
        for(int i=0; i<ws->_flapSurfs[f].size(); i++) {
            ((Surface*)ws->_flapSurfs[f].get(i))->setFlapEffectiveness(lval);
        }
    }
}


void Wing::compile()
{
    WingSection* ws;
    for (int section=0; section < _sections.size(); section++) 
    {        
        ws = (WingSection*)_sections.get(section);
        // Have we already been compiled?
        if(! ws->_surfs.empty()) return;
        
        // Assemble the start/end coordinates of all control surfaces
        // and the wing itself into an array, sort them,
        // and remove duplicates.  This gives us the boundaries of our
        // segments.
        const int NUM_BOUNDS {10};
        float bounds[NUM_BOUNDS];
        bounds[0] = ws->_flapParams[WING_FLAP0].start;   
        bounds[1] = ws->_flapParams[WING_FLAP0].end;
        bounds[2] = ws->_flapParams[WING_FLAP1].start;   
        bounds[3] = ws->_flapParams[WING_FLAP1].end;
        bounds[4] = ws->_flapParams[WING_SPOILER].start; 
        bounds[5] = ws->_flapParams[WING_SPOILER].end;
        bounds[6] = ws->_flapParams[WING_SLAT].start;    
        bounds[7] = ws->_flapParams[WING_SLAT].end;
        //and don't forget the root and the tip of the wing itself
        bounds[8] = 0;             bounds[9] = 1;

        // Sort in increasing order
        for(int i=0; i<NUM_BOUNDS; i++) {
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
        for(int i=1; i<NUM_BOUNDS; i++) {
            if(bounds[i] != last)
                bounds[nbounds++] = bounds[i];
            last = bounds[i];
        }

        // Calculate a "nominal" segment length equal to an average chord,
        // normalized to lie within 0-1 over the length of the wing.
        float segLen = ws->_meanChord / ws->_length;

        // Now go through each boundary and make segments
        for(int i=0; i<(nbounds-1); i++) {
            float start = bounds[i];
            float end = bounds[i+1];
            float mid = (start+end)/2;

            bool hasFlap0=0, hasFlap1=0, hasSlat=0, hasSpoiler=0;
            if(ws->_flapParams[WING_FLAP0].start < mid && mid < ws->_flapParams[WING_FLAP0].end)  
                hasFlap0 = 1;
            if(ws->_flapParams[WING_FLAP1].start < mid && mid < ws->_flapParams[WING_FLAP1].end)  
                hasFlap1 = 1;
            if(ws->_flapParams[WING_SLAT].start < mid && mid < ws->_flapParams[WING_SLAT].end)
                hasSlat = 1;
            if(ws->_flapParams[WING_SPOILER].start < mid && mid < ws->_flapParams[WING_SPOILER].end) 
                hasSpoiler = 1;

            // FIXME: Should probably detect an error here if both flap0
            // and flap1 are set.  Right now flap1 overrides.

            int nSegs = (int)Math::ceil((end-start)/segLen);
            if (ws->_twist != 0 && nSegs < 8) // more segments if twisted
                nSegs = 8;
            float segWid = ws->_length * (end - start)/nSegs;

            float base[3];
            _chord2float(ws->_rootChord, base);
            float tip[3];
            _chord2float(ws->_tipChord, tip);
            
            for(int j=0; j<nSegs; j++) {
                float frac = start + (j+0.5f) * (end-start)/nSegs;
                float pos[3];
                _interp(base, tip, frac, pos);

                float chord = ws->_rootChord.length * (1 - (1-ws->_taper)*frac);
                float weight = chord * segWid;
                float twist = ws->_twist * frac;

                ws->newSurface(_version, pos, ws->_orient, chord,
                               hasFlap0, hasFlap1, hasSlat, hasSpoiler, weight, twist, _flow, _Mcrit);

                if(_mirror) {
                    pos[1] = -pos[1];
                    ws->newSurface(_version, pos, ws->_rightOrient, chord,
                                hasFlap0, hasFlap1, hasSlat, hasSpoiler, weight, twist, _flow, _Mcrit);                
                }
            }
        }
        // Last of all, re-set the incidence in case setIncidence() was
        // called before we were compiled.
        setIncidence(_incidence);
    }
    _writeInfoToProptree();
}

float Wing::getArea() const 
{ 
    if (_mirror) return 2 * _area; 
    else return _area;    
};

void Wing::multiplyLiftRatio(float factor)
{
    WingSection* ws;
    for (int section=0; section < _sections.size(); section++) 
    {        
        ws = (WingSection*)_sections.get(section);
        ws->multiplyLiftRatio(factor);
    }  
}

void Wing::multiplyDragCoefficient(float factor)
{
    WingSection* ws;
    for (int section=0; section < _sections.size(); section++) 
    {        
        ws = (WingSection*)_sections.get(section);
        ws->multiplyDragCoefficient(factor);
    }
}

///update incidence for wing (rotate wing while maintaining initial twist config)
bool Wing::setIncidence(float incidence)
{
    if (incidence < _incidenceMin || incidence > _incidenceMax)
    {
        return false;
    }
    _incidence = incidence;
    WingSection* ws;
    for (int section=0; section < _sections.size(); section++) 
    {        
        ws = (WingSection*)_sections.get(section);
        ws->setIncidence(incidence);
    }    
    if (_wingN) {
        _wingN->getNode("incidence-deg", true)->setFloatValue(_incidence * RAD2DEG);
    }
    return true;
}
    
void Wing::WingSection::setDragCoefficient(float scale)
{
    _dragScale = scale;
    for(int i=0; i<_surfs.size(); i++) {
        SurfRec* s = (SurfRec*)_surfs.get(i);
        s->surface->setTotalForceCoefficient(scale * s->weight);
    }
}

void Wing::WingSection::multiplyDragCoefficient(float factor)
{
    setDragCoefficient(_dragScale * factor);
}

void Wing::WingSection::setLiftRatio(float ratio)
{
    _liftRatio = ratio;
    for(int i=0; i<_surfs.size(); i++)
        ((SurfRec*)_surfs.get(i))->surface->setLiftCoefficient(ratio);
}

void Wing::WingSection::multiplyLiftRatio(float factor)
{
    setLiftRatio(_liftRatio * factor);
}

void Wing::WingSection::newSurface(Version* _version, float* pos, float* orient, float chord, bool hasFlap0, bool hasFlap1, bool hasSlat, bool hasSpoiler, float weight, float twist, FlowRegime flow, float mcrit)
{
    Surface* s = new Surface(_version, pos, weight);

    s->setOrientation(orient);
    s->setChord(chord);
    s->setFlowRegime(flow);

    // Camber is expressed as a fraction of stall peak, so convert.
    s->setZeroAlphaLift(_camber*_stallParams.peak);

    // The "main" (i.e. normal) stall angle
    float stallAoA = _stallParams.aoa - _stallParams.width/4;
    s->setStall(0, stallAoA);
    s->setStallWidth(0, _stallParams.width);
    s->setStallPeak(0, _stallParams.peak);

    // The negative AoA stall is the same if we're using an symmetric
    // airfoil, otherwise a "little worse".
    if(_camber > 0) {
        s->setStall(1, stallAoA * 0.8f);
        s->setStallWidth(1, _stallParams.width * 0.5f);
    } else {
      s->setStall(1, stallAoA);
      if( _version->isVersionOrNewer( Version::YASIM_VERSION_2017_2 )) {
        // what was presumably meant
        s->setStallWidth(1, _stallParams.width);
      } else {
        // old code; presumably a copy&paste error
        s->setStall(1, _stallParams.width);
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
    if(flow == FLOW_TRANSONIC) {
        s->setCriticalMachNumber(mcrit);
    }
    
    SurfRec *sr = new SurfRec();
    sr->surface = s;
    sr->weight = weight;
    s->setTwist(twist);
    _surfs.add(sr);
}

void Wing::_interp(const float* v1, const float* v2, const float frac, float* out)
{
    out[0] = v1[0] + frac*(v2[0]-v1[0]);
    out[1] = v1[1] + frac*(v2[1]-v1[1]);
    out[2] = v1[2] + frac*(v2[2]-v1[2]);
}

void Wing::_writeInfoToProptree()
{
    if (_wingN == nullptr) 
        return;
    WingSection* ws = (WingSection*)_sections.get(0);
    _wingN->getNode("root-chord", true)->setFloatValue(ws->_rootChord.length);
    _wingN->getNode("tip-x", true)->setFloatValue(_tip[0]);
    _wingN->getNode("tip-y", true)->setFloatValue(_tip[1]);
    _wingN->getNode("tip-z", true)->setFloatValue(_tip[2]);
    _wingN->getNode("base-x", true)->setFloatValue(_base[0]);
    _wingN->getNode("base-y", true)->setFloatValue(_base[1]);
    _wingN->getNode("base-z", true)->setFloatValue(_base[2]);
    _wingN->getNode("taper", true)->setFloatValue(getTaper());
    _wingN->getNode("wing-span", true)->setFloatValue(getSpan());
    _wingN->getNode("wing-area", true)->setFloatValue(getArea());
    _wingN->getNode("aspect-ratio", true)->setFloatValue(getAspectRatio());
    _wingN->getNode("standard-mean-chord", true)->setFloatValue(getSMC());
    _wingN->getNode("mac", true)->setFloatValue(getMACLength());
    _wingN->getNode("mac-x", true)->setFloatValue(getMACx());
    _wingN->getNode("mac-y", true)->setFloatValue(getMACy());
    _wingN->getNode("mac-z", true)->setFloatValue(getMACz());

    float wgt = 0;
    float dragSum = 0;

    for (int section=0; section < _sections.size(); section++) {
        ws = (WingSection*)_sections.get(section);
        float mass {0};
        for (int surf=0; surf < ws->numSurfaces(); surf++) {
            Surface* s = ws->getSurface(surf);
            float drag = s->getTotalForceCoefficient();
            dragSum += drag;

            mass = ws->getSurfaceWeight(surf);
            mass = mass * Math::sqrt(mass);
            wgt += mass;
        }
        SGPropertyNode_ptr sectN = _wingN->getChild("section",section,1);
        sectN->getNode("surface-count", true)->setFloatValue(ws->numSurfaces());
        sectN->getNode("weight", true)->setFloatValue(mass);
        sectN->getNode("base-x", true)->setFloatValue(ws->_rootChord.x);
        sectN->getNode("base-y", true)->setFloatValue(ws->_rootChord.y);
        sectN->getNode("base-z", true)->setFloatValue(ws->_rootChord.z);
        sectN->getNode("chord", true)->setFloatValue(ws->_rootChord.length);
        sectN->getNode("incidence-deg", true)->setFloatValue(ws->_sectionIncidence * RAD2DEG);
        sectN->getNode("mac", true)->setFloatValue(ws->_mac.length);
        sectN->getNode("mac-x", true)->setFloatValue(ws->getMACx());
        sectN->getNode("mac-y", true)->setFloatValue(ws->getMACy());
        sectN->getNode("mac-z", true)->setFloatValue(ws->getMACz());
        
    }
    _wingN->getNode("weight", true)->setFloatValue(wgt);
    _wingN->getNode("drag", true)->setFloatValue(dragSum);
}

void Wing::weight2mass(float scale)
{
    if (_wingN == nullptr) 
        return;
    float wgt = _wingN->getNode("weight", true)->getFloatValue();
    _wingN->getNode("mass", true)->setFloatValue(wgt * scale);
}

// estimate a mass distibution and add masses to the model
// they will be scaled to match total mass of aircraft later
float Wing::updateModel(Model* model) 
{
    _weight = 0;
    WingSection* ws;
    for (int section=0; section < _sections.size(); section++) {
        ws = (WingSection*)_sections.get(section);
        for(int surf=0; surf < ws->numSurfaces(); surf++) {
            Surface* s = ws->getSurface(surf);
            model->addSurface(s);

            float weight = ws->getSurfaceWeight(surf);
            weight = weight * Math::sqrt(weight);
            _weight += weight;

            float pos[3];
            s->getPosition(pos);
            int mid = model->getBody()->addMass(weight, pos, true);
            if (_wingN != nullptr) {
                SGPropertyNode_ptr n = _wingN->getNode("surfaces", true)->getChild("surface", s->getID(), true);
                n->getNode("c0", true)->setFloatValue(s->getTotalForceCoefficient());
                n->getNode("cdrag", true)->setFloatValue(s->getDragCoefficient());
                n->getNode("clift", true)->setFloatValue(s->getLiftCoefficient());
                n->getNode("mass-id", true)->setIntValue(mid);
            }
        }
    }
    if (_wingN != nullptr) {
        _wingN->getNode("weight", true)->setFloatValue(_weight);
    }
    return _weight;
}

void Wing::printSectionInfo()
{
    WingSection* ws;
    printf("#wing sections: %d\n", _sections.size());
    for (int section=0; section < _sections.size(); section++) {
        ws = (WingSection*)_sections.get(section);
        printf("Section %d base point: x=\"%.3f\" y=\"%.3f\" z=\"%.3f\" chord=\"%.3f\" incidence=\"%.1f\" (at section root in deg)\n", section,
               ws->_rootChord.x, ws->_rootChord.y, ws->_rootChord.z, ws->_rootChord.length, ws->_sectionIncidence * RAD2DEG);
    }
}

}; // namespace yasim
