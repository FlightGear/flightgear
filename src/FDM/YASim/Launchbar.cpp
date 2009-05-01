#include "Math.hpp"
#include "BodyEnvironment.hpp"
#include "Ground.hpp"
#include "RigidBody.hpp"
#include "Launchbar.hpp"

namespace yasim {

  static const float YASIM_PI2 = 3.14159265358979323846f/2;
  static const float YASIM_PI = 3.14159265358979323846f;
  static const float RAD2DEG = 180/YASIM_PI;

Launchbar::Launchbar()
{
    int i;
    for(i=0; i<3; i++)
      _launchbar_mount[i] = _holdback_mount[i] = _launchbar_force[i]
              = _holdback_force[i] = 0;
    for(i=0; i<2; i++)
	_global_ground[i] = 0;
    _global_ground[2] = 1;
    _global_ground[3] = -1e5;
    _length = 0.0;
    _holdback_length = 2.0;
    _down_ang = 0.0;
    _up_ang = 0.0;
    _ang = 0.0;
    _extension = 0.0;
    _frac = _h_frac =0.0;
    _launch_cmd = false;
    _pos_on_cat = 0.0;
    _state = Unmounted;
    _strop = false;
    _acceleration = 0.25;
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

  void Launchbar::setHoldbackLength(float length)
  {
    _holdback_length = length;
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

void Launchbar::setAcceleration(float acceleration)
{
    _acceleration = acceleration;
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

  float Launchbar::getLaunchbarPos(int i)
  {
    return _launchbar_mount[i];
  }

void Launchbar::getHoldbackMount(float* out)
{
    int i;
    for(i=0; i<3; i++) out[i] = _holdback_mount[i];
}

  float Launchbar::getHoldbackPos(int j)
  {
    return _holdback_mount[j];
  }

  float Launchbar::getHoldbackLength(void)
  {
    return _holdback_length;
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

  float Launchbar::getAngle(void)
  {
    return _ang;
  }

  float Launchbar::getHoldbackAngle(void)
  {
    return _h_ang;
  }
float Launchbar::getExtension(void)
{
    return _extension;
}

  void Launchbar::getForce(float* force1, float* off1,
    float* force2, float* off2)
  {
    Math::set3(_launchbar_force, force1);
    Math::set3(_launchbar_mount, off1);
    Math::set3(_holdback_force, force2);
    Math::set3(_holdback_mount, off2);
  }

  const char* Launchbar::getState(void)
  {
    switch (_state) {
    case Arrested:
      return "Engaged";
    case Launch:
      return "Launching";
    case Completed:
      return "Completed";
    default:
      return "Disengaged"; 
    }
  }   

  bool Launchbar::getStrop(void)
{
    return _strop;
}

float Launchbar::getCompressFraction()
{
    return _frac;
}

  float Launchbar::getHoldbackCompressFraction()
  {
    return _h_frac;
  }

void Launchbar::getTipPosition(float* out)
{
    // The launchbar tip in local coordinates.

    _ang = _frac*(_down_ang - _up_ang ) + _up_ang ;
    float ptip[3] = { _length*Math::cos(_ang), 0, -_length*Math::sin(_ang) };
    Math::add3(_launchbar_mount, ptip, out);
  }

  float Launchbar::getTipPos(int i)
  {
    float pos_tip[3];
    getTipPosition(pos_tip);
    return pos_tip[i];
  }    

  void Launchbar::getHoldbackTipPosition(float* out)
  {
    // The holdback tip in local coordinates.
    _h_ang = _h_frac*(_down_ang - _up_ang) + _up_ang;
    float htip[3] = { -_length*Math::cos(_h_ang), 0, -_length*Math::sin(_h_ang) };
    Math::add3(_holdback_mount, htip, out);
  }

  float Launchbar::getHoldbackTipPos(int i)
  {
    float pos_tip[3];
    getHoldbackTipPosition(pos_tip);
    return pos_tip[i];
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
    Math::mul3(1.0f-perc, lendvels[1], tmp);
    Math::add3(tmp, lvel, lvel);
}

void Launchbar::calcForce(Ground *g_cb, RigidBody* body, State* s, float* lv, float* lrot)
{
    // Init the return values
    int i;
    for(i=0; i<3; i++) _launchbar_force[i] = 0;
    for(i=0; i<3; i++) _holdback_force[i] = 0;

    if (_state != Unmounted)
      _extension = 1;

    // Don't bother if it's fully retracted
    if(_extension <= 0)
	return;

    // For the first guess, the position fraction is equal to the
    // extension value.
    _frac = _h_frac = _extension;

    // The ground plane transformed to the local frame.
    float ground[4];
    s->planeGlobalToLocal(_global_ground, ground);

    // The launchbar tip in local coordinates.
    float ltip[3];
    getTipPosition(ltip);

        // Correct the extension value for no intersection.

    // Check if the tip will intersect the ground or not. That is, compute
    // the distance of the tip to the ground plane.
    float tipdist = ground[3] - Math::dot3(ltip, ground);
    if(0 <= tipdist) {
      _frac = _extension;
    } else {
      // Compute the distance of the launchbar mount point from the
      // ground plane.
      float mountdist = ground[3] - Math::dot3(_launchbar_mount, ground);

      // Compute the distance of the launchbar mount point from the
      // ground plane in the x-z plane. It holds:
      // mountdist = mountdist_xz*cos(angle(normal_yz, e_z))
      // thus
      float mountdist_xz = _length;
      if (ground[2] != 0) {
        float nrm_yz = Math::sqrt(ground[1]*ground[1]+ground[2]*ground[2]);
        mountdist_xz = -mountdist*nrm_yz/ground[2];
      }

      if (mountdist_xz < _length) { 
        // the launchbar points forward, so we need to change the signs here
        float ang = -Math::asin(mountdist_xz/_length)
          + Math::atan2(ground[2], ground[0]) + YASIM_PI2;
        ang = -ang;
            _frac = (ang - _up_ang)/(_down_ang - _up_ang);
      } else {
            _frac = _extension;
    }
    }

    // Now do it again for the holdback

    // The holdback tip in local coordinates.
    float htip[3];
    getHoldbackTipPosition(htip);

    // Check if the tip will intersect the ground or not. That is, compute
    // the distance of the tip to the ground plane.
    float h_tipdist = ground[3] - Math::dot3(htip, ground);
    if (0 <= h_tipdist) {
      _h_frac = _extension;
    } else {
      // Compute the distance of the holdback mount point from the ground
      // plane.
      float h_mountdist = ground[3] - Math::dot3(_holdback_mount, ground);

      // Compute the distance of the holdback mount point from the ground
      // plane in the x-z plane. It holds:
      //  mountdist = mountdist_xz*cos(angle(normal_yz, e_z))
      // thus
      float h_mountdist_xz = _holdback_length;
      if (ground[2] != 0) {
        float nrm_yz = Math::sqrt(ground[1]*ground[1]+ground[2]*ground[2]);
        h_mountdist_xz = -h_mountdist*nrm_yz/ground[2];
      }

      if (h_mountdist_xz < _holdback_length) {
        float h_ang = Math::asin(h_mountdist_xz/_holdback_length)
          + Math::atan2(ground[2], ground[0]) + YASIM_PI2;
        _h_frac = (h_ang - _up_ang)/(_down_ang - _up_ang);
      } else {
        _h_frac = _extension;
      }
    }

    float llb_mount[3];
    getTipPosition(llb_mount);

    // The launchbar tip in global coordinates.
    double launchbar_pos[3];
    s->posLocalToGlobal(llb_mount, launchbar_pos);

    double end[2][3]; float vel[2][3];
    float dist = g_cb->getCatapult(launchbar_pos, end, vel);
    // Work around a problem of flightgear returning totally screwed up
    // scenery when switching views.
    if (1e3 < dist)
        return;

    // Compute the positions of the catapult start and endpoints in the
    // local coordinate system
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
    Math::mul3(1.0f/lblen, llbdir, llbdir);

    // Check if we are near enough to the cat.
    if (_state == Unmounted && dist < 0.6) {
      // coarse approximation for the velocity of the launchbar.
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

      // don't let the calculation go -ve here
      if (_holdback_length*_holdback_length - hbdist*hbdist < 0)
        return;
      float offset = -Math::sqrt(_holdback_length*_holdback_length
        - hbdist*hbdist);
      _pos_on_cat = getPercentPosOnCat(_holdback_mount, offset, lend);

        // We cannot arrest if we are not at the start of the cat.
      if (_pos_on_cat < 0.0 || 0.4 < _pos_on_cat)
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
      Math::mul3(2.0, llbdir, _launchbar_force);

        // If the distance from the holdback mount at the aircraft to the
        // holdback mount on the cat is larger than the holdback length itself,
        // the holdback applies a force to the gear.
        if (_holdback_length < hldbklen) {
        // coarse approximation for the velocity of the holdback mount
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
        Math::mul3(10.f*(hldbklen - _holdback_length), lhldbkdir,
          _holdback_force);

            // The damping force here ...
            Math::mul3(2e0, lvhldbk, tmp);
        Math::sub3(_holdback_force, tmp, _holdback_force);
        }

      if (_launch_cmd) {
            _state = Launch;
        _strop = false;
      }
    }

    if (_state == Launch) {
        // Now apply a constant tension from the catapult over the launchbar.
        // We modify the max accleration 100 m/s^2 by the normalised input
        //SG_LOG(SG_FLIGHT, SG_ALERT, "acceleration " << 100 * _acceleration );
        Math::mul3(100 * _acceleration, llbdir, _launchbar_force);

      if (1.0 < dist) {
        _state = Completed;
      }
    }

    if (_state == Completed) {
      // Wait until the strop has cleared the deck
      // This is a temporary fix until we come up with something better

      if (_frac > 0.8) {
            _state = Unmounted;
        _strop = true;
      }
    }

    // Scale by the mass. That keeps the stiffness in reasonable bounds.
    float mass = body->getTotalMass();
    Math::mul3(mass, _launchbar_force, _launchbar_force);
    Math::mul3(mass, _holdback_force, _holdback_force);
}

}; // namespace yasim

