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

#include <osg/Matrix>
#include <osg/ref_ptr>
#include <osg/Referenced>
#include <osg/Node>
#include <osg/TextureRectangle>
#include <osg/Texture2D>
#include <osg/TexGen>
#include <osgUtil/RenderBin>

// For osgUtil::LineSegmentIntersector::Intersections, which is a typedef.
#include <osgUtil/LineSegmentIntersector>
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

class GraphicsWindow;

struct RenderBufferInfo {
    RenderBufferInfo(osg::Texture2D* t = 0, float s = 1.0 ) : texture(t), scaleFactor(s) {}
    osg::ref_ptr<osg::Texture2D> texture;
    float scaleFactor;
};
typedef std::map<std::string,RenderBufferInfo> RenderBufferMap;
typedef std::map<osg::Camera::BufferComponent,std::string> AttachmentMap;

struct RenderStageInfo {
    RenderStageInfo(osg::Camera* camera_ = 0, int si = -1, bool fs = false)
        : camera(camera_), slaveIndex(si), scaleFactor(1.0f), fullscreen(fs)
        , resizable(true)
    {
    }

    osg::ref_ptr<osg::Camera> camera;
    AttachmentMap buffers;
    int slaveIndex;
    float scaleFactor;
    bool fullscreen;
    bool resizable;
};

extern const char* MAIN_CAMERA;
extern const char* FAR_CAMERA;
extern const char* GEOMETRY_CAMERA;
extern const char* SHADOW_CAMERA;
extern const char* LIGHTING_CAMERA;
extern const char* DISPLAY_CAMERA;

typedef std::map<std::string,RenderStageInfo> CameraMap;

/** A wrapper around osg::Camera that contains some extra information.
 */
struct CameraInfo : public osg::Referenced
{
    CameraInfo(unsigned flags_)
        : flags(flags_),
          x(0.0), y(0.0), width(0.0), height(0.0),
          physicalWidth(0), physicalHeight(0), bezelHeightTop(0),
          bezelHeightBottom(0), bezelWidthLeft(0), bezelWidthRight(0),
          relativeCameraParent(~0u),
          bufferSize( new osg::Uniform("fg_BufferSize", osg::Vec2f() ) ),
          projInverse( new osg::Uniform( "fg_ProjectionMatrixInverse", osg::Matrixf() ) ),
          viewInverse( new osg::Uniform( "fg_ViewMatrixInverse",osg::Matrixf() ) ),
          view( new osg::Uniform( "fg_ViewMatrix",osg::Matrixf() ) ),
          worldPosCart( new osg::Uniform( "fg_CameraPositionCart", osg::Vec3f() ) ),
          worldPosGeod( new osg::Uniform( "fg_CameraPositionGeod", osg::Vec3f() ) ),
          du( new osg::Uniform( "fg_du",osg::Vec4() ) ),
          dv( new osg::Uniform( "fg_dv",osg::Vec4() ) )
    {
        shadowMatrix[0] = new osg::Uniform("fg_ShadowMatrix_0", osg::Matrixf());
        shadowMatrix[1] = new osg::Uniform("fg_ShadowMatrix_1", osg::Matrixf());
        shadowMatrix[2] = new osg::Uniform("fg_ShadowMatrix_2", osg::Matrixf());
        shadowMatrix[3] = new osg::Uniform("fg_ShadowMatrix_3", osg::Matrixf());
    }

    /** Update and resize cameras
     */
    void updateCameras();
    void resized(double w, double h);
    /** The name as given in the config file.
     */
    std::string name;
    /** Properties of the camera. @see CameraGroup::Flags.
     */
    unsigned flags;

    /** Viewport parameters.
     */
    double x;
    double y;
    double width;
    double height;
    /** Physical size parameters.
     */
    double physicalWidth;
    double physicalHeight;
    double bezelHeightTop;
    double bezelHeightBottom;
    double bezelWidthLeft;
    double bezelWidthRight;
    /** The parent camera for relative camera configurations.
     */
    unsigned relativeCameraParent;

    /** the camera objects
     */
    CameraMap cameras;
    void addCamera( const std::string& k, osg::Camera* c, int si = -1, bool fs = false ) { cameras[k].camera = c; cameras[k].slaveIndex = si; cameras[k].fullscreen = fs; }
    void addCamera( const std::string& k, osg::Camera* c, bool fs ) { cameras[k].camera = c; cameras[k].fullscreen = fs; }
    void addCamera( const std::string& k, osg::Camera* c, float s, bool fs = false ) { cameras[k].camera = c; cameras[k].scaleFactor = s; cameras[k].fullscreen = fs; }
    osg::Camera* getCamera(const std::string& k) const;
    int getMainSlaveIndex() const;
    RenderStageInfo& getRenderStageInfo( const std::string& k ) { return cameras[k]; }

    /** the buffer objects
     */
    RenderBufferMap buffers;
    void addBuffer(const std::string& k, osg::Texture2D* tex, float scale = 1.0 ) { buffers[k] = RenderBufferInfo(tex,scale); }
    osg::Texture2D* getBuffer(const std::string& k) const;

    osg::ref_ptr<osg::Uniform> bufferSize;
    osg::ref_ptr<osg::Uniform> projInverse;
    osg::ref_ptr<osg::Uniform> viewInverse;
    osg::ref_ptr<osg::Uniform> view;
    osg::ref_ptr<osg::Uniform> worldPosCart;
    osg::ref_ptr<osg::Uniform> worldPosGeod;
    osg::ref_ptr<osg::Uniform> du;
    osg::ref_ptr<osg::Uniform> dv;
    osg::ref_ptr<osg::Uniform> shadowMatrix[4];

    void setMatrices( osg::Camera* c );

    osgUtil::RenderBin::RenderBinList savedTransparentBins;
    /** The reference points in the parents projection space.
     */
    osg::Vec2d parentReference[2];
    /** The reference points in the current projection space.
     */
    osg::Vec2d thisReference[2];
};

/** Update the OSG cameras from the camera info.
 */
void updateCameras(const CameraInfo* info);

class CameraGroup : public osg::Referenced
{
public:
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
    /** Create a camera group associated with an osgViewer::Viewer.
     * @param viewer the viewer
     */
    CameraGroup(osgViewer::Viewer* viewer);
    /** Get the camera group's Viewer.
     * @return the viewer
     */
    osgViewer::Viewer* getViewer() { return _viewer.get(); }
    /** Create an osg::Camera from a property node and add it to the
     * camera group.
     * @param cameraNode the property node.
     * @return a CameraInfo object for the camera.
     */
    CameraInfo* buildCamera(SGPropertyNode* cameraNode);
    /** Create a camera from properties that will draw the GUI and add
     * it to the camera group.
     * @param cameraNode the property node. This can be 0, in which
     * case a default GUI camera is created.
     * @param window the GraphicsWindow to use for the GUI camera. If
     * this is 0, the window is determined from the property node.
     * @return a CameraInfo object for the GUI camera.
     */
    CameraInfo* buildGUICamera(SGPropertyNode* cameraNode,
                               GraphicsWindow* window = 0);
    /** Update the view for the camera group.
     * @param position the world position of the view
     * @param orientation the world orientation of the view.
     */
    void update(const osg::Vec3d& position, const osg::Quat& orientation);
    /** Set the parameters of the viewer's master camera. This won't
     * affect cameras that have CameraFlags::PROJECTION_ABSOLUTE set.
     * XXX Should znear and zfar be settable?
     * @param vfov the vertical field of view angle
     * @param aspectRatio the master camera's aspect ratio. This
     * doesn't actually change the viewport, but should reflect the
     * current viewport.
     */
    void setCameraParameters(float vfov, float aspectRatio);
    /** Set the default CameraGroup, which is the only one that
     * matters at this time.
     * @param group the group to set.
     */
    static void setDefault(CameraGroup* group) { _defaultGroup = group; }
    /** Get the default CameraGroup.
     * @return the default camera group.
     */
    static CameraGroup* getDefault() { return _defaultGroup.get(); }
    typedef std::vector<osg::ref_ptr<CameraInfo> > CameraList;
    typedef CameraList::iterator CameraIterator;
    typedef CameraList::const_iterator ConstCameraIterator;
    /** Get iterator for camera vector. The iterator's value is a ref_ptr.
     */
    CameraIterator camerasBegin() { return _cameras.begin(); }
    /** Get iteator pointing to the end of the camera list.
     */
    CameraIterator camerasEnd() { return _cameras.end(); }
    ConstCameraIterator camerasBegin() const { return _cameras.begin(); }
    ConstCameraIterator camerasEnd() const { return _cameras.end(); }
    void addCamera(CameraInfo* info) { _cameras.push_back(info); }
    /** Build a complete CameraGroup from a property node.
     * @param viewer the viewer associated with this camera group.
     * @param the camera group property node.
     */
    static CameraGroup* buildCameraGroup(osgViewer::Viewer* viewer,
                                         SGPropertyNode* node);
    /** Set the cull mask on all non-GUI cameras
     */
    void setCameraCullMasks(osg::Node::NodeMask nm);
    /** Update camera properties after a resize event.
     */
    void resized();

    void buildDistortionCamera(const SGPropertyNode* psNode,
                               osg::Camera* camera);
  
    /**
     * get aspect ratio of master camera's viewport
     */
    double getMasterAspectRatio() const;
  
    /**
     * find the GUI camera if one is defined 
     */
    const CameraInfo* getGUICamera() const;
protected:
    CameraList _cameras;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    static osg::ref_ptr<CameraGroup> _defaultGroup;
    // Near, far for the master camera if used.
    float _zNear;
    float _zFar;
    float _nearField;
    typedef std::map<std::string, osg::ref_ptr<osg::TextureRectangle> > TextureMap;
    TextureMap _textureTargets;
};

}

namespace osgGA
{
class GUIEventAdapter;
}

namespace flightgear
{
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
                          const osgGA::GUIEventAdapter* ea,
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
