/*
Implementation of 'step' view system.

The basic idea here is calculate view position and direction by a series
of explicit steps. Steps can move to the origin of the user aircraft or a
multiplayer aircraft, or modify the current position by a fixed vector (e.g.
to move from aircraft origin to the pilot's eyepoint), or rotate the current
direction by a fixed transformation etc. We can also have a step that sets the
direction to point to a previously-calculated position.

This is similar to what is already done by the View code, but making the
individual steps explicit gives us more flexibility.

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
#include <Viewer/renderer.hxx>
#include <Viewer/WindowBuilder.hxx>
#include <Viewer/WindowSystemAdapter.hxx>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/viewer/Compositor.hxx>

#include <osgViewer/CompositeViewer>
#include <osg/GraphicsContext>


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
    SGGeod  position;
    double  heading;
    double  pitch;
    double  roll;
    
    /* Only used by SviewStepTarget; usually will be from a previously
    evaluated Sview. */
    SGGeod  target;
    
    /* The final position and direction, in a form suitable for setting an
    osg::Camera's view matrix. */
    SGVec3d position2;
    SGQuatd direction2;
    
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


/* Abstract base class for a single view step. A view step modifies a
SviewPosDir, e.g. translating the position and/or rotating the direction. */
struct SviewStep
{
    /* Updates <posdir>. */
    virtual void evaluate(SviewPosDir& posdir) = 0;
    
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


/* A step that sets position to aircraft origin and direction to aircraft's
longitudal axis. */
struct SviewStepAircraft : SviewStep
{
    /* For the user aircraft, <root> should be /. For a multiplayer aircraft,
    it should be /ai/models/multiplayer[]. */
    SviewStepAircraft(SGPropertyNode* root)
    {
        /* todo: set up a listener or something so that we cope when
        multiplayer callsign disapears then reappears in a different index of
        /ai/models/multiplayer[]. */
        m_longitude     = root->getNode("position/longitude-deg");
        m_latitude      = root->getNode("position/latitude-deg");
        m_altitude      = root->getNode("position/altitude-ft");
        m_heading       = root->getNode("orientation/true-heading-deg");
        m_pitch         = root->getNode("orientation/pitch-deg");
        m_roll          = root->getNode("orientation/roll-deg");
        m_description   = root->getStringValue("callsign");
        if (m_description == "") {
            m_description = "User aircraft";
        }
        SG_LOG(SG_VIEW, SG_ALERT, "m_longitude->getPath()=" << m_longitude->getPath());
    }
    
    void evaluate(SviewPosDir& posdir) override
    {
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
        out << " <SviewStepAircraft>";
    }
    
    private:
    
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
    m_offset(right, up, forward)
    {
        SG_LOG(SG_VIEW, SG_INFO, "forward=" << forward << " up=" << up << " right=" << right);
    }
    
    void evaluate(SviewPosDir& posdir) override
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

/* Modifies heading, pitch and roll by fixed amounts; does not change position.

E.g. can be used to preserve direction (relative to aircraft) of Helicopter
view at the time it was cloned. */
struct SviewStepRotate : SviewStep
{
    SviewStepRotate(double heading=0, double pitch=0, double roll=0)
    :
    m_heading(heading),
    m_pitch(pitch),
    m_roll(roll)
    {
        SG_LOG(SG_VIEW, SG_INFO, "heading=" << heading << " pitch=" << pitch << " roll=" << roll);
    }
    
    void evaluate(SviewPosDir& posdir) override
    {
        /* Should we use SGQuatd to evaluate things? */
        posdir.heading += -m_heading;
        posdir.pitch += -m_pitch;
        posdir.roll += m_roll;
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
    
    void evaluate(SviewPosDir& posdir) override
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

/* Copies current position to posdir.target. Used by SviewEyeTarget() to make
current position be available as target later on, e.g. by SviewStepTarget. */
struct SviewStepCopyToTarget : SviewStep
{
    void evaluate(SviewPosDir& posdir) override
    {
        posdir.target = posdir.position;
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepCopyToTarget>";
    }
};


/* Move position to nearest tower. */
struct SviewStepNearestTower : SviewStep
{
    /* For user aircraft <sim> should be /sim; for multiplayer aircraft it
    should be ai/models/multiplayer[]/sim. */
    SviewStepNearestTower(SGPropertyNode* sim)
    :
    m_latitude(sim->getNode("tower/latitude-deg")),
    m_longitude(sim->getNode("tower/longitude-deg")),
    m_altitude(sim->getNode("tower/altitude-ft"))
    {
        m_description = "Nearest tower";
    }
    
    void evaluate(SviewPosDir& posdir) override
    {
        posdir.position = SGGeod::fromDegFt(
                m_longitude->getDoubleValue(),
                m_latitude->getDoubleValue(),
                m_altitude->getDoubleValue()
                );
        SG_LOG(SG_VIEW, SG_BULK, "moved posdir.postion to: " << posdir.position);
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepNearestTower>"
                << ' ' << m_latitude
                << ' ' << m_longitude
                << ' ' << m_altitude
                ;
    }
    
    SGPropertyNode_ptr  m_latitude;
    SGPropertyNode_ptr  m_longitude;
    SGPropertyNode_ptr  m_altitude;
};


/* Rotates view direction to point at a previously-calculated target. */
struct SviewStepTarget : SviewStep
{
    /* Alters posdir.direction to point to posdir.target. */
    void evaluate(SviewPosDir& posdir) override
    {
        /* See View::recalcLookAt(). */
        
        SGQuatd geodEyeOr = SGQuatd::fromYawPitchRollDeg(posdir.heading, posdir.pitch, posdir.roll);
        SGQuatd geodEyeHlOr = SGQuatd::fromLonLat(posdir.position);

        SGQuatd ec2eye = geodEyeHlOr*geodEyeOr;
        SGVec3d eyeCart = SGVec3d::fromGeod(posdir.position);

        SGVec3d atCart = SGVec3d::fromGeod(posdir.target);

        /* add target offsets to at_position...
        Compute the eyepoints orientation and position wrt the earth centered
        frame - that is global coorinates _absolute_view_pos = eyeCart; */

        /* the view direction. */
        SGVec3d dir = normalize(atCart - eyeCart);
        
        /* the up directon. */
        SGVec3d up = ec2eye.backTransform(SGVec3d(0, 0, -1));
        
        /* rotate -dir to the 2-th unit vector
        rotate up to 1-th unit vector
        Note that this matches the OpenGL camera coordinate system with
        x-right, y-up, z-back. */
        posdir.direction2 = SGQuatd::fromRotateTo(-dir, 2, up, 1);
        
        posdir.position2 = SGVec3d::fromGeod(posdir.position);
        
        SG_LOG(SG_VIEW, SG_BULK, "have set posdir.position2: " << posdir.position2);
        SG_LOG(SG_VIEW, SG_BULK, "have set posdir.direction2: " << posdir.direction2);
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepTarget>";
    }
    
};


/* Converts position, heading, pitch and roll into position2 and direction2,
which will be used to set the camera parameters. */
struct SviewStepFinal : SviewStep
{
    void evaluate(SviewPosDir& posdir) override
    {
        /* See View::recalcLookFrom(). */
        
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
        
        posdir.position2 = position;
        posdir.direction2 = ec2body * q;
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepFinal>";
    }
};


/* Contains a list of SviewStep's that are used to evaluate a SviewPosDir
(position and orientation)
*/
struct SviewSteps
{
    const std::string& description()
    {
        if (m_name == "") {
            if (!m_steps.empty()) {
                m_name = m_steps.front()->m_description;
            }
        }
        return m_name;
    }
    
    void add_step(std::shared_ptr<SviewStep> step)
    {
        m_steps.push_back(step);
    }
    
    void add_step(SviewStep* step)
    {
        return add_step(std::shared_ptr<SviewStep>(step));
    }
    
    void evaluate(SviewPosDir& posdir, bool debug=false)
    {
        if (debug) SG_LOG(SG_VIEW, SG_ALERT, "evaluating m_name=" << m_name);
        for (auto step: m_steps) {
            step->evaluate(posdir);
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


static void ShowViews();


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
    const std::string&  description2()
    {
        if (m_description2 == "") {
            char    buffer[32];
            snprintf(buffer, sizeof(buffer), "[%i] ", s_id);
            m_description2 = buffer + description();
        }
        return m_description2;
    }
    
    /* Description of this view, used in window title etc. */
    virtual const std::string& description() = 0;
    
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
            SG_LOG(SG_VIEW, SG_ALERT, "composite_viewer view i=" << i << " view=" << view);
        }
        //ShowViews();  /* unsafe because we are being removed from s_views. */
        SG_LOG(SG_VIEW, SG_ALERT, "removing m_osg_view=" << m_osg_view);
        composite_viewer->stopThreading();
        composite_viewer->removeView(m_osg_view);
        composite_viewer->startThreading();
    }
    
    /* Returns false if window has been closed. */
    virtual bool update(double dt) = 0;
    
    /* Sets this view's camera position/orientation from <posdir>. */
    void posdir_to_view(SviewPosDir posdir)
    {
        /* FGViewMgr::update(). */
        osg::Vec3d  position    = toOsg(posdir.position2);
        osg::Quat   orientation = toOsg(posdir.direction2);

        osg::Camera* camera = m_osg_view->getCamera();
        osg::Matrix old_m = camera->getViewMatrix();
        /* This calculation is copied from CameraGroup::update(). As of
        2020-10-10 we don't yet update the projection matrix so views cannot
        zoom. */
        const osg::Matrix new_m(
                osg::Matrix::translate(-position)
                * osg::Matrix::rotate(orientation.inverse())
                );
        SG_LOG(SG_VIEW, SG_BULK, "old_m: " << old_m);
        SG_LOG(SG_VIEW, SG_BULK, "new_m: " << new_m);
        camera->setViewMatrix(new_m);
    }
        
    osgViewer::View*                    m_osg_view = nullptr;
    simgear::compositor::Compositor*    m_compositor = nullptr;
    std::string                         m_description2;
    
    static int s_id;
};

int SviewView::s_id = 0;


/* A view which keeps two aircraft visible, with one at a constant distance in
the foreground.
*/
struct SviewDouble : SviewView
{
    /* <local> and <remote> should evaluate to position of local and remote
    aircraft. We ignore directions in <local> and <remote> - we are only
    interested in the two positions. */
    SviewDouble(
            osgViewer::View* view,
            const SviewSteps& steps_local,
            const SviewSteps& steps_remote,
            double local_chase_distance=25,
            double angle_deg=15
            )
    :
    SviewView(view),
    m_steps_local(steps_local),
    m_steps_remote(steps_remote),
    m_local_chase_distance(local_chase_distance),
    m_angle_rad(angle_deg * 3.1415926 / 180)
    {
        const std::string eye = m_steps_local.description();
        const std::string target = m_steps_remote.description();
        m_description = "Double view " + eye + " - " + target;
    }
    
    const std::string& description() override
    {
        return m_description;
    }
    
    virtual bool update(double dt) override
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
        
            EL is local aircraft's chase-distance (though at the moment we use
            a fixed value) so that local aircraft is in perfect view.
        
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
        
        bool valid = m_osg_view->getCamera()->getGraphicsContext()->valid();
        SG_LOG(SG_VIEW, SG_BULK, "valid=" << valid);
        if (!valid) return false;
        
        bool debug = false;
        if (0) {
            time_t t = time(NULL) / 10;
            //if (debug) SG_LOG(SG_VIEW, SG_ALERT, "m_debug_time=" << m_debug_time << " t=" << t);
            if (t != m_debug_time) {
                m_debug_time = t;
                debug = true;
            }
        }

        /* Find positions of local and remote aircraft. */
        SviewPosDir posdir_local;
        SviewPosDir posdir_remote;
        m_steps_local.evaluate(posdir_local);
        m_steps_remote.evaluate(posdir_remote);
        
        if (debug) {
            SG_LOG(SG_VIEW, SG_ALERT, "    m_steps_local: " << m_steps_local << ": " << posdir_local);
            SG_LOG(SG_VIEW, SG_ALERT, "    m_steps_remote: " << m_steps_remote << ": " << posdir_remote);
        }
        
        /* Create cartesian coordinates so we can calculate distance <lr>. */
        SGVec3d local_pos = SGVec3d::fromGeod(posdir_local.target);
        SGVec3d remote_pos = SGVec3d::fromGeod(posdir_remote.target);
        double lr = sqrt(distSqr(local_pos, remote_pos));
        const double pi = 3.1415926;
        
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
        auto move = SviewStepMove(-le, 0, 0);
        move.evaluate(posdir_local);
        
        /* At this point, posdir_local.position is eye position. We make
        posdir_local.direction2 point from this eye position to halfway between
        the remote and local aircraft. */
        double er_vertical = posdir_remote.target.getElevationM()
                - posdir_local.position.getElevationM();
        double her = asin(er_vertical / er);
        double hel = (hlr + rle) - pi;
        posdir_local.pitch = (her + hel) / 2 * 180 / pi;
        auto stepfinal = SviewStepFinal();
        stepfinal.evaluate(posdir_local);
        posdir_to_view(posdir_local);
        
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
        return true;
    }
    
    SviewSteps  m_steps_local;
    SviewSteps  m_steps_remote;
    double      m_local_chase_distance;
    double      m_angle_rad;
    
    std::string m_description;
    time_t      m_debug_time = 0;
};


/* A view defined by an eye and target. Used for clones of main window views.
*/
struct SviewViewEyeTarget : SviewView
{
    /* Construct as a clone of the current view in /sim/current-view/. We look
    in /sim/current-view/ to get view parameters, and from them we construct
    eye and target steps. */
    SviewViewEyeTarget(osgViewer::View* view)
    :
    SviewView(view)
    {
        SG_LOG(SG_VIEW, SG_INFO, "m_osg_view=" << m_osg_view);
        
        SGPropertyNode* global_view;
        SGPropertyNode* root;
        SGPropertyNode* sim;
        SGPropertyNode* view_config;
        int             view_number_raw;
        std::string     root_path;
        std::string     type;
        
        /* Unfortunately we need to look at things like
        /sim/view[]/config/eye-heading-deg-path, even when handling multiplayer
        aircraft. */
        view_number_raw = globals->get_props()->getIntValue("/sim/current-view/view-number-raw");
        global_view = globals->get_props()->getNode("/sim/view", view_number_raw /*index*/, true /*create*/);
        
        /* <root_path> is typically "" or /ai/models/multiplayer[]. */
        root_path   = global_view->getStringValue("config/root");
        type        = global_view->getStringValue("type");
        
        root = globals->get_props()->getNode(root_path);
        if (root_path == "" || root_path == "/") {
            /* user aircraft. */
            sim = root->getNode("sim");
        }
        else {
            /* Multiplayer sim is /ai/models/multiplayer[]/set/sim. */
            sim = root->getNode("set/sim");
        }
        
        {
            SGPropertyNode* view_node   = sim->getNode("view", view_number_raw, true /*create*/);
            view_config = view_node->getNode("config");
        }
        
        SG_LOG(SG_VIEW, SG_ALERT, "view_number_raw=" << view_number_raw);
        SG_LOG(SG_VIEW, SG_ALERT, "type=" << type);
        SG_LOG(SG_VIEW, SG_ALERT, "root_path=" << root_path);
        SG_LOG(SG_VIEW, SG_ALERT, "root=" << root);
        SG_LOG(SG_VIEW, SG_ALERT, "sim=" << sim);
        assert(root && sim);
        SG_LOG(SG_VIEW, SG_ALERT, "root->getPath()=" << root->getPath());
        SG_LOG(SG_VIEW, SG_ALERT, "sim->getPath()=" << sim->getPath());
        SG_LOG(SG_VIEW, SG_ALERT, "view_config->getPath()=" << view_config->getPath());
        
        if (view_config->getBoolValue("eye-fixed")) {
            /* E.g. Tower view. */
            SG_LOG(SG_VIEW, SG_INFO, "eye-fixed");
            
            /* Add a step to move to centre of aircraft. */
            m_target.add_step(new SviewStepAircraft(root));
            m_target.add_step(new SviewStepMove(
                    view_config->getDoubleValue("target-z-offset-m"),
                    view_config->getDoubleValue("target-y-offset-m"),
                    view_config->getDoubleValue("target-x-offset-m")
                    ));
            SG_LOG(SG_VIEW, SG_DEBUG, "m_target=" << m_target);
            
            /* Add a step to set pitch and roll to zero, otherwise view from
            tower (as calculated by SviewStepTarget) rolls/pitches with
            aircraft. */
            m_target.add_step(new SviewStepDirectionMultiply(
                    1 /* heading */,
                    0 /* pitch */,
                    0 /* roll */
                    ));
            SG_LOG(SG_VIEW, SG_DEBUG, "m_target=" << m_target);
            
            /* Current position is the target, so add a step that copies it to
            SviewPosDir.target. */
            m_target.add_step(new SviewStepCopyToTarget);
            
            /* Added steps to set .m_eye up so that it looks from the nearest
            tower. */
            m_eye.add_step(new SviewStepNearestTower(sim));
            m_eye.add_step(new SviewStepTarget);
            
            /* Add a step that moves towards the target a little to avoid eye
            being inside the tower walls.

            [At some point it might be good to make this movement not change
            the height too.] */
            m_eye.add_step(new SviewStepMove(30, 0, 0));
        }
        else {
            SG_LOG(SG_VIEW, SG_INFO, "not eye-fixed");
            
            /* E.g. Pilot view or Helicopter view. We assume eye position is
            relative to aircraft. */
            if (type == "lookat") {
                /* E.g. Helicopter view. Move to centre of aircraft.
                config/target-z-offset-m seems to be +ve when moving backwards
                relative to the aircraft, so we need to negate the value we
                pass to SviewStepMove(). */
                m_target.add_step(new SviewStepAircraft(root));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_target=" << m_target);
                
                m_target.add_step(new SviewStepMove(
                        view_config->getDoubleValue("target-z-offset-m"),
                        view_config->getDoubleValue("target-y-offset-m"),
                        view_config->getDoubleValue("target-x-offset-m")
                        ));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_target=" << m_target);
                
                m_target.add_step(new SviewStepCopyToTarget);
                
                m_eye.add_step(new SviewStepAircraft(root));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_eye=" << m_eye);
                
                m_eye.add_step(new SviewStepMove(
                        view_config->getDoubleValue("target-z-offset-m"),
                        view_config->getDoubleValue("target-y-offset-m"),
                        view_config->getDoubleValue("target-x-offset-m")
                        ));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_eye=" << m_eye);
                
                /* Add a step that crudely preserves or don't preserve
                aircraft's heading, pitch and roll; this enables us to mimic
                Helicopter and Chase views. In theory we should evaluate the
                specified paths, but in practise we only need to multiply
                current values by 0 or 1.

                todo: add damping. */
                m_eye.add_step(new SviewStepDirectionMultiply(
                        global_view->getStringValue("config/eye-heading-deg-path")[0] ? 1 : 0,
                        global_view->getStringValue("config/eye-pitch-deg-path")[0] ? 1 : 0,
                        global_view->getStringValue("config/eye-roll-deg-path")[0] ? 1 : 0
                        ));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_eye=" << m_eye);
                
                /* Add a step that applies the current view rotation. */
                m_eye.add_step(new SviewStepRotate(
                        root->getDoubleValue("sim/current-view/heading-offset-deg"),
                        root->getDoubleValue("sim/current-view/pitch-offset-deg"),
                        root->getDoubleValue("sim/current-view/roll-offset-deg")
                        ));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_eye=" << m_eye);
                
                /* Add step that moves eye away from aircraft.
                config/z-offset-m defaults to /sim/chase-distance-m (see
                fgdata:defaults.xml) which is -ve, e.g. -25m. */
                m_eye.add_step(new SviewStepMove(
                        -view_config->getDoubleValue("z-offset-m"),
                        -view_config->getDoubleValue("y-offset-m"),
                        -view_config->getDoubleValue("x-offset-m")
                        ));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_eye=" << m_eye);
            }
            else {
                /* E.g. pilot view.

                Add steps to move to the pilot's eye position.

                config/z-offset-m seems to be +ve when moving backwards
                relative to the aircraft, so we need to negate the value we
                pass to SviewStepMove(). */
                m_eye.add_step(new SviewStepAircraft(root));
                m_eye.add_step(new SviewStepMove(
                        view_config->getDoubleValue("z-offset-m"),
                        view_config->getDoubleValue("y-offset-m"),
                        view_config->getDoubleValue("x-offset-m")
                        ));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_eye=" << m_eye);
                
                /* Add a step to apply the current view rotation. E.g. on
                harrier-gr3 this corrects initial view direction (which is
                pitch down by 15deg).

                However this doesn't work properly - the cockpit rotates
                in a small circle when the aircraft rolls. This is because
                SviewStepRotate's simple application of heading/pitch/roll
                doesn't work if aircraft has non-zero roll. We need to use
                SGQuatd - see View::recalcLookFrom(). */
                m_eye.add_step(new SviewStepRotate(
                        -view_config->getDoubleValue("heading-offset-deg"),
                        -view_config->getDoubleValue("pitch-offset-deg"),
                        -view_config->getDoubleValue("roll-offset-deg")
                        ));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_eye=" << m_eye);
                
                /* Add a step to preserve the current user-supplied offset to
                view direction. */
                m_eye.add_step(new SviewStepRotate(
                        root->getDoubleValue("sim/current-view/heading-offset-deg")
                                - view_config->getDoubleValue("heading-offset-deg"),
                        -(root->getDoubleValue("sim/current-view/pitch-offset-deg")
                                - view_config->getDoubleValue("pitch-offset-deg")),
                        root->getDoubleValue("sim/current-view/roll-offset-deg")
                                - view_config->getDoubleValue("roll-offset-deg")
                        ));
                SG_LOG(SG_VIEW, SG_DEBUG, "m_eye=" << m_eye);
            }
            
            /* Finally add a step that converts
            lat,lon,height,heading,pitch,roll into SGVec3d position and SGQuatd
            orientation. */
            m_eye.add_step(new SviewStepFinal);
        }
        SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
        SG_LOG(SG_VIEW, SG_ALERT, "m_target=" << m_target);
        
        m_description = m_eye.description() + " - " + m_target.description();
    }
    
    const std::string& description() override
    {
        return m_description;
    }
    
    /* Construct from separate eye and target definitions. */
    SviewViewEyeTarget(
            osgViewer::View* view,
            const SviewSteps& eye,
            const SviewSteps& target
            )
    :
    SviewView(view),
    m_eye(eye),
    m_target(target)
    {
        m_target.add_step(new SviewStepCopyToTarget);
        m_eye.add_step(new SviewStepTarget);
        SG_LOG(SG_VIEW, SG_ALERT, "    m_eye:" << m_eye);
        SG_LOG(SG_VIEW, SG_ALERT, "    m_target:" << m_target);
        m_debug = true;
        SG_LOG(SG_VIEW, SG_ALERT, "    m_debug:" << m_debug);
        m_description = m_eye.description() + " - " + m_target.description();
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
            //if (debug) SG_LOG(SG_VIEW, SG_ALERT, "m_debug_time=" << m_debug_time << " t=" << t);
            if (t != m_debug_time) {
                m_debug_time = t;
                debug = true;
            }
        }
        if (debug) SG_LOG(SG_VIEW, SG_ALERT, "evaluating target:");
        m_target.evaluate(posdir, debug);
        if (debug) SG_LOG(SG_VIEW, SG_ALERT, "evaluating eye:");
        m_eye.evaluate(posdir, debug);

        posdir_to_view(posdir);
        return true;
    }
    
    SviewSteps          m_eye;
    SviewSteps          m_target;
    std::string         m_description;
    bool                m_debug = false;
    time_t              m_debug_time = 0;
};


/* All our view windows. */
static std::vector<std::shared_ptr<SviewView>>  s_views;

/* Recent views, for use by SviewAddLastPair(). */
static std::deque<std::shared_ptr<SviewViewEyeTarget>>    s_recent_views;


static void ShowViews()
{
    for (auto view: s_views) {
        if (!view) SG_LOG(SG_GENERAL, SG_ALERT, "    view is null");
        else {
            const osg::GraphicsContext* vgc = (view->m_compositor) ? view->m_compositor->getGraphicsContext() : nullptr;
            SG_LOG(SG_GENERAL, SG_ALERT, "    gc=" << vgc);
        }
    }
}


void SviewPush()
{
    if (s_recent_views.size() >= 2) {
        s_recent_views.pop_front();
    }
    /* Make a dummy view whose eye and target members will copy the current
    view. */
    std::shared_ptr<SviewViewEyeTarget> v(new SviewViewEyeTarget(nullptr /*view*/));
    s_recent_views.push_back(v);
    SG_LOG(SG_VIEW, SG_ALERT, "Have pushed view: " << v);
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


std::shared_ptr<SviewView> SviewCreate(const std::string& type)
{
    /*
    We create various things:
        An osgViewer::View, added to the global osgViewer::CompositeViewer.
        An osg::GraphicsContext (has associated top-level window).
        A simgear::compositor::Compositor.
    */

    FGRenderer* renderer = globals->get_renderer();
    osgViewer::ViewerBase* viewer_base = renderer->getViewerBase();
    osgViewer::CompositeViewer* composite_viewer = dynamic_cast<osgViewer::CompositeViewer*>(viewer_base);
    if (!composite_viewer) {
        SG_LOG(SG_GENERAL, SG_ALERT, "SviewCreate() doing nothing because not composite-viewer mode not enabled.");
        return nullptr;
    }

    SG_LOG(SG_GENERAL, SG_ALERT, "Creating new view type=" << type);
    
    osgViewer::View* main_view = renderer->getView();
    osg::Node* scene_data = main_view->getSceneData();
    
    SG_LOG(SG_GENERAL, SG_ALERT, "main_view->getNumSlaves()=" << main_view->getNumSlaves());

    osgViewer::View* view = new osgViewer::View();
    EventHandler* event_handler = new EventHandler;
    view->addEventHandler(event_handler);
    
    std::shared_ptr<SviewView>  sview_view;

    if (0) {}
    else if (type == "current") {
        sview_view.reset(new SviewViewEyeTarget(view));
    }
    else if (type == "last_pair") {
        if (s_recent_views.size() < 2) {
            SG_LOG(SG_VIEW, SG_ALERT, "Need two pushed views");
            return nullptr;
        }
        auto it = s_recent_views.end();
        std::shared_ptr<SviewViewEyeTarget> target = *(--it);
        std::shared_ptr<SviewViewEyeTarget> eye    = *(--it);
        if (!target || !eye) {
            SG_LOG(SG_VIEW, SG_ALERT, "target=" << target << " eye=" << eye);
            return nullptr;
        }
        sview_view.reset(new SviewViewEyeTarget(view, eye->m_eye, target->m_eye));
    }
    else if (type == "last_pair_double") {
        if (s_recent_views.size() < 2) {
            SG_LOG(SG_VIEW, SG_ALERT, "Need two pushed views");
            return nullptr;
        }
        auto it = s_recent_views.end();
        std::shared_ptr<SviewViewEyeTarget> remote = *(--it);
        std::shared_ptr<SviewViewEyeTarget> local    = *(--it);
        if (!local || !remote) {
            SG_LOG(SG_VIEW, SG_ALERT, "remote=" << local << " remote=" << remote);
            return nullptr;
        }
        sview_view.reset(new SviewDouble(view, local->m_target, remote->m_target));
    }
    else {
        SG_LOG(SG_GENERAL, SG_ALERT, "unrecognised type=" << type);
        return nullptr;
    }
    
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    osg::ref_ptr<osg::GraphicsContext> gc;
    
    if (1) {
        /* Create a new window. */
        
        osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
        assert(wsi);
        flightgear::WindowBuilder* window_builder = flightgear::WindowBuilder::getWindowBuilder();
        flightgear::GraphicsWindow* main_window = window_builder->getDefaultWindow();
        osg::ref_ptr<osg::GraphicsContext> main_gc = main_window->gc;
        const osg::GraphicsContext::Traits* main_traits = main_gc->getTraits();

        /* Arbitrary initial position of new window. */
        traits->x = 100;
        traits->y = 100;

        /* We set new window size as fraction of main window. This keeps the
        aspect ratio of new window same as that of the main window which allows
        us to use main window's projection matrix directly. Presumably we could
        calculate a suitable projection matrix to match an arbitrary aspect
        ratio, but i don't know the maths well enough to do this. */
        traits->width  = main_traits->width / 2;
        traits->height = main_traits->height / 2;
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
    
    SG_LOG(SG_GENERAL, SG_ALERT, "main_view->getNumSlaves()=" << main_view->getNumSlaves());
    SG_LOG(SG_GENERAL, SG_ALERT, "view->getNumSlaves()=" << view->getNumSlaves());
    
    SG_LOG(SG_VIEW, SG_ALERT, "have added extra view. views are now:");
    for (unsigned i=0; i<composite_viewer->getNumViews(); ++i) {
        osgViewer::View* view = composite_viewer->getView(i);
        SG_LOG(SG_VIEW, SG_ALERT, "composite_viewer view i=" << i << " view=" << view);
    }
    
    return sview_view;
}

void SViewSetCompositorParams(
        osg::ref_ptr<simgear::SGReaderWriterOptions> options,
        const std::string& compositor_path
        )
{
    s_compositor_options = options;
    s_compositor_path = compositor_path;
}
