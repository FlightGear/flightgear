#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

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
}

int ControlMap::newInput()
{
    Vector* v = new Vector();
    return _inputs.add(v);
}

void ControlMap::addMapping(int input, int type, void* object, int options,
			    float src0, float src1, float dst0, float dst1)
{
    addMapping(input, type, object, options);

    // The one we just added is last in the list (ugly, awful hack!)
    Vector* maps = (Vector*)_inputs.get(input);
    MapRec* m = (MapRec*)maps->get(maps->size() - 1);

    m->src0 = src0;
    m->src1 = src1;
    m->dst0 = dst0;
    m->dst1 = dst1;
}

void ControlMap::addMapping(int input, int type, void* object, int options)
{
    // See if the output object already exists
    OutRec* out = 0;
    int i;
    for(i=0; i<_outputs.size(); i++) {
	OutRec* o = (OutRec*)_outputs.get(i);
	if(o->object == object && o->type == type) {
	    out = o;
	    break;
	}
    }

    // Create one if it doesn't
    if(out == 0) {
	out = new OutRec();
	out->type = type;
	out->object = object;
        out->oldL = out->oldR = out->time = 0;
	_outputs.add(out);
    }
    
    // Make a new input record
    MapRec* map = new MapRec();
    map->out = out;
    map->opt = options;
    map->idx = out->maps.add(map);

    // The default ranges differ depending on type!
    map->src1 = map->dst1 = rangeMax(type);
    map->src0 = map->dst0 = rangeMin(type);

    // And add it to the approproate vectors.
    Vector* maps = (Vector*)_inputs.get(input);
    maps->add(map);
}

void ControlMap::reset()
{
    // Set all the values to zero
    for(int i=0; i<_outputs.size(); i++) {
	OutRec* o = (OutRec*)_outputs.get(i);
	for(int j=0; j<o->maps.size(); j++)
	    ((MapRec*)(o->maps.get(j)))->val = 0;
    }
}

void ControlMap::setInput(int input, float val)
{
    Vector* maps = (Vector*)_inputs.get(input);
    for(int i=0; i<maps->size(); i++) {
	MapRec* m = (MapRec*)maps->get(i);

	float val2 = val;

	// Do the scaling operation.  Clamp to [src0:src1], rescale to
	// [0:1] within that range, then map to [dst0:dst1].
	if(val2 < m->src0) val2 = m->src0;
	if(val2 > m->src1) val2 = m->src1;
	val2 = (val2 - m->src0) / (m->src1 - m->src0);
	val2 = m->dst0 + val2 * (m->dst1 - m->dst0);

	m->val = val2;
    }
}

int ControlMap::getOutputHandle(void* obj, int type)
{
    for(int i=0; i<_outputs.size(); i++) {
	OutRec* o = (OutRec*)_outputs.get(i);
	if(o->object == obj && o->type == type)
	    return i;
    }
    return 0;
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
    for(outrec=0; outrec<_outputs.size(); outrec++) {
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
        
            float max = (dt/o->time) * (rangeMax(o->type) - rangeMin(o->type));
            if(adl > max) dl = dl*max/adl;
            if(adr > max) dr = dr*max/adr;

            lval = o->oldL + dl;
            rval = o->oldR + dr;
        }

        o->oldL = lval;
        o->oldR = rval;

	void* obj = o->object;
	switch(o->type) {
	case THROTTLE: ((Thruster*)obj)->setThrottle(lval);        break;
	case MIXTURE:  ((Thruster*)obj)->setMixture(lval);         break;
    case CONDLEVER: ((TurbineEngine*)((PropEngine*)
                        obj)->getEngine())->setCondLever(lval); break;
	case STARTER:  ((Thruster*)obj)->setStarter(lval != 0.0);  break;
	case MAGNETOS: ((PropEngine*)obj)->setMagnetos((int)lval); break;
	case ADVANCE:  ((PropEngine*)obj)->setAdvance(lval);       break;
        case PROPPITCH: ((PropEngine*)obj)->setPropPitch(lval);    break;
        case PROPFEATHER: ((PropEngine*)obj)->setPropFeather((int)lval); break;
	case REHEAT:   ((Jet*)obj)->setReheat(lval);               break;
	case VECTOR:   ((Jet*)obj)->setRotation(lval);             break;
	case BRAKE:    ((Gear*)obj)->setBrake(lval);               break;
	case STEER:    ((Gear*)obj)->setRotation(lval);            break;
	case EXTEND:   ((Gear*)obj)->setExtension(lval);           break;
	case HEXTEND:  ((Hook*)obj)->setExtension(lval);           break;
	case LEXTEND:  ((Launchbar*)obj)->setExtension(lval);      break;
    case LACCEL:   ((Launchbar*)obj)->setAcceleration(lval);   break;
	case CASTERING:((Gear*)obj)->setCastering(lval != 0);      break;
	case SLAT:     ((Wing*)obj)->setSlat(lval);                break;
	case FLAP0:    ((Wing*)obj)->setFlap0(lval, rval);         break;
	case FLAP0EFFECTIVENESS: ((Wing*)obj)->setFlap0Effectiveness(lval); break;
	case FLAP1:    ((Wing*)obj)->setFlap1(lval, rval);         break;
	case FLAP1EFFECTIVENESS: ((Wing*)obj)->setFlap1Effectiveness(lval); break;
	case SPOILER:  ((Wing*)obj)->setSpoiler(lval, rval);       break;
        case COLLECTIVE:   ((Rotor*)obj)->setCollective(lval);     break;
        case CYCLICAIL:    ((Rotor*)obj)->setCyclicail(lval,rval); break;
        case CYCLICELE:    ((Rotor*)obj)->setCyclicele(lval,rval); break;
        case TILTPITCH:    ((Rotor*)obj)->setTiltPitch(lval);      break;
        case TILTYAW:      ((Rotor*)obj)->setTiltYaw(lval);        break;
        case TILTROLL:     ((Rotor*)obj)->setTiltRoll(lval);       break;
        case ROTORBALANCE:
                           ((Rotor*)obj)->setRotorBalance(lval);   break;
        case ROTORBRAKE:   ((Rotorgear*)obj)->setRotorBrake(lval); break;
        case ROTORENGINEON: 
                        ((Rotorgear*)obj)->setEngineOn((int)lval); break;
        case ROTORENGINEMAXRELTORQUE: 
              ((Rotorgear*)obj)->setRotorEngineMaxRelTorque(lval); break;
        case ROTORRELTARGET:
                       ((Rotorgear*)obj)->setRotorRelTarget(lval); break;
	case REVERSE_THRUST: ((Jet*)obj)->setReverse(lval != 0);   break;
	case BOOST:
	    ((PistonEngine*)((Thruster*)obj)->getEngine())->setBoost(lval);
	    break;
        case WASTEGATE:
            ((PistonEngine*)((Thruster*)obj)->getEngine())->setWastegate(lval);
            break;
        case WINCHRELSPEED: ((Hitch*)obj)->setWinchRelSpeed(lval); break;
        case HITCHOPEN:    ((Hitch*)obj)->setOpen(lval!=0);       break;
        case PLACEWINCH:    ((Hitch*)obj)->setWinchPositionAuto(lval!=0); break;
        case FINDAITOW:     ((Hitch*)obj)->findBestAIObject(lval!=0); break;
	}
    }
}

float ControlMap::rangeMin(int type)
{
    // The minimum of the range for each type of control
    switch(type) {
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

float ControlMap::rangeMax(int type)
{
    // The maximum of the range for each type of control
    switch(type) {
    case FLAP0:    return 1; // [-1:1]
    case FLAP1:    return 1;
    case STEER:    return 1;
    case MAGNETOS: return 3; // [0:3]
    case FLAP0EFFECTIVENESS: return 10;//  [0:10]
    case FLAP1EFFECTIVENESS: return 10;//  [0:10]
    default:       return 1; // [0:1]
    }
}

} // namespace yasim
