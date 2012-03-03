// fgviewer.cxx -- alternative flightgear viewer application
//
// Copyright (C) 2009 - 2011  Mathias Froehlich
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

#include <osg/ArgumentParser>
#include <osg/Fog>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgViewer/Renderer>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/TerrainManipulator>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/tgdb/TileEntry.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/model/modellib.hxx>

class DummyLoadHelper : public simgear::ModelLoadHelper {
public:
    virtual osg::Node *loadTileModel(const string& modelPath, bool)
    {
        try {
            return simgear::SGModelLib::loadModel(modelPath, simgear::getPropertyRoot());
        } catch (...) {
            std::cerr << "Error loading \"" << modelPath << "\"" << std::endl;
            return 0;
        }
    }
};

int
main(int argc, char** argv)
{
    // Just reference simgears reader writer stuff so that the globals get
    // pulled in by the linker ...
    // FIXME: make that more explicit clear and call an initialization function
    simgear::ModelRegistry::instance();
    DummyLoadHelper dummyLoadHelper;
    simgear::TileEntry::setModelLoadHelper(&dummyLoadHelper);

    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc, argv);

    // construct the viewer.
    osgViewer::Viewer viewer(arguments);

    // set up the camera manipulators.
    osgGA::KeySwitchMatrixManipulator* keyswitchManipulator;
    keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;

    keyswitchManipulator->addMatrixManipulator('1', "Trackball",
                                               new osgGA::TrackballManipulator);
    keyswitchManipulator->addMatrixManipulator('2', "Flight",
                                               new osgGA::FlightManipulator);
    keyswitchManipulator->addMatrixManipulator('3', "Drive",
                                               new osgGA::DriveManipulator);
    keyswitchManipulator->addMatrixManipulator('4', "Terrain",
                                               new osgGA::TerrainManipulator);

    viewer.setCameraManipulator(keyswitchManipulator);

    // Usefull stats
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::HelpHandler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::ThreadingHandler);
    viewer.addEventHandler(new osgViewer::LODScaleHandler);
    viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);

    // Sigh, we need our own cull visitor ...
    osg::Camera* camera = viewer.getCamera();
    osgViewer::Renderer* renderer = static_cast<osgViewer::Renderer*>(camera->getRenderer());
    for (int j = 0; j < 2; ++j) {
        osgUtil::SceneView* sceneView = renderer->getSceneView(j);
        sceneView->setCullVisitor(new simgear::EffectCullVisitor);
    }
    // Shaders expect valid fog
    osg::Fog* fog = new osg::Fog;
    fog->setMode(osg::Fog::EXP2);
    fog->setColor(osg::Vec4(1, 1, 1, 1));
    fog->setDensity(1e-6);
    camera->getOrCreateStateSet()->setAttribute(fog);

    std::string fg_root;
    if (arguments.read("--fg-root", fg_root)) {
    } else if (const char *fg_root_env = std::getenv("FG_ROOT")) {
        fg_root = fg_root_env;
    } else {
#if defined(PKGLIBDIR)
        fg_root = PKGLIBDIR;
#else
        fg_root = ".";
#endif
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
    string_list path_list = sgPathSplit(fg_scenery);
    osgDB::FilePathList filePathList;
    filePathList.push_back(fg_root);
    for (unsigned i = 0; i < path_list.size(); ++i) {
        SGPath pt(path_list[i]), po(path_list[i]);
        pt.append("Terrain");
        po.append("Objects");
        filePathList.push_back(path_list[i]);
        filePathList.push_back(pt.str());
        filePathList.push_back(po.str());
    }

    SGSharedPtr<SGPropertyNode> props = new SGPropertyNode;
    sgUserDataInit(props.get());
    try {
        SGPath preferencesFile = fg_root;
        preferencesFile.append("preferences.xml");
        readProperties(preferencesFile.str(), props);
    } catch (...) {
        // In case of an error, at least make summer :)
        props->getNode("sim/startup/season", true)->setStringValue("summer");

        std::cerr << "Problems loading FlightGear preferences.\n"
                  << "Probably FG_ROOT is not properly set." << std::endl;
    }
    SGMaterialLib* ml = new SGMaterialLib;
    SGPath mpath(fg_root);
    mpath.append("materials.xml");
    try {
        ml->load(fg_root, mpath.str(), props);
    } catch (...) {
        std::cerr << "Problems loading FlightGear materials.\n"
                  << "Probably FG_ROOT is not properly set." << std::endl;
    }
    simgear::SGModelLib::init(fg_root, props);

    // Set up the reader/writer options
    osg::ref_ptr<simgear::SGReaderWriterOptions> options;
    if (osgDB::Options* ropt = osgDB::Registry::instance()->getOptions())
        options = new simgear::SGReaderWriterOptions(*ropt);
    else
        options = new simgear::SGReaderWriterOptions;
    options->getDatabasePathList() = filePathList;
    options->setMaterialLib(ml);
    options->setPropertyNode(props);
    options->setPluginStringData("SimGear::FG_ROOT", fg_root);
    options->setPluginStringData("SimGear::FG_SCENERY", fg_scenery);
    osgDB::Registry::instance()->setOptions(options.get());

    // Here, all arguments are processed
    arguments.reportRemainingOptionsAsUnrecognized();
    arguments.writeErrorMessages(std::cerr);

    // read the scene from the list of file specified command line args.
    osg::ref_ptr<osg::Node> loadedModel;
    loadedModel = osgDB::readNodeFiles(arguments, options.get());

    // if no model has been successfully loaded report failure.
    if (!loadedModel.valid()) {
        std::cerr << arguments.getApplicationName()
                  << ": No data loaded" << std::endl;
        return EXIT_FAILURE;
    }

    // pass the loaded scene graph to the viewer.
    viewer.setSceneData(loadedModel.get());

    // We want on demand database paging
    viewer.setDatabasePager(new osgDB::DatabasePager);
    viewer.getDatabasePager()->setUpThreads(1, 1);

    return viewer.run();
}
