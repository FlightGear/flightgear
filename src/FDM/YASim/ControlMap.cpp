#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>

#include "Jet.hpp"
#include "Thruster.hpp"
#include "PropEngine.hpp"
#include "PistonEngine.hpp"
#include "TurbineEngine.hpp"
#include "Gear.hpp"
#include "Hook.hpp"
#include "Launchbar.hpp"
#include "Wing.hpp"
#include "Rotor.hpp"
#include "Math.hpp"
#include "Propeller.hpp"
#include "Hitch.hpp"

#include "ControlMap.hpp"
namespace yasim {

//! keep this list in sync with the enum ControlType in ControlMap.hpp !
static const std::vector<std::string> ControlNames = {
    "THROTTLE",
    "MIXTURE",
    "CONDLEVER",
    "STARTER", 
    "MAGNETOS",
    "ADVANCE", 
    "REHEAT", 
    "PROP",
    "BRAKE",
    "STEER",
    "EXTEND",
    "HEXTEND",
    "LEXTEND",
    "LACCEL",
    "INCIDENCE",
    "FLAP0",
    "FLAP1",
    "SLAT",
    "SPOILER",
    "VECTOR",
    "FLAP0EFFECTIVENESS",
    "FLAP1EFFECTIVENESS",
    "BOOST",
    "CASTERING",
    "PROPPITCH",
    "PROPFEATHER",
    "COLLECTIVE", 
    "CYCLICAIL", 
    "CYCLICELE", 
    "ROTORGEARENGINEON",
    "TILTYAW", 
    "TILTPITCH", 
    "TILTROLL",
    "ROTORBRAKE", 
    "ROTORENGINEMAXRELTORQUE", 
    "ROTORRELTARGET",
    "ROTORBALANCE",
    "REVERSE_THRUST",
    "WASTEGATE",
    "WINCHRELSPEED",
    "HITCHOPEN",
    "PLACEWINCH",
    "FINDAITOW"
}; //! keep this list in sync with the enum ControlType in ControlMap.hpp !

ControlMap::~ControlMap()
{
    for(int i=0; i<_inputs.size(); i++) {
        Vector* v = (Vector*)_inputs.get(i);
        for(int j=0; j<v->size(); j++)
            delete (MapRec*)v->get(j);
        delete v;
    }

    for(int i=0; i<_outputs.size(); i++)
        delete (OutRec*)_outputs.get(i);
   
    for(int i=0; i<_properties.size(); i++) {
        PropHandle* p = (PropHandle*)_properties.get(i);
        delete[] p->name;
        delete p;
    }  
}

/**
inputProp: name of input property
control: identifier (see enum OutputType)
id: object to which this input belongs to
options: bits OPT_INVERT, OPT_SPLIT, OPT_SQUARE
src,dst: input will be clamped to src range and mapped to dst range 
*/
void ControlMap::addMapping(const char* inputProp, ControlType control, ObjectID id, int options, float src0, float src1, float dst0, float dst1)
{
    OutRec* out = getOutRec(id, control);
    
    // Make a new input record
    MapRec* map = new MapRec();
    map->opt = options;
    map->id = out->maps.add(map);

    // The default ranges differ depending on type!
    map->src1 = map->dst1 = rangeMax(control);
    map->src0 = map->dst0 = rangeMin(control);

    // And add it to the approproate vectors.
    Vector* maps = (Vector*)_inputs.get(getInputPropertyHandle(inputProp));
    maps->add(map);

    map->src0 = src0;
    map->src1 = src1;
    map->dst0 = dst0;
    map->dst1 = dst1;
}

ControlMap::OutRec* ControlMap::getOutRec(ObjectID id, ControlType control)
{
    OutRec* out {nullptr};
    for(int i = 0; i < _outputs.size(); i++) {
        OutRec* o = (OutRec*)_outputs.get(i);
        if(o->oid.object == id.object && o->oid.subObj == id.subObj 
            && o->control == control) 
        {
            out = o;
            break;
        }
    }    

    // Create one if it doesn't
    if(out == nullptr) {
        out = new OutRec();
        out->control = control;
        out->oid = id;
        out->id = _outputs.add(out);
    }
    return out;
}

void ControlMap::reset()
{
    // Set all the values to zero
    for(int i = 0; i < _outputs.size(); i++) {
        OutRec* o = (OutRec*)_outputs.get(i);
        for(int j = 0; j < o->maps.size(); j++) {
            ((MapRec*)(o->maps.get(j)))->val = 0;
        }
    }
}

void ControlMap::setInput(int propHandle, float val)
{
    Vector* maps = (Vector*)_inputs.get(propHandle);
    for(int i = 0; i < maps->size(); i++) {
        MapRec* m = (MapRec*)maps->get(i);
        float val2 = val;
        // Do the scaling operation.  Clamp to [src0:src1], rescale to
        // [0:1] within that range, then map to [dst0:dst1].
        val2 = Math::clamp(val2, m->src0, m->src1);
        val2 = (val2 - m->src0) / (m->src1 - m->src0);
        m->val = m->dst0 + val2 * (m->dst1 - m->dst0);
    }
}

int ControlMap::getOutputHandle(ObjectID id, ControlType control)
{
    return getOutRec(id, control)->id;
}

void ControlMap::setTransitionTime(int handle, float time)
{
    ((OutRec*)_outputs.get(handle))->transitionTime = time;
}

float ControlMap::getOutput(int handle)
{
    return ((OutRec*)_outputs.get(handle))->oldValueLeft;
}

float ControlMap::getOutputR(int handle)
{
    return ((OutRec*)_outputs.get(handle))->oldValueRight;
}

void ControlMap::applyControls(float dt)
{
    for(int outrec=0; outrec<_outputs.size(); outrec++) 
    {
        OutRec* o = (OutRec*)_outputs.get(outrec);
    
        // Generate a summed value.  Note the check for "split"
        // control axes like ailerons.
        float lval = 0, rval = 0;
        for(int i = 0; i < o->maps.size(); i++) {
            MapRec* m = (MapRec*)o->maps.get(i);
            float val = m->val;

            if(m->opt & OPT_SQUARE) { val = val * Math::abs(val); }
            if(m->opt & OPT_INVERT) { val = -val; }
            lval += val;
            if(m->opt & OPT_SPLIT) { rval -= val; }
            else { rval += val; }
        }

        // If there is a finite transition time, clamp the values to
        // the maximum travel allowed in this dt.
        if(o->transitionTime > 0) {
            float dl = lval - o->oldValueLeft;
            float dr = rval - o->oldValueRight;
            float adl = Math::abs(dl);
            float adr = Math::abs(dr);
        
            float maxDelta = (dt/o->transitionTime) * (rangeMax(o->control) - rangeMin(o->control));
            if(adl > maxDelta) {
                dl = dl*maxDelta/adl;
                lval = o->oldValueLeft + dl;
            }
            if(adr > maxDelta) {
                dr = dr*maxDelta/adr;
                rval = o->oldValueRight + dr;
            }
        }

        o->oldValueLeft = lval;
        o->oldValueRight = rval;

        void* obj = o->oid.object;
        switch(o->control) {
            case THROTTLE:
                ((Thruster*)obj)->setThrottle(lval);
                break;
            case MIXTURE:
                ((Thruster*)obj)->setMixture(lval);
                break;
            case CONDLEVER:
                ((TurbineEngine*)((PropEngine*)obj)->getEngine())->setCondLever(lval);
                break;
            case STARTER:
                ((Thruster*)obj)->setStarter(lval != 0.0);
                break;
            case MAGNETOS:
                ((PropEngine*)obj)->setMagnetos((int)lval);
                break;
            case ADVANCE:
                ((PropEngine*)obj)->setAdvance(lval);
                break;
            case PROPPITCH:
                ((PropEngine*)obj)->setPropPitch(lval);
                break;
            case PROPFEATHER:
                ((PropEngine*)obj)->setPropFeather((int)lval);
                break;
            case REHEAT:
                ((Jet*)obj)->setReheat(lval);
                break;
            case VECTOR:
                ((Jet*)obj)->setRotation(lval);
                break;
            case BRAKE:
                ((Gear*)obj)->setBrake(lval);
                break;
            case STEER:
                ((Gear*)obj)->setRotation(lval);
                break;
            case EXTEND:
                ((Gear*)obj)->setExtension(lval);
                break;
            case HEXTEND:
                ((Hook*)obj)->setExtension(lval);
                break;
            case LEXTEND:
                ((Launchbar*)obj)->setExtension(lval);
                break;
            case LACCEL:
                ((Launchbar*)obj)->setAcceleration(lval);
                break;
            case CASTERING:
                ((Gear*)obj)->setCastering(lval != 0);
                break;
            case SLAT:
                ((Wing*)obj)->setFlapPos(WING_SLAT,lval);
                break;
            case FLAP0:
                ((Wing*)obj)->setFlapPos(WING_FLAP0, lval, rval);
                break;
            case FLAP0EFFECTIVENESS:
                ((Wing*)obj)->setFlapEffectiveness(WING_FLAP0,lval);
                break;
            case FLAP1:
                ((Wing*)obj)->setFlapPos(WING_FLAP1,lval, rval);         
                break;
            case FLAP1EFFECTIVENESS:
                ((Wing*)obj)->setFlapEffectiveness(WING_FLAP1,lval);
                break;
            case SPOILER:
                ((Wing*)obj)->setFlapPos(WING_SPOILER, lval, rval);       
                break;
            case COLLECTIVE:
                ((Rotor*)obj)->setCollective(lval);     
                break;
            case CYCLICAIL:
                ((Rotor*)obj)->setCyclicail(lval,rval); 
                break;
            case CYCLICELE:
                ((Rotor*)obj)->setCyclicele(lval,rval); 
                break;
            case TILTPITCH:
                ((Rotor*)obj)->setTiltPitch(lval);      
                break;
            case TILTYAW:
                ((Rotor*)obj)->setTiltYaw(lval);        
                break;
            case TILTROLL:
                ((Rotor*)obj)->setTiltRoll(lval);       
                break;
            case ROTORBALANCE:
                ((Rotor*)obj)->setRotorBalance(lval);   
                break;
            case ROTORBRAKE:   
                ((Rotorgear*)obj)->setRotorBrake(lval); 
                break;
            case ROTORGEARENGINEON: 
                ((Rotorgear*)obj)->setEngineOn((int)lval); 
                break;
            case ROTORENGINEMAXRELTORQUE: 
                ((Rotorgear*)obj)->setRotorEngineMaxRelTorque(lval); 
                break;
            case ROTORRELTARGET:
                ((Rotorgear*)obj)->setRotorRelTarget(lval);
                break;
            case REVERSE_THRUST: 
                ((Jet*)obj)->setReverse(lval != 0);
                break;
            case BOOST:
                ((PistonEngine*)((Thruster*)obj)->getEngine())->setBoost(lval);
                break;
            case WASTEGATE:
                ((PistonEngine*)((Thruster*)obj)->getEngine())->setWastegate(lval);
                break;
            case WINCHRELSPEED:
                ((Hitch*)obj)->setWinchRelSpeed(lval); 
                break;
            case HITCHOPEN:
                ((Hitch*)obj)->setOpen(lval!=0);
                break;
            case PLACEWINCH:
                ((Hitch*)obj)->setWinchPositionAuto(lval!=0);
                break;
            case FINDAITOW:
                ((Hitch*)obj)->findBestAIObject(lval!=0);
                break;
            case PROP:
                break;
            case INCIDENCE:
                ((Wing*)obj)->setIncidence(lval);
                break;
        }
    }
}

float ControlMap::rangeMin(ControlType control)
{
    // The minimum of the range for each type of control
    switch(control) {
        case INCIDENCE: return INCIDENCE_MIN;
        case FLAP0:    return -1;  // [-1:1]
        case FLAP1:    return -1;
        case STEER:    return -1;
        case CYCLICELE: return -1;
        case CYCLICAIL: return -1;
        case COLLECTIVE: return -1;
        case WINCHRELSPEED: return -1;
        case MAGNETOS: return 0;   // [0:3]
        case FLAP0EFFECTIVENESS: return 1;  // [0:10]
        case FLAP1EFFECTIVENESS: return 1;  // [0:10]
        default:       return 0;   // [0:1]
    }
}

float ControlMap::rangeMax(ControlType control)
{
    // The maximum of the range for each type of control
    switch(control) {
        case INCIDENCE: return INCIDENCE_MAX;
        case FLAP0:    return 1; // [-1:1]
        case FLAP1:    return 1;
        case STEER:    return 1;
        case MAGNETOS: return 3; // [0:3]
        case FLAP0EFFECTIVENESS: return 10;//  [0:10]
        case FLAP1EFFECTIVENESS: return 10;//  [0:10]
        default:       return 1; // [0:1]
    }
}

/// register property name, return ID (int)
int ControlMap::getInputPropertyHandle(const char* name)
{
    // search for existing
    for(int i=0; i < _properties.size(); i++) {
        PropHandle* p = (PropHandle*)_properties.get(i);
        if(!strcmp(p->name, name))
            return p->handle;
    }

    // else create new
    PropHandle* p = new PropHandle();
    p->name = strdup(name);
    
    fgGetNode(p->name, true); 

    Vector* v = new Vector();
    p->handle = _inputs.add(v);

    _properties.add(p);
    return p->handle;
}

ControlMap::ControlType ControlMap::getControlByName(const std::string& name)
{
    auto it = std::find(ControlNames.begin(), ControlNames.end(), name);
    if (it == ControlNames.end()) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Unrecognized control type '" << name 
            << "' in YASim aircraft description.");
        exit(1);
    }
    return static_cast<ControlType>(std::distance(ControlNames.begin(), it));
}

std::string ControlMap::getControlName(ControlType c)
{
    return ControlNames.at(static_cast<int>(c));
}

ControlMap::ControlType ControlMap::parseControl(const char* name)
{
    std::string n(name);
    return getControlByName(n);
}

ControlMap::ObjectID ControlMap::getObjectID(void* object, int subObj)
{
    assert(object != nullptr);
    ObjectID o;
    o.object = object;
    o.subObj = subObj;
    return o;
}

// used at runtime in FGFDM::getExternalInput
ControlMap::PropHandle* ControlMap::getProperty(const int i) {
    assert((i >= 0) && (i < _properties.size()));
    return ((PropHandle*)_properties.get(i));
}

} // namespace yasim
