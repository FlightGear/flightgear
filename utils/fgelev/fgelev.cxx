// fgelev.cxx -- compute scenery elevation
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

#include <iostream>
#include <cstdlib>
#include <iomanip>

#include <osg/ArgumentParser>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/bvh/BVHNode.hxx>
#include <simgear/bvh/BVHLineSegmentVisitor.hxx>
#include <simgear/bvh/BVHPager.hxx>
#include <simgear/bvh/BVHPageNode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/BVHPageNodeOSG.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/tgdb/userdata.hxx>

namespace sg = simgear;

class Visitor : public sg::BVHLineSegmentVisitor {
public:
    Visitor(const SGLineSegmentd& lineSegment, sg::BVHPager& pager) :
        BVHLineSegmentVisitor(lineSegment, 0),
        _pager(pager)
    { }
    virtual ~Visitor()
    { }
    virtual void apply(sg::BVHPageNode& node)
    {
        // we have a non threaded pager so load just right here.
        _pager.use(node);
        BVHLineSegmentVisitor::apply(node);
    }
private:
    sg::BVHPager& _pager;
};

static bool
intersect(sg::BVHNode& node, sg::BVHPager& pager,
          const SGVec3d& start, SGVec3d& end, double offset)
{
    SGVec3d perp = offset*perpendicular(start - end);
    Visitor visitor(SGLineSegmentd(start + perp, end + perp), pager);
    node.accept(visitor);
    if (visitor.empty())
        return false;
    end = visitor.getLineSegment().getEnd();
    return true;
}

int
main(int argc, char** argv)
{
    /// Read arguments and environment variables.

    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc, argv);

    std::string fg_root;
    if (arguments.read("--fg-root", fg_root)) {
    } else if (const char *fg_root_env = std::getenv("FG_ROOT")) {
        fg_root = fg_root_env;
    } else {
        fg_root = PKGLIBDIR;
    }

    std::string fg_scenery;
    if (arguments.read("--fg-scenery", fg_scenery)) {
    } else if (const char *fg_scenery_env = std::getenv("FG_SCENERY")) {
        fg_scenery = fg_scenery_env;
    } else {
        SGPath path(fg_root);
        path.append("Scenery");
        fg_scenery = path.str();
    }

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
    options->setPluginStringData("SimGear::FG_ROOT", fg_root);
    // we do not need the builtin boundingvolumes
    options->setPluginStringData("SimGear::BOUNDINGVOLUMES", "OFF");
    // And we only want terrain, no objects on top.
    options->setPluginStringData("SimGear::FG_ONLY_TERRAIN", "ON");
    props->getNode("sim/rendering/random-objects", true)->setBoolValue(false);
    props->getNode("sim/rendering/random-vegetation", true)->setBoolValue(false);

    // Here, all arguments are processed
    arguments.reportRemainingOptionsAsUnrecognized();
    arguments.writeErrorMessages(std::cerr);

    // Get the whole world bvh tree
    SGSharedPtr<sg::BVHNode> node;
    node = sg::BVHPageNodeOSG::load("w180s90-360x180.spt", options);

    // if no model has been successfully loaded report failure.
    if (!node.valid()) {
        SG_LOG(SG_GENERAL, SG_ALERT, arguments.getApplicationName()
               << ": No data loaded");
        return EXIT_FAILURE;
    }

    // We assume that the above is a paged database.
    sg::BVHPager pager;

    while (std::cin.good()) {
        // Increment the paging relevant number
        pager.setUseStamp(1 + pager.getUseStamp());
        // and expire everything not accessed for the past 30 requests
        pager.update(10);

        std::string id;
        std::cin >> id;
        double lon, lat;
        std::cin >> lon >> lat;
        if (std::cin.fail())
            return EXIT_FAILURE;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        SGVec3d start = SGVec3d::fromGeod(SGGeod::fromDegM(lon, lat, 10000));
        SGVec3d end = SGVec3d::fromGeod(SGGeod::fromDegM(lon, lat, -1000));

        // Try to find an intersection
        bool found = intersect(*node, pager, start, end, 0);
        double scale = 1e-5;
        while (!found && scale <= 1) {
            found = intersect(*node, pager, start, end, scale);
            scale *= 2;
        }
        if (1e-5 < scale)
            std::cerr << "Found hole of minimum diameter "
                      << scale << "m at lon = " << lon
                      << "deg lat = " << lat << "deg" << std::endl;

        std::cout << id << ": ";
        if (!found) {
            std::cout << "-1000" << std::endl;
        } else {
            SGGeod geod = SGGeod::fromCart(end);
            std::cout << std::fixed << std::setprecision(3) << geod.getElevationM() << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
