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

#include "HLACameraManipulator.hxx"

#include <simgear/scene/util/OsgMath.hxx>
#include "HLAPerspectiveViewer.hxx"
#include "Viewer.hxx"

namespace fgviewer  {

HLACameraManipulator::HLACameraManipulator(HLAPerspectiveViewer* perspectiveViewer) :
    _viewMatrix(osg::Matrixd::identity()),
    _inverseViewMatrix(osg::Matrixd::identity()),
    _lastMousePos(0, 0),
    _perspectiveViewer(perspectiveViewer)
{
}

HLACameraManipulator::HLACameraManipulator(const HLACameraManipulator& cameraManipulator, const osg::CopyOp& copyOp) :
    osgGA::CameraManipulator(cameraManipulator, copyOp),
    _viewMatrix(cameraManipulator._viewMatrix),
    _inverseViewMatrix(cameraManipulator._inverseViewMatrix),
    _lastMousePos(cameraManipulator._lastMousePos),
    _perspectiveViewer(cameraManipulator._perspectiveViewer)
{
}

HLACameraManipulator::~HLACameraManipulator()
{
}

void
HLACameraManipulator::setByMatrix(const osg::Matrixd& matrix)
{
}

void
HLACameraManipulator::setByInverseMatrix(const osg::Matrixd& matrix)
{
}

osg::Matrixd
HLACameraManipulator::getMatrix() const
{
    return _viewMatrix;
}

osg::Matrixd
HLACameraManipulator::getInverseMatrix() const
{
    return _inverseViewMatrix;
}

bool
HLACameraManipulator::handle(const osgGA::GUIEventAdapter& eventAdapter, osgGA::GUIActionAdapter& actionAdapter)
{
    if (_handle(eventAdapter, static_cast<Viewer&>(actionAdapter)))
        return true;
    return osgGA::CameraManipulator::handle(eventAdapter, actionAdapter);
}

bool
HLACameraManipulator::_handle(const osgGA::GUIEventAdapter& eventAdapter, Viewer& viewer)
{
    switch (eventAdapter.getEventType()) {
    case osgGA::GUIEventAdapter::PUSH:
        _lastMousePos = osg::Vec2(eventAdapter.getXnormalized(), eventAdapter.getYnormalized());
        break;
    case osgGA::GUIEventAdapter::RELEASE:
        break;
    case osgGA::GUIEventAdapter::DOUBLECLICK:
        break;
    case osgGA::GUIEventAdapter::DRAG:
        if (!eventAdapter.getModKeyMask())
            _rotateView(osg::Vec2(eventAdapter.getXnormalized(), eventAdapter.getYnormalized()) - _lastMousePos);
        else if (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL)
            _rotateSun(osg::Vec2(eventAdapter.getXnormalized(), eventAdapter.getYnormalized()) - _lastMousePos, viewer);
        _lastMousePos = osg::Vec2(eventAdapter.getXnormalized(), eventAdapter.getYnormalized());
        break;
    case osgGA::GUIEventAdapter::MOVE:
        break;
    case osgGA::GUIEventAdapter::SCROLL:
        break;
        
    case osgGA::GUIEventAdapter::KEYDOWN:
        _handleKeyDownEvent(eventAdapter, viewer);
        break;
    case osgGA::GUIEventAdapter::KEYUP:
        _handleKeyUpEvent(eventAdapter, viewer);
        break;
        
    case osgGA::GUIEventAdapter::FRAME:
        _handleFrameEvent(viewer);
        break;
        
    default:
        break;
    }
    return false;
}

void
HLACameraManipulator::_handleFrameEvent(osgGA::GUIActionAdapter& actionAdapter)
{
    // Note that eventAdapter.getTime() returns the reference time instead of the simulation time
    osg::View* view = actionAdapter.asView();
    if (!view)
        return;
    osg::FrameStamp* frameStamp = view->getFrameStamp();
    if (!frameStamp)
        return;
    if (!_perspectiveViewer.valid())
        return;

    SGLocationd location;
    location = _perspectiveViewer->getLocation(SGTimeStamp::fromSec(frameStamp->getSimulationTime()));

    // Update the main cameras view matrix
    _viewMatrix = osg::Matrixd::identity();
    // transform from the simulation typical x-forward/y-right/z-down
    // to the opengl camera system x-right/y-up/z-back
    // _viewMatrix.postMultRotate(toOsg(SGQuatd::fromEulerDeg(90, 0, -90)));
    _viewMatrix.postMultRotate(toOsg(SGQuatd(-0.5, -0.5, 0.5, 0.5)));
    // the orientation of the view
    _viewMatrix.postMultRotate(toOsg(location.getOrientation()));
    // the position of the view
    _viewMatrix.postMultTranslate(toOsg(location.getPosition()));
    _inverseViewMatrix = osg::Matrixd::inverse(_viewMatrix);
}

bool
HLACameraManipulator::_handleKeyDownEvent(const osgGA::GUIEventAdapter& eventAdapter, Viewer& viewer)
{
    if (!_perspectiveViewer.valid())
        return false;

    switch (eventAdapter.getKey()) {
    case osgGA::GUIEventAdapter::KEY_Space:
    case osgGA::GUIEventAdapter::KEY_Home:
        _resetView();
        return false;

    case osgGA::GUIEventAdapter::KEY_Left:
        _incrementEyePosition(SGVec3d(-0.1, 0, 0));
        return false;
    case osgGA::GUIEventAdapter::KEY_Right:
        _incrementEyePosition(SGVec3d(0.1, 0, 0));
        return false;
    case osgGA::GUIEventAdapter::KEY_Up:
        if (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL) {
            _perspectiveViewer->setZoomFactor(_perspectiveViewer->getZoomFactor()*1.1);
        } else if (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT) {
            _incrementEyePosition(SGVec3d(0, 0, 0.1));
        } else {
            _incrementEyePosition(SGVec3d(0, 0.1, 0));
        }
        return false;
    case osgGA::GUIEventAdapter::KEY_Down:
        if (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL) {
            _perspectiveViewer->setZoomFactor(_perspectiveViewer->getZoomFactor()/1.1);
        } else if (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT) {
            _incrementEyePosition(SGVec3d(0, 0, -0.1));
        } else {
            _incrementEyePosition(SGVec3d(0, -0.1, 0));
        }
        return false;
    default:
        return false;
    }
}

bool
HLACameraManipulator::_handleKeyUpEvent(const osgGA::GUIEventAdapter& eventAdapter, Viewer& viewer)
{
    return false;
}

void
HLACameraManipulator::_incrementEyePosition(const SGVec3d& offset)
{
    if (!_perspectiveViewer.valid())
        return;
    HLAEyeTracker* eyeTracker = _perspectiveViewer->getEyeTracker();
    if (!eyeTracker)
        return;
    eyeTracker->setLeftEyeOffset(eyeTracker->getLeftEyeOffset() + offset);
    eyeTracker->setRightEyeOffset(eyeTracker->getRightEyeOffset() + offset);
}

void
HLACameraManipulator::_resetView()
{
    if (!_perspectiveViewer.valid())
        return;
    _perspectiveViewer->setPosition(SGVec3d::zeros());
    _perspectiveViewer->setOrientation(SGQuatd::unit());
    _perspectiveViewer->setZoomFactor(1);
    HLAEyeTracker* eyeTracker = _perspectiveViewer->getEyeTracker();
    if (!eyeTracker)
        return;
    eyeTracker->setLeftEyeOffset(SGVec3d::zeros());
    eyeTracker->setRightEyeOffset(SGVec3d::zeros());
}

void
HLACameraManipulator::_rotateView(const osg::Vec2& inc)
{
    if (!_perspectiveViewer.valid())
        return;
    double zDeg, yDeg, xDeg;
    _perspectiveViewer->getOrientation().getEulerRad(zDeg, yDeg, xDeg);
    zDeg += inc[0];
    yDeg += inc[1];
    _perspectiveViewer->setOrientation(SGQuatd::fromEulerRad(zDeg, yDeg, xDeg));
}

void
HLACameraManipulator::_rotateSun(const osg::Vec2& inc, Viewer& viewer)
{
    osg::Light* light = viewer.getLight();
    if (!light)
        return;
    osg::Matrix m = osg::Matrix::inverse(viewer.getCamera()->getViewMatrix());
    osg::Vec3 position(light->getPosition()[0], light->getPosition()[1], light->getPosition()[2]) ;
    position = osg::Quat(-0.2*inc[0], osg::Matrix::transform3x3(osg::Vec3(0, 1, 0), m))*position;
    position = osg::Quat(0.2*inc[1], osg::Matrix::transform3x3(osg::Vec3(1, 0, 0), m))*position;
    light->setPosition(osg::Vec4(position, 0));
}

} // namespace fgviewer
