#include "Math.hpp"
#include "RigidBody.hpp"

#include "Gear.hpp"
namespace yasim {

Gear::Gear()
{
    for(int i=0; i<3; i++)
	_pos[i] = _cmpr[i] = 0;
    _spring = 1;
    _damp = 0;
    _sfric = 0.8;
    _dfric = 0.7;
    _brake = 0;
    _rot = 0;
    _extension = 1;
}

void Gear::setPosition(float* position)
{
    for(int i=0; i<3; i++) _pos[i] = position[i];
}

void Gear::setCompression(float* compression)
{
    for(int i=0; i<3; i++) _cmpr[i] = compression[i];
}

void Gear::setSpring(float spring)
{
    _spring = spring;
}

void Gear::setDamping(float damping)
{
    _damp = damping;
}

void Gear::setStaticFriction(float sfric)
{
    _sfric = sfric;
}

void Gear::setDynamicFriction(float dfric)
{
    _dfric = dfric;
}

void Gear::setBrake(float brake)
{
    _brake = Math::clamp(brake, 0, 1);
}

void Gear::setRotation(float rotation)
{
    _rot = rotation;
}

void Gear::setExtension(float extension)
{
    _extension = Math::clamp(extension, 0, 1);
}

void Gear::getPosition(float* out)
{
    for(int i=0; i<3; i++) out[i] = _pos[i];
}

void Gear::getCompression(float* out)
{
    for(int i=0; i<3; i++) out[i] = _cmpr[i];    
}

float Gear::getSpring()
{
    return _spring;
}

float Gear::getDamping()
{
    return _damp;
}

float Gear::getStaticFriction()
{
    return _sfric;
}

float Gear::getDynamicFriction()
{
    return _dfric;
}

float Gear::getBrake()
{
    return _brake;
}

float Gear::getRotation()
{
    return _rot;
}

float Gear::getExtension()
{
    return _extension;
}

void Gear::getForce(float* force, float* contact)
{
    Math::set3(_force, force);
    Math::set3(_contact, contact);
}

float Gear::getWoW()
{
    return _wow;
}

float Gear::getCompressFraction()
{
    return _frac;
}

void Gear::calcForce(RigidBody* body, float* v, float* rot, float* ground)
{
    // Init the return values
    for(int i=0; i<3; i++) _force[i] = _contact[i] = 0;

    // Don't bother if it's not down
    if(_extension < 1)
	return;

    float tmp[3];

    // First off, make sure that the gear "tip" is below the ground.
    // If it's not, there's no force.
    float a = ground[3] - Math::dot3(_pos, ground);
    if(a > 0)
	return;

    // Now a is the distance from the tip to ground, so make b the
    // distance from the base to ground.  We can get the fraction
    // (0-1) of compression from a/(a-b). Note the minus sign -- stuff
    // above ground is negative.
    Math::add3(_cmpr, _pos, tmp);
    float b = ground[3] - Math::dot3(tmp, ground);

    // Calculate the point of ground _contact.
    _frac = a/(a-b);
    if(b < 0) _frac = 1;
    for(int i=0; i<3; i++)
	_contact[i] = _pos[i] + _frac*_cmpr[i];

    // Turn _cmpr into a unit vector and a magnitude
    float cmpr[3];
    float clen = Math::mag3(_cmpr);
    Math::mul3(1/clen, _cmpr, cmpr);

    // Now get the velocity of the point of contact
    float cv[3];
    body->pointVelocity(_contact, rot, cv);
    Math::add3(cv, v, cv);

    // Finally, we can start adding up the forces.  First the
    // spring compression (note the clamping of _frac to 1):
    _frac = (_frac > 1) ? 1 : _frac;
    float fmag = _frac*clen*_spring;
    Math::mul3(fmag, cmpr, _force);

    // Then the damping.  Use the only the velocity into the ground
    // (projection along "ground") projected along the compression
    // axis.  So Vdamp = ground*(ground dot cv) dot cmpr
    Math::mul3(Math::dot3(ground, cv), ground, tmp);
    float dv = Math::dot3(cmpr, tmp);
    float damp = _damp * dv;
    if(damp > fmag) damp = fmag; // can't pull the plane down!
    if(damp < -fmag) damp = -fmag; // sanity
    Math::mul3(-damp, cmpr, tmp);
    Math::add3(_force, tmp, _force);

    _wow = fmag - damp;    

    // Wheels are funky.  Split the velocity along the ground plane
    // into rolling and skidding components.  Assuming small angles,
    // we generate "forward" and "left" unit vectors (the compression
    // goes "up") for the gear, make a "steer" direction from these,
    // and then project it onto the ground plane.  Project the
    // velocity onto the ground plane too, and extract the "steer"
    // component.  The remainder is the skid velocity.

    float gup[3]; // "up" unit vector from the ground
    Math::set3(ground, gup);
    Math::mul3(-1, gup, gup);

    float xhat[] = {1,0,0};
    float steer[3], skid[3];
    Math::cross3(gup, xhat, skid);  // up cross xhat =~ skid
    Math::unit3(skid, skid);        //               == skid

    Math::cross3(skid, gup, steer); // skid cross up == steer

    if(_rot != 0) {
	// Correct for a (small) rotation
	Math::mul3(_rot, steer, tmp);
	Math::add3(tmp, skid, skid);
	Math::unit3(skid, skid);
	Math::cross3(skid, gup, steer);
    }

    float vsteer = Math::dot3(cv, steer);
    float vskid  = Math::dot3(cv, skid);
    float wgt = Math::dot3(_force, gup); // force into the ground

    float fsteer = _brake * calcFriction(wgt, vsteer);
    float fskid  = calcFriction(wgt, vskid);
    if(vsteer > 0) fsteer = -fsteer;
    if(vskid > 0) fskid = -fskid;

    // Phoo!  All done.  Add it up and get out of here.
    Math::mul3(fsteer, steer, tmp);
    Math::add3(tmp, _force, _force);

    Math::mul3(fskid, skid, tmp);
    Math::add3(tmp, _force, _force);
}

float Gear::calcFriction(float wgt, float v)
{
    // How slow is stopped?  50 cm/second?
    const float STOP = 0.5;
    const float iSTOP = 1/STOP;
    v = Math::abs(v);
    if(v < STOP) return v*iSTOP * wgt * _sfric;
    else         return wgt * _dfric;
}

}; // namespace yasim

