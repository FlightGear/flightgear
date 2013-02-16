// Copyright (C) 2009 - 2012  Mathias Froehlich
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef AIPhysics_hxx
#define AIPhysics_hxx

#include <simgear/math/SGMath.hxx>
#include "AISubsystem.hxx"

namespace fgai {

class AIPhysics : public AISubsystem {
public:
    /// Initial conditions need to be set at creation time.
    /// Just setting the position underway will result in unphysical motion.
    AIPhysics(const AIPhysics& physics);
    AIPhysics(const SGLocationd& location, const SGVec3d& linearBodyVelocity = SGVec3d::zeros(),
                 const SGVec3d& angularBodyVelocity = SGVec3d::zeros());
    virtual ~AIPhysics();

    /// The default is unaccelerated movement
    virtual void update(AIObject& object, const SGTimeStamp& dt);

    /// The current state
    const SGLocationd& getLocation() const
    { return _location; }
    const SGVec3d& getPosition() const
    { return _location.getPosition(); }
    const SGQuatd& getOrientation() const
    { return _location.getOrientation(); }
    const SGGeod& getGeodPosition() const
    { return _geodPosition; }
    const SGQuatd& getHorizontalLocalOrientation() const
    { return _horizontalLocalOrientation; }
    /// The orientation of the body wrt the geodetic ned coordinate system
    SGQuatd getGeodOrientation() const
    { return inverse(_horizontalLocalOrientation)*_location.getOrientation(); }
    const SGVec3d& getLinearBodyVelocity() const
    { return _linearBodyVelocity; }
    const SGVec3d& getAngularBodyVelocity() const
    { return _angularBodyVelocity; }
    /// The velocity in global cartesian coordinates
    SGVec3d getLinearVelocity() const
    { return _location.getOrientation().backTransform(_linearBodyVelocity); }
    /// The velocity in the north east down coordinate system
    /// Note that this gets undefined at the poles.
    SGVec3d getGeodVelocity() const
    { return getGeodOrientation().backTransform(getLinearBodyVelocity()); }
    double getDownVelocity() const
    { return getGeodVelocity()[2]; }
    SGVec2d getHorizontalVelocity() const
    { SGVec3d v = getGeodVelocity(); return SGVec2d(v[0], v[1]); }

protected:
    /// The below methods change the position and velocity of the vehicle
    /// in a way that is consistent in that it matches position, velocity
    /// values to keep the velocity a numerical derivative of the position.

    /// Given the accelerations at the current simulation time mSimTime,
    /// update the position and velocity to mSimTime + dt.
    /// This is the primary advance mode for physically simulated motion.
    /// Compute the forces on the single body, compute the accelerations
    /// from the forces by newtons law and accelerate by this amount.
    void advanceByBodyAcceleration(const double& dt,
                                   const SGVec3d& linearAcceleration,
                                   const SGVec3d& angularAcceleration);

    /// Given the velocities at the next simulation time mSimTime,
    /// update the position and velocity to mSimTime + dt.
    void advanceByBodyVelocity(const double& dt,
                               const SGVec3d& linearVelocity,
                               const SGVec3d& angularVelocity);

    /// Given the desired position and orientation, a pair of velocities
    /// is computed to reach that position and orientation in the
    /// next advance step. This one advance step latency is important
    /// for other participants to correctly extrapolate the position
    /// and orientation based on the velocities.
    /// Note that this only works when the next update is called with
    /// the same time increment.
    void advanceToLocation(const double& dt, const SGLocationd& location);

private:
    AIPhysics& operator=(const AIPhysics& physics);

    SGLocationd _location;
    SGVec3d _linearBodyVelocity;
    SGVec3d _angularBodyVelocity;
    SGGeod _geodPosition;
    SGQuatd _horizontalLocalOrientation;
};

} // namespace fgai

#endif
