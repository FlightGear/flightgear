// fgviewer.cxx -- alternative flightgear viewer application
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

#include <osg/Version>
#include <osgDB/ReadFile>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/TerrainManipulator>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/misc/ResourceManager.hxx>

#include "ArgumentParser.hxx"
#include "Renderer.hxx"
#include "Viewer.hxx"

#if FG_HAVE_HLA
#include "HLACameraManipulator.hxx"
#include "HLAViewerFederate.hxx"
#endif


int
main(int argc, char** argv)
{
    /// Read arguments and environment variables.
    ArgumentParser arguments(argc, argv);

    sglog().set_log_classes(SG_ALL);
    sglog().set_log_priority(SG_ALERT);

    SGPath fg_root;
    std::string r;
    if (arguments.read("--fg-root", r)) {
        fg_root = SGPath::fromLocal8Bit(r.c_str());
    } else if (std::getenv("FG_ROOT")) {
        fg_root = SGPath::fromEnv("FG_ROOT");
    } else {
        fg_root = SGPath(PKGLIBDIR);
    }

    SGPath fg_scenery;
    std::string s;
    if (arguments.read("--fg-scenery", s)) {
        fg_scenery = SGPath::fromLocal8Bit(s.c_str());
    } else if (std::getenv("FG_SCENERY")) {
        fg_scenery = SGPath::fromEnv("FG_SCENERY");
    } else {
        SGPath path(fg_root);
        path.append("Scenery");
        fg_scenery = path;
    }

    SGSharedPtr<SGPropertyNode> props = new SGPropertyNode;
    try {
        SGPath preferencesFile = fg_root;
        preferencesFile.append("defaults.xml");
        readProperties(preferencesFile, props);
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

    std::string renderer;
    while (arguments.read("--renderer", renderer));

    if (arguments.read("--hla")) {
        props->setStringValue("hla/federate/federation", "rti:///FlightGear");
    }
    std::string federation;
    if (arguments.read("--federation", federation)) {
        props->setStringValue("hla/federate/federation", federation);
    }

    /// Start setting up the viewer windows and start feeding them.

    // construct the viewer.
    fgviewer::Viewer viewer(arguments);

    if (renderer.empty()) {
        // Currently just the defautl renderer. More to come.
        viewer.setRenderer(new fgviewer::Renderer);
        
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "Unknown renderer configuration \"" << renderer
               << "\" given on the command line.");
        return EXIT_FAILURE;
    }

    // A viewer configuration
    if (const SGPropertyNode* viewerNode = props->getChild("viewer")) {
        if (!viewer.readCameraConfig(*viewerNode)) {
            SG_LOG(SG_GENERAL, SG_ALERT, "Reading camera configuration failed.");
            return EXIT_FAILURE;
        }
    }

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
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getSceneDataGroup()->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::HelpHandler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::ThreadingHandler);
    viewer.addEventHandler(new osgViewer::LODScaleHandler);
    viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);

    // We want on demand database paging
    viewer.setDatabasePager(new osgDB::DatabasePager);
    viewer.getDatabasePager()->setUpThreads(1, 1);

    /// now set up the simgears required model stuff

    simgear::ResourceManager::instance()->addBasePath(fg_root, simgear::ResourceManager::PRIORITY_DEFAULT);
    // Just reference simgears reader writer stuff so that the globals get
    // pulled in by the linker ...
    // FIXME: make that more explicit clear and call an initialization function
    simgear::ModelRegistry::instance();

    // FIXME Ok, replace this by querying the root of the property tree
    sgUserDataInit(props.get());
    SGSceneFeatures::instance()->setTextureCompression(SGSceneFeatures::DoNotUseCompression);
    SGMaterialLibPtr ml = new SGMaterialLib;
    SGPath mpath(fg_root);
    mpath.append("Materials/default/materials.xml");
    try {
        ml->load(fg_root.local8BitStr(), mpath.local8BitStr(), props);
    } catch (...) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Problems loading FlightGear materials.\n"
               << "Probably FG_ROOT is not properly set.");
    }
    simgear::SGModelLib::init(fg_root.local8BitStr(), props);

    // Set up the reader/writer options
    osg::ref_ptr<simgear::SGReaderWriterOptions> options;
    if (osgDB::Options* ropt = osgDB::Registry::instance()->getOptions())
        options = new simgear::SGReaderWriterOptions(*ropt);
    else
        options = new simgear::SGReaderWriterOptions;
    osgDB::convertStringPathIntoFilePathList(fg_scenery.local8BitStr(),
                                             options->getDatabasePathList());
    options->setMaterialLib(ml);
    options->setPropertyNode(props);
    options->setPluginStringData("SimGear::FG_ROOT", fg_root.local8BitStr());
    // Omit building bounding volume trees, as the viewer will not run a simulation
    options->setPluginStringData("SimGear::BOUNDINGVOLUMES", "OFF");
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    options->setPluginStringData("SimGear::MAXTEXTURESIZE", std::to_string(max_texture_size? max_texture_size:8192));
    viewer.setReaderWriterOptions(options.get());

    // Here, all arguments are processed
    arguments.reportRemainingOptionsAsUnrecognized();
    arguments.writeErrorMessages(std::cerr);

    if (props->getNode("hla/federate/federation")) {
#if FG_HAVE_HLA
        const SGPropertyNode* federateNode = props->getNode("hla/federate");
        
        SGSharedPtr<fgviewer::HLAViewerFederate> viewerFederate;
        viewerFederate = new fgviewer::HLAViewerFederate;
        viewerFederate->setVersion(federateNode->getStringValue("version", "RTI13"));
        // viewerFederate->setConnectArguments(federateNode->getStringValue("connect-arguments", ""));
        viewerFederate->setFederateType(federateNode->getStringValue("type", "ViewerFederate"));
        viewerFederate->setFederateName(federateNode->getStringValue("name", ""));
        viewerFederate->setFederationExecutionName(federateNode->getStringValue("federation", ""));
        std::string objectModel;
        objectModel = federateNode->getStringValue("federation-object-model", "HLA/fg-local-fom.xml");
        if (SGPath(objectModel).isRelative()) {
            SGPath path = fg_root;
            path.append(objectModel);
            objectModel = path.str();
        }
        viewerFederate->setFederationObjectModel(objectModel);
        
        if (!viewerFederate->init()) {
            SG_LOG(SG_NETWORK, SG_ALERT, "Got error from federate init!");
        } else {
            viewer.setViewerFederate(viewerFederate.get());
            viewer.setCameraManipulator(new fgviewer::HLACameraManipulator(viewerFederate->getViewer()));
        }

#else
        SG_LOG(SG_GENERAL, SG_ALERT, "Unable to enter HLA/RTI viewer mode: HLA/RTI disabled at compile time.");
#endif
    }

    /// Read the model files that are configured.

    osg::ref_ptr<osg::Node> loadedModel;
    if (1 < arguments.argc()) {
        // read the scene from the list of file specified command line args.
        loadedModel = osgDB::readRefNodeFiles(arguments, options.get());
    } else {
        // if no arguments given resort to the whole world scenery
        options->setPluginStringData("SimGear::FG_EARTH", "ON");
        loadedModel = osgDB::readRefNodeFile("w180s90-360x180.spt", options.get());
    }

    // if no model has been successfully loaded report failure.
    if (!loadedModel.valid()) {
        SG_LOG(SG_GENERAL, SG_ALERT, arguments.getApplicationName()
               << ": No data loaded");
        return EXIT_FAILURE;
    }

    // pass the loaded scene graph to the viewer.
    viewer.insertSceneData(loadedModel.get());

    // Note that this does not affect the hla camera manipulator
    viewer.home();

    return viewer.run();
}
