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

ControlMap::~ControlMap()
{
  int i;
  for(i=0; i<_inputs.size(); i++) {
    Vector* v = (Vector*)_inputs.get(i);
    int j;
    for(j=0; j<v->size(); j++)
	    delete (MapRec*)v->get(j);
    delete v;
  }

  for(i=0; i<_outputs.size(); i++)
    delete (OutRec*)_outputs.get(i);
   
	for(i=0; i<_properties.size(); i++) {
    PropHandle* p = (PropHandle*)_properties.get(i);
    delete[] p->name;
    delete p;
  }  
}

/**
prop: name of input property
control: identifier (see enum OutputType)
object: object to which this input belongs to
options: bits OPT_INVERT, OPT_SPLIT, OPT_SQUARE
*/
void ControlMap::addMapping(const char* prop, Control control, ObjectID id, int options, float src0, float src1, float dst0, float dst1)
{
    addMapping(prop, control, id, options);
    int inputPropHandle = getPropertyHandle(prop);

    // The one we just added is last in the list (ugly, awful hack!)
    Vector* maps = (Vector*)_inputs.get(inputPropHandle);
    MapRec* m = (MapRec*)maps->get(maps->size() - 1);

    m->src0 = src0;
    m->src1 = src1;
    m->dst0 = dst0;
    m->dst1 = dst1;
}

/**
prop: name of input property
control: identifier (see enum OutputType)
object: object to which this input belongs to
options: bits OPT_INVERT, OPT_SPLIT, OPT_SQUARE
*/
void ControlMap::addMapping(const char* prop, Control control, ObjectID id, int options)
{
    int inputPropHandle = getPropertyHandle(prop);
    // See if the output object already exists
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
        _outputs.add(out);
    }
    
    // Make a new input record
    MapRec* map = new MapRec();
    map->out = out;
    map->opt = options;
    map->idx = out->maps.add(map);

    // The default ranges differ depending on type!
    map->src1 = map->dst1 = rangeMax(control);
    map->src0 = map->dst0 = rangeMin(control);

    // And add it to the approproate vectors.
    Vector* maps = (Vector*)_inputs.get(inputPropHandle);
    maps->add(map);
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

void ControlMap::setInput(int input, float val)
{
    Vector* maps = (Vector*)_inputs.get(input);
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

int ControlMap::getOutputHandle(ObjectID id, Control control)
{
    for(int i = 0; i < _outputs.size(); i++) {
        OutRec* o = (OutRec*)_outputs.get(i);
	    if(o->oid.object == id.object && o->oid.subObj == id.subObj 
            && o->control == control)
	        return i;
    }
    fprintf(stderr, "ControlMap::getOutputHandle cannot find *%ld, control %d \n", (long)id.object, control);
    return -1;
}

void ControlMap::setTransitionTime(int handle, float time)
{
    ((OutRec*)_outputs.get(handle))->time = time;
}

float ControlMap::getOutput(int handle)
{
    return ((OutRec*)_outputs.get(handle))->oldL;
}

float ControlMap::getOutputR(int handle)
{
    return ((OutRec*)_outputs.get(handle))->oldR;
}

void ControlMap::applyControls(float dt)
{
    int outrec;
    for(outrec=0; outrec<_outputs.size(); outrec++) 
    {
        OutRec* o = (OutRec*)_outputs.get(outrec);
    
        // Generate a summed value.  Note the check for "split"
        // control axes like ailerons.
        float lval = 0, rval = 0;
        int i;
        for(i=0; i<o->maps.size(); i++) {
            MapRec* m = (MapRec*)o->maps.get(i);
            float val = m->val;

            if(m->opt & OPT_SQUARE)
            val = val * Math::abs(val);
            if(m->opt & OPT_INVERT)
            val = -val;
            lval += val;
            if(m->opt & OPT_SPLIT)
            rval -= val;
            else
            rval += val;
        }

        // If there is a finite transition time, clamp the values to
        // the maximum travel allowed in this dt.
        if(o->time > 0) {
            float dl = lval - o->oldL;
            float dr = rval - o->oldR;
            float adl = Math::abs(dl);
            float adr = Math::abs(dr);
        
            float max = (dt/o->time) * (rangeMax(o->control) - rangeMin(o->control));
            if(adl > max) dl = dl*max/adl;
            if(adr > max) dr = dr*max/adr;

            lval = o->oldL + dl;
            rval = o->oldR + dr;
        }

        o->oldL = lval;
        o->oldR = rval;

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
            case ROTORENGINEON: 
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
            case INCIDENCE:
                break;
        }
    }
}

float ControlMap::rangeMin(Control control)
{
    // The minimum of the range for each type of control
    switch(control) {
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

float ControlMap::rangeMax(Control control)
{
    // The maximum of the range for each type of control
    switch(control) {
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
int ControlMap::getPropertyHandle(const char* name)
{
	for(int i=0; i < _properties.size(); i++) {
		PropHandle* p = (PropHandle*)_properties.get(i);
		if(!strcmp(p->name, name))
			return p->handle;
	}

	// create new
	PropHandle* p = new PropHandle();
	p->name = strdup(name);
	
	fgGetNode(p->name, true); 

	Vector* v = new Vector();
	p->handle = _inputs.add(v);
	_properties.add(p);
	return p->handle;
}


ControlMap::Control ControlMap::parseControl(const char* name)
{
    if(!strcmp(name, "THROTTLE"))  return THROTTLE;
    if(!strcmp(name, "MIXTURE"))   return MIXTURE;
    if(!strcmp(name, "CONDLEVER")) return CONDLEVER;
    if(!strcmp(name, "STARTER"))   return STARTER;
    if(!strcmp(name, "MAGNETOS"))  return MAGNETOS;
    if(!strcmp(name, "ADVANCE"))   return ADVANCE;
    if(!strcmp(name, "REHEAT"))    return REHEAT;
    if(!strcmp(name, "BOOST"))     return BOOST;
    if(!strcmp(name, "VECTOR"))    return VECTOR;
    if(!strcmp(name, "PROP"))      return PROP;
    if(!strcmp(name, "BRAKE"))     return BRAKE;
    if(!strcmp(name, "STEER"))     return STEER;
    if(!strcmp(name, "EXTEND"))    return EXTEND;
    if(!strcmp(name, "HEXTEND"))   return HEXTEND;
    if(!strcmp(name, "LEXTEND"))   return LEXTEND;
    if(!strcmp(name, "LACCEL"))    return LACCEL;
    if(!strcmp(name, "INCIDENCE")) return INCIDENCE;
    if(!strcmp(name, "FLAP0"))     return FLAP0;
    if(!strcmp(name, "FLAP0EFFECTIVENESS"))   return FLAP0EFFECTIVENESS;
    if(!strcmp(name, "FLAP1"))     return FLAP1;
    if(!strcmp(name, "FLAP1EFFECTIVENESS"))   return FLAP1EFFECTIVENESS;
    if(!strcmp(name, "SLAT"))      return SLAT;
    if(!strcmp(name, "SPOILER"))   return SPOILER;
    if(!strcmp(name, "CASTERING")) return CASTERING;
    if(!strcmp(name, "PROPPITCH")) return PROPPITCH;
    if(!strcmp(name, "PROPFEATHER")) return PROPFEATHER;
    if(!strcmp(name, "COLLECTIVE")) return COLLECTIVE;
    if(!strcmp(name, "CYCLICAIL")) return CYCLICAIL;
    if(!strcmp(name, "CYCLICELE")) return CYCLICELE;
    if(!strcmp(name, "TILTROLL")) return TILTROLL;
    if(!strcmp(name, "TILTPITCH")) return TILTPITCH;
    if(!strcmp(name, "TILTYAW")) return TILTYAW;
    if(!strcmp(name, "ROTORGEARENGINEON")) return ROTORENGINEON;
    if(!strcmp(name, "ROTORBRAKE")) return ROTORBRAKE;
    if(!strcmp(name, "ROTORENGINEMAXRELTORQUE")) return ROTORENGINEMAXRELTORQUE;
    if(!strcmp(name, "ROTORRELTARGET")) return ROTORRELTARGET;
    if(!strcmp(name, "ROTORBALANCE")) return ROTORBALANCE;
    if(!strcmp(name, "REVERSE_THRUST")) return REVERSE_THRUST;
    if(!strcmp(name, "WASTEGATE")) return WASTEGATE;
    if(!strcmp(name, "WINCHRELSPEED")) return WINCHRELSPEED;
    if(!strcmp(name, "HITCHOPEN")) return HITCHOPEN;
    if(!strcmp(name, "PLACEWINCH")) return PLACEWINCH;
    if(!strcmp(name, "FINDAITOW")) return FINDAITOW;
    SG_LOG(SG_FLIGHT,SG_ALERT,"Unrecognized control type '" << name 
        << "' in YASim aircraft description.");
    exit(1);
}

ControlMap::ObjectID ControlMap::getObjectID(void* object, int subObj)
{
    ObjectID o;
    o.object = object;
    o.subObj = subObj;
    return o;
}

} // namespace yasim
