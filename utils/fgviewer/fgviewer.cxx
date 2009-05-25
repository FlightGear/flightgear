#include <iostream>
#include <cstdlib>

#include <osg/ArgumentParser>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <simgear/props/props.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/SGReaderWriterBTGOptions.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/tgdb/TileEntry.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/model/modellib.hxx>

class DummyLoadHelper : public simgear::ModelLoadHelper {
public:
    virtual osg::Node *loadTileModel(const string& modelPath, bool)
    {
        try {
            SGSharedPtr<SGPropertyNode> prop = new SGPropertyNode;
            return simgear::SGModelLib::loadModel(modelPath, prop);
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
    sgUserDataInit(0);
    DummyLoadHelper dummyLoadHelper;
    simgear::TileEntry::setModelLoadHelper(&dummyLoadHelper);

    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc, argv);

    // construct the viewer.
    osgViewer::Viewer viewer(arguments);
    // ... for some reason, get rid of that FIXME!
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // Usefull stats
    viewer.addEventHandler(new osgViewer::HelpHandler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    // Same FIXME ...
    // viewer.addEventHandler(new osgViewer::ThreadingHandler);
    viewer.addEventHandler(new osgViewer::LODScaleHandler);
    viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);

    const char *fg_root_env = std::getenv("FG_ROOT");
    std::string fg_root;
    if (fg_root_env)
        fg_root = fg_root_env;
    else
        fg_root = ".";

    const char *fg_scenery_env = std::getenv("FG_SCENERY");
    string_list path_list;
    if (fg_scenery_env) {
        path_list = sgPathSplit(fg_scenery_env);
    } else {
        SGPath path(fg_root);
        path.append("Scenery");
        path_list.push_back(path.str());
    }
    osgDB::FilePathList filePathList;
    for (unsigned i = 0; i < path_list.size(); ++i) {
        SGPath pt(path_list[i]), po(path_list[i]);
        pt.append("Terrain");
        po.append("Objects");
        filePathList.push_back(path_list[i]);
        filePathList.push_back(pt.str());
        filePathList.push_back(po.str());
    }

    SGSharedPtr<SGPropertyNode> props = new SGPropertyNode;
    props->getNode("sim/startup/season", true)->setStringValue("summer");
    SGMaterialLib* ml = new SGMaterialLib;
    SGPath mpath(fg_root);
    mpath.append("materials.xml");
    try {
        ml->load(fg_root, mpath.str(), props);
    } catch (...) {
        std::cerr << "Problems loading FlightGear materials.\n"
                  << "Probably FG_ROOT is not properly set." << std::endl;
    }
    
    SGReaderWriterBTGOptions* btgOptions = new SGReaderWriterBTGOptions;
    btgOptions->getDatabasePathList() = filePathList;
    btgOptions->setMatlib(ml);
    
    // read the scene from the list of file specified command line args.
    osg::ref_ptr<osg::Node> loadedModel;
    loadedModel = osgDB::readNodeFiles(arguments, btgOptions);

    // if no model has been successfully loaded report failure.
    if (!loadedModel.valid()) {
        std::cerr << arguments.getApplicationName()
                  << ": No data loaded" << std::endl;
        return EXIT_FAILURE;
    }
    
    // pass the loaded scene graph to the viewer.
    viewer.setSceneData(loadedModel.get());
    
    return viewer.run();
}
