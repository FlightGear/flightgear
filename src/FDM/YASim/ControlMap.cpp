#include "Jet.hpp"
#include "Thruster.hpp"
#include "Gear.hpp"
#include "Wing.hpp"
#include "Math.hpp"

#include "ControlMap.hpp"
namespace yasim {

ControlMap::~ControlMap()
{
    for(int i=0; i<_inputs.size(); i++) {
	Vector* v = (Vector*)_inputs.get(i);
	for(int j=0; j<v->size(); j++)
	    delete (MapRec*)v->get(j);
	delete v;
    }

    for(int i=0; i<_outputs.size(); i++) {
	OutRec* o = (OutRec*)_outputs.get(i);
	delete[] o->values;
	delete o;
    }
}

int ControlMap::newInput()
{
    Vector* v = new Vector();
    return _inputs.add(v);
}

void ControlMap::addMapping(int input, int type, void* object, int options)
{
    // See if the output object already exists
    OutRec* out = 0;
    for(int i=0; i<_outputs.size(); i++) {
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
	out->n = 0;
	out->values = 0;
	_outputs.add(out);
    }

    // Make space for the new input value
    int idx = out->n++;
    delete[] out->values;
    out->values = new float[out->n];

    // Add the new option tag
    out->options.add((void*)options);

    // Make a new input record
    MapRec* map = new MapRec();
    map->out = out;
    map->idx = idx;

    // And add it to the approproate vector.
    Vector* maps = (Vector*)_inputs.get(input);
    maps->add(map);
}

void ControlMap::reset()
{
    // Set all the values to zero
    for(int i=0; i<_outputs.size(); i++) {
	OutRec* o = (OutRec*)_outputs.get(i);
	for(int j=0; j<o->n; j++)
	    o->values[j] = 0;
    }
}

void ControlMap::setInput(int input, float value)
{
    Vector* maps = (Vector*)_inputs.get(input);
    for(int i=0; i<maps->size(); i++) {
	MapRec* map = (MapRec*)maps->get(i);
	map->out->values[map->idx] = value;
    }
}

void ControlMap::applyControls()
{
    for(int outrec=0; outrec<_outputs.size(); outrec++) {
	OutRec* o = (OutRec*)_outputs.get(outrec);
	
	// Generate a summed value.  Note the check for "split"
	// control axes like ailerons.
	float lval = 0, rval = 0;
	for(int i=0; i<o->n; i++) {
	    float val = o->values[i];
	    int opt = (int)o->options.get(i);
	    if(opt & OPT_SQUARE)
		val = val * Math::abs(val);
 	    if(opt & OPT_INVERT)
		val = -val;
	    lval += val;
	    if(opt & OPT_SPLIT)
		rval -= val;
	    else
		rval += val;
	}

	void* obj = o->object;
	switch(o->type) {
	case THROTTLE: ((Thruster*)obj)->setThrottle(lval);    break;
	case MIXTURE:  ((Thruster*)obj)->setMixture(lval);     break;
	case PROP:     ((Thruster*)obj)->setPropAdvance(lval); break;
	case REHEAT:   ((Jet*)obj)->setReheat(lval);           break;
	case BRAKE:    ((Gear*)obj)->setBrake(lval);           break;
	case STEER:    ((Gear*)obj)->setRotation(lval);        break;
	case EXTEND:   ((Gear*)obj)->setExtension(lval);       break;
	case SLAT:     ((Wing*)obj)->setSlat(lval);            break;
	case FLAP0:    ((Wing*)obj)->setFlap0(lval, rval);     break;
	case FLAP1:    ((Wing*)obj)->setFlap1(lval, rval);     break;
	case SPOILER:  ((Wing*)obj)->setSpoiler(lval, rval);   break;
	}
    }
}

}; // namespace yasim
