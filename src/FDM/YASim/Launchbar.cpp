#include "Math.hpp"
#include "BodyEnvironment.hpp"
#include "Ground.hpp"
#include "RigidBody.hpp"

#include "Launchbar.hpp"

#include <iostream>
using namespace std;
namespace yasim {

Launchbar::Launchbar()
{
    int i;
    for(i=0; i<3; i++)
	_launchbar_mount[i] = _holdback_mount[i] = _force[i] = 0;
    for(i=0; i<2; i++)
	_global_ground[i] = 0;
    _global_ground[2] = 1;
    _global_ground[3] = -1e5;
    _length = 0.0;
    _holdback_length = 2.0;
    _down_ang = 0.0;
    _up_ang = 0.0;
    _extension = 0.0;
    _frac = 0.0;
    _launch_cmd = false;
    _pos_on_cat = 0.0;
    _state = Unmounted;
}

void Launchbar::setLaunchbarMount(float* position)
{
    int i;
    for(i=0; i<3; i++) _launchbar_mount[i] = position[i];
}

void Launchbar::setHoldbackMount(float* position)
{
    int i;
    for(i=0; i<3; i++) _holdback_mount[i] = position[i];
}

void Launchbar::setLength(float length)
{
    _length = length;
}

void Launchbar::setDownAngle(float ang)
{
    _down_ang = ang;
}

void Launchbar::setUpAngle(float ang)
{
    _up_ang = ang;
}

void Launchbar::setExtension(float extension)
{
    _extension = extension;
}

void Launchbar::setLaunchCmd(bool cmd)
{
    _launch_cmd = cmd;
}

void Launchbar::setGlobalGround(double *global_ground)
{
    int i;
    for(i=0; i<4; i++) _global_ground[i] = global_ground[i];
}

void Launchbar::getLaunchbarMount(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _launchbar_mount[i];
}

void Launchbar::getHoldbackMount(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _holdback_mount[i];
}

float Launchbar::getLength(void)
{
    return _length;
}

float Launchbar::getDownAngle(void)
{
    return _down_ang;
}

float Launchbar::getUpAngle(void)
{
    return _up_ang;
}

float Launchbar::getExtension(void)
{
    return _extension;
}

void Launchbar::getForce(float* force, float* off)
{
    Math::set3(_force, force);
    Math::set3(_launchbar_mount, off);
}

float Launchbar::getCompressFraction()
{
    return _frac;
}

void Launchbar::getTipPosition(float* out)
{
    // The launchbar tip in local coordinates.
    float ang = _frac*(_down_ang - _up_ang) + _up_ang;
    float pos_tip[3] = { _length*Math::cos(ang), 0.0,-_length*Math::sin(ang) };
    Math::add3(_launchbar_mount, pos_tip, out);
}

void Launchbar::getTipGlobalPosition(State* s, double* out)
{
    // The launchbar tip in local coordinates.
    float pos_tip[3];
    getTipPosition(pos_tip);
    // The launchbar tip in global coordinates.
    s->posLocalToGlobal(pos_tip, out);
}

float Launchbar::getPercentPosOnCat(float* lpos, float off, float lends[2][3])
{
    // Compute the forward direction of the cat.
    float lforward[3];
    Math::sub3(lends[1], lends[0], lforward);
    float ltopos[3];
    Math::sub3(lpos, lends[0], ltopos);
    float fwlen = Math::mag3(lforward);
    
    return (Math::dot3(ltopos, lforward)/fwlen + off)/fwlen;
}

void Launchbar::getPosOnCat(float perc, float* lpos, float* lvel,
                            float lends[2][3], float lendvels[2][3])
{
    if (perc < 0.0)
        perc = 0.0;
    if (1.0 < perc)
        perc = 1.0;

    // Compute the forward direction of the cat.
    float lforward[3];
    Math::sub3(lends[1], lends[0], lforward);
    Math::mul3(perc, lforward, lpos);
    Math::add3(lends[0], lpos, lpos);
    
    float tmp[3];
    Math::mul3(perc, lendvels[0], lvel);
    Math::mul3(1.0-perc, lendvels[1], tmp);
    Math::add3(tmp, lvel, lvel);
}

void Launchbar::calcForce(Ground *g_cb, RigidBody* body, State* s, float* lv, float* lrot)
{
    // Init the return values
    int i;
    for(i=0; i<3; i++) _force[i] = 0;

    if (_state != Unmounted)
      _extension = 1;

    // Don't bother if it's fully retracted
    if(_extension <= 0)
	return;

    if (_extension < _frac)
        _frac = _extension;

    // The launchbar tip in global coordinates.
    double launchbar_pos[3];
    getTipGlobalPosition(s, launchbar_pos);

    // If the launchbars tip is less extended than it could be.
    if(_frac < _extension) {
        // Correct the extension value for no intersection.
        // Compute the distance of the mount point from the ground plane.
        double a = - _global_ground[3] + launchbar_pos[0]*_global_ground[0]
          + launchbar_pos[1]*_global_ground[1]
          + launchbar_pos[2]*_global_ground[2];
        if(a < _length) {
            float ang = Math::asin(a/_length);
            _frac = (ang - _up_ang)/(_down_ang - _up_ang);
        } else
            // FIXME: this will jump 
            _frac = _extension;
    }

    // Recompute the launchbar tip.
    float llb_mount[3];
    getTipPosition(llb_mount);
    // The launchbar tip in global coordinates.
    s->posLocalToGlobal(llb_mount, launchbar_pos);

    double end[2][3]; float vel[2][3];
    float dist = g_cb->getCatapult(launchbar_pos, end, vel);
    // Work around a problem of flightgear returning totally screwed up
    // scenery when switching views.
    if (1e3 < dist)
        return;

    // Compute the positions of the catapult start and endpoints in the
    // local coordiante system
    float lend[2][3];
    s->posGlobalToLocal(end[0], lend[0]);
    s->posGlobalToLocal(end[1], lend[1]);
    
    // Transform the velocities of the endpoints to the
    // local coordinate sytem.
    float lvel[2][3];
    s->velGlobalToLocal(vel[0], lvel[0]);
    s->velGlobalToLocal(vel[1], lvel[1]);

    // Compute the position of the launchbar tip relative to the cat.
    float tip_pos_on_cat = getPercentPosOnCat(llb_mount, 0.0, lend);
    float llbtip[3], lvlbtip[3];
    getPosOnCat(tip_pos_on_cat, llbtip, lvlbtip, lend, lvel);

    // Compute the direction from the launchbar mount at the gear
    // to the lauchbar mount on the cat.
    float llbdir[3];
    Math::sub3(llbtip, _launchbar_mount, llbdir);
    float lblen = Math::mag3(llbdir);
    Math::mul3(1.0/lblen, llbdir, llbdir);

    // Check if we are near enough to the cat.
    if (_state == Unmounted && dist < 0.5) {
        // croase approximation for the velocity of the launchbar.
        // Might be sufficient because arresting at the cat makes only
        // sense when the aircraft does not rotate much.
        float lv_mount[3];
        float tmp[3];
        float lrot[3], lv[3];
        Math::vmul33(s->orient, s->rot, lrot);
        Math::vmul33(s->orient, s->v, lv);
        body->pointVelocity(llb_mount, lrot, tmp);
        Math::sub3(tmp, lvlbtip, lv_mount);
        Math::add3(lv, lv_mount, lv_mount);

        // We cannot arrest at the cat if we move too fast wrt the cat.
        if (0.2 < Math::mag3(lv_mount))
            return;

        // Compute the position of the holdback mount relative to the cat.
        double dd[2][3]; float fd[2][3]; double ghldbkpos[3];
        s->posLocalToGlobal(_holdback_mount, ghldbkpos);
        float hbdist = g_cb->getCatapult(ghldbkpos, dd, fd);
        float offset = -Math::sqrt(_holdback_length*_holdback_length - hbdist*hbdist);
        _pos_on_cat = getPercentPosOnCat(_holdback_mount, offset, lend);


        // We cannot arrest if we are not at the start of the cat.
        if (_pos_on_cat < 0.0 || 0.2 < _pos_on_cat)
            return;

        // Now we are arrested at the cat.
        // The force is applied at the next step.
        _state = Arrested;
        return;
    }

    // Get the actual distance from the holdback to its mountpoint
    // on the cat. If it is longer than the holdback apply a force.
    float lhldbk_cmount[3]; float lvhldbk_cmount[3];
    getPosOnCat(_pos_on_cat, lhldbk_cmount, lvhldbk_cmount, lend, lvel);
    // Compute the direction of holdback.
    float lhldbkdir[3];
    Math::sub3(lhldbk_cmount, _holdback_mount, lhldbkdir);
    float hldbklen = Math::mag3(lhldbkdir);
    Math::mul3(1/hldbklen, lhldbkdir, lhldbkdir);
    
    if (_state == Arrested) {
        // Now apply a constant tension from the catapult over the launchbar.
        Math::mul3(2.0, llbdir, _force);

        // If the distance from the holdback mount at the aircraft to the
        // holdback mount on the cat is larger than the holdback length itself,
        // the holdback applies a force to the gear.
        if (_holdback_length < hldbklen) {
            // croase approximation for the velocity of the holdback mount
            // at the gear.
            // Might be sufficient because arresting at the cat makes only
            // sense when the aircraft does not rotate much.
            float lvhldbk_gmount[3];
            float lrot[3], lv[3];
            Math::vmul33(s->orient, s->rot, lrot);
            Math::vmul33(s->orient, s->v, lv);
            body->pointVelocity(_holdback_mount, lrot, lvhldbk_gmount);
            Math::add3(lv, lvhldbk_gmount, lvhldbk_gmount);

            // The velocity of the holdback mount at the gear wrt the
            // holdback mount at the cat.
            float lvhldbk[3];
            Math::sub3(lvhldbk_gmount, lvhldbk_cmount, lvhldbk);

            // The spring force the holdback will apply to the gear
            float tmp[3];
            Math::mul3(1e1*(hldbklen - _holdback_length), lhldbkdir, tmp);
            Math::add3(tmp, _force, _force);

            // The damping force here ...
            Math::mul3(2e0, lvhldbk, tmp);
            Math::sub3(_force, tmp, _force);
        }

        if (_launch_cmd)
            _state = Launch;
    }

    if (_state == Launch) {
        // Now apply a constant tension from the catapult over the launchbar.
        Math::mul3(25.0, llbdir, _force);

        if (1.0 < dist)
            _state = Unmounted;
    }

    // Scale by the mass. That keeps the stiffness in reasonable bounds.
    float mass = body->getTotalMass();
    Math::mul3(mass, _force, _force);
}

}; // namespace yasim

