#include "Jet.hpp"
#include "Thruster.hpp"
#include "PropEngine.hpp"
#include "PistonEngine.hpp"
#include "Gear.hpp"
#include "Wing.hpp"
#include "Math.hpp"

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
	_outputs.add(out);
    }
    
    // Make a new input record
    MapRec* map = new MapRec();
    map->out = out;
    map->opt = options;
    map->idx = out->maps.add(map);

    // The default ranges differ depending on type!
    map->src1 = map->dst1 = 1;
    map->src0 = map->dst0 = 0;
    if(type==FLAP0 || type==FLAP1 || type==STEER)
	map->src0 = map->dst0 = -1;
    if(type==MAGNETOS)
	map->src1 = map->dst1 = 3;

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
	    ((MapRec*)o->maps.get(j))->val = 0;
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

void ControlMap::applyControls()
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

	void* obj = o->object;
	switch(o->type) {
	case THROTTLE: ((Thruster*)obj)->setThrottle(lval);        break;
	case MIXTURE:  ((Thruster*)obj)->setMixture(lval);         break;
	case STARTER:  ((Thruster*)obj)->setStarter((bool)lval);   break;
	case MAGNETOS: ((PropEngine*)obj)->setMagnetos((int)lval); break;
	case ADVANCE:  ((PropEngine*)obj)->setAdvance(lval);       break;
	case REHEAT:   ((Jet*)obj)->setReheat(lval);               break;
	case VECTOR:   ((Jet*)obj)->setRotation(lval);             break;
	case BRAKE:    ((Gear*)obj)->setBrake(lval);               break;
	case STEER:    ((Gear*)obj)->setRotation(lval);            break;
	case EXTEND:   ((Gear*)obj)->setExtension(lval);           break;
	case SLAT:     ((Wing*)obj)->setSlat(lval);                break;
	case FLAP0:    ((Wing*)obj)->setFlap0(lval, rval);         break;
	case FLAP1:    ((Wing*)obj)->setFlap1(lval, rval);         break;
	case SPOILER:  ((Wing*)obj)->setSpoiler(lval, rval);       break;
	case BOOST:
	    ((Thruster*)obj)->getPistonEngine()->setBoost(lval);
	    break;
	}
    }
}

}; // namespace yasim
