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

#include "Renderer.hxx"

#include <osgViewer/Renderer>
#include <simgear/scene/material/EffectCullVisitor.hxx>

#include "Drawable.hxx"
#include "Viewer.hxx"
#include "SlaveCamera.hxx"

namespace fgviewer  {

// FIXME I understand why we currently need this, but this seems like
// weird to require an own cull visitor.
static void installCullVisitor(osg::Camera& camera)
{
    osgViewer::Renderer* renderer = static_cast<osgViewer::Renderer*>(camera.getRenderer());
    for (int i = 0; i < 2; ++i) {
        osgUtil::SceneView* sceneView = renderer->getSceneView(i);
        osg::ref_ptr<osgUtil::CullVisitor::Identifier> identifier;

        identifier = sceneView->getCullVisitor()->getIdentifier();
        sceneView->setCullVisitor(new simgear::EffectCullVisitor);
        sceneView->getCullVisitor()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorLeft()->getIdentifier();
        sceneView->setCullVisitorLeft(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorLeft()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorRight()->getIdentifier();
        sceneView->setCullVisitorRight(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorRight()->setIdentifier(identifier.get());
    }
}

class Renderer::_SlaveCamera : public SlaveCamera {
public:
    _SlaveCamera(const std::string& name) :
        SlaveCamera(name)
    { 
    }
    virtual ~_SlaveCamera()
    {
    }
    virtual osg::Camera* _realizeImplementation(Viewer& viewer)
    {
        osg::Camera* camera = SlaveCamera::_realizeImplementation(viewer);
        if (!camera)
            return 0;

        camera->addChild(viewer.getSceneDataGroup());

        viewer.addSlave(camera, osg::Matrix::identity(), getViewOffset(), false /*useMastersSceneData*/);
        installCullVisitor(*camera);
    
        return camera;
    }
    virtual bool _updateImplementation(Viewer& viewer)
    {
        getCamera()->setViewMatrix(getEffectiveViewOffset(viewer));
        getCamera()->setProjectionMatrix(getEffectiveFrustum(viewer).getMatrix(osg::Vec2(10, 8e4)));
        return SlaveCamera::_updateImplementation(viewer);
    }
};

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

Drawable*
Renderer::createDrawable(Viewer&, const std::string& name)
{
    return new Drawable(name);
}

SlaveCamera*
Renderer::createSlaveCamera(Viewer&, const std::string& name)
{
    return new _SlaveCamera(name);
}

bool
Renderer::realize(Viewer& viewer)
{
    if (!viewer.realizeDrawables())
        return false;
    return viewer.realizeSlaveCameras();
}

bool
Renderer::update(Viewer& viewer)
{
    return viewer.updateSlaveCameras();
}

} // namespace fgviewer
