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

#ifndef HLAViewerFederate_hxx
#define HLAViewerFederate_hxx

#include <osg/ref_ptr>
#include <osg/observer_ptr>
#include <osg/Group>
#include <osg/Node>
#include <osgDB/Options>

#include <simgear/hla/HLAFederate.hxx>

#include "HLASceneObjectClass.hxx"
#include "HLAView.hxx"
#include "HLAViewClass.hxx"
#include "HLAPerspectiveViewer.hxx"
#include "HLAPerspectiveViewerClass.hxx"
#include "HLAMPAircraftClass.hxx"

namespace fgviewer {

class Viewer;

class HLAViewerFederate : public simgear::HLAFederate {
public:
    HLAViewerFederate();
    virtual ~HLAViewerFederate();

    const HLAPerspectiveViewer* getViewer() const;
    HLAPerspectiveViewer* getViewer();
    void setViewer(HLAPerspectiveViewer* viewer);

    const HLAView* getView() const;
    HLAView* getView();
    void setView(HLAView* view);

    virtual simgear::HLAObjectClass* createObjectClass(const std::string& name);

    virtual bool init();
    virtual bool update();
    virtual bool shutdown();

    /// The node containing all the dynamic models attached to this viewer
    osg::Node* getDynamicModelNode();
    void addDynamicModel(osg::Node* node);
    void removeDynamicModel(osg::Node* node);

    /// Dynamic models use this options struct for loading/paging
    osg::ref_ptr<const osgDB::Options> getReaderWriterOptions() const;

    /// Called by the viewer.
    void attachToViewer(Viewer* viewer);

    /// Temporary application management
    void insertView(HLAView* view);
    void eraseView(HLAView* view);

private:
    /// Explicitly known object classes
    SGSharedPtr<HLAPerspectiveViewerClass> _perspectiveViewerClass;
    SGSharedPtr<HLAViewClass> _viewClass;

    /// The perspective viewer of this viewer federate. FIXME: also allow orthographic viewers for instruments!
    SGSharedPtr<HLAPerspectiveViewer> _perspectiveViewer;

    /// The default absolute view
    SGSharedPtr<HLAView> _defaultView;

    /// A list of all available views
    typedef std::list<SGSharedPtr<HLAView> > ViewList;
    ViewList _viewList;

    /// The openscenegraphs side of this component
    osg::ref_ptr<osg::Group> _group;

    /// The backward reference to the viewer
    osg::observer_ptr<Viewer> _viewer;
};

} // namespace fgviewer

#endif
