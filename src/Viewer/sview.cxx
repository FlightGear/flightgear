/*
Implementation of 'step' view system.

The basic idea here is calculate view position and direction by a series
of explicit steps. Steps can move to the origin of the user aircraft or a
multiplayer aircraft, or modify the current position by a fixed vector (e.g.
to move from aircraft origin to the pilot's eyepoint), or rotate the current
direction by a fixed transformation etc. We can also have a step that sets the
direction to point to a previously-calculated target position.

This is similar to what is already done by FlightGear's existing View code, but
making the individual steps explicit gives us more flexibility.

The dynamic nature of step views allows view cloning with composite-viewer.

We also allow views to be defined and created at runtime instead of being
hard-coded in *-set.xml files. For example this makes it possible to define a
view from the user's aircraft's pilot to the centre of a multiplayer aircraft
(or to a multiplayer aircraft's pilot).
*/

/*
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "sview.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Viewer/FGEventHandler.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/WindowBuilder.hxx>
#include <Viewer/WindowSystemAdapter.hxx>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/viewer/Compositor.hxx>

#include <osg/CameraView>
#include <osg/GraphicsContext>
#include <osgViewer/CompositeViewer>

static const double pi = 3.141592653589793238463;

static std::ostream& operator << (std::ostream& out, const osg::Vec3f& vec)
{
    return out << "Vec3f{"
            << " x=" << vec._v[0]
            << " y=" << vec._v[1]
            << " z=" << vec._v[2]
            << "}";
}

static std::ostream& operator << (std::ostream& out, const osg::Quat& quat)
{
    return out << "Quat{"
            << " x=" << quat._v[0]
            << " y=" << quat._v[1]
            << " z= " << quat._v[2]
            << " w=" << quat._v[3]
            << "}";
}

static std::ostream& operator << (std::ostream& out, const osg::Matrixd& matrix)
{
    osg::Vec3f  translation;
    osg::Quat   rotation;
    osg::Vec3f  scale;
    osg::Quat   so;
    matrix.decompose(translation, rotation, scale, so);
    return out << "Matrixd {"
            << " translation=" << translation
            << " rotation=" << rotation
            << " scale=" << scale
            << " so=" << so
            << "}";
}


/* A position and direction. Repeatedly modified by SviewStep::evaluate() when
calculating final camera position/orientation. */
struct SviewPosDir
{
    SviewPosDir()
    :
    heading(0),
    pitch(0),
    roll(0),
    target_is_set(false)
    {
    }
    
    SGGeod  position;
    double  heading;
    double  pitch;
    double  roll;
    
    SGGeod  target;
    bool    target_is_set;
    
    /* The final position and direction, in a form suitable for setting an
    osg::Camera's view matrix. */
    SGVec3d position2;
    SGQuatd direction2;
    
    /* If a step sets either/both of these to non-zero, the view will alter
    zoom to accomodate the required field of views (in degreea). */
    double fov_h = 0;
    double fov_v = 0;
    
    friend std::ostream& operator<< (std::ostream& out, const SviewPosDir& posdir)
    {
        out << "SviewPosDir {"
                << " position=" << posdir.position
                << " heading=" << posdir.heading
                << " pitch=" << posdir.pitch
                << " roll=" << posdir.roll
                << " target=" << posdir.target
                << " position2=" << posdir.position2
                << " direction2=" << posdir.direction2
                << "}"
                ;
        return out;
    }
};


/* Generic damping support. Improved version of class in view.hxx and view.cxx.

We model dx/dt = (target-current)/damping_time, so damping_time is time for
value to change by a factor of e. */
struct Damping {

    /* If m_wrap_max is non-zero, we wrap to ensure values are always between 0
    and m_wrap_max. E.g. use m_wrap_max=360 for an angle in degrees. */
    Damping(double damping_time, double wrap_max=0, double current=0)
    :
    m_damping_time(damping_time),
    m_current(current),
    m_wrap_max(wrap_max)
    {}
    
    /* Updates and returns new smoothed value. */
    double  update(double dt, double target)
    {
        const double e = 2.718281828459045;
        double delta = target - m_current;
        if (m_wrap_max) {
            if (delta < -m_wrap_max/2)  delta += m_wrap_max;
            if (delta >= m_wrap_max/2)  delta -= m_wrap_max;
        }
        m_current = target - delta * pow(e, -dt/m_damping_time);
        if (m_wrap_max) {
            if (m_current < 0)  m_current += m_wrap_max;
            if (m_current >= m_wrap_max)  m_current -= m_wrap_max;
        }
        return m_current;
    }
    
    /* Forces current value to be <current>. */
    double reset(double current)
    {
        m_current = current;
        return m_current;
    }

    private:
        double  m_damping_time;
        double  m_current;
        double  m_wrap_max;
};

/* Abstract base class for a single view step. A view step modifies a
SviewPosDir, e.g. translating the position and/or rotating the direction. */
struct SviewStep
{
    /* Updates <posdir>. */
    virtual void evaluate(SviewPosDir& posdir, double dt=0) = 0;
    
    /* Modify view angle. */
    virtual void mouse_drag(double delta_x_deg, double delta_y_deg)
    {
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStep>";
    }
    
    virtual ~SviewStep() {}
    
    std::string m_description;
    
    friend std::ostream& operator<< (std::ostream& out, const SviewStep& step)
    {
        out << ' ' << typeid(step).name();
        step.stream(out);
        return out;
    }
};

struct Callsign
/* Info about aircraft with a particular callsign. "" means the user
aircraft. We use the user aircraft if specified callsign does not exist.

update() must be called before m_root is dereferenced. */
{
    Callsign(const std::string& callsign)
    :
    m_callsign(callsign)
    {
    }
    
    bool update()
    /* Returns true if we have changed m_root. */
    {
        if (m_callsign == "") {
            if (m_root) return false;
        }
        else if (m_root && m_root->getStringValue("callsign") == m_callsign) {
            return false;
        }
        m_root = nullptr;
        if (m_callsign != "") {
            /* Find multiplayer with matching callsign. */
            SGPropertyNode* c = globals->get_props()->getNode("ai/models/callsigns")->getNode(m_callsign);
            if (c) {
                int i = c->getIntValue();
                m_root = globals->get_props()->getNode("ai/models/multiplayer", i);
                assert(m_root);
            }
        }
        if (!m_root) {
            /* Default to user aircraft. */
            m_root = globals->get_props();
        }
        return true;
    }
    
    std::string         m_callsign;
    SGPropertyNode_ptr  m_root;
    
};

/* A step that sets position to aircraft origin and direction to aircraft's
longitudal axis. */
struct SviewStepAircraft : SviewStep
{
    SviewStepAircraft(const std::string& callsign)
    :
    m_callsign(callsign)
    {
    }
    
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        if (m_callsign.update()) {
            m_longitude     = m_callsign.m_root->getNode("position/longitude-deg");
            m_latitude      = m_callsign.m_root->getNode("position/latitude-deg");
            m_altitude      = m_callsign.m_root->getNode("position/altitude-ft");
            m_heading       = m_callsign.m_root->getNode("orientation/true-heading-deg");
            m_pitch         = m_callsign.m_root->getNode("orientation/pitch-deg");
            m_roll          = m_callsign.m_root->getNode("orientation/roll-deg");
        }
        posdir.position = SGGeod::fromDegFt(
                m_longitude->getDoubleValue(),
                m_latitude->getDoubleValue(),
                m_altitude->getDoubleValue()
                );

        posdir.heading  = m_heading->getDoubleValue();
        posdir.pitch    = m_pitch->getDoubleValue();
        posdir.roll     = m_roll->getDoubleValue();
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepAircraft:" + m_callsign.m_callsign + ">";
    }
    
    private:
    
    Callsign            m_callsign;
    
    SGPropertyNode_ptr  m_longitude;
    SGPropertyNode_ptr  m_latitude;
    SGPropertyNode_ptr  m_altitude;
    
    SGPropertyNode_ptr  m_heading;
    SGPropertyNode_ptr  m_pitch;
    SGPropertyNode_ptr  m_roll;
};


/* Moves position by fixed vector, does not change direction.

E.g. for moving from aircraft origin to pilot viewpoint. */
struct SviewStepMove : SviewStep
{
    SviewStepMove(double forward, double up, double right)
    :
    m_offset(-right, -up, -forward)
    {
        SG_LOG(SG_VIEW, SG_INFO, "forward=" << forward << " up=" << up << " right=" << right);
    }
    
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        /* These calculations are copied from View::recalcLookFrom(). */

        /* The rotation rotating from the earth centerd frame to the horizontal
        local frame. */
        SGQuatd hlOr = SGQuatd::fromLonLat(posdir.position);
        
        /* The rotation from the horizontal local frame to the basic view
        orientation. */
        SGQuatd hlToBody = SGQuatd::fromYawPitchRollDeg(posdir.heading, posdir.pitch, posdir.roll);
        
        /* Compute the eyepoints orientation and position wrt the earth
        centered frame - that is global coorinates. */
        SGQuatd ec2body = hlOr * hlToBody;
        
        /* The cartesian position of the basic view coordinate. */
        SGVec3d position = SGVec3d::fromGeod(posdir.position);
        
        /* This is rotates the x-forward, y-right, z-down coordinate system the
        where simulation runs into the OpenGL camera system with x-right, y-up,
        z-back. */
        SGQuatd q(-0.5, -0.5, 0.5, 0.5);
        
        position += (ec2body * q).backTransform(m_offset);
        posdir.position = SGGeod::fromCart(position);
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepMove>" << m_offset;
    }
    
    private:
    SGVec3d m_offset;
};

/* Modifies heading, pitch and roll by fixed amounts; does not change
position.

E.g. can be used to preserve direction (relative to aircraft) of Helicopter
view at the time it was cloned. */
struct SviewStepRotate : SviewStep
{
    SviewStepRotate(
            double heading,
            double pitch,
            double roll,
            double damping_heading = 0,
            double damping_pitch = 0,
            double damping_roll = 0
            )
    :
    m_heading(heading),
    m_pitch(pitch),
    m_roll(roll),
    m_damping_heading(damping_heading, 360 /*m_wrap_max*/),
    m_damping_pitch(damping_pitch, 360 /*m_wrap_max*/),
    m_damping_roll(damping_roll, 360 /*m_wrap_max*/)
    {
        SG_LOG(SG_VIEW, SG_INFO, "heading=" << heading << " pitch=" << pitch << " roll=" << roll);
    }
    
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        posdir.heading  = m_damping_heading.update(dt, posdir.heading + m_heading);
        posdir.pitch    = m_damping_pitch.update(dt, posdir.pitch + m_pitch);
        posdir.roll     = m_damping_roll.update(dt, posdir.roll + m_roll);
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepRotate>"
                << ' ' << m_heading
                << ' ' << m_pitch
                << ' ' << m_roll
                ;
    }
    
    private:
    double  m_heading;
    double  m_pitch;
    double  m_roll;
    Damping m_damping_heading;
    Damping m_damping_pitch;
    Damping m_damping_roll;
};

/* Modify a view's heading and pitch in response to mouse dragging. */
struct SviewStepMouseDrag : SviewStep
{
    SviewStepMouseDrag(double heading_scale, double pitch_scale)
    :
    m_heading_scale(heading_scale),
    m_pitch_scale(pitch_scale)
    {}
    
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        posdir.heading += m_heading;
        posdir.pitch += m_pitch;
    }
    
    void mouse_drag(double delta_x_deg, double delta_y_deg) override
    {
        m_heading += m_heading_scale * delta_x_deg;
        m_pitch += m_pitch_scale * delta_y_deg;
    }
    
    double m_heading_scale;
    double m_pitch_scale;
    double m_heading = 0;
    double m_pitch = 0;
};

/* Multiply heading, pitch and roll by constants. */
struct SviewStepDirectionMultiply : SviewStep
{
    SviewStepDirectionMultiply(double heading=0, double pitch=0, double roll=0)
    :
    m_heading(heading),
    m_pitch(pitch),
    m_roll(roll)
    {
        SG_LOG(SG_VIEW, SG_INFO, "heading=" << heading << " pitch=" << pitch << " roll=" << roll);
    }
    
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        posdir.heading *= m_heading;
        posdir.pitch *= m_pitch;
        posdir.roll *= m_roll;
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepDirectionMultiply>"
                << ' ' << m_heading
                << ' ' << m_pitch
                << ' ' << m_roll
                ;
    }
    
    private:
    double  m_heading;
    double  m_pitch;
    double  m_roll;
};

/* Copies current position to posdir.target. Used by SviewEyeTarget()
to make current position be available as target later on, e.g. by
SviewStepFinal with to_target=true. */
struct SviewStepCopyToTarget : SviewStep
{
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        assert(!posdir.target_is_set);
        posdir.target = posdir.position;
        posdir.target_is_set = true;
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepCopyToTarget>";
    }
};

/* Move position to nearest tower. */
struct SviewStepNearestTower : SviewStep
{
    SviewStepNearestTower(const std::string& callsign)
    :
    m_callsign(callsign)
    {
        m_description = "Nearest tower";
    }
    
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        if (m_callsign.update()) {
            m_latitude = m_callsign.m_root->getNode("sim/tower/latitude-deg", true /*create*/);
            m_longitude = m_callsign.m_root->getNode("sim/tower/longitude-deg", true /*create*/);
            m_altitude = m_callsign.m_root->getNode("sim/tower/altitude-ft", true /*create*/);
        }
        posdir.position = SGGeod::fromDegFt(
                m_longitude->getDoubleValue(),
                m_latitude->getDoubleValue(),
                m_altitude->getDoubleValue()
                );
        posdir.heading = 0;
        posdir.pitch = 0;
        posdir.roll = 0;
        SG_LOG(SG_VIEW, SG_BULK, "moved posdir.postion to: " << posdir.position);
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepNearestTower:" + m_callsign.m_callsign + ">";
    }
    
    Callsign            m_callsign;
    SGPropertyNode_ptr  m_latitude;
    SGPropertyNode_ptr  m_longitude;
    SGPropertyNode_ptr  m_altitude;
};


/*
Converts posdir's eye position/direction to global cartesian
position/quarternion direction in position2 and direction2, which will be used
to set the camera parameters.

If posdir.target_is_set is true (i.e. posdir.target is valid), we also change
the direction to point at the target, using the original direction to determine
the 'up' vector.
*/
struct SviewStepFinal : SviewStep
{
    SviewStepFinal()
    {
    }
    
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        /* See View::recalcLookFrom(). */
        
        /* The rotation rotating from the earth centerd frame to the horizontal
        local frame. */
        SGQuatd eye_position_direction = SGQuatd::fromLonLat(posdir.position);

        /* The rotation from the horizontal local frame to the basic view
        orientation. */
        SGQuatd eye_local_direction = SGQuatd::fromYawPitchRollDeg(posdir.heading, posdir.pitch, posdir.roll);

        /* The cartesian position of the basic view coordinate. */
        SGVec3d eye_position = SGVec3d::fromGeod(posdir.position);

        /* Compute the eye direction in global coordinates. */
        SGQuatd eye_direction = eye_position_direction * eye_local_direction;
        
        if (posdir.target_is_set)
        {
            /* Rotate eye direction to point at posdir.target. */
            
            SGVec3d target_position = SGVec3d::fromGeod(posdir.target);

            /* add target offsets to at_position...
            Compute the eyepoints orientation and position wrt the earth centered
            frame - that is global coorinates _absolute_view_pos = eye_position; */

            /* the view direction. */
            SGVec3d eye_to_target_direction = normalize(target_position - eye_position);

            /* the up directon. */
            SGVec3d up = eye_direction.backTransform(SGVec3d(0, 0, -1));

            /* rotate -dir to the 2-th unit vector
            rotate up to 1-th unit vector
            Note that this matches the OpenGL camera coordinate system with
            x-right, y-up, z-back. */
            posdir.direction2 = SGQuatd::fromRotateTo(-eye_to_target_direction, 2, up, 1);
        }
        else
        {
            /* This is rotates the x-forward, y-right, z-down coordinate system the
            where simulation runs into the OpenGL camera system with x-right, y-up,
            z-back. */
            SGQuatd q(-0.5, -0.5, 0.5, 0.5);

            posdir.direction2 = eye_direction * q;
        }
        
        posdir.position2 = eye_position;
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepFinal>";
    }
};


/* Change angle and field of view so that we can see an aircraft and the ground
immediately below it. */
struct SviewStepAGL : SviewStep
{
    SviewStepAGL(const std::string& callsign, double damping_time)
    :
    m_callsign(callsign),
    relative_height_ground_damping(damping_time)
    {
    }
    
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        if (m_callsign.update()) {
            m_chase_distance = -25;
            if (m_callsign.m_callsign == "") {
                m_chase_distance = globals->get_props()->getDoubleValue("/sim/chase-distance-m", m_chase_distance);
            }
            else {
                m_chase_distance = m_callsign.m_root->getDoubleValue("set/sim/chase-distance-m", m_chase_distance);
            }
        }
        assert(posdir.target_is_set);
        double _fov_user_deg = 30;
        double _configFOV_deg = 30;
        /* Some aircraft appear to have elevation that is slightly below ground
        level when on the ground, e.g. SenecaII, which makes get_elevation_m()
        fail. So we pass a slightly incremented elevation. */
        double                      ground_altitude = 0;
        const simgear::BVHMaterial* material = NULL;
        SGGeod                      target0 = posdir.target;
        SGGeod                      target_plus = posdir.target;
        target_plus.setElevationM(target_plus.getElevationM() + 1);
        bool ok = globals->get_scenery()->get_elevation_m(target_plus, ground_altitude, &material);
        if (ok) {
            m_ground_altitude = ground_altitude;
        }
        else {
            /* get_elevation_m() can fail if scenery has been un-cached, which
            appears to happen quite often with remote multiplayer aircraft, so
            we preserve the previous ground altitude to give some consistency
            and avoid confusing zooming when switching between views. */
            ground_altitude = m_ground_altitude;
        }
        
        double    h_distance = SGGeodesy::distanceM(posdir.position, posdir.target);
        if (h_distance == 0) {
            /* Not sure this should ever happen, but we need to cope with this
            here otherwise we'll get divide-by-zero. */
            return;
        }
        /* Find vertical region we want to be able to see. */
        double  relative_height_target = posdir.target.getElevationM() - posdir.position.getElevationM();
        double  relative_height_ground = ground_altitude - posdir.position.getElevationM();

        /* We expand the field of view so that it hopefully shows the whole
        aircraft and a little more of the ground.

        We use chase-distance as a crude measure of the aircraft's
        size. There doesn't seem to be any more definitive information.

        We damp our measure of ground level, to avoid the view jumping around
        if an aircraft flies over buildings. */

        relative_height_ground -= 2;

        double    aircraft_size_vertical = fabs(m_chase_distance) * 0.3;
        double    aircraft_size_horizontal = fabs(m_chase_distance) * 0.9;

        double    relative_height_target_plus = relative_height_target + aircraft_size_vertical;
        double    relative_height_ground_ = relative_height_ground;
        relative_height_ground = relative_height_ground_damping.update(dt, relative_height_ground);
        if (relative_height_ground > relative_height_target) {
            /* Damping of relative_height_ground can result in it being
            temporarily above the aircraft, so we ensure the aircraft is
            visible. */
            relative_height_ground = relative_height_ground_damping.reset(relative_height_ground_);
        }

        /* Not implemented yet: apply scaling from user field of view
        setting, altering only relative_height_ground so that the aircraft
        is always in view. */
        {
            double  delta = relative_height_target_plus - relative_height_ground;
            delta *= (_fov_user_deg / _configFOV_deg);
            relative_height_ground = relative_height_target_plus - delta;
        }

        double  angle_v_target = atan(relative_height_target_plus / h_distance);
        double  angle_v_ground = atan(relative_height_ground / h_distance);

        /* The target we want to use is determined by the midpoint of the two
        angles we've calculated. */
        double  angle_v_mid = (angle_v_target + angle_v_ground) / 2;
        double posdir_target_old_elevation = posdir.target.getElevationM();
        posdir.target.setElevationM(posdir.position.getElevationM() + h_distance * tan(angle_v_mid));

        /* Set required vertical field of view. We use fabs to avoid
        things being upside down if target is below ground level (e.g. new
        multiplayer aircraft are briefly at -9999ft). */
        double  fov_v = fabs(angle_v_target - angle_v_ground);

        /* Set required horizontal field of view so that we can see entire
        horizontal extent of the aircraft (assuming worst case where
        airplane is horizontal and square-on to viewer). */
        double  fov_h = 2 * atan(aircraft_size_horizontal / 2 / h_distance);

        posdir.fov_v = fov_v * 180 / pi;
        posdir.fov_h = fov_h * 180 / pi;

        bool verbose = false;
        if (0) {
            static time_t t0 = 0;
            time_t t = time(NULL);
            if (0 && t - t0 >= 3) {
                t0 = t;
                verbose = true;
            }
        }
        if (verbose) SG_LOG(SG_VIEW, SG_ALERT, ""
                << " target0=" << target0
                << " fov_v=" << fov_v * 180/pi
                << " ground_altitude=" << ground_altitude
                << " relative_height_target_plus=" << relative_height_target_plus
                << " h_distance=" << h_distance
                << " relative_height_ground=" << relative_height_ground
                << " m_chase_distance=" << m_chase_distance
                << " angle_v_mid=" << angle_v_mid * 180/pi
                << " posdir_target_old_elevation=" << posdir_target_old_elevation
                << " posdir.target=" << posdir.target
                );
    }
    
    double      m_chase_distance;
    Callsign    m_callsign;
    double      m_ground_altitude = 0;
    Damping     relative_height_ground_damping;
};

/* A step that takes the SviewPosDir's eye and target positions and treats
them as local and remote points respectively. We choose an eye position and
direction such that the local and remote points are both visible, with the
original eye position in the foreground at a fixed distance. */
struct SviewStepDouble : SviewStep
{
    SviewStepDouble()
    {
        m_local_chase_distance = 25;
        m_angle_rad = 15 * pi / 180;
    }
    
    SviewStepDouble(SGPropertyNode* config)
    {
        m_local_chase_distance = config->getDoubleValue("chase-distance");
        m_angle_rad = config->getDoubleValue("angle") * pi / 180;
    }
    void evaluate(SviewPosDir& posdir, double dt) override
    {
        /*
        We choose eye position so that we show the local aircraft a fixed
        amount below the view midpoint, and the remote aircraft the same fixed
        amount above the view midpoint.
        
        L: middle of local aircraft.
        R: middle of remote aircraft.
        E: desired eye-point
        
           ----             R
         /      \ 
        E        |
        |    L   | .................... H (horizon)
        |        |
         \      /
           ----
        
        We require that:
        
            EL is local aircraft's chase-distance so that local aircraft is in
            perfect view.
        
            Angle LER is fixed to give good view of both aircraft in
            window. (Should be related to the vertical angular size of the
            window, but at the moment we use a fixed value.)
        
        We need to calculate angle RLE, and add to HLR, in order to find
        position of E (eye) relative to L. Then for view pitch we use midpoint
        of angle of ER and angle EL, so that local and remote aircraft are
        symmetrically below and above the centre of the view.
        
        We find angle RLE by using cosine rule twice in the triangle RLE:
            ER^2 = EL^2 + LR^2 - 2*EL*LR*cos(RLE)
            LR^2 = ER^2 + EL^2 - 2*ER*EL*cos(LER)
        
        Wen end up with a quadratic for ER with solution:
            ER = EL * cos(LER) + sqrt(LR^2 - EL^22*sin(LER)^2)
            (We discard the -sqrt because it ends up with ER being negative.)
        
        and:
            cos(RLE) = (LR^2 + LE^2 - ER^2) / (2*LE*LR)
        
        So we can find RLE using acos().
        */
        bool debug = false;
        static time_t t0 = 0;
        time_t t = time(NULL);
        if (0 && t - t0 > 3) {
            t0 = t;
            debug = true;
        }
        
        assert(posdir.target_is_set);
        SviewPosDir posdir_remote = posdir;
        SviewPosDir posdir_local = posdir;
        
        posdir_local.target = posdir_local.position;
        
        if (debug) {
            SG_LOG(SG_VIEW, SG_ALERT, " posdir       =" << posdir);
            SG_LOG(SG_VIEW, SG_ALERT, " posdir_local =" << posdir_local);
            SG_LOG(SG_VIEW, SG_ALERT, " posdir_remote=" << posdir_remote);
        }
        
        /* Create cartesian coordinates so we can calculate distance <lr>. */
        SGVec3d local_pos = SGVec3d::fromGeod(posdir_local.target);
        SGVec3d remote_pos = SGVec3d::fromGeod(posdir_remote.target);
        double lr = sqrt(distSqr(local_pos, remote_pos));
        
        /* Desired angle between local and remote aircraft in final view. */
        double ler = m_angle_rad;
        
        /* Distance of eye from local aircraft. */
        double le = m_local_chase_distance;
        
        /* Find <er>, the distance of eye from remote aircraft. Have to be
        careful to cope when there is no solution if remote is too close, and
        choose the +ve sqrt(). */
        double er_root_term = lr*lr - le*le*sin(ler)*sin(ler);
        if (er_root_term < 0) {
            /* This can happen if LR is too small, i.e. remote aircraft too
            close to local aircraft. */
            er_root_term = 0;
        }
        double er = le * cos(ler) + sqrt(er_root_term);
        
        /* Now find rle, angle at local aircraft between vector to remote
        aircraft and vector to desired eye position. Again we have to cope when
        a real solution is not possible. */
        double cos_rle = (lr*lr + le*le - er*er) / (2*le*lr);
        if (cos_rle > 1) cos_rle = 1;
        if (cos_rle < -1) cos_rle = -1;
        double rle = acos(cos_rle);
        double rle_deg = rle * 180 / pi;
        
        /* Now find the actual eye position. We do this by calculating heading
        and pitch from local aircraft L to eye position E, then using a
        temporary SviewStepMove. */
        double lr_vertical = posdir_remote.target.getElevationM() - posdir_local.target.getElevationM();
        double lr_horizontal = SGGeodesy::distanceM(posdir_local.target, posdir_remote.target);
        double hlr = atan2(lr_vertical, lr_horizontal);
        posdir_local.heading = SGGeodesy::courseDeg(
                posdir_local.target,
                posdir_remote.target
                );
        posdir_local.pitch = (hlr + rle) * 180 / pi;
        posdir_local.roll = 0;
        auto move = SviewStepMove(le, 0, 0);
        move.evaluate(posdir_local, 0 /*dt*/);
        
        /* At this point, posdir_local.position is eye position. We make
        posdir_local.pitch point from this eye position to halfway between the
        remote and local aircraft. */
        double er_vertical = posdir_remote.target.getElevationM()
                - posdir_local.position.getElevationM();
        double her = asin(er_vertical / er);
        double hel = (hlr + rle) - pi;
        posdir_local.pitch = (her + hel) / 2 * 180 / pi;
        posdir = posdir_local;
        
        /* Need to ensure that SviewStepFinal will not rotate the view to point
        at posdir.target. */
        posdir.target_is_set = false;
        
        if (debug) {
            SG_LOG(SG_VIEW, SG_ALERT, ""
                    << " lr=" << lr
                    << " ler=" << ler
                    << " er_root_term=" << er_root_term
                    << " er=" << er
                    << " cos_rle=" << cos_rle
                    << " rle_deg=" << rle_deg
                    << " lr_vertical=" << lr_vertical
                    << " lr_horizontal=" << lr_horizontal
                    << " hlr=" << hlr
                    << " posdir_local=" << posdir_local
                    << " posdir_remote=" << posdir_remote
                    );
        }
    }
    
    double m_local_chase_distance;
    double m_angle_rad;
};

/* Contains a list of SviewStep's that are used to evaluate a SviewPosDir
(position and orientation)
*/
struct SviewSteps
{
    void add_step(std::shared_ptr<SviewStep> step)
    {
        m_steps.push_back(step);
    }
    
    void add_step(SviewStep* step)
    {
        return add_step(std::shared_ptr<SviewStep>(step));
    }
    
    void evaluate(SviewPosDir& posdir, double dt, bool debug=false)
    {
        if (debug) SG_LOG(SG_VIEW, SG_ALERT, "evaluating m_name=" << m_name);
        for (auto step: m_steps) {
            step->evaluate(posdir, dt);
            if (debug) SG_LOG(SG_VIEW, SG_ALERT, "posdir=" << posdir);
        }
    };
    
    std::string m_name;
    std::vector<std::shared_ptr<SviewStep>>   m_steps;
    
    friend std::ostream& operator << (std::ostream& out, const SviewSteps& viewpos)
    {
        out << viewpos.m_name << " (" << viewpos.m_steps.size() << ")";
        for (auto step: viewpos.m_steps) {
            out << " " << *step;
        }
        return out;
    }
};


/* Base class for views.
*/
struct SviewView
{
    SviewView(osgViewer::View* osg_view)
    :
    m_osg_view(osg_view)
    {
        s_id += 1;
    }
    
    /* Description that also includes integer identifier. */
    const std::string  description2()
    {
        char    buffer[32];
        snprintf(buffer, sizeof(buffer), "[%i] ", s_id);
        return buffer + description();
    }
    
    /* Description of this view, used in window title etc. */
    virtual const std::string description() = 0;
    
    virtual ~SviewView()
    {
        if (!m_osg_view) {
            return;
        }
        osgViewer::ViewerBase* viewer_base = m_osg_view->getViewerBase();
        auto composite_viewer = dynamic_cast<osgViewer::CompositeViewer*>(viewer_base);
        assert(composite_viewer);
        for (unsigned i=0; i<composite_viewer->getNumViews(); ++i) {
            osgViewer::View* view = composite_viewer->getView(i);
            SG_LOG(SG_VIEW, SG_DEBUG, "composite_viewer view i=" << i << " view=" << view);
        }
        SG_LOG(SG_VIEW, SG_DEBUG, "removing m_osg_view=" << m_osg_view);
        composite_viewer->stopThreading();
        composite_viewer->removeView(m_osg_view);
        composite_viewer->startThreading();
    }
    
    /* Returns false if window has been closed. */
    virtual bool update(double dt) = 0;
    
    virtual void mouse_drag(double delta_x_deg, double delta_y_deg) = 0;
    
    /* Sets this view's camera position/orientation from <posdir>. */
    void posdir_to_view(SviewPosDir posdir)
    {
        /* FGViewMgr::update(). */
        osg::Vec3d  position    = toOsg(posdir.position2);
        osg::Quat   orientation = toOsg(posdir.direction2);

        osg::Camera* camera = m_osg_view->getCamera();
        osg::Matrix old_m = camera->getViewMatrix();
        /* This calculation is copied from CameraGroup::update(). */
        const osg::Matrix new_m(
                osg::Matrix::translate(-position)
                * osg::Matrix::rotate(orientation.inverse())
                );
        SG_LOG(SG_VIEW, SG_BULK, "old_m: " << old_m);
        SG_LOG(SG_VIEW, SG_BULK, "new_m: " << new_m);
        camera->setViewMatrix(new_m);
        
        if (posdir.fov_v || posdir.fov_h) {
            /* Update zoom to accomodate required vertical/horizontal field of
            views. */
            double fovy;
            double aspect_ratio;
            double zNear;
            double zFar;
            camera->getProjectionMatrixAsPerspective(fovy, aspect_ratio, zNear, zFar);
            if (posdir.fov_v) {
                camera->setProjectionMatrixAsPerspective(
                        posdir.fov_v,
                        aspect_ratio,
                        zNear,
                        zFar
                        );
                if (posdir.fov_h) {
                    /* Increase fov if necessary so that we include both
                    fov_h and fov_v. */
                    double aspect_ratio;
                    camera->getProjectionMatrixAsPerspective(
                            fovy,
                            aspect_ratio,
                            zNear,
                            zFar
                            );
                    if (fovy * aspect_ratio < posdir.fov_h) {
                        camera->setProjectionMatrixAsPerspective(
                                posdir.fov_v * posdir.fov_h / (fovy * aspect_ratio),
                                aspect_ratio,
                                zNear,
                                zFar
                                );
                    }
                }
            }
            else {
                camera->setProjectionMatrixAsPerspective(
                        posdir.fov_h / aspect_ratio,
                        aspect_ratio,
                        zNear,
                        zFar
                        );
            }
        }
    }
    
    osgViewer::View*                    m_osg_view = nullptr;
    simgear::compositor::Compositor*    m_compositor = nullptr;
    
    bool                                m_mouse_button2;
    double                              m_mouse_x = 0;
    double                              m_mouse_y = 0;
    
    static int s_id;
};

/* Converts legacy damping values (e.g. at-model-heading-damping) to a damping
time suitable for use by our Damping class. */
static double legacy_damping_time(double damping)
{
    if (damping <= 0)  return 0;
    return 1 / damping;
}

int SviewView::s_id = 0;

/* A view defined by a series of steps that defines an eye and a target. Used
for clones of main window views. */
struct SviewViewEyeTarget : SviewView
{
    SviewViewEyeTarget(osgViewer::View* view, SGPropertyNode* config)
    :
    SviewView(view)
    {
        if (!strcmp(config->getStringValue("type"), "legacy"))
        {
            /* Legacy view. */
            std::string callsign = config->getStringValue("callsign");
            std::string callsign_desc = (callsign == "") ? "" : ": " + callsign;
            SG_LOG(SG_VIEW, SG_INFO, "callsign=" << callsign);

            if (config->getBoolValue("view/config/eye-fixed")) {
                SG_LOG(SG_VIEW, SG_INFO, "eye-fixed");
                m_steps.m_name = std::string() + "legacy tower" + callsign_desc;
                
                if (!strcmp(config->getStringValue("view/type"), "lookat")) {
                    /* E.g. Tower view or Tower view AGL. */

                    /* Add a step to move to centre of aircraft. target offsets appear
                    to have reversed sign compared to what we require. */
                    m_steps.add_step(new SviewStepAircraft(callsign));
                    m_steps.add_step(new SviewStepMove(
                            -config->getDoubleValue("view/config/target-z-offset-m"),
                            -config->getDoubleValue("view/config/target-y-offset-m"),
                            -config->getDoubleValue("view/config/target-x-offset-m")
                            ));

                    /* Add a step to set pitch and roll to zero, otherwise view
                    from tower (as calculated by SviewStepFinal) rolls/pitches
                    with aircraft. */
                    m_steps.add_step(new SviewStepDirectionMultiply(
                            1 /* heading */,
                            0 /* pitch */,
                            0 /* roll */
                            ));

                    /* Current position is the target, so add a step that copies it to
                    SviewPosDir.target. */
                    m_steps.add_step(new SviewStepCopyToTarget);

                    /* Added steps to set .m_eye up so that it looks from the nearest
                    tower. */
                    m_steps.add_step(new SviewStepNearestTower(callsign));
                    
                    if (config->getBoolValue("view/config/lookat-agl")) {
                        double damping = config->getDoubleValue("view/config/lookat-agl-damping");
                        double damping_time = log(10) * legacy_damping_time(damping);
                        SG_LOG(SG_VIEW, SG_DEBUG, "lookat-agl");
                        m_steps.add_step(new SviewStepAGL(callsign, damping_time));
                    }
                    
                    m_steps.add_step(new SviewStepFinal);

                    /* Would be nice to add a step that moves towards the
                    target a little, like we do with Tower view look from. But
                    simply adding a SviewStepMove doesn't work because the
                    preceding SviewStepFinal has finalised the view angle etc.
                    */
                }
                else {
                    /* E.g. Tower view look from. */
                    m_steps.add_step(new SviewStepNearestTower(callsign));
                    
                    /* Looks like Tower view look from's heading-offset is reversed. */
                    m_steps.add_step(new SviewStepRotate(
                            -globals->get_props()->getDoubleValue("sim/current-view/heading-offset-deg"),
                            globals->get_props()->getDoubleValue("sim/current-view/pitch-offset-deg"),
                            globals->get_props()->getDoubleValue("sim/current-view/roll-offset-deg")
                            ));
                    /* Move forward a little as though one was walking towards
                    the window inside the tower; this might improve view of
                    aircraft near the tower on the ground. Ideally each tower
                    would have a 'diameter' property, but for now we just use
                    a hard-coded value. Also it would be nice to make this
                    movement not change the height. */
                    m_steps.add_step(new SviewStepMove(1, 0, 0));
                    
                    m_steps.add_step(new SviewStepMouseDrag(
                            1 /*mouse_heading_scale*/,
                            1 /*mouse_pitch_scale*/
                            ));
                    m_steps.add_step(new SviewStepFinal);
                }
            }
            else {
                SG_LOG(SG_VIEW, SG_INFO, "not eye-fixed");
                /* E.g. Pilot view and Helicopter/Chase views. */
                
                SGPropertyNode* global_sim_view = globals->get_props()
                        ->getNode("sim/view", config->getIntValue("view-number-raw"));

                if (!strcmp(global_sim_view->getStringValue("type"), "lookat")) {
                    /* E.g. Helicopter view and Chase views. */
                    m_steps.m_name = std::string() + "legacy helicopter/chase" + callsign_desc;
                    m_steps.add_step(new SviewStepAircraft(callsign));
                    
                    /* Move to centre of aircraft. config/target-z-offset-m
                    seems to use +ve to indicate movement backwards relative
                    to the aircraft, so we need to negate the value we pass to
                    SviewStepMove(). */
                    m_steps.add_step(new SviewStepMove(
                            -config->getDoubleValue("view/config/target-z-offset-m"),
                            -config->getDoubleValue("view/config/target-y-offset-m"),
                            -config->getDoubleValue("view/config/target-x-offset-m")
                            ));

                    /* Add a step that crudely preserves or don't preserve
                    aircraft's heading, pitch and roll; this enables us to mimic
                    Helicopter and Chase views. In theory we should evaluate the
                    specified paths, but in practise we only need to multiply
                    current values by 0 or 1. */
                    assert(global_sim_view);
                    m_steps.add_step(new SviewStepDirectionMultiply(
                            global_sim_view->getStringValue("config/eye-heading-deg-path")[0] ? 1 : 0,
                            global_sim_view->getStringValue("config/eye-pitch-deg-path")[0] ? 1 : 0,
                            global_sim_view->getStringValue("config/eye-roll-deg-path")[0] ? 1 : 0
                            ));

                    /* Add a step that applies the current view rotation. We
                    use -ve heading and pitch because in the legacy
                    viewing system's Helicopter view etc, increments to
                    heading-offset-deg and pitch-offset-deg actually correspond
                    to a decreasing actual heading and pitch values. */
                    double  damping_heading = legacy_damping_time(config->getDoubleValue("view/config/at-model-heading-damping"));
                    double  damping_pitch = legacy_damping_time(config->getDoubleValue("view/config/at-model-pitch-damping"));
                    double  damping_roll = legacy_damping_time(config->getDoubleValue("view/config/at-model-roll-damping"));
                    
                    m_steps.add_step(new SviewStepRotate(
                            -globals->get_props()->getDoubleValue("sim/current-view/heading-offset-deg"),
                            -globals->get_props()->getDoubleValue("sim/current-view/pitch-offset-deg"),
                            globals->get_props()->getDoubleValue("sim/current-view/roll-offset-deg"),
                            damping_heading,
                            damping_pitch,
                            damping_roll
                            ));

                    m_steps.add_step(new SviewStepMouseDrag(
                            1 /*mouse_heading_scale*/,
                            -1 /*mouse_pitch_scale*/
                            ));
                    
                    /* Set current position as target. This isn't actually
                    necessary for this view because the direction implied by
                    heading/pitch/roll will still point to the centre of the
                    aircraft after the following steps. But it allows double
                    views to work better - they will use the centre of the
                    aircraft instead of the eye position. */
                    m_steps.add_step(new SviewStepCopyToTarget);
                    
                    /* Add step that moves eye away from aircraft.
                    config/z-offset-m defaults to /sim/chase-distance-m (see
                    fgdata:defaults.xml) which is -ve, e.g. -25m. */
                    m_steps.add_step(new SviewStepMove(
                            config->getDoubleValue("view/config/z-offset-m"),
                            -config->getDoubleValue("view/config/y-offset-m"),
                            config->getDoubleValue("view/config/x-offset-m")
                            ));
                    
                    /* Finally add a step that converts
                    lat,lon,height,heading,pitch,roll into SGVec3d position and
                    SGQuatd orientation. */
                    m_steps.add_step(new SviewStepFinal);
                }
                else {
                    /* E.g. pilot view.

                    Add steps to move to the pilot's eye position.

                    config/z-offset-m seems to be +ve when moving backwards
                    relative to the aircraft, so we need to negate the value we
                    pass to SviewStepMove(). */
                    m_steps.m_name = std::string() + "legacy pilot" + callsign_desc;
                    m_steps.add_step(new SviewStepAircraft(callsign));
                    m_steps.add_step(new SviewStepMove(
                            -config->getDoubleValue("view/config/z-offset-m"),
                            -config->getDoubleValue("view/config/y-offset-m"),
                            -config->getDoubleValue("view/config/x-offset-m")
                            ));
                    
                    double current_heading_offset = globals->get_props()->getDoubleValue("sim/current-view/heading-offset-deg");
                    double current_pitch_offset   = globals->get_props()->getDoubleValue("sim/current-view/pitch-offset-deg");
                    double current_roll_offset    = globals->get_props()->getDoubleValue("sim/current-view/roll-offset-deg");
                    /*double default_heading_offset = config->getDoubleValue("view/config/heading-offset-deg");
                    double default_pitch_offset   = config->getDoubleValue("view/config/pitch-offset-deg");
                    double default_roll_offset    = config->getDoubleValue("view/config/roll-offset-deg");*/
                    
                    /* Apply final rotation. */
                    m_steps.add_step(new SviewStepRotate(
                            current_heading_offset,
                            current_pitch_offset,
                            current_roll_offset
                            ));

                    m_steps.add_step(new SviewStepMouseDrag(
                            1 /*mouse_heading_scale*/,
                            1 /*mouse_pitch_scale*/
                            ));
                    m_steps.add_step(new SviewStepFinal);
                    SG_LOG(SG_VIEW, SG_INFO, "m_steps=" << m_steps);
                }
            }
        }
        else
        {
            /* New-style Sview specification, where each step is specified for
            us. */
            simgear::PropertyList steps = config->getChildren("step");
            if (steps.empty()) {
                throw std::runtime_error(std::string() + "No steps specified");
            }
            for (SGPropertyNode* step: steps) {
                const char* type = step->getStringValue("type");
                if (0) {}
                else if (!strcmp(type, "aircraft")) {
                    std::string callsign = step->getStringValue("callsign");
                    m_steps.add_step(new SviewStepAircraft(callsign));
                    if (callsign != "") {
                        m_steps.m_name += " callsign=" + callsign;
                    }
                }
                else if (!strcmp(type, "move")) {
                    m_steps.add_step(new SviewStepMove(
                            step->getDoubleValue("forward"),
                            step->getDoubleValue("up"),
                            step->getDoubleValue("right")
                            ));
                }
                else if (!strcmp(type, "direction-multiply")) {
                    m_steps.add_step(new SviewStepDirectionMultiply(
                            step->getDoubleValue("heading"),
                            step->getDoubleValue("pitch"),
                            step->getDoubleValue("roll")
                            ));
               }
               else if (!strcmp(type, "copy-to-target")) {
                    m_steps.add_step(new SviewStepCopyToTarget);
                }
                else if (!strcmp(type, "nearest-tower")) {
                    std::string callsign = step->getStringValue("callsign");
                    m_steps.add_step(new SviewStepNearestTower(callsign));
                    m_steps.m_name += " tower";
                    if (callsign != "") m_steps.m_name += " callsign=" + callsign;
                }
                else if (!strcmp(type, "rotate")) {
                    m_steps.add_step(new SviewStepRotate(
                            step->getDoubleValue("heading"),
                            step->getDoubleValue("pitch"),
                            step->getDoubleValue("roll"),
                            step->getDoubleValue("damping-heading"),
                            step->getDoubleValue("damping-pitch"),
                            step->getDoubleValue("damping-roll")
                            ));
                }
                else if (!strcmp(type, "rotate-current-view")) {
                    m_steps.add_step(new SviewStepRotate(
                            globals->get_props()->getDoubleValue("heading"),
                            globals->get_props()->getDoubleValue("pitch"),
                            globals->get_props()->getDoubleValue("roll")
                            ));
                }
                else if (!strcmp(type, "mouse-drag"))
                {
                    m_steps.add_step(new SviewStepMouseDrag(
                            step->getDoubleValue("heading-scale", 1),
                            step->getDoubleValue("pitch-scale", 1)
                            ));
                }
                else if (!strcmp(type, "double")) {
                    m_steps.add_step(new SviewStepDouble(step));
                    m_steps.m_name += " double";
                }
                else if (!strcmp(type, "agl")) {
                    const char* callsign = step->getStringValue("callsign");
                    double damping_time = step->getDoubleValue("damping-time");
                    m_steps.add_step(new SviewStepAGL(callsign, damping_time));
                }
                else {
                    throw std::runtime_error(std::string() + "Unrecognised step name: '" + type + "'");
                }
            }
            m_steps.add_step(new SviewStepFinal);
        }
        SG_LOG(SG_VIEW, SG_DEBUG, "m_steps=" << m_steps);
    }

    /* Construct a double view using two step sequences. */
    SviewViewEyeTarget(
            osgViewer::View* view,
            SGPropertyNode* config,
            SviewSteps& a,
            SviewSteps& b
            )
    :
    SviewView(view)
    {
        const char* type = config->getStringValue("type");
        if (!type) {
            throw std::runtime_error("double-type not specified");
        }
        if (!strcmp(type, "last_pair") || !strcmp(type, "last_pair_double"))
        {
            /* Copy steps from <b> that will set .target. */
            for (auto step: b.m_steps)
            {
                if (0
                        || dynamic_cast<SviewStepCopyToTarget*>(step.get())
                        || dynamic_cast<SviewStepFinal*>(step.get())
                        )
                {
                    break;
                }
                m_steps.add_step(step);
            }
            m_steps.add_step(new SviewStepCopyToTarget);

            /* Copy steps from <a> that will set .position. */
            for (auto step: a.m_steps)
            {
                if (0
                        || dynamic_cast<SviewStepCopyToTarget*>(step.get())
                        || dynamic_cast<SviewStepFinal*>(step.get())
                        )
                {
                    break;
                }
                m_steps.add_step(step);
            }
            
            if (!strcmp(type, "last_pair_double"))
            {
                /* We need a final SviewStepDouble step. */
                m_steps.add_step(new SviewStepDouble);
                m_steps.m_name = std::string() + " double: " + a.m_name + " - " + b.m_name;
            }
            else
            {
                m_steps.m_name = std::string() + " pair: " + a.m_name + " - " + b.m_name;
            }
            m_steps.add_step(new SviewStepFinal);
            SG_LOG(SG_VIEW, SG_INFO, "    m_steps:" << m_steps);
        }
        else {
            throw std::runtime_error(std::string("Unrecognised double view: ") + type);
        }
            
        /* Disable our mouse_drag() method - doesn't make sense for double views. */
        m_mouse_drag = false;
        SG_LOG(SG_VIEW, SG_DEBUG, "m_steps=" << m_steps);
    }

    const std::string description() override
    {
        return m_steps.m_name;
    }
    
    bool update(double dt) override
    {
        bool valid = m_osg_view->getCamera()->getGraphicsContext()->valid();
        SG_LOG(SG_VIEW, SG_BULK, "valid=" << valid);
        if (!valid) return false;
        
        SviewPosDir posdir;
        bool debug = false;
        if (m_debug) {
            time_t t = time(NULL) / 10;
            if (t != m_debug_time) {
                m_debug_time = t;
                debug = true;
            }
        }
        if (debug) SG_LOG(SG_VIEW, SG_INFO, "evaluating m_steps:");
        m_steps.evaluate(posdir, dt, debug);

        posdir_to_view(posdir);
        return true;
    }
    
    void mouse_drag(double delta_x_deg, double delta_y_deg) override
    {
        if (!m_mouse_drag)  return;
        for (auto step: m_steps.m_steps) {
            step->mouse_drag(delta_x_deg, delta_y_deg);
        }
    }
    
    SviewSteps          m_steps;
    bool                m_mouse_drag = true;
    bool                m_debug = false;
    time_t              m_debug_time = 0;
};


/* All our view windows. */
static std::vector<std::shared_ptr<SviewView>>  s_views;

/* Recent views, for use by SviewAddLastPair(). */
static std::deque<std::shared_ptr<SviewViewEyeTarget>>    s_recent_views;


/* Returns an independent property tree that defines an Sview that is a copy of
the current view. */
static SGPropertyNode_ptr SviewConfigForCurrentView()
{
    SGPropertyNode_ptr  config = new SGPropertyNode;
    int view_number_raw = globals->get_props()->getIntValue("/sim/current-view/view-number-raw");
    config->setIntValue("view-number-raw", view_number_raw);
    SGPropertyNode* global_view = globals->get_props()->getNode("/sim/view", view_number_raw /*index*/, true /*create*/);
    std::string root_path = global_view->getStringValue("config/root");   /* "" or /ai/models/multiplayer[]. */
    SGPropertyNode* root = globals->get_props()->getNode(root_path);
    std::string callsign = root->getStringValue("callsign");
    
    config->setStringValue("type", "legacy");
    config->setStringValue("callsign", callsign);
    SGPropertyNode* config_view = config->getNode("view", true /*create*/);
    if (callsign == "") {
        /* User aircraft. */
        copyProperties(globals->get_props()->getNode("/sim/view", view_number_raw), config_view);
    }
    else {
        /* Multiplayer aircraft. */
        copyProperties(root->getNode("set/sim/view", view_number_raw), config_view);
    }
    
    config->setDoubleValue(
            "direction-delta/heading",
            globals->get_props()->getDoubleValue("sim/current-view/heading-offset-deg")
            );
    config->setDoubleValue(
            "direction-delta/pitch",
            globals->get_props()->getDoubleValue("sim/current-view/pitch-offset-deg")
            );
    config->setDoubleValue(
            "direction-delta/roll",
            globals->get_props()->getDoubleValue("sim/current-view/roll-offset-deg")
            );
    
    config->setDoubleValue("zoom-delta", 1);
    
    SG_LOG(SG_VIEW, SG_INFO, "returning:\n" << writePropertiesInline(config, true /*write_all*/));
    return config;
}


void SviewPush()
{
    if (s_recent_views.size() >= 2) {
        s_recent_views.pop_front();
    }
    /* Make a dummy view whose eye and target members will copy the current
    view. */
    SGPropertyNode_ptr config = SviewConfigForCurrentView();
    std::shared_ptr<SviewViewEyeTarget> v(new SviewViewEyeTarget(nullptr /*view*/, config));
    s_recent_views.push_back(v);
    SG_LOG(SG_VIEW, SG_DEBUG, "Have pushed view: " << v);
}


void SviewUpdate(double dt)
{
    bool verbose = 0;
    for (size_t i=0; i<s_views.size(); /* incremented in loop*/) {
        if (verbose) {
            SG_LOG(SG_VIEW, SG_INFO, "updating i=" << i
                    << ": " << s_views[i]->m_osg_view
                    << ' ' << s_views[i]->m_compositor
                    );
        }
        bool valid = s_views[i]->update(dt);
        if (valid) {
            const osg::Matrix& view_matrix = s_views[i]->m_osg_view->getCamera()->getViewMatrix();
            const osg::Matrix& projection_matrix = s_views[i]->m_osg_view->getCamera()->getProjectionMatrix();
            s_views[i]->m_compositor->update(view_matrix, projection_matrix);
            i += 1;
        }
        else {
            /* Window has been closed.

            This doesn't work reliably with OpenSceneGraph-3.4. We seem to
            sometimes get valid=false when a different window from s_views[i]
            is closed. And sometimes OSG seems to end up sending a close-window
            even to the main window when the user has closed an sview window.
            */
            auto view_it = s_views.begin() + i;
            SG_LOG(SG_VIEW, SG_INFO, "deleting SviewView i=" << i << ": " << (*view_it)->description2());
            for (size_t j=0; j<s_views.size(); ++j) {
                SG_LOG(SG_VIEW, SG_INFO, "    " << j
                        << ": " << s_views[j]->m_osg_view
                        << ' ' << s_views[j]->m_compositor
                        );
            }
            s_views.erase(view_it);
            
            for (size_t j=0; j<s_views.size(); ++j) {
                SG_LOG(SG_VIEW, SG_INFO, "    " << j
                        << ": " << s_views[j]->m_osg_view
                        << ' ' << s_views[j]->m_compositor
                        );
            }
            verbose = true;
        }
    }
}

void SviewClear()
{
    s_views.clear();
}


/* These are set by SViewSetCompositorParams(). */
static osg::ref_ptr<simgear::SGReaderWriterOptions> s_compositor_options;
static std::string s_compositor_path;


struct EventHandler : osgGA::EventHandler
{
    bool handle(osgGA::Event* event, osg::Object* object, osg::NodeVisitor* nv) override
    {
        osgGA::GUIEventAdapter* ea = event->asGUIEventAdapter();
        if (ea) {
            osgGA::GUIEventAdapter::EventType et = ea->getEventType();
            if (et != osgGA::GUIEventAdapter::FRAME) {
                SG_LOG(SG_GENERAL, SG_BULK, "sview event handler called. ea->getEventType()=" << ea->getEventType());
            }
        }
        else {
            SG_LOG(SG_GENERAL, SG_BULK, "sview event handler called...");
        }
        return true;
    }
};

#include "Viewer/FGEventHandler.hxx"

std::shared_ptr<SviewView> SviewCreate(SGPropertyNode* config)
{    
    assert(config);
    
    FGRenderer* renderer = globals->get_renderer();
    osgViewer::ViewerBase* viewer_base = renderer->getViewerBase();
    osgViewer::CompositeViewer* composite_viewer = dynamic_cast<osgViewer::CompositeViewer*>(viewer_base);
    if (!composite_viewer) {
        SG_LOG(SG_GENERAL, SG_ALERT, "SviewCreate() doing nothing because not composite-viewer mode not enabled.");
        return nullptr;
    }

    osgViewer::View* main_view = renderer->getView();
    osg::Node* scene_data = main_view->getSceneData();
    
    SG_LOG(SG_GENERAL, SG_DEBUG, "main_view->getNumSlaves()=" << main_view->getNumSlaves());

    osgViewer::View* view = new osgViewer::View();
    flightgear::FGEventHandler* event_handler = globals->get_renderer()->getEventHandler();
    view->addEventHandler(event_handler);
    
    std::shared_ptr<SviewView>  sview_view;
    
    const char* type = config->getStringValue("type");
    SG_LOG(SG_VIEW, SG_DEBUG, "type=" << type);
    if (0) {
    }
    else if (!strcmp(type, "current")) {
        SGPropertyNode_ptr config2 = SviewConfigForCurrentView();
        copyProperties(config, config2);
        config2->setStringValue("type", "legacy"); /* restore it after copy() sets to "current". */
        sview_view.reset(new SviewViewEyeTarget(view, config2));
    }
    else if (!strcmp(type, "last_pair")) {
        if (s_recent_views.size() < 2) {
            SG_LOG(SG_VIEW, SG_ALERT, "Need two pushed views");
            return nullptr;
        }
        auto it = s_recent_views.end();
        std::shared_ptr<SviewViewEyeTarget> target = *(--it);
        std::shared_ptr<SviewViewEyeTarget> eye    = *(--it);
        sview_view.reset(new SviewViewEyeTarget(view, config, eye->m_steps, target->m_steps));
    }
    else if (!strcmp(type, "last_pair_double")) {
        if (s_recent_views.size() < 2) {
            SG_LOG(SG_VIEW, SG_ALERT, "Need two pushed views");
            return nullptr;
        }
        auto it = s_recent_views.end();
        std::shared_ptr<SviewViewEyeTarget> remote = *(--it);
        std::shared_ptr<SviewViewEyeTarget> local    = *(--it);
        sview_view.reset(new SviewViewEyeTarget(view, config, local->m_steps, remote->m_steps));
    }
    else {
        SG_LOG(SG_VIEW, SG_ALERT, "config is:\n" << writePropertiesInline(config, true /*write_all*/));
        sview_view.reset(new SviewViewEyeTarget(view, config));
    }
    
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    osg::ref_ptr<osg::GraphicsContext> gc;
    
    /* When we implement canvas views, we won't create a new window here. */
    if (1) {
        /* Create a new window. */
        
        osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
        assert(wsi);
        flightgear::WindowBuilder* window_builder = flightgear::WindowBuilder::getWindowBuilder();
        flightgear::GraphicsWindow* main_window = window_builder->getDefaultWindow();
        osg::ref_ptr<osg::GraphicsContext> main_gc = main_window->gc;
        const osg::GraphicsContext::Traits* main_traits = main_gc->getTraits();

        /* Arbitrary initial position of new window. */
        traits->x = config->getIntValue("window-x", 100);
        traits->y = config->getIntValue("window-y", 100);

        /* We set new window size as fraction of main window. This keeps the
        aspect ratio of new window same as that of the main window which allows
        us to use main window's projection matrix directly. Presumably we could
        calculate a suitable projection matrix to match an arbitrary aspect
        ratio, but i don't know the maths well enough to do this. */
        traits->width  = config->getIntValue("window-width", main_traits->width / 2);
        traits->height = config->getIntValue("window-height", main_traits->height / 2);
        traits->windowDecoration = true;
        traits->doubleBuffer = true;
        traits->sharedContext = 0;

        traits->readDISPLAY();
        if (traits->displayNum < 0) traits->displayNum = 0;
        if (traits->screenNum < 0)  traits->screenNum = 0;

        int bpp = fgGetInt("/sim/rendering/bits-per-pixel");
        int cbits = (bpp <= 16) ?  5 :  8;
        int zbits = (bpp <= 16) ? 16 : 24;
        traits->red = traits->green = traits->blue = cbits;
        traits->depth = zbits;

        traits->mipMapGeneration = true;
        traits->windowName = "Flightgear " + sview_view->description2();
        traits->sampleBuffers = fgGetInt("/sim/rendering/multi-sample-buffers", traits->sampleBuffers);
        traits->samples = fgGetInt("/sim/rendering/multi-samples", traits->samples);
        traits->vsync = fgGetBool("/sim/rendering/vsync-enable", traits->vsync);
        traits->stencil = 8;

        gc = osg::GraphicsContext::createGraphicsContext(traits);
        assert(gc.valid());
    }

    /* need to ensure that the window is cleared make sure that the complete
    window is set the correct colour rather than just the parts of the window
    that are under the camera's viewports. */
    //gc->setClearColor(osg::Vec4f(0.2f,0.2f,0.6f,1.0f));
    //gc->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    view->setSceneData(scene_data);
    view->setDatabasePager(FGScenery::getPagerSingleton());
        
    /* https://www.mail-archive.com/osg-users@lists.openscenegraph.org/msg29820.html
    Passing (false, false) here seems to cause a hang on startup. */
    view->getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, false);
    osg::GraphicsContext::createNewContextID();
    
    osg::Camera* main_camera = main_view->getCamera();
    osg::Camera* camera = view->getCamera();
    camera->setGraphicsContext(gc.get());

    if (0) {
        /* Show the projection matrix. */
        double left;
        double right;
        double bottom;
        double top;
        double zNear;
        double zFar;
        auto projection_matrix = main_camera->getProjectionMatrix();
        bool ok = projection_matrix.getFrustum(left, right, bottom, top, zNear, zFar);
        SG_LOG(SG_GENERAL, SG_ALERT, "projection_matrix:"
                << " ok=" << ok
                << " left=" << left
                << " right=" << right
                << " bottom=" << bottom
                << " top=" << top
                << " zNear=" << zNear
                << " zFar=" << zFar
                );
    }
    
    camera->setProjectionMatrix(main_camera->getProjectionMatrix());
    camera->setViewMatrix(main_camera->getViewMatrix());
    camera->setCullMask(0xffffffff);
    camera->setCullMaskLeft(0xffffffff);
    camera->setCullMaskRight(0xffffffff);
    
    /* This appears to avoid unhelpful culling of nearby objects. Though the
    above SG_LOG() says zNear=0.1 zFar=120000, so not sure what's going on. */
    camera->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);
    
    /*
    from CameraGroup::buildGUICamera():
    camera->setInheritanceMask(osg::CullSettings::ALL_VARIABLES
                               & ~(osg::CullSettings::COMPUTE_NEAR_FAR_MODE
                                   | osg::CullSettings::CULLING_MODE
                                   | osg::CullSettings::CLEAR_MASK
                                   ));
    camera->setCullingMode(osg::CullSettings::NO_CULLING);
    

    main_viewport seems to be null so this doesn't work.
    osg::Viewport* main_viewport = main_view->getCamera()->getViewport();
    SG_LOG(SG_GENERAL, SG_ALERT, "main_viewport=" << main_viewport);
    
    osg::Viewport* viewport = new osg::Viewport(*main_viewport);
    
    view->getCamera()->setViewport(viewport);
    */
    view->getCamera()->setViewport(0, 0, traits->width, traits->height);
    
    view->setName("Cloned view");
    
    view->setFrameStamp(composite_viewer->getFrameStamp());
    
    simgear::compositor::Compositor* compositor = simgear::compositor::Compositor::create(
            view,
            gc,
            view->getCamera()->getViewport(),
            s_compositor_path,
            s_compositor_options
            );
    
    sview_view->m_compositor = compositor;
    s_views.push_back(sview_view);
    
    /* stop/start threading:
    https://www.mail-archive.com/osg-users@lists.openscenegraph.org/msg54341.html
    */
    composite_viewer->stopThreading();
    composite_viewer->addView(view);
    composite_viewer->startThreading();
    
    SG_LOG(SG_GENERAL, SG_DEBUG, "main_view->getNumSlaves()=" << main_view->getNumSlaves());
    SG_LOG(SG_GENERAL, SG_DEBUG, "view->getNumSlaves()=" << view->getNumSlaves());
    
    SG_LOG(SG_VIEW, SG_DEBUG, "have added extra view. views are now:");
    for (unsigned i=0; i<composite_viewer->getNumViews(); ++i) {
        osgViewer::View* view = composite_viewer->getView(i);
        SG_LOG(SG_VIEW, SG_DEBUG, "composite_viewer view i=" << i << " view=" << view);
    }
    
    return sview_view;
}

std::shared_ptr<SviewView> SviewCreate(const SGPropertyNode* config)
{
    return SviewCreate(const_cast<SGPropertyNode*>(config));
}

simgear::compositor::Compositor* SviewGetEventViewport(const osgGA::GUIEventAdapter& ea)
{
    const osg::GraphicsContext* gc = ea.getGraphicsContext();
    for (std::shared_ptr<SviewView> sview_view: s_views) {
        if (sview_view->m_compositor->getGraphicsContext() == gc) {
            return sview_view->m_compositor;
        }
    }
    return nullptr;
}

void SViewSetCompositorParams(
        osg::ref_ptr<simgear::SGReaderWriterOptions> options,
        const std::string& compositor_path
        )
{
    s_compositor_options = options;
    s_compositor_path = compositor_path;
}

bool SviewMouseMotion(int x, int y, const osgGA::GUIEventAdapter& ea)
{
    const osg::GraphicsContext* gc = ea.getGraphicsContext();
    std::shared_ptr<SviewView>  sview_view;
    for (auto v: s_views) {
        if (v->m_compositor->getGraphicsContext() == gc) {
            sview_view = v;
            break;
        }
    }
    if (!sview_view) {
        return false;
    }
    double xx;
    double yy;
    if (!flightgear::eventToWindowCoords(&ea, xx, yy)) {
        return false;
    }

    SG_LOG(SG_GENERAL, SG_DEBUG, "sview_view=" << sview_view.get() << " xx=" << xx << " yy=" << yy);
    bool button2 = globals->get_props()->getBoolValue("/devices/status/mice/mouse/button[2]");
    if (button2 && sview_view->m_mouse_button2) {
        /* Button2 drag. */
        double  delta_x = xx - sview_view->m_mouse_x;
        double  delta_y = yy - sview_view->m_mouse_y;
        if (delta_x || delta_y) {
            /* Convert delta (which is mouse movement in pixels) to a change
            in viewing angle in degrees. We do this using the fov and window
            size so that the result is the image moving as though it was being
            panned by the mouse movement. So angle changes are smaller for
            high zoom values or small windows, making high zoom views easy to
            control. */
            osg::Camera* camera = sview_view->m_osg_view->getCamera();
            double fov_y;
            double aspect_ratio;
            double z_near;
            double z_far;
            camera->getProjectionMatrixAsPerspective(fov_y, aspect_ratio, z_near, z_far);
            double fov_x = fov_y * aspect_ratio;
            
            simgear::compositor::Compositor*    compositor = sview_view->m_compositor;
            osg::Viewport*                      viewport = compositor->getViewport();
            double delta_x_deg = delta_x / viewport->width() * fov_x;
            double delta_y_deg = delta_y / viewport->height() * fov_y;
            
            /* Scale movement a little to make things more similar to normal
            operation. */
            double scale = 5;
            delta_x_deg *= scale;
            delta_y_deg *= scale;
            sview_view->mouse_drag(delta_x_deg, delta_y_deg);
        }
    }
    sview_view->m_mouse_button2 = button2;
    sview_view->m_mouse_x = xx;
    sview_view->m_mouse_y = yy;
    
    return true;
}
