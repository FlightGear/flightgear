#include "Math.hpp"
#include "BodyEnvironment.hpp"
#include "Ground.hpp"
#include "RigidBody.hpp"

#include "Hook.hpp"
namespace yasim {

static const float YASIM_PI2 = 3.14159265358979323846/2;

Hook::Hook()
{
    int i;
    for(i=0; i<3; i++)
	_pos[i] = _force[i] = 0;
    for(i=0; i<2; i++)
	_global_ground[i] = 0;
    _global_ground[2] = 1;
    _global_ground[3] = -1e5;
    _length = 0.0;
    _down_ang = 0.0;
    _up_ang = 0.0;
    _extension = 0.0;
    _frac = 0.0;
    _has_wire = false;
}

void Hook::setPosition(float* position)
{
    int i;
    for(i=0; i<3; i++) _pos[i] = position[i];
}

void Hook::setLength(float length)
{
    _length = length;
}

void Hook::setDownAngle(float ang)
{
    _down_ang = ang;
}

void Hook::setUpAngle(float ang)
{
    _up_ang = ang;
}

void Hook::setExtension(float extension)
{
    _extension = extension;
}

void Hook::setGlobalGround(double *global_ground)
{
    int i;
    for(i=0; i<4; i++) _global_ground[i] = global_ground[i];
}

void Hook::getPosition(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _pos[i];
}

float Hook::getHookPos(int i)
{
    return _pos[i];
}

float Hook::getLength(void)
{
    return _length;
}

float Hook::getDownAngle(void)
{
    return _down_ang;
}

float Hook::getUpAngle(void)
{
    return _up_ang;
}

float Hook::getAngle(void)
{
    return _ang;
}

float Hook::getExtension(void)
{
    return _extension;
}

void Hook::getForce(float* force, float* off)
{
    Math::set3(_force, force);
    Math::set3(_pos, off);
}

float Hook::getCompressFraction()
{
    return _frac;
}

void Hook::getTipPosition(float* out)
{
    // The hook tip in local coordinates.
    _ang = _frac*(_down_ang - _up_ang) + _up_ang;
    float pos_tip[3] = { _length*Math::cos(_ang), 0, _length*Math::sin(_ang) };
    Math::sub3(_pos, pos_tip, out);
}

void Hook::getTipGlobalPosition(State* s, double* out)
{
    // The hook tip in local coordinates.
    float pos_tip[3];
    getTipPosition(pos_tip);
    // The hook tip in global coordinates.
    s->posLocalToGlobal(pos_tip, out);
}

void Hook::calcForce(Ground* g_cb, RigidBody* body, State* s, float* lv, float* lrot)
{
    // Init the return values
    int i;
    for(i=0; i<3; i++) _force[i] = 0;

    // Don't bother if it's fully retracted
    if(_extension <= 0)
	return;

    // For the first guess, the position fraction is equal to the
    // extension value.
    _frac = _extension;

    // The ground plane transformed to the local frame.
    float ground[4];
    s->planeGlobalToLocal(_global_ground, ground);

    // The hook tip in local coordinates.
    float ltip[3];
    getTipPosition(ltip);


    // Correct the extension value for no intersection.
    
    // Check if the tip will intersect the ground or not. That is, compute
    // the distance of the tip to the ground plane.
    float tipdist = ground[3] - Math::dot3(ltip, ground);
    if(0 <= tipdist) {
        _frac = _extension;
    } else {
        // Compute the distance of the hooks mount point from the ground plane.
        float mountdist = ground[3] - Math::dot3(_pos, ground);

        // Compute the distance of the hooks mount point from the ground plane
        // in the x-z plane. It holds:
        //  mountdist = mountdist_xz*cos(angle(normal_yz, e_z))
        // thus
        float mountdist_xz = _length;
        if (ground[2] != 0) {
            float nrm_yz = Math::sqrt(ground[1]*ground[1]+ground[2]*ground[2]);
            mountdist_xz = -mountdist*nrm_yz/ground[2];
        }

        if (mountdist_xz < _length) {
            float ang = Math::asin(mountdist_xz/_length)
              + Math::atan2(ground[2], ground[0]) + YASIM_PI2;
            _frac = (ang - _up_ang)/(_down_ang - _up_ang);
        } else {
            _frac = _extension;
        }
    }

    double hook_area[4][3];
    // The hook mount in global coordinates.
    s->posLocalToGlobal(_pos, hook_area[1]);

    // Recompute the hook tip in global coordinates.
    getTipGlobalPosition(s, hook_area[0]);

    // The old positions.
    hook_area[2][0] = _old_mount[0];
    hook_area[2][1] = _old_mount[1];
    hook_area[2][2] = _old_mount[2];
    hook_area[3][0] = _old_tip[0];
    hook_area[3][1] = _old_tip[1];
    hook_area[3][2] = _old_tip[2];


    // Check if we caught a wire.
    // Returns true if we caught one.
    if (!_has_wire && g_cb->caughtWire(hook_area))
        _has_wire = true;


    // save actual position as old position ...
    _old_mount[0] = hook_area[1][0];
    _old_mount[1] = hook_area[1][1];
    _old_mount[2] = hook_area[1][2];
    _old_tip[0] = hook_area[0][0];
    _old_tip[1] = hook_area[0][1];
    _old_tip[2] = hook_area[0][2];

    if (!_has_wire)
        return;

    // Get the wire endpoints and their velocities wrt the earth.
    double dpos[2][3];
    float wire_vel[2][3];
    g_cb->getWire(dpos, wire_vel);

    // Transform those to the local coordinate system
    float wire_lpos[2][3];
    s->posGlobalToLocal(dpos[0], wire_lpos[0]);
    s->posGlobalToLocal(dpos[1], wire_lpos[1]);
    s->velGlobalToLocal(wire_vel[0], wire_vel[0]);
    s->velGlobalToLocal(wire_vel[1], wire_vel[1]);

    // Compute the velocity of the hooks mount point in the local frame.
    float mount_vel[3];
    body->pointVelocity(_pos, lrot, mount_vel);
    Math::add3(lv, mount_vel, mount_vel);

    // The velocity of the hook mount point wrt the earth in
    // the local frame.
    float v_wrt_we[2][3];
    Math::sub3(mount_vel, wire_vel[0], v_wrt_we[0]);
    Math::sub3(mount_vel, wire_vel[1], v_wrt_we[1]);

    float f[2][3];
    // The vector from the wire ends to the hook mount point.
    Math::sub3(_pos, wire_lpos[0], f[0]);
    Math::sub3(_pos, wire_lpos[1], f[1]);

    // We only need the direction.
    float mf0 = Math::mag3(f[0]);
    float mf1 = Math::mag3(f[1]);
    Math::mul3(1.0/mf0, f[0], f[0]);
    Math::mul3(1.0/mf1, f[1], f[1]);

    // The velocity of the wire wrt the wire ends at the wire
    // mount points.
    float v0 = Math::dot3(v_wrt_we[0], f[0]);
    float v1 = Math::dot3(v_wrt_we[1], f[1]);

    // We assume that the wire slips through the hook. So the velocity
    // will be equal at both sides. So take the mean of both.
    float v = 0.5*(v0+v1);

    // Release wire when we reach zero velocity.
    if (v <= 0.0) {
      _has_wire = false;
      g_cb->releaseWire();
      return;
    }

    // The trick is to multiply with the current mass of the aircraft.
    // That way we control the acceleration and not the force. This is
    // the implicit calibration of the wires for aircrafts of
    // different mass.
    float mass = body->getTotalMass();
  
    // The local force is the vector sum of the force on each wire.
    // The force is computed with some constant tension on the wires
    // (80000N) plus a velocity dependent component.
    Math::add3(f[0], f[1], _force);
    Math::mul3(-mass*( 1.0 + ((mf0+mf1)/70) + 0.2*v ), _force, _force);
  
    // Now in the body coordinate system, eliminate the Y coord part
    // of the hook force. Physically this means that the wire will just
    // slip through the hook instead of applying a side force.
    _force[1] = 0.0;
}

}; // namespace yasim

