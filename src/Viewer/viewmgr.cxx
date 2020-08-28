// viewmgr.cxx -- class for managing all the views in the flightgear world.
//
// Written by Curtis Olson, started October 2000.
//   partially rewritten by Jim Wilson March 2002
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//
// $Id$

#include "config.h"

#include <algorithm> // for std::clamp

#include "viewmgr.hxx"
#include "ViewPropertyEvaluator.hxx"

#include <simgear/compiler.h>
#include <simgear/scene/util/OsgMath.hxx>

#include <Main/fg_props.hxx>
#include "view.hxx"
#include "sview.hxx"
#include "renderer_compositor.hxx"

#include "CameraGroup.hxx"
#include "Scenery/scenery.hxx"


// Constructor
FGViewMgr::FGViewMgr(void)
{
}

// Destructor
FGViewMgr::~FGViewMgr( void )
{
}

void
FGViewMgr::init ()
{
    if (_inited) {
        SG_LOG(SG_VIEW, SG_WARN, "duplicate init of view manager");
        return;
    }

    _inited = true;
    config_list = fgGetNode("/sim", true)->getChildren("view");
    _current = fgGetInt("/sim/current-view/view-number");

    for (unsigned int i = 0; i < config_list.size(); i++) {
        SGPropertyNode* n = config_list[i];
        SGPropertyNode* config = n->getChild("config", 0, true);

        flightgear::View* v = flightgear::View::createFromProperties(config, n->getIndex());
        if (v) {
            add_view(v);
        } else {
            SG_LOG(SG_VIEW, SG_DEV_WARN, "Failed to create view from:" << config->getPath());
        }
  }

  if (get_current_view()) {
      get_current_view()->bind();
  } else {
      SG_LOG(SG_VIEW, SG_DEV_WARN, "FGViewMgr::init: current view " << _current << " failed to create");
  }
}

void
FGViewMgr::postinit()
{
    // force update now so many properties of the current view are valid,
    // eg view position and orientation (as exposed via globals)
    update(0.0);
}

void
FGViewMgr::shutdown()
{
    if (!_inited) {
        return;
    }

    _inited = false;
    views.clear();
}

void
FGViewMgr::reinit ()
{
    viewer_list::iterator it;
    for (it = views.begin(); it != views.end(); ++it) {
        (*it)->resetOffsetsAndFOV();
    }
}

void
FGViewMgr::bind()
{
    // these are bound to the current view properties
    _tiedProperties.setRoot(fgGetNode("/sim/current-view", true));


    _tiedProperties.Tie("view-number", this,
                        &FGViewMgr::getCurrentViewIndex,
                        &FGViewMgr::setCurrentViewIndex, false);
    _viewNumberProp = _tiedProperties.getRoot()->getNode("view-number");
    _viewNumberProp->setAttribute(SGPropertyNode::ARCHIVE, false);
    _viewNumberProp->setAttribute(SGPropertyNode::PRESERVE, true);
    _viewNumberProp->setAttribute(SGPropertyNode::LISTENER_SAFE, true);
}


void
FGViewMgr::unbind ()
{
    flightgear::View* v = get_current_view();
    if (v) {
        v->unbind();
    }

  _tiedProperties.Untie();
    _viewNumberProp.clear();
    
    ViewPropertyEvaluator::clear();
    SviewClear();
}

void
FGViewMgr::update (double dt)
{
    flightgear::View* currentView = get_current_view();
    if (!currentView) {
        return;
    }

  // Update the current view
  currentView->update(dt);

// update the camera now
    osg::ref_ptr<flightgear::CameraGroup> cameraGroup = flightgear::CameraGroup::getDefault();
    if (cameraGroup) {
        cameraGroup->setCameraParameters(currentView->get_v_fov(),
                                         cameraGroup->getMasterAspectRatio());
        cameraGroup->update(toOsg(currentView->getViewPosition()),
                            toOsg(currentView->getViewOrientation()));
    }
    SviewUpdate(dt);
}

void FGViewMgr::clear()
{
    views.clear();
}

flightgear::View*
FGViewMgr::get_current_view()
{
    const auto numViews = static_cast<int>(views.size());
    if ((_current >= 0) && (_current < numViews)) {
        return views.at(_current);
    } else {
        return nullptr;
    }
}

const flightgear::View*
FGViewMgr::get_current_view() const
{
    const auto numViews = static_cast<int>(views.size());
    if ((_current >= 0) && (_current < numViews)) {
        return views.at(_current);
    } else {
        return nullptr;
    }
}


flightgear::View*
FGViewMgr::get_view( int i )
{
    const auto lastView = static_cast<int>(views.size()) - 1;
    const int c = std::clamp(i, 0, lastView);
    return views.at(c);
}

const flightgear::View*
FGViewMgr::get_view( int i ) const
{
    const auto lastView = static_cast<int>(views.size()) - 1;
    const int c = std::clamp(i, 0, lastView);
    return views.at(c);
}

flightgear::View*
FGViewMgr::next_view()
{
    const auto numViews = static_cast<int>(views.size());
    setCurrentViewIndex((_current + 1) % numViews);
    _viewNumberProp->fireValueChanged();
    return get_current_view();
}

flightgear::View*
FGViewMgr::prev_view()
{
    const auto numViews = static_cast<int>(views.size());
    // subtract 1, but add a full numViews, to ensure the integer
    // modulo returns a +ve result; negative values mean something
    // else to setCurrentViewIndex
    setCurrentViewIndex((_current - 1 + numViews) % numViews);
    _viewNumberProp->fireValueChanged();
    return get_current_view();
}

void FGViewMgr::clone_current_view()
{
    FGRenderer*                 renderer            = globals->get_renderer();
    osgViewer::ViewerBase*      viewer_base         = renderer->getViewerBase();
    osgViewer::CompositeViewer* composite_viewer    = dynamic_cast<osgViewer::CompositeViewer*>(viewer_base);

    if (!composite_viewer) {
        SG_LOG(SG_GENERAL, SG_ALERT, "FGViewMgr::clone_current_view() doing nothing because not composite-viewer mode not enabled.");
        return;
    }

    SG_LOG(SG_GENERAL, SG_ALERT, "Attempting to clone current view.");
    osgViewer::View*    rhs_view    = renderer->getView();
    osg::Node*          scene_data  = rhs_view->getSceneData();

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
    traits->windowName = "FlightGear Cloned View";
    traits->sampleBuffers = fgGetInt("/sim/rendering/multi-sample-buffers", traits->sampleBuffers);
    traits->samples = fgGetInt("/sim/rendering/multi-samples", traits->samples);
    traits->vsync = fgGetBool("/sim/rendering/vsync-enable", traits->vsync);
    traits->stencil = 8;
    
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    assert(gc.valid());

    // need to ensure that the window is cleared make sure that the complete window is set the correct colour
    // rather than just the parts of the window that are under the camera's viewports
    gc->setClearColor(osg::Vec4f(0.2f,0.2f,0.6f,1.0f));
    gc->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    osgViewer::View* view = new osgViewer::View();

    view->setSceneData(scene_data);
    view->setDatabasePager(FGScenery::getPagerSingleton());
    
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
    
    camera->setClearMask(0);
    
    view->setName("Cloned view");
    composite_viewer->addView(view);
    SviewAddClone(view);
}

void
FGViewMgr::add_view( flightgear::View * v )
{
  views.push_back(v);
  v->init();
}

int FGViewMgr::getCurrentViewIndex() const
{
    return _current;
}

void FGViewMgr::setCurrentViewIndex(int newview)
{
    if (newview == _current) {
        return;
    }

    // negative numbers -> set view with node index -newview
    if (newview < 0) {
        for (int i = 0; i < (int)config_list.size(); i++) {
            int index = -config_list[i]->getIndex();
            if (index == newview)
                newview = i;
        }
        if (newview < 0)
            return;
    }

    // not sure it really makes sense to be doing this wrapping logic
    // here, it could mask various strange inputs, But keeping for compat
    // for now.
    const auto numViews = static_cast<int>(views.size());
    if (newview < 0) { // wrap to last
        newview = numViews - 1;
    } else if (newview >= numViews) { //  wrap to zero
        newview = 0;
    }

    if (get_current_view()) {
	get_current_view()->unbind();
    }

    // set new view
    _current = newview;

    if (get_current_view()) {
        get_current_view()->bind();
    }
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGViewMgr> registrantFGViewMgr(
    SGSubsystemMgr::DISPLAY);
