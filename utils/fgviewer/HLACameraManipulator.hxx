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

#ifndef _HLACameraManipulator_hxx
#define _HLACameraManipulator_hxx

#include <osgGA/CameraManipulator>
#include <simgear/scene/util/OsgMath.hxx>
#include "HLAPerspectiveViewer.hxx"

namespace fgviewer  {

class Viewer;

class HLACameraManipulator : public osgGA::CameraManipulator {
public:
    HLACameraManipulator(HLAPerspectiveViewer* perspectiveViewer = 0);
    HLACameraManipulator(const HLACameraManipulator& cameraManipulator, const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY);
    virtual ~HLACameraManipulator();

    META_Object(fgviewer, HLACameraManipulator);

    virtual void setByMatrix(const osg::Matrixd& matrix);
    virtual void setByInverseMatrix(const osg::Matrixd& matrix);

    virtual osg::Matrixd getMatrix() const;
    virtual osg::Matrixd getInverseMatrix() const;

    /** Handle events, return true if handled, false otherwise. */
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter, osgGA::GUIActionAdapter& actionAdapter);

protected:
    bool _handle(const osgGA::GUIEventAdapter& eventAdapter, Viewer& viewer);
    void _handleFrameEvent(osgGA::GUIActionAdapter& actionAdapter);
    bool _handleKeyDownEvent(const osgGA::GUIEventAdapter& eventAdapter, Viewer& viewer);
    bool _handleKeyUpEvent(const osgGA::GUIEventAdapter& eventAdapter, Viewer& viewer);
    void _incrementEyePosition(const SGVec3d& offset);
    void _resetView();
    void _rotateView(const osg::Vec2& inc);
    void _rotateSun(const osg::Vec2& inc, Viewer& viewer);

private:
    osg::Matrixd _viewMatrix;
    osg::Matrixd _inverseViewMatrix;
    osg::Vec2 _lastMousePos;
    SGSharedPtr<HLAPerspectiveViewer> _perspectiveViewer;
};

} // namespace fgviewer

#endif
