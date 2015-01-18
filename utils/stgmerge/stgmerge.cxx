// stgmerge.cxx -- combined STG models
//
// Copyright (C) 2015 Stuart Buchanan
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

#include <osg/MatrixTransform>
#include <osg/ArgumentParser>
#include <osg/Image>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <osgUtil/Optimizer>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/bvh/BVHNode.hxx>
#include <simgear/bvh/BVHLineSegmentVisitor.hxx>
#include <simgear/bvh/BVHPager.hxx>
#include <simgear/bvh/BVHPageNode.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/BVHPageNodeOSG.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/tgdb/apt_signs.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/util/OptionsReadFileCallback.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>


namespace sg = simgear;

struct _ObjectStatic {
    _ObjectStatic() : _lon(0), _lat(0), _elev(0), _hdg(0), _pitch(0), _roll(0), _shared(false) { }
    std::string _token;
    std::string _name;
    double _lon, _lat, _elev;
    double _hdg, _pitch, _roll;
    bool _shared;
    osg::ref_ptr<sg::SGReaderWriterOptions> _options;
};

std::list<_ObjectStatic> _objectStaticList;

sg::SGReaderWriterOptions* staticOptions(const std::string& filePath, const osgDB::Options* options)
{
    osg::ref_ptr<sg::SGReaderWriterOptions> staticOptions;
    staticOptions = sg::SGReaderWriterOptions::copyOrCreate(options);
    staticOptions->getDatabasePathList().clear();

    staticOptions->getDatabasePathList().push_back(filePath);
    staticOptions->setObjectCacheHint(osgDB::Options::CACHE_NONE);

    // Every model needs its own SGModelData to ensure load/unload is
    // working properly
    staticOptions->setModelData
    (
        staticOptions->getModelData()
      ? staticOptions->getModelData()->clone()
      : 0
    );

    return staticOptions.release();
}

sg::SGReaderWriterOptions* sharedOptions(const std::string& filePath, const osgDB::Options* options)
{
    osg::ref_ptr<sg::SGReaderWriterOptions> sharedOptions;
    sharedOptions = sg::SGReaderWriterOptions::copyOrCreate(options);
    sharedOptions->getDatabasePathList().clear();

    SGPath path = filePath;
    path.append(".."); path.append(".."); path.append("..");
    sharedOptions->getDatabasePathList().push_back(path.str());

    // ensure Models directory synced via TerraSync is searched before the copy in
    // FG_ROOT, so that updated models can be used.
    std::string terrasync_root = options->getPluginStringData("SimGear::TERRASYNC_ROOT");
    if (!terrasync_root.empty()) {
        sharedOptions->getDatabasePathList().push_back(terrasync_root);
    }

    std::string fg_root = options->getPluginStringData("SimGear::FG_ROOT");
    sharedOptions->getDatabasePathList().push_back(fg_root);

    // TODO how should we handle this for OBJECT_SHARED?
    sharedOptions->setModelData
    (
        sharedOptions->getModelData()
      ? sharedOptions->getModelData()->clone()
      : 0
    );

    return sharedOptions.release();
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

    std::string stg;
    if (! arguments.read("--stg", stg)) {
        SG_LOG(SG_GENERAL, SG_ALERT, "No --stg argument");
        return EXIT_FAILURE;
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
    SGMaterialLibPtr ml = new SGMaterialLib;
    SGPath mpath(fg_root);

    // TODO: Pick up correct materials.xml file.  Urrgh - this can't be runtime dependent...
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

    // Here, all arguments are processed
    arguments.reportRemainingOptionsAsUnrecognized();
    arguments.writeErrorMessages(std::cerr);

    // Get the STG file
    if (stg.empty()) {
    	SG_LOG(SG_TERRAIN, SG_ALERT, "STG file empty");
    	return EXIT_FAILURE;
    }

	SG_LOG(SG_TERRAIN, SG_ALERT, "Loading stg file " << stg);

	sg_gzifstream stream(stg);
	if (!stream.is_open()) {
    	SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to open STG file " << stg);
		return EXIT_FAILURE;
	}

	// Extract the bucket from the filename
	std::istringstream ss(osgDB::getStrippedName(stg));
	long index;
	ss >> index;
	if (ss.fail()) {
    	SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to determine bucket from STG filename " << ss);
		return EXIT_FAILURE;
	}

	// Work out the transform to the center of the tile
	SGBucket bucket = SGBucket(index);
    osg::Matrix tile_transform;
    tile_transform = makeZUpFrame(bucket.get_center());

    // Inverse used to translate individual matrices
    SGVec3d shift;
    SGGeodesy::SGGeodToCart(bucket.get_center(), shift);

	std::string filePath = osgDB::getFilePath(stg);

	while (!stream.eof()) {
		// read a line
		std::string line;
		std::getline(stream, line);

		// strip comments
		std::string::size_type hash_pos = line.find('#');
		if (hash_pos != std::string::npos)
			line.resize(hash_pos);

		// and process further
		std::stringstream in(line);

		std::string token;
		in >> token;

		// No comment
		if (token.empty())
			continue;

		// Then there is always a name
		std::string name;
		in >> name;

		SGPath path = filePath;
		path.append(name);

		//  The following tokens are ignored
		// OBJECT_BASE - base scenery, not relevant
		// OBJECT - airport BTG files
		// OBJECT_STATIC_AGL - elevation needs to be calculated at runtime
		// OBJECT_SHARED_AGL - elevation needs to be calculated at runtime
		// OBJECT_SIGN - depends on materials library, which is runtime dependent

		if (token == "OBJECT_STATIC") {
			osg::ref_ptr<sg::SGReaderWriterOptions> opt;
			opt = staticOptions(filePath, options);
			//if (SGPath(name).lower_extension() == "ac")
			//opt->setInstantiateEffects(true);  // Is this output correctly?
			_ObjectStatic obj;
			obj._token = token;
			obj._name = name;
			in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg >> obj._pitch >> obj._roll;
			obj._shared = false;
			obj._options = opt;
			_objectStaticList.push_back(obj);
		} else if (token == "OBJECT_SHARED") {
			osg::ref_ptr<sg::SGReaderWriterOptions> opt;
			opt = sharedOptions(filePath, options);
			//if (SGPath(name).lower_extension() == "ac")
			//opt->setInstantiateEffects(true);  // Is this output correctly?
			_ObjectStatic obj;
			obj._token = token;
			obj._name = name;
			in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg >> obj._pitch >> obj._roll;
			obj._shared = true;
			obj._options = opt;
			_objectStaticList.push_back(obj);
		} else {
			SG_LOG( SG_TERRAIN, SG_ALERT, "Ignoring token '" << token << "'" );
			std::cout << line << "\n";
		}
	}

	//  We now have a list of objects and signs to process.

    osg::ref_ptr<osg::Group> group = new osg::Group;
    group->setName("STG merge");
    group->setDataVariance(osg::Object::STATIC);
    int files_loaded = 0;

    for (std::list<_ObjectStatic>::iterator i = _objectStaticList.begin(); i != _objectStaticList.end(); ++i) {

    	// We don't process XML files, as they typically include animations which we can't output
    	if (SGPath(i->_name).lower_extension() == "xml") {
    		//SG_LOG(SG_TERRAIN, SG_ALERT, "Ignoring non-static "
 			//	   << i->_token << " '" << i->_name << "'");
    		std::cout << (i->_shared ? "OBJECT_SHARED " : "OBJECT_STATIC ");
    		std::cout << i->_lon << " " << i->_lat << " " << i->_elev << " " << i->_hdg;
    		std::cout << " " << i->_pitch << i->_roll << "\n";
    		continue;
    	}

		SG_LOG( SG_TERRAIN, SG_ALERT, "Processing " << i->_name);

        osg::ref_ptr<osg::Node> node;
		node = osgDB::readRefNodeFile(i->_name, i->_options.get());
		if (!node.valid()) {
			SG_LOG(SG_TERRAIN, SG_ALERT, stg << ": Failed to load "
				   << i->_token << " '" << i->_name << "'");
			continue;
		}
		files_loaded++;

        if (SGPath(i->_name).lower_extension() == "ac")
            node->setNodeMask(~sg::MODELLIGHT_BIT);

        osg::Matrix matrix;
        matrix = makeZUpFrame(SGGeod::fromDegM(i->_lon, i->_lat, i->_elev));
        matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(i->_hdg), osg::Vec3(0, 0, 1)));
        matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(i->_pitch), osg::Vec3(0, 1, 0)));
        matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(i->_roll), osg::Vec3(1, 0, 0)));

        osg::MatrixTransform* matrixTransform;
        matrixTransform = new osg::MatrixTransform(matrix);
        matrixTransform->setName("positionStaticObject");
        matrixTransform->setDataVariance(osg::Object::STATIC);
        matrixTransform->addChild(node.get());

        // Shift the models so they are centered on the center of the tile.
        // We will place the object at the center of the tile later.
        osg::Matrix unshift;
        unshift.makeTranslate(- toOsg(shift));
        osg::MatrixTransform* unshiftTransform = new osg::MatrixTransform();
        unshiftTransform->addChild(matrixTransform);
        group->addChild(unshiftTransform);
    }

    if (files_loaded == 0) {
    	// Nothing to do - no models were changed.

    }

    // Create a viewer - required for some Optimizers
    osgViewer::Viewer viewer;
    viewer.setSceneData(group.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(
    		new osgGA::StateSetManipulator(
    			viewer.getCamera()->getOrCreateStateSet()));
    			viewer.setCameraManipulator(new osgGA::TrackballManipulator());
    viewer.realize();

    // Write out the pre-optimized version
    osgDB::writeNodeFile(*group, "old.osgb",
    		new osgDB::Options("WriteImageHint=IncludeData Compressor=zlib"));

    // Run the Optimizer
    osgUtil::Optimizer optimizer;
    optimizer.optimize( group, osgUtil::Optimizer::ALL_OPTIMIZATIONS );

    // Serialize the result as a binary OSG file, including textures:
    osgDB::writeNodeFile(*group, "new.osgb",
    		new osgDB::Options("WriteImageHint=IncludeData Compressor=zlib"));

    // Write out the required STG entry for this merged set of objects, centered
    // on the center of the tile.
    std::cout << "Files loaded " << files_loaded << "\n";
    std::cout << "OBJECT_STATIC static.osgb "
    	 << bucket.get_center_lon() << " "
    	 << bucket.get_center_lat() << "0.0 0.0 0.0 0.0\n";

    viewer.run();
    return EXIT_SUCCESS;
}
