/* Implementation of 'step' view system.

The basic idea here is calculate view position and direction by a series
of explicit steps. Steps can move to the origin of the user aircraft or a
multiplayer aircraft, or modify the current position by a fixed vector (e.g.
to move from aircraft origin to the pilot's eyepoint), or rotate the current
direction by a fixed transformation etc. We can also have a step that sets the
direction to point to a previously-calculated position.

This is similar to what is already done by the View code, but making the
individual steps explicit gives us more flexibility.

We also allow views to be defined and created at runtime instead of being
hard-coded in *-set.xml files. For example this would make it possible to
define a view from the user's aircraft's pilot to the centre of a multiplayer
aircraft (or to a multiplayer aircraft's pilot).

The dynamic nature of step views allows view cloning with composite-viewer.

*/     

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

#include "sview.hxx"


#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Viewer/renderer_compositor.hxx>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/OsgMath.hxx>

#include <osgViewer/CompositeViewer>


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


/* Position and direction. */
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
};

std::ostream& operator<< (std::ostream& out, const SviewPosDir& posdir)
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
};

std::ostream& operator<< (std::ostream& out, const SviewStep& step)
{
    out << ' ' << typeid(step).name();
    step.stream(out);
    return out;
}

/* Sets position to aircraft origin and direction to aircraft's longitudal
axis. */
struct SviewStepAircraft : SviewStep
{
    /* For the user aircraft, <root> should be /. For a multiplayer
    aircraft, it should be /ai/models/multiplayer[]. */
    SviewStepAircraft(SGPropertyNode* root)
    {
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
    m_offset(right, -up, -forward)
    {
        SG_LOG(SG_VIEW, SG_INFO, "forward=" << forward << " up=" << up << " right=" << right);
    }
    
    void evaluate(SviewPosDir& posdir) override
    {
        /* These calculations are copied from View::recalcLookFrom(). */
        
        // The rotation rotating from the earth centerd frame to
        // the horizontal local frame
        SGQuatd hlOr = SGQuatd::fromLonLat(posdir.position);
        
        // The rotation from the horizontal local frame to the basic view orientation
        SGQuatd hlToBody = SGQuatd::fromYawPitchRollDeg(posdir.heading, posdir.pitch, posdir.roll);
        
        // Compute the eyepoints orientation and position
        // wrt the earth centered frame - that is global coorinates
        SGQuatd ec2body = hlOr * hlToBody;
        
        // The cartesian position of the basic view coordinate
        SGVec3d position = SGVec3d::fromGeod(posdir.position);
        
        // This is rotates the x-forward, y-right, z-down coordinate system the where
        // simulation runs into the OpenGL camera system with x-right, y-up, z-back.
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

        // add target offsets to at_position...
        // Compute the eyepoints orientation and position
        // wrt the earth centered frame - that is global coorinates
        //_absolute_view_pos = eyeCart;

        // the view direction
        SGVec3d dir = normalize(atCart - eyeCart);
        // the up directon
        SGVec3d up = ec2eye.backTransform(SGVec3d(0, 0, -1));
        // rotate -dir to the 2-th unit vector
        // rotate up to 1-th unit vector
        // Note that this matches the OpenGL camera coordinate system
        // with x-right, y-up, z-back.
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
        
        // The rotation rotating from the earth centerd frame to
        // the horizontal local frame
        SGQuatd hlOr = SGQuatd::fromLonLat(posdir.position);

        // The rotation from the horizontal local frame to the basic view orientation
        SGQuatd hlToBody = SGQuatd::fromYawPitchRollDeg(posdir.heading, posdir.pitch, posdir.roll);

        // Compute the eyepoints orientation and position
        // wrt the earth centered frame - that is global coorinates
        SGQuatd ec2body = hlOr * hlToBody;

        // The cartesian position of the basic view coordinate
        SGVec3d position = SGVec3d::fromGeod(posdir.position);

        // This is rotates the x-forward, y-right, z-down coordinate system the where
        // simulation runs into the OpenGL camera system with x-right, y-up, z-back.
        SGQuatd q(-0.5, -0.5, 0.5, 0.5);
        
        posdir.position2 = position;
        posdir.direction2 = ec2body * q;
    }
    
    virtual void stream(std::ostream& out) const
    {
        out << " <SviewStepFinal>";
    }
};


struct SviewPos
{
    #if 0
    SviewPos(SGPropertyNode* node)
    {
        bool finished = false;
        m_name = node->getStringValue("name");
        simgear::PropertyList   children = node->getChildren("step");
        for (SGPropertyNode* child: children) {
            std::string type = child->getStringValue("type");
            std::shared_ptr<SviewStep> step;
            if (0) {}
            else if (type == "nearest-tower") {
                step.reset(new SviewStepNearestTower(child));
            }
            else if (type == "aircraft") {
                step.reset(new SviewStepAircraft(child));
            }
            else if (type == "move") {
                step.reset(new SviewStepMove(child));
            }
            else if (type == "rotate") {
                step.reset(new SviewStepRotate(child));
            }
            else if (type == "rotate-to-target") {
                step.reset(SviewStepRotateToTarget(child));
                finished = true;
            }
            else {
                SG_LOG(SG_GENERAL, SG_ALERT, "unrecognised step type: " << type);
                continue;
            }
            m_steps.push_back(step);
        }
        if (!finished) {
            std::unique_ptr<SviewStep> step = new SviewStepFinal;
        }
    }
    #endif
    
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
};

static std::ostream& operator << (std::ostream& out, const SviewPos& viewpos)
{
    out << viewpos.m_name << " (" << viewpos.m_steps.size() << ")";
    for (auto step: viewpos.m_steps) {
        out << " " << *step;
    }
    return out;
}

#include <simgear/scene/viewer/Compositor.hxx>


struct SviewView
{
    SviewView(osgViewer::View* view)
    :
    m_view(view)
    {
        s_id += 1;
    }
    
    const std::string&  description2()
    {
        if (m_description2 == "") {
            char    buffer[32];
            snprintf(buffer, sizeof(buffer), "[%i] ", s_id);
            m_description2 = buffer + description();
        }
        return m_description2;
    }
    
    virtual const std::string& description() = 0;
    
    virtual ~SviewView()
    {
        if (!m_view) {
            return;
        }
        osgViewer::ViewerBase* viewer_base = m_view->getViewerBase();
        auto composite_viewer = dynamic_cast<osgViewer::CompositeViewer*>(viewer_base);
        assert(composite_viewer);
        for (unsigned i=0; i<composite_viewer->getNumViews(); ++i) {
            osgViewer::View* view = composite_viewer->getView(i);
            SG_LOG(SG_VIEW, SG_ALERT, "composite_viewer view i=" << i << " view=" << view);
        }
        SG_LOG(SG_VIEW, SG_ALERT, "removing m_view=" << m_view);
        composite_viewer->stopThreading();
        composite_viewer->removeView(m_view);
        composite_viewer->startThreading();
    }
    
    /* Returns false if window has been closed. */
    virtual bool update(double dt) = 0;
    
    void posdir_to_view(SviewPosDir posdir)
    {
        /* FGViewMgr::update(). */
        osg::Vec3d  position = toOsg(posdir.position2);
        osg::Quat   orientation = toOsg(posdir.direction2);

        /* CameraGroup::update() */
        auto camera = m_view->getCamera();
        osg::Matrix old_m = camera->getViewMatrix();
        const osg::Matrix new_m(osg::Matrix::translate(-position)
                                     * osg::Matrix::rotate(orientation.inverse()));
        SG_LOG(SG_VIEW, SG_BULK, "old_m: " << old_m);
        SG_LOG(SG_VIEW, SG_BULK, "new_m: " << new_m);
        camera->setViewMatrix(new_m);
    }
        
    osgViewer::View*                    m_view = nullptr;
    simgear::compositor::Compositor*    m_compositor = nullptr;
    std::string                         m_description2;
    
    static int s_id;
};

int SviewView::s_id = 0;


// A view which keeps two aircraft visible, with one at a constant distance in
// the foreground.
//
struct SviewDouble : SviewView
{
    // <local> and <remote> should evaluate to position of local and remote
    // aircraft. We ignore directions.
    SviewDouble(
            osgViewer::View* view,
            const SviewPos& local,
            const SviewPos& remote
            )
    :
    SviewView(view),
    m_local(local),
    m_remote(remote)
    {
        auto eye = m_local.description();
        auto target = m_remote.description();
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
        
          -----             R
         /      \
        E        |
        |    L   | .................... H (horizon)
        |        |
         \      /
           -----
        
        We require that:
        
            EL is local aircraft's chase-distance (though at the moment we use
            a fixed value) so that local aircraft is in perfect view.

            Angle LER is fixed to give good view of both aircraft in
            window. (Should be related to the vertical angular size of the
            window, but at the moment we use a fixed value.)
        
        We need to calculate angle RLE, and add to HLR, in order to find
        position of E (eye). Then for view pitch we use midpoint of angle of ER
        and angle EL, so that local and remote aircraft are symmetrically below
        and above the centre of the view.
        
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
        bool valid = m_view->getCamera()->getGraphicsContext()->valid();
        SG_LOG(SG_VIEW, SG_BULK, "valid=" << valid);
        if (!valid) return false;
        
        bool debug = false;
        if (1) {
            time_t t = time(NULL) / 10;
            //if (debug) SG_LOG(SG_VIEW, SG_ALERT, "m_debug_time=" << m_debug_time << " t=" << t);
            if (t != m_debug_time) {
                m_debug_time = t;
                debug = true;
            }
        }

        // Find positions of local and remote aircraft.
        SviewPosDir posdir_local;
        m_local.evaluate(posdir_local);
        SviewPosDir posdir_remote;
        m_remote.evaluate(posdir_remote);
        
        if (debug) {
            SG_LOG(SG_VIEW, SG_ALERT, "    m_local: " << m_local << ": " << posdir_local);
            SG_LOG(SG_VIEW, SG_ALERT, "    m_remote: " << m_remote << ": " << posdir_remote);
        }
        
        // Create cartesian coordinates so we can calculate distance <lr>.
        SGVec3d local_pos = SGVec3d::fromGeod(posdir_local.target);
        SGVec3d remote_pos = SGVec3d::fromGeod(posdir_remote.target);
        double lr = sqrt(distSqr(local_pos, remote_pos));
        const double pi = 3.1415926;
        
        // Desired angle between local and remote aircraft in final view.
        double ler = 15 * pi / 180;
        
        // Distance of eye from local aircraft.
        double le = 25; /* should use chase_distance. */
        
        // Find <er>, the distance of eye from remote aircraft. Have to be
        // careful to cope when there is no solution if remote is too close,
        // and choose the +ve sqrt().
        //
        double er_root_term = lr*lr - le*le*sin(ler)*sin(ler);
        if (er_root_term < 0) er_root_term = 0;
        double er = le * cos(ler) + sqrt(er_root_term);
        
        // Now find rle, angle at local aircraft between vector to remote
        // aircraft and vector to desired eye position. Again we have to cope
        // when a solution is not possible.
        double cos_rle = (lr*lr + le*le - er*er) / (2*le*lr);
        if (cos_rle > 1) cos_rle = 1;
        if (cos_rle < -1) cos_rle = -1;
        double rle = acos(cos_rle);
        double rle_deg = rle * 180 / pi;
        
        // Now find the actual eye position. We do this by calculating heading
        // and pitch from local aircraft L to eye position E, then using a
        // temporary SviewStepMove.
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
        move.evaluate(posdir_local);
        
        // At this point, posdir_local.position is eye position. We make
        // posdir_local.direction2 point from this eye position to halfway
        // between the remote and local aircraft.
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
    
    SviewPos    m_local;
    SviewPos    m_remote;
    std::string m_description;
    time_t      m_debug_time = 0;
};


/* A clone of current view. */
struct SviewViewClone : SviewView
{
    SviewViewClone(osgViewer::View* view=nullptr)
    :
    SviewView(view)
    {
        SG_LOG(SG_VIEW, SG_INFO, "m_view=" << m_view);
        
        SGPropertyNode* global_view;
        SGPropertyNode* root;
        SGPropertyNode* sim;
        SGPropertyNode* view_config;
        int             view_number_raw;
        std::string     root_path;
        std::string     type;
        
        // Unfortunately we need to look at things like
        // /sim/view[]/config/eye-heading-deg-path, even when handling
        // multiplayer aircraft.
        //
        view_number_raw = globals->get_props()->getIntValue("/sim/current-view/view-number-raw");
        global_view = globals->get_props()->getNode("/sim/view", view_number_raw /*index*/, true /*create*/);
        
        /* <root_path> is typically "" or /ai/models/multiplayer[]. */
        root_path   = global_view->getStringValue("config/root");
        type        = global_view->getStringValue("type");
        
        root = globals->get_props()->getNode(root_path);
        if (root_path == "" || root_path == "/") {
            /* user aircraft */
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
            
            /* First move to centre of aircraft. */
            m_target.add_step(new SviewStepAircraft(root));
            m_target.add_step(new SviewStepMove(
                    -view_config->getDoubleValue("target-z-offset-m"),
                    -view_config->getDoubleValue("target-y-offset-m"),
                    -view_config->getDoubleValue("target-x-offset-m")
                    ));
            SG_LOG(SG_VIEW, SG_ALERT, "m_target=" << m_target);
            
            /* Set pitch and roll to zero, otherwise view from tower (as
            calculated by SviewStepTarget) rolls/pitches with aircraft. */
            m_target.add_step(new SviewStepDirectionMultiply(
                    1 /* heading */,
                    0 /* pitch */,
                    0 /* roll */
                    ));
            SG_LOG(SG_VIEW, SG_ALERT, "m_target=" << m_target);
            
            /* Current position is the target, so add a step that copies it to
            SviewPosDir.target. */
            m_target.add_step(new SviewStepCopyToTarget);
            
            /* Now set .m_eye up so that it looks from the nearest tower. */
            m_eye.add_step(new SviewStepNearestTower(sim));
            m_eye.add_step(new SviewStepTarget);
            
            /* At this point would like to move towards the target a little to
            avoid eye being inside the tower walls. But SviewStepTarget will
            have simply set posdir.direction2, so modifying posdir.heading etc
            will have no affect. Prob need to modify SviewStepTarget to set
            dirpos.heading,pitch,roll, then append a SviewStepFinal to convert
            to posdir.direction2. Alternatively, could we make all SviewStep*'s
            always work in terms of posdir.position2 and posdir.direction2,
            i.e. use SGQuatd for everything?

            [At some point it might be good to make this movement not change
            the height too.] */
            m_eye.add_step(new SviewStepMove(-30, 0, 0));
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
                SG_LOG(SG_VIEW, SG_ALERT, "m_target=" << m_target);
                
                m_target.add_step(new SviewStepMove(
                        -view_config->getDoubleValue("target-z-offset-m"),
                        -view_config->getDoubleValue("target-y-offset-m"),
                        -view_config->getDoubleValue("target-x-offset-m")
                        ));
                SG_LOG(SG_VIEW, SG_ALERT, "m_target=" << m_target);
                
                m_target.add_step(new SviewStepCopyToTarget);
                
                m_eye.add_step(new SviewStepAircraft(root));
                SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
                
                m_eye.add_step(new SviewStepMove(
                        -view_config->getDoubleValue("target-z-offset-m"),
                        -view_config->getDoubleValue("target-y-offset-m"),
                        -view_config->getDoubleValue("target-x-offset-m")
                        ));
                SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
                
                /* Crudely preserve or don't preserve aircraft's heading,
                pitch and roll; this enables us to mimic Helicopter and Chase
                views. In theory we should evaluate the specified paths, but
                in practise we only need to multiply current values by 0 or 1.
                todo: add damping. */
                m_eye.add_step(new SviewStepDirectionMultiply(
                        global_view->getStringValue("config/eye-heading-deg-path")[0] ? 1 : 0,
                        global_view->getStringValue("config/eye-pitch-deg-path")[0] ? 1 : 0,
                        global_view->getStringValue("config/eye-roll-deg-path")[0] ? 1 : 0
                        ));
                SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
                
                /* Apply the current view rotation. */
                m_eye.add_step(new SviewStepRotate(
                        root->getDoubleValue("sim/current-view/heading-offset-deg"),
                        root->getDoubleValue("sim/current-view/pitch-offset-deg"),
                        root->getDoubleValue("sim/current-view/roll-offset-deg")
                        ));
                SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
                if (1) {
                    /* E.g. Helicopter view. Move eye away from aircraft.
                    config/z-offset-m defaults to /sim/chase-distance-m (see
                    fgdata:defaults.xml) which is -ve, e.g. -25m. */
                    m_eye.add_step(new SviewStepMove(
                            view_config->getDoubleValue("z-offset-m"),
                            view_config->getDoubleValue("y-offset-m"),
                            view_config->getDoubleValue("x-offset-m")
                            ));
                    SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
                }
            }
            else {
                /* E.g. pilot view. Move to the eye position.

                config/z-offset-m seems to be +ve when moving backwards
                relative to the aircraft, so we need to negate the value we
                pass to SviewStepMove(). */
                m_eye.add_step(new SviewStepAircraft(root));
                m_eye.add_step(new SviewStepMove(
                        -view_config->getDoubleValue("z-offset-m"),
                        -view_config->getDoubleValue("y-offset-m"),
                        -view_config->getDoubleValue("x-offset-m")
                        ));
                SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
                
                /* Apply the current view rotation. On harrier-gr3 this
                corrects initial view direction (which is pitch down by 15deg),
                but seems to cause a problem where the cockpit rotates in a
                small circle when the aircraft rolls. */
                m_eye.add_step(new SviewStepRotate(
                        -view_config->getDoubleValue("heading-offset-deg"),
                        -view_config->getDoubleValue("pitch-offset-deg"),
                        -view_config->getDoubleValue("roll-offset-deg")
                        ));
                SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
                
                /* Preserve the current user-supplied offset to view direction. */
                m_eye.add_step(new SviewStepRotate(
                        root->getDoubleValue("sim/current-view/heading-offset-deg")
                                - view_config->getDoubleValue("heading-offset-deg"),
                        -(root->getDoubleValue("sim/current-view/pitch-offset-deg")
                                - view_config->getDoubleValue("pitch-offset-deg")),
                        root->getDoubleValue("sim/current-view/roll-offset-deg")
                                - view_config->getDoubleValue("roll-offset-deg")
                        ));
                SG_LOG(SG_VIEW, SG_ALERT, "m_eye=" << m_eye);
            }
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
    
    SviewViewClone(
            osgViewer::View* view,
            const SviewPos& eye,
            const SviewPos& target
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
        bool valid = m_view->getCamera()->getGraphicsContext()->valid();
        SG_LOG(SG_VIEW, SG_BULK, "valid=" << valid);
        if (!valid) return false;
        
        SviewPosDir posdir;
        //static time_t t0 = time(NULL);
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
    
    //private:
    
    SviewPos            m_eye;
    SviewPos            m_target;
    std::string         m_description;
    bool                m_debug = false;
    time_t              m_debug_time = 0;
};


/* Cloned views. */
static std::vector<std::shared_ptr<SviewView>>  s_views;

/* Recent views, for use by SviewAddLastPair(). */
static std::deque<std::shared_ptr<SviewViewClone>>    s_recent_views;

void SviewPush()
{
    if (s_recent_views.size() >= 2) {
        s_recent_views.pop_front();
    }
    std::shared_ptr<SviewViewClone> v(new SviewViewClone);
    s_recent_views.push_back(v);
    SG_LOG(SG_VIEW, SG_ALERT, "Have pushed view: " << v);
}


void SviewUpdate(double dt)
{
    bool verbose = 0;
    for (size_t i=0; i<s_views.size(); /* inc in loop*/) {
        if (verbose) {
            SG_LOG(SG_VIEW, SG_INFO, "updating i=" << i
                    << ": " << s_views[i]->m_view
                    << ' ' << s_views[i]->m_compositor
                    );
        }
        bool valid = s_views[i]->update(dt);
        if (valid) {
            const osg::Matrix& view_matrix = s_views[i]->m_view->getCamera()->getViewMatrix();
            const osg::Matrix& projection_matrix = s_views[i]->m_view->getCamera()->getProjectionMatrix();
            s_views[i]->m_compositor->update(view_matrix, projection_matrix);
            i += 1;
        }
        else {
            auto pview = s_views.begin() + i;
            SG_LOG(SG_VIEW, SG_INFO, "deleting SviewView i=" << i << ": " << (*pview)->description2());
            for (size_t j=0; j<s_views.size(); ++j) {
                SG_LOG(SG_VIEW, SG_INFO, "    " << j
                        << ": " << s_views[j]->m_view
                        << ' ' << s_views[j]->m_compositor
                        );
            }
            
            s_views.erase(pview);
            
            for (size_t j=0; j<s_views.size(); ++j) {
                SG_LOG(SG_VIEW, SG_INFO, "    " << j
                        << ": " << s_views[j]->m_view
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


static osg::ref_ptr<simgear::SGReaderWriterOptions> s_compositor_options;
static std::string s_compositor_path;

void SviewCreate(const std::string type)
{
    FGRenderer*                 renderer            = globals->get_renderer();
    osgViewer::ViewerBase*      viewer_base         = renderer->getViewerBase();
    osgViewer::CompositeViewer* composite_viewer    = dynamic_cast<osgViewer::CompositeViewer*>(viewer_base);

    if (!composite_viewer) {
        SG_LOG(SG_GENERAL, SG_ALERT, "FGViewMgr::clone_current_view() doing nothing because not composite-viewer mode not enabled.");
        return;
    }

    SG_LOG(SG_GENERAL, SG_ALERT, "Cloning current view.");
    osgViewer::View*    rhs_view    = renderer->getView();
    osg::Node*          scene_data  = rhs_view->getSceneData();
    
    SG_LOG(SG_GENERAL, SG_ALERT, "rhs_view->getNumSlaves()=" << rhs_view->getNumSlaves());



    // Using copy-constructor here doesn't seem to make any difference.
    //osgViewer::View* view = new osgViewer::View(rhs_view);    
    osgViewer::View* view = new osgViewer::View();


    std::shared_ptr<SviewView>  view2;
    if (type == "last_pair") {
        if (s_recent_views.size() < 2) {
            SG_LOG(SG_VIEW, SG_ALERT, "Need two cloned views");
            return;
        }
        else {
            auto it = s_recent_views.end();
            auto target = (--it)->get();
            auto eye    = (--it)->get();
            if (!target || !eye) {
                SG_LOG(SG_VIEW, SG_ALERT, "target=" << target << " eye=" << eye);
                return;
            }

            view2.reset(new SviewViewClone(
                    view,
                    eye->m_eye,
                    target->m_eye
                    ));
        }
    }
    else if (type == "last_pair_double") {
        if (s_recent_views.size() < 2) {
            SG_LOG(SG_VIEW, SG_ALERT, "Need two cloned views");
            return;
        }
        else {
            auto it = s_recent_views.end();
            auto remote = (--it)->get();
            auto local    = (--it)->get();
            if (!local || !remote) {
                SG_LOG(SG_VIEW, SG_ALERT, "remote=" << local << " remote=" << remote);
                return;
            }

            view2.reset(new SviewDouble(
                    view,
                    local->m_target,
                    remote->m_target
                    ));
        }
    }
    else if (type == "current") {
        view2.reset(new SviewViewClone(view));
    }
    else {
        SG_LOG(SG_GENERAL, SG_ALERT, "unrecognised type=" << type);
        return;
    }
    




    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    assert(wsi);
    unsigned int width, height;
    wsi->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), width, height);

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    
    traits->x = 100;
    traits->y = 100;
    traits->width = 400;
    traits->height = 300;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    
    traits->readDISPLAY();
    if (traits->displayNum < 0)
        traits->displayNum = 0;
    if (traits->screenNum < 0)
        traits->screenNum = 0;

    int bpp = fgGetInt("/sim/rendering/bits-per-pixel");
    int cbits = (bpp <= 16) ?  5 :  8;
    int zbits = (bpp <= 16) ? 16 : 24;
    traits->red = traits->green = traits->blue = cbits;
    traits->depth = zbits;
    
    traits->mipMapGeneration = true;
    traits->windowName = "Flightgear " + view2->description2();
    traits->sampleBuffers = fgGetInt("/sim/rendering/multi-sample-buffers", traits->sampleBuffers);
    traits->samples = fgGetInt("/sim/rendering/multi-samples", traits->samples);
    traits->vsync = fgGetBool("/sim/rendering/vsync-enable", traits->vsync);
    traits->stencil = 8;
    
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    assert(gc.valid());

    // need to ensure that the window is cleared make sure that the complete window is set the correct colour
    // rather than just the parts of the window that are under the camera's viewports
    //gc->setClearColor(osg::Vec4f(0.2f,0.2f,0.6f,1.0f));
    //gc->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    view->setSceneData(scene_data);
    view->setDatabasePager(FGScenery::getPagerSingleton());
        
    // https://www.mail-archive.com/osg-users@lists.openscenegraph.org/msg29820.html
    // Passing (false, false) here seems to cause a hang on startup.
    view->getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, false);
    osg::GraphicsContext::createNewContextID();
    
    osg::Camera* rhs_camera = rhs_view->getCamera();
    osg::Camera* camera = view->getCamera();
    camera->setGraphicsContext(gc.get());

    if (1) {
        double left;
        double right;
        double bottom;
        double top;
        double zNear;
        double zFar;
        auto projection_matrix = rhs_camera->getProjectionMatrix();
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
    
    camera->setProjectionMatrix(rhs_camera->getProjectionMatrix());
    camera->setViewMatrix(rhs_camera->getViewMatrix());
    camera->setCullMask(0xffffffff);
    camera->setCullMaskLeft(0xffffffff);
    camera->setCullMaskRight(0xffffffff);
    
    /* This appears to avoid unhelpful culling of nearby objects. Though the above
    SG_LOG() says zNear=0.1 zFar=120000, so not sure what's going on. */
    camera->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);
    
    /* from CameraGroup::buildGUICamera():
    camera->setInheritanceMask(osg::CullSettings::ALL_VARIABLES
                               & ~(osg::CullSettings::COMPUTE_NEAR_FAR_MODE
                                   | osg::CullSettings::CULLING_MODE
                                   | osg::CullSettings::CLEAR_MASK
                                   ));
    camera->setCullingMode(osg::CullSettings::NO_CULLING);
    */

    /* rhs_viewport seems to be null so this doesn't work.
    osg::Viewport* rhs_viewport = rhs_view->getCamera()->getViewport();
    SG_LOG(SG_GENERAL, SG_ALERT, "rhs_viewport=" << rhs_viewport);

    osg::Viewport* viewport = new osg::Viewport(*rhs_viewport);

    view->getCamera()->setViewport(viewport);
    */
    view->getCamera()->setViewport(0, 0, traits->width, traits->height);
        
    //camera->setClearMask(0);
    
    view->setName("Cloned view");
    
    view->setFrameStamp(composite_viewer->getFrameStamp());
    
    simgear::compositor::Compositor *compositor = simgear::compositor::Compositor::create(
            view,
            gc,
            view->getCamera()->getViewport(),
            s_compositor_path,
            s_compositor_options
            );
    
    view2->m_compositor = compositor;
    s_views.push_back(view2);
    
    // stop/start threading:
    // https://www.mail-archive.com/osg-users@lists.openscenegraph.org/msg54341.html
    //
    composite_viewer->stopThreading();
    composite_viewer->addView(view);
    composite_viewer->startThreading();
    
    SG_LOG(SG_GENERAL, SG_ALERT, "rhs_view->getNumSlaves()=" << rhs_view->getNumSlaves());
    SG_LOG(SG_GENERAL, SG_ALERT, "view->getNumSlaves()=" << view->getNumSlaves());
}

void SViewSetCompositorParams(
        osg::ref_ptr<simgear::SGReaderWriterOptions> options,
        const std::string& compositor_path
        )
{
    s_compositor_options = options;
    s_compositor_path = compositor_path;
}