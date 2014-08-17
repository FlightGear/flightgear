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
#include <osg/Referenced>

#include "AIBVHPager.hxx"

#include <simgear/bvh/BVHPageNode.hxx>
#include <simgear/bvh/BVHSubTreeCollector.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/BVHPageNodeOSG.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/OptionsReadFileCallback.hxx>
#include <simgear/scene/tgdb/userdata.hxx>

namespace fgai {

// Short circuit reading image files.
class AIBVHPager::ReadFileCallback : public simgear::OptionsReadFileCallback {
public:
    virtual ~ReadFileCallback()
    { }
        
    virtual osgDB::ReaderWriter::ReadResult readImage(const std::string& name, const osgDB::Options*)
    { return new osg::Image; }
};
    
// A sub tree collector that pages in the nodes that it needs
class AIBVHPager::SubTreeCollector : public simgear::BVHSubTreeCollector {
public:
    SubTreeCollector(simgear::BVHPager& pager, const SGSphered& sphere) :
        BVHSubTreeCollector(sphere),
        _pager(pager),
        _complete(true)
    { }
    virtual ~SubTreeCollector()
    { }
    
    virtual void apply(simgear::BVHPageNode& pageNode)
    {
        _pager.use(pageNode);
        BVHSubTreeCollector::apply(pageNode);
        _complete = _complete && 0 != pageNode.getNumChildren();
    }
    bool complete() const
    { return _complete; }
    
private:
    simgear::BVHPager& _pager;
    bool _complete;
};

AIBVHPager::AIBVHPager()
{
}

AIBVHPager::~AIBVHPager()
{
}
    
void
AIBVHPager::setScenery(const std::string& fg_root, const std::string& fg_scenery)
{
    SGSharedPtr<SGPropertyNode> props = new SGPropertyNode;
    try {
        SGPath preferencesFile = fg_root;
        preferencesFile.append("preferences.xml");
        readProperties(preferencesFile.str(), props);
    } catch (...) {
        // In case of an error, at least make summer :)
        props->getNode("sim/startup/season", true)->setStringValue("summer");
        
        SG_LOG(SG_GENERAL, SG_ALERT, "Problems loading FlightGear preferences.\n"
               << "Probably FG_ROOT is not properly set.");
    }
    
    /// now set up the simgears required model stuff
    
    simgear::ResourceManager::instance()->addBasePath(fg_root, simgear::ResourceManager::PRIORITY_DEFAULT);
    // Just reference simgears reader writer stuff so that the globals get
    // pulled in by the linker ...
    simgear::ModelRegistry::instance();
    
    sgUserDataInit(props.get());
    SGMaterialLib* ml = new SGMaterialLib;
    SGPath mpath(fg_root);
    mpath.append("Materials/default/materials.xml");
    try {
        ml->load(fg_root, mpath.str(), props);
    } catch (...) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Problems loading FlightGear materials.\n"
               << "Probably FG_ROOT is not properly set.");
    }
    simgear::SGModelLib::init(fg_root, props);
    
    // Set up the reader/writer options
    osg::ref_ptr<simgear::SGReaderWriterOptions> options;
    if (osgDB::Options* ropt = osgDB::Registry::instance()->getOptions())
        options = new simgear::SGReaderWriterOptions(*ropt);
    else
        options = new simgear::SGReaderWriterOptions;
    osgDB::convertStringPathIntoFilePathList(fg_scenery,
                                             options->getDatabasePathList());
    options->setMaterialLib(ml);
    options->setPropertyNode(props);
    options->setReadFileCallback(new ReadFileCallback);
    options->setPluginStringData("SimGear::FG_ROOT", fg_root);
    // we do not need the builtin boundingvolumes
    options->setPluginStringData("SimGear::BOUNDINGVOLUMES", "OFF");
    // And we only want terrain, no objects on top.
    options->setPluginStringData("SimGear::FG_ONLY_TERRAIN", "ON");
    // Hmm, ??!!
    options->setPluginStringData("SimGear::FG_ONLY_AIRPORTS", "ON");
    props->getNode("sim/rendering/random-objects", true)->setBoolValue(false);
    props->getNode("sim/rendering/random-vegetation", true)->setBoolValue(false);
    
    // Get the whole world bvh tree
    _node = simgear::BVHPageNodeOSG::load("w180s90-360x180.spt", options);
}

SGSharedPtr<simgear::BVHNode>
AIBVHPager::getBoundingVolumes(const SGSphered& sphere)
{
    if (!_node.valid())
        return SGSharedPtr<simgear::BVHNode>();
    SubTreeCollector subTreeCollector(*this, sphere);
    _node->accept(subTreeCollector);
    return subTreeCollector.getNode();
}

} // namespace fgai
