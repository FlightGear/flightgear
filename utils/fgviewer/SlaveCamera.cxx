// Viewer.hxx -- alternative flightgear viewer application
//
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

#include "SlaveCamera.hxx"

#include <simgear/scene/util/OsgMath.hxx>

#include "Viewer.hxx"

#if FG_HAVE_HLA
#include "HLAViewerFederate.hxx"    
#include "HLAPerspectiveViewer.hxx"    
#endif

namespace fgviewer  {

class NoUpdateCallback : public osg::NodeCallback {
public:
    virtual ~NoUpdateCallback()
    { }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nodeVisitor)
    { }
};

SlaveCamera::SlaveCamera(const std::string& name) :
    _name(name),
    _viewport(new osg::Viewport())
{
    _referencePointMap["lowerLeft"] = osg::Vec2(-1, -1);
    _referencePointMap["lowerRight"] = osg::Vec2(1, -1);
    _referencePointMap["upperRight"] = osg::Vec2(1, 1);
    _referencePointMap["upperLeft"] = osg::Vec2(-1, 1);
}

SlaveCamera::~SlaveCamera()
{
}

bool
SlaveCamera::setDrawableName(const std::string& drawableName)
{
    if (_camera.valid())
        return false;
    _drawableName = drawableName;
    return true;
}

bool
SlaveCamera::setViewport(const SGVec4i& viewport)
{
    _viewport->setViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    _frustum.setAspectRatio(getAspectRatio());
    return true;
}

bool
SlaveCamera::setViewOffset(const osg::Matrix& viewOffset)
{
    _viewOffset = viewOffset;
    return true;
}

bool
SlaveCamera::setViewOffsetDeg(double headingDeg, double pitchDeg, double rollDeg)
{
    osg::Matrix viewOffset = osg::Matrix::identity();
    viewOffset.postMultRotate(osg::Quat(SGMiscd::deg2rad(headingDeg), osg::Vec3(0, 1, 0)));
    viewOffset.postMultRotate(osg::Quat(SGMiscd::deg2rad(pitchDeg), osg::Vec3(-1, 0, 0)));
    viewOffset.postMultRotate(osg::Quat(SGMiscd::deg2rad(rollDeg), osg::Vec3(0, 0, 1)));
    return setViewOffset(viewOffset);
}

bool
SlaveCamera::setFrustum(const Frustum& frustum)
{
    _frustum = frustum;
    return true;
}

void
SlaveCamera::setFustumByFieldOfViewDeg(double fieldOfViewDeg)
{
    Frustum frustum(getAspectRatio());
    frustum.setFieldOfViewDeg(fieldOfViewDeg);
    setFrustum(frustum);
}

bool
SlaveCamera::setRelativeFrustum(const std::string names[2], const SlaveCamera& referenceCameraData,
                                const std::string referenceNames[2])
{
    // Track the way from one projection space to the other:
    // We want
    //  P = T2*S*T*P0
    // where P0 is the projection template sensible for the given window size,
    // S a scale matrix and T is a translation matrix.
    // We need to determine T and S so that the reference points in the parents
    // projection space match the two reference points in this cameras projection space.
    
    // Starting from the parents camera projection space, we get into this cameras
    // projection space by the transform matrix:
    //  P*R*inv(pP*pR) = T2*S*T*P0*R*inv(pP*pR)
    // So, at first compute that matrix without T2*S*T and determine S and T* from that
    
    // The initial projeciton matrix to build upon
    osg::Matrix P = Frustum(getAspectRatio()).getMatrix();

    osg::Matrix R = getViewOffset();
    osg::Matrix pP = referenceCameraData.getFrustum().getMatrix();
    osg::Matrix pR = referenceCameraData.getViewOffset();
    
    // Transform from the reference cameras projection space into this cameras eye space.
    osg::Matrix pPtoEye = osg::Matrix::inverse(pR*pP)*R;
    
    osg::Vec2 pRef[2] = {
        referenceCameraData.getProjectionReferencePoint(referenceNames[0]),
        referenceCameraData.getProjectionReferencePoint(referenceNames[1])
    };
    
    // The first reference point transformed to this cameras projection space
    osg::Vec3d pRefInThis0 = P.preMult(pPtoEye.preMult(osg::Vec3d(pRef[0], 1)));
    // Translate this proejction matrix so that the first reference point is at the origin
    P.postMultTranslate(-pRefInThis0);
    
    // Transform the second reference point and get the scaling correct.
    osg::Vec3d pRefInThis1 = P.preMult(pPtoEye.preMult(osg::Vec3d(pRef[1], 1)));
    double s = osg::Vec2d(pRefInThis1[0], pRefInThis1[1]).length();
    if (s <= std::numeric_limits<double>::min())
        return false;
    osg::Vec2 ref[2] = {
        getProjectionReferencePoint(names[0]),
        getProjectionReferencePoint(names[1])
    };
    s = (ref[0] - ref[1]).length()/s;
    P.postMultScale(osg::Vec3d(s, s, 1));
    
    // The first reference point still maps to the origin in this projection space.
    // Translate the origin to the desired first reference point.
    P.postMultTranslate(osg::Vec3d(ref[0], 1));
    
    // Now osg::Matrix::inverse(pR*pP)*R*P should map pRef[i] exactly onto ref[i] for i = 0, 1.
    // Note that osg::Matrix::inverse(pR*pP)*R*P should exactly map pRef[0] at the near plane
    // to ref[0] at the near plane. The far plane is not taken care of.
    
    Frustum frustum;
    if (!frustum.setMatrix(P))
        return false;

    return setFrustum(frustum);
}

void
SlaveCamera::setProjectionReferencePoint(const std::string& name, const osg::Vec2& point)
{
    _referencePointMap[name] = point;
}

osg::Vec2
SlaveCamera::getProjectionReferencePoint(const std::string& name) const
{
    NameReferencePointMap::const_iterator i = _referencePointMap.find(name);
    if (i != _referencePointMap.end())
        return i->second;
    return osg::Vec2(0, 0);
}

void
SlaveCamera::setMonitorProjectionReferences(double width, double height,
                                            double bezelTop, double bezelBottom,
                                            double bezelLeft, double bezelRight)
{
    double left = 1 + 2*bezelLeft/width;
    double right = 1 + 2*bezelRight/width;
    
    double bottom = 1 + 2*bezelBottom/height;
    double top = 1 + 2*bezelTop/height;

    setProjectionReferencePoint("lowerLeft", osg::Vec2(-left, -bottom));
    setProjectionReferencePoint("lowerRight", osg::Vec2(right, -bottom));
    setProjectionReferencePoint("upperRight", osg::Vec2(right, top));
    setProjectionReferencePoint("upperLeft", osg::Vec2(-left, top));
}
   
osg::Vec3
SlaveCamera::getLeftEyeOffset(const Viewer& viewer) const
{
#if FG_HAVE_HLA
    const HLAViewerFederate* viewerFederate = viewer.getViewerFederate();
    if (!viewerFederate)
        return osg::Vec3(0, 0, 0);
    const HLAPerspectiveViewer* perspectiveViewer = viewerFederate->getViewer();
    if (!perspectiveViewer)
        return osg::Vec3(0, 0, 0);
    return toOsg(perspectiveViewer->getLeftEyeOffset());
#else
    return osg::Vec3(0, 0, 0);
#endif
}

osg::Vec3
SlaveCamera::getRightEyeOffset(const Viewer& viewer) const
{
#if FG_HAVE_HLA
    const HLAViewerFederate* viewerFederate = viewer.getViewerFederate();
    if (!viewerFederate)
        return osg::Vec3(0, 0, 0);
    const HLAPerspectiveViewer* perspectiveViewer = viewerFederate->getViewer();
    if (!perspectiveViewer)
        return osg::Vec3(0, 0, 0);
    return toOsg(perspectiveViewer->getRightEyeOffset());
#else
    return osg::Vec3(0, 0, 0);
#endif
}

double
SlaveCamera::getZoomFactor(const Viewer& viewer) const
{
#if FG_HAVE_HLA
    const HLAViewerFederate* viewerFederate = viewer.getViewerFederate();
    if (!viewerFederate)
        return 1;
    const HLAPerspectiveViewer* perspectiveViewer = viewerFederate->getViewer();
    if (!perspectiveViewer)
        return 1;
    return perspectiveViewer->getZoomFactor();
#else
    return 1;
#endif
}

osg::Matrix
SlaveCamera::getEffectiveViewOffset(const Viewer& viewer) const
{
    // The eye offset in the master cameras coordinates.
    osg::Vec3 eyeOffset = getLeftEyeOffset(viewer);

    // Transform the eye offset into this slaves coordinates
    eyeOffset = eyeOffset*getViewOffset();
    
    // The slaves view matrix is composed of the master matrix
    osg::Matrix viewOffset = viewer.getCamera()->getViewMatrix();
    // ... its view offset ...
    viewOffset.postMult(getViewOffset());
    // ... and the inverse of the eye offset that is required
    // to keep the world at the same position wrt the projection walls
    viewOffset.postMultTranslate(-eyeOffset);
    return viewOffset;
}

Frustum
SlaveCamera::getEffectiveFrustum(const Viewer& viewer) const
{
    // The eye offset in the master cameras coordinates.
    osg::Vec3 eyeOffset = getLeftEyeOffset(viewer);
    
    // Transform the eye offset into this slaves coordinates
    eyeOffset = eyeOffset*getViewOffset();
    
    /// FIXME read that from external
    osg::Vec3 zoomScaleCenter(0, 0, -1);
    double zoomFactor = getZoomFactor(viewer);
    
    /// Transform into the local cameras orientation.
    zoomScaleCenter = getViewOffset().preMult(zoomScaleCenter);

    // The unmodified frustum
    Frustum frustum = getFrustum();

    // For unresized views this is a noop
    frustum.setAspectRatio(getAspectRatio());

    // need to correct this for the eye position within the projection system
    frustum.translate(-eyeOffset);

    // Scale the whole geometric extent of the projection surfaces by the zoom factor
    frustum.scale(1/zoomFactor, zoomScaleCenter);
    
    return frustum;
}

bool
SlaveCamera::realize(Viewer& viewer)
{
    if (_camera.valid())
        return false;
    _camera = _realizeImplementation(viewer);
    return _camera.valid();
}
    
bool
SlaveCamera::update(Viewer& viewer)
{
    return _updateImplementation(viewer);
}
    
osg::Camera*
SlaveCamera::_realizeImplementation(Viewer& viewer)
{
    Drawable* drawable = viewer.getDrawable(_drawableName);
    if (!drawable)
        return 0;
    osg::GraphicsContext* graphicsContext = drawable->getGraphicsContext();
    if (!graphicsContext)
        return 0;

    osg::Camera* camera = new osg::Camera;
    camera->setName(getName());
    camera->setGraphicsContext(graphicsContext);
    camera->setViewport(_viewport.get());
    camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    
    // Not seriously consider someting different
    camera->setDrawBuffer(GL_BACK);
    camera->setReadBuffer(GL_BACK);

    camera->setUpdateCallback(new NoUpdateCallback);
    
    return camera;
}

bool
SlaveCamera::_updateImplementation(Viewer& viewer)
{
    if (!_camera.valid())
        return false;
    return true;
}

} // namespace fgviewer
