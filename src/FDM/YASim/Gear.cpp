#include "Math.hpp"
#include "BodyEnvironment.hpp"
#include "RigidBody.hpp"

#include "Gear.hpp"
namespace yasim {

Gear::Gear()
{
    int i;
    for(i=0; i<3; i++)
	_pos[i] = _cmpr[i] = 0;
    _spring = 1;
    _damp = 0;
    _sfric = 0.8f;
    _dfric = 0.7f;
    _brake = 0;
    _rot = 0;
    _extension = 1;
    _castering = false;
    _frac = 0;

    for(i=0; i<3; i++)
	_global_ground[i] = _global_vel[i] = 0;
    _global_ground[2] = 1;
    _global_ground[3] = -1e3;
}

void Gear::setPosition(float* position)
{
    int i;
    for(i=0; i<3; i++) _pos[i] = position[i];
}

void Gear::setCompression(float* compression)
{
    int i;
    for(i=0; i<3; i++) _cmpr[i] = compression[i];
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

void Gear::setCastering(bool c)
{
    _castering = c;
}

void Gear::setGlobalGround(double *global_ground, float* global_vel)
{
    int i;
    for(i=0; i<4; i++) _global_ground[i] = global_ground[i];
    for(i=0; i<3; i++) _global_vel[i] = global_vel[i];
}

void Gear::getPosition(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _pos[i];
}

void Gear::getCompression(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _cmpr[i];    
}

void Gear::getGlobalGround(double* global_ground)
{
    int i;
    for(i=0; i<4; i++) global_ground[i] = _global_ground[i];
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

bool Gear::getCastering()
{
    return _castering;
}

void Gear::calcForce(RigidBody* body, State *s, float* v, float* rot)
{
    // Init the return values
    int i;
    for(i=0; i<3; i++) _force[i] = _contact[i] = 0;

    // Don't bother if it's not down
    if(_extension < 1)
	return;

    // The ground plane transformed to the local frame.
    float ground[4];
    s->planeGlobalToLocal(_global_ground, ground);
        
    // The velocity of the contact patch transformed to local coordinates.
    float glvel[3];
    s->velGlobalToLocal(_global_vel, glvel);

    // First off, make sure that the gear "tip" is below the ground.
    // If it's not, there's no force.
    float a = ground[3] - Math::dot3(_pos, ground);
    if(a > 0) {
	_wow = 0;
	_frac = 0;
	return;
    }

    // Now a is the distance from the tip to ground, so make b the
    // distance from the base to ground.  We can get the fraction
    // (0-1) of compression from a/(a-b). Note the minus sign -- stuff
    // above ground is negative.
    float tmp[3];
    Math::add3(_cmpr, _pos, tmp);
    float b = ground[3] - Math::dot3(tmp, ground);

    // Calculate the point of ground _contact.
    _frac = a/(a-b);
    if(b < 0) _frac = 1;
    for(i=0; i<3; i++)
	_contact[i] = _pos[i] + _frac*_cmpr[i];

    // Turn _cmpr into a unit vector and a magnitude
    float cmpr[3];
    float clen = Math::mag3(_cmpr);
    Math::mul3(1/clen, _cmpr, cmpr);

    // Now get the velocity of the point of contact
    float cv[3];
    body->pointVelocity(_contact, rot, cv);
    Math::add3(cv, v, cv);
    Math::sub3(cv, glvel, cv);

    // Finally, we can start adding up the forces.  First the spring
    // compression.   (note the clamping of _frac to 1):
    _frac = (_frac > 1) ? 1 : _frac;
    float fmag = _frac*clen*_spring;

    // Then the damping.  Use the only the velocity into the ground
    // (projection along "ground") projected along the compression
    // axis.  So Vdamp = ground*(ground dot cv) dot cmpr
    Math::mul3(Math::dot3(ground, cv), ground, tmp);
    float dv = Math::dot3(cmpr, tmp);
    float damp = _damp * dv;
    if(damp > fmag) damp = fmag; // can't pull the plane down!
    if(damp < -fmag) damp = -fmag; // sanity

    // The actual force applied is only the component perpendicular to
    // the ground.  Side forces come from velocity only.
    _wow = (fmag - damp) * -Math::dot3(cmpr, ground);
    Math::mul3(-_wow, ground, _force);

    // Castering gear feel no force in the ground plane
    if(_castering)
	return;

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
	// Correct for a rotation
        float srot = Math::sin(_rot);
        float crot = Math::cos(_rot);
        float tx = steer[0];
        float ty = steer[1];
        steer[0] =  crot*tx + srot*ty;
        steer[1] = -srot*tx + crot*ty;

        tx = skid[0];
        ty = skid[1];
        skid[0] =  crot*tx + srot*ty;
        skid[1] = -srot*tx + crot*ty;
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
    // How slow is stopped?  10 cm/second?
    const float STOP = 0.1f;
    const float iSTOP = 1.0f/STOP;
    v = Math::abs(v);
    if(v < STOP) return v*iSTOP * wgt * _sfric;
    else         return wgt * _dfric;
}

}; // namespace yasim

