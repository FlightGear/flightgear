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
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/model/modellib.hxx>

int
main(int argc, char** argv)
{
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

        std::cerr << "Problems loading FlightGear preferences.\n"
                  << "Probably FG_ROOT is not properly set." << std::endl;
    }

    std::string config;
    while (arguments.read("--config", config)) {
        try {
            readProperties(config, props);
        } catch (...) {
            std::cerr << "Problems loading config file \"" << config
                      << "\" given on the command line." << std::endl;
        }
    }

    std::string prop, value;
    while (arguments.read("--prop", prop, value)) {
        props->setStringValue(prop, value);
    }

    // Just reference simgears reader writer stuff so that the globals get
    // pulled in by the linker ...
    // FIXME: make that more explicit clear and call an initialization function
    simgear::ModelRegistry::instance();

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

    sgUserDataInit(props.get());
    SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::DoNotUseCompression);
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
    osgDB::convertStringPathIntoFilePathList(fg_scenery,
                                             options->getDatabasePathList());
    options->setMaterialLib(ml);
    options->setPropertyNode(props);
    options->setPluginStringData("SimGear::FG_ROOT", fg_root);
    // We have some problems here, rethink them at some time
    options->setPluginStringData("SimGear::PARTICLESYSTEM", "OFF");
    // Omit building bounding volume trees, as the viewer will not run a simulation
    options->setPluginStringData("SimGear::BOUNDINGVOLUMES", "OFF");

    // Here, all arguments are processed
    arguments.reportRemainingOptionsAsUnrecognized();
    arguments.writeErrorMessages(std::cerr);

    osg::ref_ptr<osg::Node> loadedModel;
    if (arguments.argc() != 1) {
        // read the scene from the list of file specified command line args.
        loadedModel = osgDB::readNodeFiles(arguments, options.get());
    } else {
        // if no arguments given resort to the whole world scenery
        options->setPluginStringData("SimGear::FG_EARTH", "ON");
        loadedModel = osgDB::readNodeFile("w180s90-360x180.spt", options.get());
    }

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
