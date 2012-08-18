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
#include <osg/Camera>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/Transform>
#include <osgDB/ReadFile>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/scene/bvh/BVHNode.hxx>
#include <simgear/scene/bvh/BVHLineSegmentVisitor.hxx>

class FGSceneryIntersect : public osg::NodeVisitor {
public:
    FGSceneryIntersect(const SGLineSegmentd& lineSegment) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
        _lineSegment(lineSegment),
        _haveHit(false)
    { }

    bool getHaveHit() const
    { return _haveHit; }
    const SGLineSegmentd& getLineSegment() const
    { return _lineSegment; }

    virtual void apply(osg::Node& node)
    {
        if (!testBoundingSphere(node.getBound()))
            return;

        addBoundingVolume(node);
    }

    virtual void apply(osg::Group& group)
    {
        if (!testBoundingSphere(group.getBound()))
            return;

        traverse(group);
        addBoundingVolume(group);
    }

    virtual void apply(osg::Transform& transform)
    { handleTransform(transform); }
    virtual void apply(osg::Camera& camera)
    {
        if (camera.getRenderOrder() != osg::Camera::NESTED_RENDER)
            return;
        handleTransform(camera);
    }

    virtual void apply(osg::ProxyNode& proxyNode)
    {
        unsigned numFileNames = proxyNode.getNumFileNames();
        for (unsigned i = 0; i < numFileNames; ++i) {
            if (i < proxyNode.getNumChildren() && proxyNode.getChild(i))
                continue;
            // FIXME evaluate pagedLOD.getDatabasePath()
            osg::ref_ptr<osg::Node> node;
            node = osgDB::readNodeFile(proxyNode.getFileName(i),
              static_cast<const osgDB::Options*>(proxyNode.getDatabaseOptions()));
            if (!node.valid())
                node = new osg::Group;
            if (i < proxyNode.getNumChildren())
                proxyNode.setChild(i, node);
            else
                proxyNode.addChild(node);
        }

        apply(static_cast<osg::Group&>(proxyNode));
    }
    virtual void apply(osg::PagedLOD& pagedLOD)
    {
        float range = std::numeric_limits<float>::max();
        unsigned numFileNames = pagedLOD.getNumFileNames();
        for (unsigned i = 0; i < numFileNames; ++i) {
            if (range < pagedLOD.getMaxRange(i))
                continue;
            range = pagedLOD.getMaxRange(i);
        }

        for (unsigned i = pagedLOD.getNumChildren(); i < numFileNames; ++i) {
            if (i < pagedLOD.getNumChildren() && pagedLOD.getChild(i))
                continue;
            osg::ref_ptr<osg::Node> node;
            if (pagedLOD.getMaxRange(i) <= range) {
                // FIXME evaluate pagedLOD.getDatabasePath()
                node = osgDB::readNodeFile(pagedLOD.getFileName(i),
                  static_cast<const osgDB::Options*>(pagedLOD.getDatabaseOptions()));
            }
            if (!node.valid())
                node = new osg::Group;
            pagedLOD.addChild(node.get());
        }

        apply(static_cast<osg::LOD&>(pagedLOD));
    }

private:
    void handleTransform(osg::Transform& transform)
    {
        // Hmm, may be this needs to be refined somehow ...
        if (transform.getReferenceFrame() != osg::Transform::RELATIVE_RF)
            return;

        if (!testBoundingSphere(transform.getBound()))
            return;

        osg::Matrix inverseMatrix;
        if (!transform.computeWorldToLocalMatrix(inverseMatrix, this))
            return;
        osg::Matrix matrix;
        if (!transform.computeLocalToWorldMatrix(matrix, this))
            return;

        SGLineSegmentd lineSegment = _lineSegment;
        bool haveHit = _haveHit;

        _haveHit = false;
        _lineSegment = lineSegment.transform(SGMatrixd(inverseMatrix.ptr()));

        addBoundingVolume(transform);
        traverse(transform);

        if (_haveHit) {
            _lineSegment = _lineSegment.transform(SGMatrixd(matrix.ptr()));
        } else {
            _lineSegment = lineSegment;
            _haveHit = haveHit;
        }
    }

    simgear::BVHNode* getNodeBoundingVolume(osg::Node& node)
    {
        SGSceneUserData* userData = SGSceneUserData::getSceneUserData(&node);
        if (!userData)
            return 0;
        return userData->getBVHNode();
    }
    void addBoundingVolume(osg::Node& node)
    {
        simgear::BVHNode* bvNode = getNodeBoundingVolume(node);
        if (!bvNode)
            return;

        // Find ground intersection on the bvh nodes
        simgear::BVHLineSegmentVisitor lineSegmentVisitor(_lineSegment,
                                                          0/*startTime*/);
        bvNode->accept(lineSegmentVisitor);
        if (!lineSegmentVisitor.empty()) {
            _lineSegment = lineSegmentVisitor.getLineSegment();
            _haveHit = true;
        }
    }

    bool testBoundingSphere(const osg::BoundingSphere& bound) const
    {
        if (!bound.valid())
            return false;

        SGSphered sphere(toVec3d(toSG(bound._center)), bound._radius);
        return intersects(_lineSegment, sphere);
    }

    SGLineSegmentd _lineSegment;
    bool _haveHit;
};

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

    std::string config;
    while (arguments.read("--config", config)) {
        try {
            readProperties(config, props);
        } catch (...) {
            SG_LOG(SG_GENERAL, SG_ALERT, "Problems loading config file \"" << config
                   << "\" given on the command line.");
        }
    }

    std::string prop, value;
    while (arguments.read("--prop", prop, value)) {
        props->setStringValue(prop, value);
    }

    /// now set up the simgears required model stuff

    simgear::ResourceManager::instance()->addBasePath(fg_root, simgear::ResourceManager::PRIORITY_DEFAULT);
    // Just reference simgears reader writer stuff so that the globals get
    // pulled in by the linker ...
    simgear::ModelRegistry::instance();

    sgUserDataInit(props.get());
    SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::DoNotUseCompression);
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
    options->setPluginStringData("SimGear::BOUNDINGVOLUMES", "ON");

    // Here, all arguments are processed
    arguments.reportRemainingOptionsAsUnrecognized();
    arguments.writeErrorMessages(std::cerr);

    /// Read the whole world paged model.
    osg::ref_ptr<osg::Node> loadedModel;
    loadedModel = osgDB::readNodeFile("w180s90-360x180.spt", options.get());

    // if no model has been successfully loaded report failure.
    if (!loadedModel.valid()) {
        SG_LOG(SG_GENERAL, SG_ALERT, arguments.getApplicationName()
               << ": No data loaded");
        return EXIT_FAILURE;
    }

    // now handle all the position pairs
    for(int i = 1; i < arguments.argc(); ++i) {
        if (arguments.isOption(i))
            continue;

        std::istringstream ss(arguments[i]);
        double lon, lat;
        char sep;
        ss >> lon >> sep >> lat;
        if (ss.fail()) {
            return EXIT_FAILURE;
        }

        SGVec3d start = SGVec3d::fromGeod(SGGeod::fromDegM(lon, lat, 10000));
        SGVec3d end = SGVec3d::fromGeod(SGGeod::fromDegM(lon, lat, -1000));
        FGSceneryIntersect intersectVisitor(SGLineSegmentd(start, end));
        intersectVisitor.setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
        loadedModel->accept(intersectVisitor);

        std::cout << arguments[i] << ": ";
        if (!intersectVisitor.getHaveHit()) {
            std::cout << "-1000" << std::endl;
        } else {
            SGGeod geod = SGGeod::fromCart(intersectVisitor.getLineSegment().getEnd());
            std::cout << std::fixed << std::setprecision(3) << geod.getElevationM() << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
