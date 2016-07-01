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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>

#include "AIObject.hxx"
#include "AIManager.hxx"

#include "HLAMPAircraft.hxx"
#include "HLAMPAircraftClass.hxx"
#include "AIPhysics.hxx"

namespace fgai {

#if 0
class AIVehiclePhysics : public AIPhysics {
public:
    AIVehiclePhysics(const SGLocationd& location, const SGVec3d& linearBodyVelocity = SGVec3d::zeros(),
                     const SGVec3d& angularBodyVelocity = SGVec3d::zeros()) :
        AIPhysics(location, linearBodyVelocity, angularBodyVelocity)
    {
        setMass(1);
        setInertia(1, 1, 1);
    }
    virtual ~AIVehiclePhysics()
    { }

    double getMass() const
    { return _mass; }
    void setMass(double mass)
    { _mass = mass; }

    void setInertia(double ixx, double iyy, double izz, double ixy = 0, double ixz = 0, double iyz = 0)
    {
        _inertia = SGMatrixd(ixx, ixy, ixz, 0,
                             ixy, iyy, iyz, 0,
                             ixz, iyz, izz, 0,
                             0,   0,   0, 1);
        invert(_inertiaInverse, _inertia);
    }

protected:
    void advanceByBodyForce(const double& dt,
                            const SGVec3d& force,
                            const SGVec3d& torque)
    {
        SGVec3d linearVelocity = getLinearBodyVelocity();
        SGVec3d angularVelocity = getAngularBodyVelocity();

        SGVec3d linearAcceleration = (1/_mass)*(force - cross(angularVelocity, linearVelocity));
        SGVec3d angularAcceleration = _inertiaInverse.xformVec(torque - cross(angularVelocity, _inertia.xformVec(angularVelocity)));

        advanceByBodyAcceleration(dt, linearAcceleration, angularAcceleration);
    }

    SGVec3d getGravityAcceleration() const
    {
        return SGQuatd::fromLonLat(getGeodPosition()).backTransform(SGVec3d(0, 0, 9.81));
    }

private:
    // unsimulated motion, hide this for this kind of class here
    using AIPhysics::advanceByBodyAcceleration;
    using AIPhysics::advanceByBodyVelocity;
    using AIPhysics::advanceToCartPosition;

    double _mass;
    // FIXME this is a symmetric 3x3 matrix ...
    SGMatrixd _inertia;
    SGMatrixd _inertiaInverse;
};

// FIXME Totally unfinished simple aerodynamics model for an ai aircraft
// also just here for a sketch of an idea
class AIAircraftPhysics : public AIVehiclePhysics {
public:
    AIAircraftPhysics(const SGLocationd& location, const SGVec3d& linearBodyVelocity = SGVec3d::zeros(),
                      const SGVec3d& angularBodyVelocity = SGVec3d::zeros()) :
        AIVehiclePhysics(location, linearBodyVelocity, angularBodyVelocity)
    { }
    virtual ~AIAircraftPhysics()
    { }

    double getElevatorPosition() const
    { return _elevatorPosition; }
    void setElevatorPosition(double elevatorPosition)
    { _elevatorPosition = elevatorPosition; }

    double getAileronPosition() const
    { return _aileronPosition; }
    void setAileronPosition(double aileronPosition)
    { _aileronPosition = aileronPosition; }

    double getRudderPosition() const
    { return _rudderPosition; }
    void setRudderPosition(double rudderPosition)
    { _rudderPosition = rudderPosition; }

    // double getFlapsPosition() const
    // { return _flapsPosition; }
    // void setFlapsPosition(double flapsPosition)
    // { _flapsPosition = flapsPosition; }

    double getThrust() const
    { return _thrust; }
    void setThrust(double thrust)
    { _thrust = thrust; }

    virtual void update(AIObject& object, const SGTimeStamp& dt)
    {
        // const AIEnvironment& environment = object.getEnvironment();
        const AIEnvironment environment;

        /// Compute the forces on the aircraft. This is a very simple fdm.

        // The velocity of the aircraft wrt the surrounding fluid
        SGVec3d windVelocity = getLinearBodyVelocity();
        windVelocity -= getOrientation().transform(environment.getWindVelocity());

        // The true airspeed of the bird
        double airSpeed = norm(windVelocity);
        // simple density with(out FIXME) altitude
        double density = environment.getDensity();
        // The dynaimc pressure - most important value in aerodynamics
        double dynamicPressure = 0.5*density*airSpeed*airSpeed;

        // The angle of attack and sideslip angle
        double alpha = 0, beta = 0;
        if (1e-3 < SGMiscd::max(fabs(windVelocity[0]), fabs(windVelocity[2])))
            alpha = atan2(windVelocity[2], windVelocity[0]);
        double uw = sqrt(windVelocity[0]*windVelocity[0] + windVelocity[2]*windVelocity[2]);
        if (1e-3 < SGMiscd::max(fabs(windVelocity[1]), fabs(uw)))
            beta = atan2(windVelocity[1], uw);
        // Transform from the stability axis to body axis
        SGQuatd stabilityToBody = SGQuatd::fromEulerRad(beta, alpha, 0);

        // We assume a simple angular dependency for the
        // lift, drag and side force coefficients.

        // lift for alpha = 0
        double _Cl0 = 0.5;
        // lift at stall angle of attack
        double _ClStall = 2;
        // stall angle of attack
        double _alphaStall = 18;
        // Drag for alpha = 0
        double _Cd0 = 0.05;
        // Drag for alpha = 90
        double _Cd90 = 1.05;
        // Side force coefficient for maximum side angle
        double _Cs90 = 1.05;

        /// FIXME
        double _area = 1;
        SGVec3d _aerodynamicReferencePoint(0, 0, 0);
        SGVec3d _centerOfGravity(0, 0, 0);

        // So compute the lift drag and side force coefficient for the
        // current stream conditions.
        double Cl = _Cl0 + (_ClStall - _Cl0)*sin(SGMiscd::clip(90/_alphaStall*alpha, -0.5*SGMiscd::pi(), SGMiscd::pi()));
        double Cd = _Cd0 + (_Cd90 - _Cd0)*(0.5 - 0.5*cos(2*alpha));
        double Cs = _Cs90*sin(beta);

        // Forces in the stability axes
        double lift = Cl*dynamicPressure*_area;
        double drag = Cd*dynamicPressure*_area;
        double side = Cs*dynamicPressure*_area;

        // Torque in body axes
        double p = 0;
        double q = 0;
        double r = 0;

        // Compute the force in stability coordinates ...
        SGVec3d stabilityForce(-drag, side, -lift);
        // ... and torque in body coordinates
        SGVec3d torque(p, q, r);

        SGVec3d force = stabilityToBody.transform(stabilityForce);
        torque += cross(force, _aerodynamicReferencePoint);

        std::pair<SGVec3d, SGVec3d> velocity;
        for (_GearVector::iterator i = _gearVector.begin(); i != _gearVector.end(); ++i) {
            std::pair<SGVec3d, SGVec3d> torqueForce;
            torqueForce = i->force(getLocation(), velocity, object);
            torque += torqueForce.first;
            force += torqueForce.second;
        }

        // Transform the torque back to the center of gravity
        torque -= cross(force, _centerOfGravity);

        // Advance the equations of motion.
        advanceByBodyForce(dt.toSecs(), force, torque);
    }

    /// The normalized elevator position
    double _elevatorPosition;
    /// The normalized aileron position
    double _aileronPosition;
    /// The normalized rudder position
    double _rudderPosition;
    // /// The normalized flaps position
    // double _flapsPosition;
    /// Normalized thrust
    double _thrust;

    struct _Gear {
        SGVec3d _position;
        SGVec3d _direction;
        double _spring;
        double _damping;

        std::pair<SGVec3d, SGVec3d>
        force(const SGLocationd& location, const std::pair<SGVec3d, SGVec3d>& velocity, AIObject& object)
        {
            SGVec3d start = location.getAbsolutePosition(_position - _direction);
            SGVec3d end = location.getAbsolutePosition(_position);
            SGLineSegmentd lineSegment(start, end);

            SGVec3d point;
            SGVec3d normal;
            // if (!object.getGroundIntersection(point, normal, lineSegment))
            //     return std::pair<SGVec3d, SGVec3d>(SGVec3d(0, 0, 0), SGVec3d(0, 0, 0));

            // FIXME just now only the spring force ...

            // The compression length
            double compressLength = norm(point - start);
            SGVec3d springForce = -_spring*compressLength*_direction;

            SGVec3d dampingForce(0, 0, 0);

            SGVec3d force = springForce + dampingForce;
            SGVec3d torque(0, 0, 0);

            return std::pair<SGVec3d, SGVec3d>(torque, force);
        }
    };

    typedef std::vector<_Gear> _GearVector;
    _GearVector _gearVector;

    /// The total mass of the bird in kg. No fluel is burned.
    /// Some sensible inertia values are derived from the mass.
    // double _mass;
    /// The thrust to mass ratio which tells us someting about
    /// the possible accelerations.
    // double _thrustMassRatio;

    /// The stall speed
    // double _stallSpeed;
    // double _maximumSpeed;
    // // double _approachSpeed;
    // // double _takeoffSpeed;
    // // double _cruiseSpeed;
    /// The maximum density altitude this aircraft can fly
    // double _densityAltitudeLimit;

    /// statistical evaluation shows:
    /// wingarea = C*wingspan^2, C in [0.1, 0.4], say 0.2
    /// ixx = C*wingarea*mass, C in [1e-3, 1e-2]
    /// iyy = C*wingarea*mass, C in [1e-2, 0.02]
    /// izz = C*wingarea*mass, C in [1e-2, 0.02]
    /// Hmm, let's say, weight relates to wingarea?
    /// probably, since lift is linear dependent on wingarea

    /// So, define a 'size' in meters.
    /// the derive
    ///  wingspan = size
    ///  wingarea = 0.2*size*size
    ///  mass = C*wingarea
    ///  ixx = 0.005*wingarea*mass
    ///  iyy = 0.05*wingarea*mass
    ///  izz = 0.05*wingarea*mass


    /// an other idea:
    /// define a bird of some weight class. That means mass given.
    /// Then derive
    ///  i* ??? must be mass^2 accodring to the thoughts above
    /// Then do Cl, Cd, Cs.
    ///  according to approach speed at sea level with 5 deg aoa and 2,5 deg glideslope and 25 % thrust.
    ///  according to cruise altitude and cruise speed at 75% thrust compute this at altitude
    ///  interpolate between these two sets of C*'s based on altitude.
};
#endif

/// An automated lego aircraft, constant linear and angular speed
class AIOgel : public AIObject {
public:
    AIOgel(const SGGeod& geod) :
        _geod(geod),
        _radius(500),
        _turnaroundTime(3*60),
        _velocity(10)
    { }
    virtual ~AIOgel()
    { }

    virtual void init(AIManager& manager)
    {
        AIObject::init(manager);

        SGLocationd location(SGVec3d::fromGeod(_geod), SGQuatd::fromLonLat(_geod));
        SGVec3d linearVelocity(_velocity, 0, 0);
        SGVec3d angularVelocity(0, 0, SGMiscd::twopi()/_turnaroundTime);
        setPhysics(new AIPhysics(location, linearVelocity, angularVelocity));

        HLAMPAircraftClass* objectClass = dynamic_cast<HLAMPAircraftClass*>(manager.getObjectClass("MPAircraft"));
        if (!objectClass)
            return;
        _objectInstance = new HLAMPAircraft(objectClass);
        if (!_objectInstance.valid())
            return;
        _objectInstance->registerInstance();
        _objectInstance->setModelPath("Aircraft/ogel/Models/SinglePiston.xml");

        manager.schedule(*this, getSimTime() + SGTimeStamp::fromSecMSec(0, 1));
    }

    virtual void update(AIManager& manager, const SGTimeStamp& simTime)
    {
        SGTimeStamp dt = simTime - getSimTime();

        setGroundCache(getPhysics(), manager.getPager(), dt);
        getEnvironment().update(*this, dt);
        getSubsystemGroup().update(*this, dt);
        getPhysics().update(*this, dt);

        AIObject::update(manager, simTime);

        if (!_objectInstance.valid())
            return;

        _objectInstance->setLocation(getPhysics());
        _objectInstance->setSimTime(getSimTime().toSecs());
        _objectInstance->updateAttributeValues(getSimTime(), simgear::RTIData("update"));

        manager.schedule(*this, getSimTime() + SGTimeStamp::fromSecMSec(0, 100));
    }

    virtual void shutdown(AIManager& manager)
    {
        if (_objectInstance.valid())
            _objectInstance->removeInstance(simgear::RTIData("shutdown"));
        _objectInstance = 0;
        AIObject::shutdown(manager);
    }

private:
    SGGeod _geod;
    double _radius;
    double _turnaroundTime;
    double _velocity;
    SGSharedPtr<HLAMPAircraft> _objectInstance;
};

/// an ogle in a traffic circuit at lowi
class AIOgelInTrafficCircuit : public AIObject {
public:
    /// Also nothing to really use for a long time, but to demonstrate how it basically works
    class Physics : public AIPhysics {
    public:
        Physics(const SGLocationd& location, const SGVec3d& linearBodyVelocity = SGVec3d::zeros(),
                const SGVec3d& angularBodyVelocity = SGVec3d::zeros()) :
            AIPhysics(location, linearBodyVelocity, angularBodyVelocity),
            _targetVelocity(30),
            _gearOffset(2.5)
        { }
        virtual ~Physics()
        { }

        virtual void update(AIObject& object, const SGTimeStamp& dt)
        {
            SGVec3d down = getHorizontalLocalOrientation().backTransform(SGVec3d(0, 0, 1));
            SGVec3d distToAimingPoint = getAimingPoint() - getPosition();
            if (norm(distToAimingPoint - down*dot(down, distToAimingPoint)) <= 10*dt.toSecs()*norm(getLinearVelocity()))
                rotateAimingPoint();

            SGVec3d aimingVector = normalize(getAimingPoint() - getPosition());
            SGVec3d bodyAimingVector = getLocation().getOrientation().transform(aimingVector);

            SGVec3d angularVelocity = 0.2*cross(SGVec3d(1, 0, 0), bodyAimingVector);
            SGVec3d bodyDownVector = getLocation().getOrientation().transform(down);
            // keep an upward orientation
            angularVelocity += cross(SGVec3d(0, 0, 1), SGVec3d(0, bodyDownVector[1], bodyDownVector[2]));

            SGVec3d linearVelocity(_targetVelocity, 0, 0);

            SGVec3d gearPosition = getPosition() + _gearOffset*down;
            SGLineSegmentd lineSegment(gearPosition - 10*down, gearPosition + 10*down);
            SGVec3d point, normal;
            if (object.getGroundIntersection(point, normal, lineSegment)) {
                double agl = dot(down, point - gearPosition);
                if (agl < 0)
                    linearVelocity -= down*(0.5*agl/dt.toSecs());
            }

            advanceByBodyVelocity(dt.toSecs(), linearVelocity, angularVelocity);
        }

        const SGVec3d& getAimingPoint() const
        { return _waypoints.front(); }
        void rotateAimingPoint()
        { _waypoints.splice(_waypoints.end(), _waypoints, _waypoints.begin()); }

        std::list<SGVec3d> _waypoints;
        double _targetVelocity;
        double _gearOffset;
    };

    AIOgelInTrafficCircuit()
    { }
    virtual ~AIOgelInTrafficCircuit()
    { }

    virtual void init(AIManager& manager)
    {
        AIObject::init(manager);

        /// Put together somw waypoints
        /// This needs to be replaced by something generic
        SGGeod rwyStart = SGGeod::fromDegM(11.35203755, 47.26109606, 574);
        SGGeod rwyEnd = SGGeod::fromDegM(11.33741688, 47.25951967, 576);
        SGQuatd hl = SGQuatd::fromLonLat(rwyStart);
        SGVec3d down = hl.backTransform(SGVec3d(0, 0, 1));

        SGVec3d cartRwyStart = SGVec3d::fromGeod(rwyStart);
        SGVec3d cartRwyEnd = SGVec3d::fromGeod(rwyEnd);

        SGVec3d centerline = normalize(cartRwyEnd - cartRwyStart);
        SGVec3d left = normalize(cross(centerline, down));

        SGGeod endClimb = SGGeod::fromGeodM(SGGeod::fromCart(cartRwyEnd + 500*centerline), 700);
        SGGeod startDescend = SGGeod::fromGeodM(SGGeod::fromCart(cartRwyStart - 500*centerline + 150*left), 650);

        SGGeod startDownwind = SGGeod::fromGeodM(SGGeod::fromCart(cartRwyEnd + 500*centerline + 800*left), 750);
        SGGeod endDownwind = SGGeod::fromGeodM(SGGeod::fromCart(cartRwyStart - 500*centerline + 800*left), 750);

        SGLocationd location(SGVec3d::fromGeod(rwyStart), SGQuatd::fromLonLat(rwyStart)*SGQuatd::fromEulerDeg(-100, 0, 0));
        Physics* physics = new Physics(location, SGVec3d(0, 0, 0), SGVec3d(0, 0, 0));
        physics->_waypoints.push_back(SGVec3d::fromGeod(rwyStart));
        physics->_waypoints.push_back(SGVec3d::fromGeod(rwyEnd));
        physics->_waypoints.push_back(SGVec3d::fromGeod(endClimb));
        physics->_waypoints.push_back(SGVec3d::fromGeod(startDownwind));
        physics->_waypoints.push_back(SGVec3d::fromGeod(endDownwind));
        physics->_waypoints.push_back(SGVec3d::fromGeod(startDescend));
        setPhysics(physics);

        /// Ok, this is part of the official sketch
        HLAMPAircraftClass* objectClass = dynamic_cast<HLAMPAircraftClass*>(manager.getObjectClass("MPAircraft"));
        if (!objectClass)
            return;
        _objectInstance = new HLAMPAircraft(objectClass);
        if (!_objectInstance.valid())
            return;
        _objectInstance->registerInstance();
        _objectInstance->setModelPath("Aircraft/ogel/Models/SinglePiston.xml");

        /// Need to schedule something else we get deleted
        manager.schedule(*this, getSimTime() + SGTimeStamp::fromSecMSec(0, 100));
    }

    virtual void update(AIManager& manager, const SGTimeStamp& simTime)
    {
        SGTimeStamp dt = simTime - getSimTime();

        setGroundCache(getPhysics(), manager.getPager(), dt);
        getEnvironment().update(*this, dt);
        getSubsystemGroup().update(*this, dt);
        getPhysics().update(*this, dt);

        AIObject::update(manager, simTime);

        if (!_objectInstance.valid())
            return;

        _objectInstance->setLocation(getPhysics());
        _objectInstance->setSimTime(getSimTime().toSecs());
        _objectInstance->updateAttributeValues(getSimTime(), simgear::RTIData("update"));

        /// Need to schedule something else we get deleted
        manager.schedule(*this, getSimTime() + SGTimeStamp::fromSecMSec(0, 100));
    }

    virtual void shutdown(AIManager& manager)
    {
        if (_objectInstance.valid())
            _objectInstance->removeInstance(simgear::RTIData("shutdown"));
        _objectInstance = 0;

        AIObject::shutdown(manager);
    }

private:
    SGSharedPtr<HLAMPAircraft> _objectInstance;
};

} // namespace fgai

// getopt
#include <unistd.h>

int
main(int argc, char* argv[])
{
    SGSharedPtr<fgai::AIManager> manager = new fgai::AIManager;

    /// FIXME include some argument parsing stuff
    std::string fg_root;
    std::string fg_scenery;

    int c;
    while ((c = getopt(argc, argv, "cCf:F:n:O:p:RsS")) != EOF) {
        switch (c) {
        case 'c':
            manager->setCreateFederationExecution(true);
            break;
        case 'C':
            manager->setTimeConstrained(true);
            break;
        case 'f':
            manager->setFederateType(optarg);
            break;
        case 'F':
            manager->setFederationExecutionName(optarg);
            break;
        case 'O':
            manager->setFederationObjectModel(optarg);
            break;
        case 'p':
            sglog().set_log_classes(SG_ALL);
            sglog().set_log_priority(sgDebugPriority(atoi(optarg)));
            break;
        case 'R':
            manager->setTimeRegulating(true);
            break;
        case 's':
            manager->setTimeConstrainedByLocalClock(false);
            break;
        case 'S':
            manager->setTimeConstrainedByLocalClock(true);
            break;
        case 'r':
            fg_root = optarg;
            break;
        case 'y':
            fg_scenery = optarg;
            break;
        }
    }

    if (fg_root.empty()) {
        if (const char *fg_root_env = std::getenv("FG_ROOT")) {
            fg_root = fg_root_env;
        } else {
            fg_root = PKGLIBDIR;
        }
    }
    if (fg_scenery.empty()) {
        if (const char *fg_scenery_env = std::getenv("FG_SCENERY")) {
            fg_scenery = fg_scenery_env;
        } else if (!fg_root.empty()) {
            SGPath path(fg_root);
            path.append("Scenery");
            fg_scenery = path.str();
        }
    }

    manager->getPager().setScenery(fg_root, fg_scenery);

    if (manager->getFederationObjectModel().empty()) {
        SGPath path(fg_root);
        path.append("HLA");
        path.append("fg-local-fom.xml");
        manager->setFederationObjectModel(path.local8BitStr());
    }

    /// EDDS
    manager->insert(new fgai::AIOgel(SGGeod::fromDegFt(9.19298, 48.6895, 2000)));
    /// LOWI
    manager->insert(new fgai::AIOgel(SGGeod::fromDegFt(11.344, 47.260, 2500)));
    /// LOWI traffic circuit
    manager->insert(new fgai::AIOgelInTrafficCircuit);

    return manager->exec();
}
