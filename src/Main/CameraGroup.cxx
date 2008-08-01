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

#include "CameraGroup.hxx"

#include "globals.hxx"
#include "renderer.hxx"
#include "FGManipulator.hxx"
#include "WindowBuilder.hxx"
#include "WindowSystemAdapter.hxx"
#include <simgear/props/props.hxx>

#include <algorithm>
#include <cstring>
#include <string>

#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osg/Math>
#include <osg/Matrix>
#include <osg/Quat>
#include <osg/Vec3d>
#include <osg/Viewport>

#include <osgUtil/IntersectionVisitor>

#include <osgViewer/GraphicsWindow>

namespace flightgear
{
using namespace osg;

using std::strcmp;
using std::string;

ref_ptr<CameraGroup> CameraGroup::_defaultGroup;

CameraGroup::CameraGroup(osgViewer::Viewer* viewer) :
    _viewer(viewer)
{
}

CameraInfo* CameraGroup::addCamera(unsigned flags, Camera* camera,
                                   const Matrix& view,
                                   const Matrix& projection,
                                   bool useMasterSceneData)
{
    if ((flags & (VIEW_ABSOLUTE | PROJECTION_ABSOLUTE)) != 0)
        camera->setReferenceFrame(Transform::ABSOLUTE_RF);
    else
        camera->setReferenceFrame(Transform::RELATIVE_RF);
    CameraInfo* info = new CameraInfo(flags, camera);
    _cameras.push_back(info);
    _viewer->addSlave(camera, view, projection, useMasterSceneData);
    info->slaveIndex = _viewer->getNumSlaves() - 1;
    return info;
}

void CameraGroup::update(const osg::Vec3d& position,
                         const osg::Quat& orientation)
{
    FGManipulator *manipulator
        = dynamic_cast<FGManipulator*>(_viewer->getCameraManipulator());
    if (!manipulator)
        return;
    manipulator->setPosition(position);
    manipulator->setAttitude(orientation);
    const Matrix masterView(manipulator->getInverseMatrix());
    const Matrix& masterProj = _viewer->getCamera()->getProjectionMatrix();
    for (CameraList::iterator i = _cameras.begin(); i != _cameras.end(); ++i) {
        const CameraInfo* info = i->get();
        if ((info->flags & (VIEW_ABSOLUTE | PROJECTION_ABSOLUTE)) == 0) {
            // Camera has relative reference frame and is updated by
            // osg::View.
            continue;
        }
        const View::Slave& slave = _viewer->getSlave(info->slaveIndex);
        Camera* camera = info->camera.get();
        if ((info->flags & VIEW_ABSOLUTE) != 0)
            camera->setViewMatrix(slave._viewOffset);
        else
            camera->setViewMatrix(masterView * slave._viewOffset);
        if ((info->flags & PROJECTION_ABSOLUTE) != 0)
            camera->setProjectionMatrix(slave._projectionOffset);
        else
            camera->setViewMatrix(masterProj * slave._projectionOffset);
    }
}

void CameraGroup::setCameraParameters(float vfov, float aspectRatio)
{
    const float zNear = .1f;
    const float zFar = 120000.0f;
    _viewer->getCamera()->setProjectionMatrixAsPerspective(vfov,
                                                           1.0f / aspectRatio,
                                                           zNear, zFar);
}
}

namespace
{
osg::Viewport* buildViewport(const SGPropertyNode* viewportNode)
{
    double x = viewportNode->getDoubleValue("x", 0.0);
    double y = viewportNode->getDoubleValue("y", 0.0);
    double width = viewportNode->getDoubleValue("width", 0.0);
    double height = viewportNode->getDoubleValue("height", 0.0);
    return new osg::Viewport(x, y, width, height);
}
}

namespace flightgear
{
CameraInfo* CameraGroup::buildCamera(const SGPropertyNode* cameraNode)
{
    WindowBuilder *wBuild = WindowBuilder::getWindowBuilder();
    const SGPropertyNode* windowNode = cameraNode->getNode("window");
    GraphicsWindow* window = 0;
    static int cameraNum = 0;
    int cameraFlags = DO_INTERSECTION_TEST;
    if (windowNode) {
        // New style window declaration / definition
        window = wBuild->buildWindow(windowNode);
    } else {
        // Old style: suck window params out of camera block
        window = wBuild->buildWindow(cameraNode);
    }
    if (!window) {
        return 0;
    }
    Camera* camera = new Camera;
    camera->setAllowEventFocus(false);
    camera->setGraphicsContext(window->gc.get());
    // If a viewport isn't set on the camera, then it's hard to dig it
    // out of the SceneView objects in the viewer, and the coordinates
    // of mouse events are somewhat bizzare.
    const SGPropertyNode* viewportNode = cameraNode->getNode("viewport");
    Viewport* viewport = 0;
    if (viewportNode) {
        viewport = buildViewport(viewportNode);
    } else {
        const GraphicsContext::Traits *traits = window->gc->getTraits();
        viewport = new Viewport(0, 0, traits->width, traits->height);
    }
    camera->setViewport(viewport);
    osg::Matrix pOff;
    osg::Matrix vOff;
    const SGPropertyNode* viewNode = cameraNode->getNode("view");
    if (viewNode) {
        double heading = viewNode->getDoubleValue("heading-deg", 0.0);
        double pitch = viewNode->getDoubleValue("pitch-deg", 0.0);
        double roll = viewNode->getDoubleValue("roll-deg", 0.0);
        double x = viewNode->getDoubleValue("x", 0.0);
        double y = viewNode->getDoubleValue("y", 0.0);
        double z = viewNode->getDoubleValue("z", 0.0);
        // Build a view matrix, which is the inverse of a model
        // orientation matrix.
        vOff = (Matrix::translate(-x, -y, -z)
                * Matrix::rotate(-DegreesToRadians(heading),
                                 Vec3d(0.0, 1.0, 0.0),
                                 -DegreesToRadians(pitch),
                                 Vec3d(1.0, 0.0, 0.0),
                                 -DegreesToRadians(roll),
                                 Vec3d(0.0, 0.0, 1.0)));
        if (viewNode->getBoolValue("absolute", false))
            cameraFlags |= VIEW_ABSOLUTE;
    } else {
        // Old heading parameter, works in the opposite direction
        double heading = cameraNode->getDoubleValue("heading-deg", 0.0);
        vOff.makeRotate(DegreesToRadians(heading), osg::Vec3(0, 1, 0));
    }
    const SGPropertyNode* projectionNode = 0;
    if ((projectionNode = cameraNode->getNode("perspective")) != 0) {
        double fovy = projectionNode->getDoubleValue("fovy-deg", 55.0);
        double aspectRatio = projectionNode->getDoubleValue("aspect-ratio",
                                                            1.0);
        double zNear = projectionNode->getDoubleValue("near", 0.0);
        double zFar = projectionNode->getDoubleValue("far", 0.0);
        double offsetX = projectionNode->getDoubleValue("offset-x", 0.0);
        double offsetY = projectionNode->getDoubleValue("offset-y", 0.0);
        double tan_fovy = tan(DegreesToRadians(fovy*0.5));
        double right = tan_fovy * aspectRatio * zNear + offsetX;
        double left = -tan_fovy * aspectRatio * zNear + offsetX;
        double top = tan_fovy * zNear + offsetY;
        double bottom = -tan_fovy * zNear + offsetY;
        pOff.makeFrustum(left, right, bottom, top, zNear, zFar);
        cameraFlags |= PROJECTION_ABSOLUTE;
    } else if ((projectionNode = cameraNode->getNode("frustum")) != 0
               || (projectionNode = cameraNode->getNode("ortho")) != 0) {
        double top = projectionNode->getDoubleValue("top", 0.0);
        double bottom = projectionNode->getDoubleValue("bottom", 0.0);
        double left = projectionNode->getDoubleValue("left", 0.0);
        double right = projectionNode->getDoubleValue("right", 0.0);
        double zNear = projectionNode->getDoubleValue("near", 0.0);
        double zFar = projectionNode->getDoubleValue("far", 0.0);
        if (cameraNode->getNode("frustum")) {
            pOff.makeFrustum(left, right, bottom, top, zNear, zFar);
            cameraFlags |= PROJECTION_ABSOLUTE;
        } else {
            pOff.makeOrtho(left, right, bottom, top, zNear, zFar);
            cameraFlags |= (PROJECTION_ABSOLUTE | ORTHO);
        }
    } else {
        // old style shear parameters
        double shearx = cameraNode->getDoubleValue("shear-x", 0);
        double sheary = cameraNode->getDoubleValue("shear-y", 0);
        pOff.makeTranslate(-shearx, -sheary, 0);
    }
    return addCamera(cameraFlags, camera, pOff, vOff);
}

CameraInfo* CameraGroup::buildGUICamera(const SGPropertyNode* cameraNode,
                                        GraphicsWindow* window)
{
    WindowBuilder *wBuild = WindowBuilder::getWindowBuilder();
    const SGPropertyNode* windowNode = (cameraNode
                                        ? cameraNode->getNode("window")
                                        : 0);
    static int cameraNum = 0;
    if (!window) {
        if (windowNode) {
            // New style window declaration / definition
            window = wBuild->buildWindow(windowNode);
            
        } else {
            return 0;
        }
    }
    Camera* camera = new Camera;
    camera->setAllowEventFocus(false);
    camera->setGraphicsContext(window->gc.get());
    const SGPropertyNode* viewportNode = (cameraNode
                                          ? cameraNode->getNode("viewport")
                                          : 0);
    Viewport* viewport = 0;
    if (viewportNode) {
        viewport = buildViewport(viewportNode);
    } else {
        const GraphicsContext::Traits *traits = window->gc->getTraits();
        viewport = new Viewport(0, 0, traits->width, traits->height);
    }
    camera->setViewport(viewport);
    // XXX Camera needs to be drawn last; eventually the render order
    // should be assigned by a camera manager.
    camera->setRenderOrder(osg::Camera::POST_RENDER, 100);
        camera->setClearMask(0);
    camera->setInheritanceMask(CullSettings::ALL_VARIABLES
                               & ~(CullSettings::COMPUTE_NEAR_FAR_MODE
                                   | CullSettings::CULLING_MODE));
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setCullingMode(osg::CullSettings::NO_CULLING);
    camera->setProjectionResizePolicy(Camera::FIXED);
    camera->setReferenceFrame(Transform::ABSOLUTE_RF);
    const int cameraFlags = GUI;
    return addCamera(cameraFlags, camera,
                     Matrixd::identity(), Matrixd::identity(), false);
}

CameraGroup* CameraGroup::buildCameraGroup(osgViewer::Viewer* viewer,
                                           const SGPropertyNode* gnode)
{
    CameraGroup* cgroup = new CameraGroup(viewer);
    for (int i = 0; i < gnode->nChildren(); ++i) {
        const SGPropertyNode* pNode = gnode->getChild(i);
        const char* name = pNode->getName();
        if (!strcmp(name, "camera")) {
            cgroup->buildCamera(pNode);
        } else if (!strcmp(name, "window")) {
            WindowBuilder::getWindowBuilder()->buildWindow(pNode);
        } else if (!strcmp(name, "gui")) {
            cgroup->buildGUICamera(pNode);
        }
    }
    return cgroup;
}

Camera* getGUICamera(CameraGroup* cgroup)
{
    CameraGroup::CameraIterator end = cgroup->camerasEnd();
    CameraGroup::CameraIterator result
        = std::find_if(cgroup->camerasBegin(), end,
                       FlagTester<CameraInfo>(CameraGroup::GUI));
    if (result != end)
        return (*result)->camera.get();
    else
        return 0;
}

bool computeIntersections(const CameraGroup* cgroup,
                          const osgGA::GUIEventAdapter* ea,
                          osgUtil::LineSegmentIntersector::Intersections& intersections)
{
    using osgUtil::Intersector;
    using osgUtil::LineSegmentIntersector;
    double x, y;
    eventToWindowCoords(ea, x, y);
    // Find camera that contains event
    for (CameraGroup::ConstCameraIterator iter = cgroup->camerasBegin(),
             e = cgroup->camerasEnd();
         iter != e;
         ++iter) {
        const CameraInfo* cinfo = iter->get();
        if ((cinfo->flags & CameraGroup::DO_INTERSECTION_TEST) == 0)
            continue;
        const Camera* camera = cinfo->camera.get();
        if (camera->getGraphicsContext() != ea->getGraphicsContext())
            continue;
        const Viewport* viewport = camera->getViewport();
        double epsilon = 0.5;
        if (!(x >= viewport->x() - epsilon
              && x < viewport->x() + viewport->width() -1.0 + epsilon
              && y >= viewport->y() - epsilon
              && y < viewport->y() + viewport->height() -1.0 + epsilon))
            continue;
        LineSegmentIntersector::CoordinateFrame cf = Intersector::WINDOW;
        ref_ptr<LineSegmentIntersector> picker
            = new LineSegmentIntersector(cf, x, y);
        osgUtil::IntersectionVisitor iv(picker.get());
        const_cast<Camera*>(camera)->accept(iv);
        if (picker->containsIntersections()) {
            intersections = picker->getIntersections();
            return true;
        } else {
            break;
        }
    }
    intersections.clear();
    return false;
}

void warpGUIPointer(CameraGroup* cgroup, int x, int y)
{
    using osgViewer::GraphicsWindow;
    Camera* guiCamera = getGUICamera(cgroup);
    if (!guiCamera)
        return;
    Viewport* vport = guiCamera->getViewport();
    GraphicsWindow* gw
        = dynamic_cast<GraphicsWindow*>(guiCamera->getGraphicsContext());
    if (!gw)
        return;
    globals->get_renderer()->getManipulator()->setMouseWarped();    
    // Translate the warp request into the viewport of the GUI camera,
    // send the request to the window, then transform the coordinates
    // for the Viewer's event queue.
    double wx = x + vport->x();
    double wyUp = vport->height() + vport->y() - y;
    double wy;
    const GraphicsContext::Traits* traits = gw->getTraits();
    if (gw->getEventQueue()->getCurrentEventState()->getMouseYOrientation()
        == osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS) {
        wy = traits->height - wyUp;
    } else {
        wy = wyUp;
    }
    gw->getEventQueue()->mouseWarped(wx, wy);
    gw->requestWarpPointer(wx, wy);
    osgGA::GUIEventAdapter* eventState
        = cgroup->getViewer()->getEventQueue()->getCurrentEventState();
    double viewerX
        = (eventState->getXmin()
           + ((wx / double(traits->width))
              * (eventState->getXmax() - eventState->getXmin())));
    double viewerY
        = (eventState->getYmin()
           + ((wyUp / double(traits->height))
              * (eventState->getYmax() - eventState->getYmin())));
    cgroup->getViewer()->getEventQueue()->mouseWarped(viewerX, viewerY);
}
}
