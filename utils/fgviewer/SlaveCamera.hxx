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

#ifndef _fgviewer_SlaveCamera_hxx
#define _fgviewer_SlaveCamera_hxx

#include <string>
#include <osg/Camera>
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGWeakReferenced.hxx>

#include "Frustum.hxx"

namespace fgviewer  {

class Viewer;

class SlaveCamera : public SGWeakReferenced {
public:
    SlaveCamera(const std::string& name);
    virtual ~SlaveCamera();
    
    const std::string& getName() const
    { return _name; }

    /// The drawable this camera renders to
    bool setDrawableName(const std::string& drawableName);
    const std::string& getDrawableName() const
    { return _drawableName; }
    
    /// The viewport into the above drawable
    virtual bool setViewport(const SGVec4i& viewport);
    SGVec4i getViewport() const
    { return SGVec4i(_viewport->x(), _viewport->y(), _viewport->width(), _viewport->height()); }
    double getAspectRatio() const
    { return _viewport->aspectRatio(); }
    
    /// The view offset, usually an orientation offset
    virtual bool setViewOffset(const osg::Matrix& viewOffset);
    const osg::Matrix& getViewOffset() const
    { return _viewOffset; }
    bool setViewOffsetDeg(double headingDeg, double pitchDeg, double rollDeg);
    
    /// The frustum for this camera
    virtual bool setFrustum(const Frustum& frustum);
    const Frustum& getFrustum() const
    { return _frustum; }
    void setFustumByFieldOfViewDeg(double fieldOfViewDeg);
    bool setRelativeFrustum(const std::string names[2], const SlaveCamera& referenceCameraData,
                            const std::string referenceNames[2]);

    /// For relative cameras configure the reference points that should match
    void setProjectionReferencePoint(const std::string& name, const osg::Vec2& point);
    osg::Vec2 getProjectionReferencePoint(const std::string& name) const;
    /// Set the reference points for a monitor configuration
    void setMonitorProjectionReferences(double width, double height,
                                        double bezelTop, double bezelBottom,
                                        double bezelLeft, double bezelRight);

    /// Access parameters of this view/camera
    osg::Vec3 getLeftEyeOffset(const Viewer& viewer) const;
    osg::Vec3 getRightEyeOffset(const Viewer& viewer) const;
    double getZoomFactor(const Viewer& viewer) const;
    osg::Matrix getEffectiveViewOffset(const Viewer& viewer) const;
    Frustum getEffectiveFrustum(const Viewer& viewer) const;

    /// The top level camera holding the context
    osg::Camera* getCamera()
    { return _camera.get(); }
    
    /// Top level entry points
    bool realize(Viewer& viewer);
    bool update(Viewer& viewer);
    
protected:
    virtual osg::Camera* _realizeImplementation(Viewer& viewer);
    virtual bool _updateImplementation(Viewer& viewer);
    
private:
    SlaveCamera(const SlaveCamera&);
    SlaveCamera& operator=(const SlaveCamera&);

    /// The immutable name that is used to reference this slave camera.
    const std::string _name;
    std::string _drawableName;
    osg::ref_ptr<osg::Viewport> _viewport;
    osg::Matrix _viewOffset;
    Frustum _frustum;
    /// The camera that is attached to the slave and that has a context attached.
    osg::ref_ptr<osg::Camera> _camera;

    /// For relative views, the named reference points in the projection space
    typedef std::map<std::string, osg::Vec2> NameReferencePointMap;
    NameReferencePointMap _referencePointMap;
};

} // namespace fgviewer

#endif
