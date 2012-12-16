// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "HLAViewerFederate.hxx"

#include <osg/Group>
#include <osgDB/Options>

#include "HLAEyeTrackerClass.hxx"
#include "HLAMPAircraftClass.hxx"
#include "HLAPerspectiveViewer.hxx"
#include "HLAPerspectiveViewerClass.hxx"
#include "HLASceneObjectClass.hxx"
#include "HLAView.hxx"
#include "HLAViewClass.hxx"
#include "Viewer.hxx"

namespace fgviewer {

HLAViewerFederate::HLAViewerFederate() :
    _group(new osg::Group)
{
}

HLAViewerFederate::~HLAViewerFederate()
{
}

const HLAPerspectiveViewer*
HLAViewerFederate::getViewer() const
{
    return _perspectiveViewer.get();
}

HLAPerspectiveViewer*
HLAViewerFederate::getViewer()
{
    return _perspectiveViewer.get();
}

void
HLAViewerFederate::setViewer(HLAPerspectiveViewer* perspectiveViewer)
{
    if (_perspectiveViewer.get() == perspectiveViewer)
        return;
    _perspectiveViewer = perspectiveViewer;
}

const HLAView*
HLAViewerFederate::getView() const
{
    if (!_perspectiveViewer.valid())
        return 0;
    return _perspectiveViewer->getView();
}

HLAView*
HLAViewerFederate::getView()
{
    if (!_perspectiveViewer.valid())
        return 0;
    return _perspectiveViewer->getView();
}

void
HLAViewerFederate::setView(HLAView* view)
{
    if (!_perspectiveViewer.valid())
        return;
    return _perspectiveViewer->setView(view);
}

simgear::HLAObjectClass*
HLAViewerFederate::createObjectClass(const std::string& name)
{
    if (name == "FGView") {
        _viewClass = new HLAViewClass(name, this);
        return _viewClass.get();
    } else if (name == "FGPerspectiveViewer") {
        _perspectiveViewerClass = new HLAPerspectiveViewerClass(name, this);
        return _perspectiveViewerClass.get();
    // } else if (name == "FGOrthographicViewer") {
    //     _orthographicViewerClass = new HLAOrthograpicViewerClass(name, this);
    //     return _orthographicViewerClass.get();
    } else if (name == "FGEyeTracker") {
        return new HLAEyeTrackerClass(name, this);
    // } else if (name == "FGPerspectiveCamera") {
    //     _perspectiveCameraClass = new HLAPerspectiveCameraClass(name, this);
    //     return _perspectiveCameraClass.get();
    // } else if (name == "FGOrthographicCamera") {
    //     _orthographicCameraClass = new HLAOrthographicCameraClass(name, this);
    //     return _orthographicCameraClass.get();
    // } else if (name == "FGWindowDrawable") {
    //     _windowDrawableClass = new HLAWindowDrawableClass(name, this);
    //     return _windowDrawableClass.get();
    // } else if (name == "FGRenderer") {
    //     _rendererClass = new HLARendererClass(name, this);
    //     return _rendererClass.get();
    }
    /// Classes for model type objects.
    else if (name == "FGSceneObject") {
        return new HLASceneObjectClass(name, this);
    } else if (name == "MPAircraft") {
        return new HLAMPAircraftClass(name, this);
    }
    /// Do not subscribe to anything else
    else {
        return 0;
    }
}

bool
HLAViewerFederate::init()
{
    if (!HLAFederate::init())
        return false;
    
    if (!_perspectiveViewerClass.valid())
        return false;
    if (!_viewClass.valid())
        return false;
    
    // Create a viewer ...
    _perspectiveViewer = new HLAPerspectiveViewer(_perspectiveViewerClass.get(), this);
    _perspectiveViewer->registerInstance(_perspectiveViewerClass.get());
    // ... with a nice view
    _defaultView = new HLAView(_viewClass.get(), this);
    _defaultView->registerInstance(_viewClass.get());
    _perspectiveViewer->setView(_defaultView.get());
    
    return true;
}

bool
HLAViewerFederate::update()
{
    // if (_view.valid())
    //     _view->updateAttributeValues(simgear::RTIData("frame"));
    if (_perspectiveViewer.valid())
        _perspectiveViewer->updateAttributeValues(simgear::RTIData("frame"));

    // Just go ahead as far as possible.
    // This is just fine for a viewer.
    return HLAFederate::timeAdvanceAvailable();
}

bool
HLAViewerFederate::shutdown()
{
    if (_defaultView.valid())
        _defaultView->deleteInstance(simgear::RTIData("shutdown"));
    _defaultView.clear();

    if (_perspectiveViewer.valid())
        _perspectiveViewer->deleteInstance(simgear::RTIData("shutdown"));
    _perspectiveViewer.clear();

    _group->removeChildren(0, _group->getNumChildren());

    return HLAFederate::shutdown();
}

osg::Node*
HLAViewerFederate::getDynamicModelNode()
{
    return _group.get();
}

void
HLAViewerFederate::addDynamicModel(osg::Node* node)
{
    /// FIXME can do something more intelligent like a dequeue
    /// when returning here something to identify the entry, removal is about O(1)
    _group->addChild(node);
}

void
HLAViewerFederate::removeDynamicModel(osg::Node* node)
{
    _group->removeChild(node);
}

osg::ref_ptr<const osgDB::Options>
HLAViewerFederate::getReaderWriterOptions() const
{
    // Cannot use the thread safe variant of the observed_ptr methods here since the
    // viewer is currently allocated on the stack.
    if (!_viewer.valid())
        return osg::ref_ptr<const osgDB::Options>();
    return _viewer->getReaderWriterOptions();
}

void
HLAViewerFederate::attachToViewer(Viewer* viewer)
{
    if (!viewer) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAViewerFederate::attachToViewer(): Ignoring zero viewer!");
        return;
    }
    if (_viewer.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAViewerFederate::attachToViewer(): Not attaching a second viewer!");
        return;
    }
    _viewer = viewer;
    _viewer->insertSceneData(getDynamicModelNode());
}

void
HLAViewerFederate::insertView(HLAView* view)
{
    _viewList.push_back(view);
    /// FIXME not always
    setView(view);
}

void
HLAViewerFederate::eraseView(HLAView* view)
{
    //// FIXME store the iterator!!!!
    for (ViewList::iterator i = _viewList.begin(); i != _viewList.end(); ++i) {
        if (i->get() != view)
            continue;
        _viewList.erase(i);
        break;
    }
    if (getView() == view) {
        if (_viewList.empty()) {
            setView(_defaultView.get());
        } else {
            setView(_viewList.back().get());
        }
    }
}

} // namespace fgviewer
