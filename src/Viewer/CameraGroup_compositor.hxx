// Copyright (C) 2008  Tim Moore
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

#ifndef CAMERAGROUP_HXX
#define CAMERAGROUP_HXX 1

#include <map>
#include <string>
#include <vector>
#include <memory>

#include <osg/Matrix>
#include <osg/ref_ptr>
#include <osg/Referenced>
#include <osg/Node>
#include <osg/TextureRectangle>
#include <osg/Texture2D>
#include <osg/TexGen>
#include <osgUtil/RenderBin>
#include <osgViewer/View>

#include <simgear/scene/viewer/Compositor.hxx>

namespace osg
{
class Camera;
}

namespace osgViewer
{
class Viewer;
}

class SGPropertyNode;

namespace flightgear
{

class CameraGroupListener;
class GraphicsWindow;

/** A wrapper around osg::Camera that contains some extra information.
 */
struct CameraInfo : public osg::Referenced
{
    /** properties of a camera.
     */
    enum Flags
    {
        VIEW_ABSOLUTE = 0x1, /**< The camera view is absolute, not
                                relative to the master camera. */
        PROJECTION_ABSOLUTE = 0x2, /**< The projection is absolute. */
        ORTHO = 0x4,               /**< The projection is orthographic */
        GUI = 0x8,                 /**< Camera draws the GUI. */
        DO_INTERSECTION_TEST = 0x10,/**< scene intersection tests this
                                       camera. */
        FIXED_NEAR_FAR = 0x20,     /**< take the near far values in the
                                      projection for real. */
        ENABLE_MASTER_ZOOM = 0x40  /**< Can apply the zoom algorithm. */
    };

    CameraInfo(unsigned flags_)     :
        flags(flags_),
        physicalWidth(0), physicalHeight(0), bezelHeightTop(0),
        bezelHeightBottom(0), bezelWidthLeft(0), bezelWidthRight(0),
        relativeCameraParent(0) { }
    /** The name as given in the config file.
     */
    std::string name;
    /** Properties of the camera. @see CameraGroup::Flags.
     */
    unsigned flags;
    /** Physical size parameters.
     */
    double physicalWidth;
    double physicalHeight;
    double bezelHeightTop;
    double bezelHeightBottom;
    double bezelWidthLeft;
    double bezelWidthRight;
    /** Non-owning reference to the parent camera for relative camera
     * configurations.
     */
    const CameraInfo *relativeCameraParent;
    /** The reference points in the parents projection space.
     */
    osg::Vec2d parentReference[2];
    /** The reference points in the current projection space.
     */
    osg::Vec2d thisReference[2];
    /** View offset from the viewer master camera.
     */
    osg::Matrix viewOffset;
    /** Projection offset from the viewer master camera.
     */
    osg::Matrix projOffset;
    /** Current view and projection matrices for this camera.
     * They are only used by other child cameras through relativeCameraParent
     * so they can avoid recalculating them.
     */
    osg::Matrix viewMatrix, projMatrix;
    /** The Compositor used to manage the pipeline of this camera.
     */
    osg::ref_ptr<simgear::compositor::Compositor> compositor;
};

class CameraGroup : public osg::Referenced
{
public:
    /** Create a camera group associated with an osgViewer::Viewer.
     * @param viewer the viewer
     */
    CameraGroup(osgViewer::View* viewer);
    virtual ~CameraGroup();

    /** Set the default CameraGroup, which is the only one that
     * matters at this time.
     * @param group the group to set.
     */
    static void buildDefaultGroup(osgViewer::View* view);
    static void setDefault(CameraGroup* group) { _defaultGroup = group; }
    /** Get the default CameraGroup.
     * @return the default camera group.
     */
    static CameraGroup* getDefault() { return _defaultGroup.get(); }
    /** Get the camera group's Viewer.
     * @return the viewer
     */
    osgViewer::View* getView() { return _viewer.get(); }
    /** Create an osg::Camera from a property node and add it to the
     * camera group.
     * @param cameraNode the property node.
     * @return a CameraInfo object for the camera.
     */
    void buildCamera(SGPropertyNode* cameraNode);
    /** Create a camera from properties that will draw the GUI and add
     * it to the camera group.
     * @param cameraNode the property node. This can be 0, in which
     * case a default GUI camera is created.
     * @param window the GraphicsWindow to use for the GUI camera. If
     * this is 0, the window is determined from the property node.
     * @return a CameraInfo object for the GUI camera.
     */
    void buildGUICamera(SGPropertyNode* cameraNode,
                        GraphicsWindow* window = 0);
    /** Update the view for the camera group.
     * @param position the world position of the view
     * @param orientation the world orientation of the view.
     */
    void update(const osg::Vec3d& position,
                const osg::Quat& orientation);
    /** Set the parameters of the viewer's master camera. This won't
     * affect cameras that have CameraFlags::PROJECTION_ABSOLUTE set.
     * XXX Should znear and zfar be settable?
     * @param vfov the vertical field of view angle
     * @param aspectRatio the master camera's aspect ratio. This
     * doesn't actually change the viewport, but should reflect the
     * current viewport.
     */
    void setCameraParameters(float vfov, float aspectRatio);
    /** Update camera properties after a resize event.
     */
    void resized();

    void buildDistortionCamera(const SGPropertyNode* psNode,
                               osg::Camera* camera);
    /**
     * get aspect ratio of master camera's viewport
     */
    double getMasterAspectRatio() const;

    CameraInfo *getGUICamera() const;

protected:
    friend CameraGroupListener;
    friend bool computeIntersections(const CameraGroup* cgroup,
                                     const osg::Vec2d& windowPos,
                                     osgUtil::LineSegmentIntersector::Intersections&
                                     intersections);

    typedef std::vector<osg::ref_ptr<CameraInfo>> CameraList;
    CameraList _cameras;
    osg::ref_ptr<osgViewer::View> _viewer;
    static osg::ref_ptr<CameraGroup> _defaultGroup;
    std::unique_ptr<CameraGroupListener> _listener;

    // Near, far for the master camera if used.
    float _zNear;
    float _zFar;
    float _nearField;

    /** Build a complete CameraGroup from a property node.
     * @param viewer the viewer associated with this camera group.
     * @param wbuilder the window builder to be used for this camera group.
     * @param the camera group property node.
     */
    static CameraGroup* buildCameraGroup(osgViewer::View* viewer,
                                         SGPropertyNode* node);
};

/** Get the osg::Camera that draws the GUI, if any, from a camera
 * group.
 * @param cgroup the camera group
 * @return the GUI camera or 0
 */
osg::Camera* getGUICamera(CameraGroup* cgroup);
/** Choose a camera using an event and do intersection testing on its
 * view of the scene. Only cameras with the DO_INTERSECTION_TEST flag
 * set are considered.
 * @param cgroup the CameraGroup
 * @param ea the event containing a window and mouse coordinates
 * @param intersections container for the result of intersection
 * testing.
 * @return true if any intersections are found
 */
bool computeIntersections(const CameraGroup* cgroup,
                          const osg::Vec2d& windowPos,
                          osgUtil::LineSegmentIntersector::Intersections&
                          intersections);
/** Warp the pointer to coordinates in the GUI camera of a camera group.
 * @param cgroup the camera group
 * @param x x window coordinate of pointer
 * @param y y window coordinate of pointer, in "y down" coordinates.
 */
void warpGUIPointer(CameraGroup* cgroup, int x, int y);

}

#endif
