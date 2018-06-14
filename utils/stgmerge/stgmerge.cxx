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
#include <string>
#include <cstdlib>
#include <iomanip>
#include <dirent.h>

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
#include <simgear/math/SGGeodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
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
	_ObjectStatic() :
			_lon(0), _lat(0), _elev(0), _hdg(0), _pitch(0), _roll(0), _shared(false) {
	}
	std::string _token;
	std::string _name;
	double _lon, _lat, _elev;
	double _hdg, _pitch, _roll;
	bool _shared;
	osg::ref_ptr<sg::SGReaderWriterOptions> _options;
};

std::string fg_root;
std::string fg_scenery;
std::string fg_terrasync_root;
std::string input;
std::string output;
bool display_viewer = false;
bool osg_optimizer = false;
bool copy_files = false;
int group_size = 5000;

sg::SGReaderWriterOptions* staticOptions(const std::string& filePath,
		const osgDB::Options* options) {
	osg::ref_ptr<sg::SGReaderWriterOptions> staticOptions;
	staticOptions = sg::SGReaderWriterOptions::copyOrCreate(options);
	staticOptions->getDatabasePathList().clear();

	staticOptions->getDatabasePathList().push_back(filePath);
	staticOptions->setObjectCacheHint(osgDB::Options::CACHE_NONE);

	// Every model needs its own SGModelData to ensure load/unload is
	// working properly
	staticOptions->setModelData(
			staticOptions->getModelData() ?
					staticOptions->getModelData()->clone() : 0);

	return staticOptions.release();
}

sg::SGReaderWriterOptions* sharedOptions(const std::string& filePath,
		const osgDB::Options* options) {
	osg::ref_ptr<sg::SGReaderWriterOptions> sharedOptions;
	sharedOptions = sg::SGReaderWriterOptions::copyOrCreate(options);
	sharedOptions->getDatabasePathList().clear();

	SGPath path(filePath);
	path.append("..");
	path.append("..");
	path.append("..");
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
	sharedOptions->setModelData(
			sharedOptions->getModelData() ?
					sharedOptions->getModelData()->clone() : 0);

	return sharedOptions.release();
}

int processSTG(osg::ref_ptr<simgear::SGReaderWriterOptions> options,
		std::string stg) {

	// List of .ac files to remove
	std::vector<std::string> ac_files;

	// Get the STG file
	if (stg.empty()) {
		SG_LOG(SG_TERRAIN, SG_ALERT, "STG file empty");
		return EXIT_FAILURE;
	}

	SGPath sourcestg(input);
	sourcestg.append(stg);

	SGPath deststg(output);
	deststg.append(stg);

	SG_LOG(SG_TERRAIN, SG_INFO, "Processing " << sourcestg.c_str());

	sg_gzifstream stream((std::string) sourcestg.c_str());
	if (!stream.is_open()) {
		SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to open STG file " << sourcestg.c_str());
		return EXIT_FAILURE;
	}

	// Extract the bucket from the filename
	std::istringstream ss(osgDB::getStrippedName(sourcestg.file().c_str()));
	long index;
	ss >> index;
	if (ss.fail()) {
		SG_LOG(SG_TERRAIN, SG_ALERT,
				"Unable to determine bucket from STG filename " << ss.rdbuf());
		return EXIT_FAILURE;
	}

	SGBucket bucket = SGBucket(index);

	// We will group the object into group_size x group_size
	// block, so work out the number and dimensions in degrees
	// of each block.
	int lon_blocks = (int) ceil(bucket.get_width_m() / group_size);
	int lat_blocks = (int) ceil(bucket.get_height_m() / group_size);
	float lat_delta = bucket.get_height() / lat_blocks;
	float lon_delta = bucket.get_width() / lon_blocks;
	SG_LOG(SG_TERRAIN, SG_DEBUG,
			"Splitting into (" << lon_blocks << "," << lat_blocks << ") blocks"
			<< "of size (" << lon_delta << "," << lat_delta << ") degrees\n");

	std::list<_ObjectStatic> _objectStaticList[lon_blocks][lat_blocks];

	// Write out the STG files.
	SG_LOG(SG_TERRAIN, SG_DEBUG, "Writing to " << deststg.c_str());
	std::ofstream stgout(deststg.c_str(), std::ofstream::out);

	if (!stgout.is_open()) {
		SG_LOG(SG_TERRAIN, SG_ALERT,
				"Unable to open STG file to write " << deststg.c_str());
		return EXIT_FAILURE;
	}

	while (!stream.eof()) {
		// read a line
		std::string line;
		std::getline(stream, line);

		// strip comments
		std::string::size_type hash_pos = line.find('#');

		if (hash_pos == 0)
			stgout << line << "\n";

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

		//  The following tokens are ignored
		// OBJECT_BASE - base scenery, not relevant
		// OBJECT - airport BTG files
		// OBJECT_STATIC_AGL - elevation needs to be calculated at runtime
		// OBJECT_SHARED_AGL - elevation needs to be calculated at runtime
		// OBJECT_SIGN - depends on materials library, which is runtime dependent

		if ((token == "OBJECT_STATIC") || (token == "OBJECT_SHARED")) {
			if (SGPath(name).lower_extension() != "ac") {
				// We only attempt to merge static .ac objects, as they don't have any
				// animations
				SG_LOG(SG_TERRAIN, SG_DEBUG, "Ignoring non .ac file '" << name << "'");
				stgout << line << "\n";
				continue;
			}

			// If we merge it, then we should remove the .ac file from the output.
			ac_files.push_back(name);

			SGPath filePath(sourcestg.dir());
			filePath.append(name);

			_ObjectStatic obj;
			osg::ref_ptr<sg::SGReaderWriterOptions> opt;

			if (token == "OBJECT_STATIC") {
				opt = staticOptions(deststg.dir().c_str(), options);
				//if (SGPath(name).lower_extension() == "ac")
				//opt->setInstantiateEffects(true);  // Is this output correctly?
				obj._shared = false;
			} else if (token == "OBJECT_SHARED") {
				opt = sharedOptions(deststg.dir().c_str(), options);
				//if (SGPath(name).lower_extension() == "ac")
				//opt->setInstantiateEffects(true);  // Is this output correctly?
				obj._shared = true;
			} else {
				SG_LOG(SG_TERRAIN, SG_ALERT, "Broken code - unexpected object '" << token << "'");
			}

			obj._token = token;
			obj._name = name;
			obj._options = opt;
			in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg >> obj._pitch
					>> obj._roll;

			// Sanity check this object is in the correct bucket
			if ((obj._lon < bucket.get_corner(0).getLongitudeDeg()) ||
					(obj._lon > bucket.get_corner(2).getLongitudeDeg()) ||
					(obj._lat < bucket.get_corner(0).getLatitudeDeg())  ||
					(obj._lat > bucket.get_corner(2).getLatitudeDeg())     ) {
				SG_LOG(SG_TERRAIN, SG_ALERT,
						"File " << sourcestg.c_str() << " contains object outside bounds of bucket:\n" <<
						" Bucket bounds (lat, lon) : (" <<
						bucket.get_corner(0).getLatitudeDeg() << ", " <<
						bucket.get_corner(0).getLongitudeDeg() << ") - (" <<
						bucket.get_corner(2).getLatitudeDeg() << ", " <<
						bucket.get_corner(2).getLongitudeDeg() << ") \n" <<
						line << "\n");

					// Simply leave it in place for the moment.
					stgout << line << "\n";
			} else {
				// Determine the correct bucket for this object.
				int x = (int) floor((obj._lon - bucket.get_corner(0).getLongitudeDeg()) / lon_delta);
				int y = (int) floor((obj._lat - bucket.get_corner(0).getLatitudeDeg()) / lat_delta);
				SG_LOG(SG_TERRAIN, SG_INFO,
						"Assigned (" << obj._lon << "," << obj._lat << ") to block (" << x << "," << y << ")");

				_objectStaticList[x][y].push_back(obj);
			}
		} else {
			SG_LOG(SG_TERRAIN, SG_DEBUG, "Ignoring token '" << token << "'");
			stgout << line << "\n";
		}
	}

	stream.close();

	for (int x = 0; x < lon_blocks; ++x) {
		for (int y = 0; y < lat_blocks; ++y) {

			if (_objectStaticList[x][y].size() == 0) {
				// Nothing to do, so skip
				continue;
			}

			//SG_LOG(SG_TERRAIN, SG_ALERT, "Object files " << _objectStaticList[x][y].size());

			osg::ref_ptr<osg::Group> group = new osg::Group;
			group->setName("STG merge");
			group->setDataVariance(osg::Object::STATIC);
			int files_loaded = 0;

			// Calculate center of this block
			const SGGeod center = SGGeod::fromDegM(
					bucket.get_center_lon() - 0.5 * bucket.get_width() + (double) ((x +0.5) * lon_delta),
					bucket.get_center_lat() - 0.5 * bucket.get_height() + (double) ((y + 0.5) * lat_delta),
					0.0);

			//SG_LOG(SG_TERRAIN, SG_ALERT,
			//		"Center of block: " << center.getLongitudeDeg() << ", " << center.getLatitudeDeg());

			// Inverse used to translate individual matrices
			SGVec3d shift;
			SGGeodesy::SGGeodToCart(center , shift);

			for (std::list<_ObjectStatic>::iterator i = _objectStaticList[x][y].begin();
					i != _objectStaticList[x][y].end(); ++i) {

				SG_LOG(SG_TERRAIN, SG_INFO, "Processing " << i->_name);

				osg::ref_ptr<osg::Node> node;
				node = osgDB::readRefNodeFile(i->_name, i->_options.get());
				if (!node.valid()) {
					SG_LOG(SG_TERRAIN, SG_ALERT,
							stg << ": Failed to load " << i->_token << " '" << i->_name << "'");
					continue;
				}
				files_loaded++;

				if (SGPath(i->_name).lower_extension() == "ac")
					node->setNodeMask(~sg::MODELLIGHT_BIT);

				const SGGeod q = SGGeod::fromDegM(i->_lon, i->_lat, i->_elev);
				SGVec3d coord;
				SGGeodesy::SGGeodToCart(q, coord);
				coord = coord - shift;

				// Create an matrix to convert from global coordinates to the
				// Z-Up local coordinate system used by scenery models.
				// This is simply the inverse of the normal scenery model
				// matrix.
				osg::Matrix m = makeZUpFrameRelative(center);
				osg::Matrix inv = osg::Matrix::inverse(m);
				osg::Vec3f v = toOsg(coord) * inv;

				osg::Matrix matrix;
				matrix.setTrans(v);
				matrix.preMultRotate(
						osg::Quat(SGMiscd::deg2rad(i->_hdg), osg::Vec3(0, 0, 1)));
				matrix.preMultRotate(
						osg::Quat(SGMiscd::deg2rad(i->_pitch), osg::Vec3(0, 1, 0)));
				matrix.preMultRotate(
						osg::Quat(SGMiscd::deg2rad(i->_roll), osg::Vec3(1, 0, 0)));

				osg::MatrixTransform* matrixTransform;
				matrixTransform = new osg::MatrixTransform(matrix);
				matrixTransform->setName("positionStaticObject");
				matrixTransform->setDataVariance(osg::Object::STATIC);
				matrixTransform->addChild(node.get());

				// Shift the models so they are centered on the center of the block.
				// We will place the object at the right position in the tile later.
				group->addChild(matrixTransform);
			}

			osgViewer::Viewer viewer;

			if (osg_optimizer || display_viewer) {
				// Create a viewer - required for some Optimizers and if we are to display
				// the results
				viewer.setSceneData(group.get());
				viewer.addEventHandler(new osgViewer::StatsHandler);
				viewer.addEventHandler(new osgViewer::WindowSizeHandler);
				viewer.addEventHandler(
						new osgGA::StateSetManipulator(
								viewer.getCamera()->getOrCreateStateSet()));
				viewer.setCameraManipulator(new osgGA::TrackballManipulator());
				viewer.realize();
			}

			if (osg_optimizer) {
				// Run the Optimizer
				osgUtil::Optimizer optimizer;

				//  See osgUtil::Optimizer for list of optimizations available.
				//optimizer.optimize(group, osgUtil::Optimizer::ALL_OPTIMIZATIONS);
				int optimizationOptions = osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS
						| osgUtil::Optimizer::REMOVE_REDUNDANT_NODES
						| osgUtil::Optimizer::COMBINE_ADJACENT_LODS
						| osgUtil::Optimizer::SHARE_DUPLICATE_STATE
						| osgUtil::Optimizer::MERGE_GEOMETRY
						| osgUtil::Optimizer::MAKE_FAST_GEOMETRY
						| osgUtil::Optimizer::SPATIALIZE_GROUPS
						| osgUtil::Optimizer::OPTIMIZE_TEXTURE_SETTINGS
						| osgUtil::Optimizer::TEXTURE_ATLAS_BUILDER
						| osgUtil::Optimizer::CHECK_GEOMETRY
						| osgUtil::Optimizer::STATIC_OBJECT_DETECTION;

				optimizer.optimize(group, optimizationOptions);
			}

			// Serialize the result as a binary OSG file, including textures:
			std::string filename = sourcestg.file();

			// Include both the STG name and the indexes for uniqueness.
			// ostringstream required for compilers that don't support C++11
			std::ostringstream oss;
			oss << x << y;
			filename.append(oss.str());
			filename.append(".osg");
			SGPath osgpath = SGPath(deststg.dir(), filename);
			osgDB::writeNodeFile(*group, osgpath.c_str(),
					new osgDB::Options("WriteImageHint=IncludeData Compressor=zlib"));

			// Write out the required STG entry for this merged set of objects, centered
			// on the center of the tile.
			stgout << "OBJECT_STATIC " << osgpath.file() << " " << center.getLongitudeDeg()
					<< " " << center.getLatitudeDeg() << " 0.0 0.0 0.0 0.0\n";

			if (display_viewer) {
				viewer.run();
			}
		}
	}

	if (copy_files) {
		std::vector<std::string>::const_iterator iter;

		for (iter = ac_files.begin(); iter != ac_files.end(); ++iter) {
			SGPath acfile = SGPath(deststg.dir());
			acfile.append(*iter);
			if (acfile.isFile()) {
				if (remove(acfile.c_str()) != 0) {
					SG_LOG(SG_GENERAL, SG_ALERT, "Unable to remove " << acfile.c_str());
				}
			}
		}
	}

	// Finished with this file.
	stgout.flush();
	stgout.close();

	return EXIT_SUCCESS;
}

int processDirectory(osg::ref_ptr<simgear::SGReaderWriterOptions> options,
		SGPath directory_name) {

	// List of STG files to process
	std::vector<std::string> stg_files;

	SGPath source = SGPath(input);
	source.append(directory_name.c_str());

	SG_LOG(SG_GENERAL, SG_INFO, "Processing " << source.c_str());

	DIR *dir = NULL;
	dir = opendir(source.c_str());

	if (dir == NULL) {
		SG_LOG(SG_GENERAL, SG_ALERT, "Unable to open " << source.c_str());
		return EXIT_FAILURE;
	}

	if (copy_files) {
		// Create the directory in the output directory
		SGPath destination = SGPath(output);
		destination.append(directory_name.c_str());
		destination.append(".");  //  Do this so the path is a directory rather than a file!
		if (destination.exists()) {
			if (! destination.isDir()) {
				SG_LOG(SG_GENERAL, SG_ALERT, destination.c_str() << " exists and is not a directory.");
				return EXIT_FAILURE;
			}
			if (! destination.canWrite()) {
				SG_LOG(SG_GENERAL, SG_ALERT, destination.c_str() << " cannot be written.");
				return EXIT_FAILURE;
			}
		} else {

			// Note that this create the directory part of the path,
			int success = destination.create_dir();

			if (success != 0) {
				SG_LOG(SG_GENERAL, SG_ALERT, "Unable to create " << destination.dir());
				return EXIT_FAILURE;
			}

			if (! destination.exists()) {
				SG_LOG(SG_GENERAL, SG_ALERT, "Failed to create " << destination.dir());
				return EXIT_FAILURE;
			}
		}
	}

	struct dirent *pent = NULL;
	pent = readdir(dir);

	while (pent != NULL) {
		std::string fname = pent->d_name;
		SG_LOG(SG_GENERAL, SG_DEBUG, "  checking " << fname.c_str());

		if ((fname.compare(".") == 0) || (fname.compare("..") == 0)) {
			pent = readdir(dir);
			continue;
		}

		SGPath fpath = SGPath(input);
		fpath.append(directory_name.c_str());
		fpath.append(fname);

	  if (fpath.isDir()) {
			SGPath dirname = SGPath(directory_name);
			dirname.append(fname);
			if (processDirectory(options, dirname) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
    } else if (SGPath(fname).lower_extension() == "stg") {
			stg_files.push_back(fname);
		} else if (copy_files) {
			// Copy it over if we're copying all files.
			SGPath fsource = SGPath(input);
			fsource.append(directory_name.c_str());
			fsource.append(fname);
			SGPath fdestination = SGPath(output);
			fdestination.append(directory_name.c_str());
			fdestination.append(fname);

			SG_LOG(SG_GENERAL, SG_DEBUG, "Copying " << fsource.c_str() << " to " << fdestination.c_str());
			std::ifstream src(fsource.c_str(), std::ios::binary);
			std::ofstream dst(fdestination.c_str(), std::ios::binary);

			dst << src.rdbuf();
		}
		pent = readdir(dir);
	}

	closedir(dir);

	// Now we've copied the data, process the STG files
	std::vector<std::string>::const_iterator iter;

	for (iter = stg_files.begin(); iter != stg_files.end(); ++iter) {
		SGPath stg = SGPath(directory_name);
		stg.append(*iter);
		processSTG(options, stg.c_str());
	}

	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	osg::ApplicationUsage* usage = new osg::ApplicationUsage();
	usage->setApplicationName("stgmerge");
	usage->setCommandLineUsage(
			"Merge static model files within a given STG file.");
	usage->addCommandLineOption("--input <dir>", "Scenery directory to read");
	usage->addCommandLineOption("--output <dir>",
			"Output directory for STGs and merged models");
	usage->addCommandLineOption("--fg-root <dir>", "FG root directory",
			"$FG_ROOT");
	usage->addCommandLineOption("--fg-scenery <dir>", "FG scenery path",
			"$FG_SCENERY");
	usage->addCommandLineOption("--terrasync <dir>", "Terrasync path (for Models)");
	usage->addCommandLineOption("--group-size <N>", "Group size (m)", "5000");
	usage->addCommandLineOption("--optimize", "Optimize scene-graph");
	usage->addCommandLineOption("--viewer", "Display loaded objects");
	usage->addCommandLineOption("--copy-files", "Copy all contents of input directory into output directory");

	// use an ArgumentParser object to manage the program arguments.
	osg::ArgumentParser arguments(&argc, argv);

	arguments.setApplicationUsage(usage);

	if (arguments.read("--fg-root", fg_root)) {
	} else if (const char *fg_root_env = std::getenv("FG_ROOT")) {
		fg_root = fg_root_env;
	} else {
		fg_root = PKGLIBDIR;
	}

	if (arguments.read("--fg-scenery", fg_scenery)) {
	} else if (const char *fg_scenery_env = std::getenv("FG_SCENERY")) {
		fg_scenery = fg_scenery_env;
	} else {
		SGPath path(fg_root);
		path.append("Scenery");
		fg_scenery = path.str();
	}

	if (arguments.read("--terrasync", fg_terrasync_root)) {
	} else {
		fg_scenery = "";
	}

	if (!arguments.read("--input", input)) {
		arguments.reportError("--input argument required.");
	} else {
		SGPath s(input);
		if (!s.isDir()) {
			arguments.reportError(
					"--input directory does not exist or is not directory.");
		} else if (!s.canRead()) {
			arguments.reportError(
					"--input directory cannot be read. Check permissions.");
		}
	}

	if (!arguments.read("--output", output)) {
		arguments.reportError("--output argument required.");
	} else {
		// Check directory exists, we can write to it, and we're not about to write
		// to the same location as the STG file.
		SGPath p(output);
		SGPath s(input);
		if (!p.isDir()) {
			arguments.reportError("--output directory does not exist.");
		}

		if (!p.canWrite()) {
			arguments.reportError(
					"--output directory is not writeable. Check permissions.");
		}

		if (s == p) {
			arguments.reportError(
					"--output directory must differ from STG directory.");
		}
	}

	if (arguments.read("--group-size")) {
		if (! arguments.read("--group-size", group_size)) {
			arguments.reportError(
								"--group-size argument number be a positive integer.");
		}
	}

	if (arguments.read("--viewer")) {
		display_viewer = true;
	}

	if (arguments.read("--optimize")) {
		osg_optimizer = true;
	}

	if (arguments.read("--copy-files")) {
		copy_files = true;
	}

	if (arguments.errors()) {
		arguments.writeErrorMessages(std::cout);
		arguments.getApplicationUsage()->write(std::cout,
				osg::ApplicationUsage::COMMAND_LINE_OPTION
						| osg::ApplicationUsage::ENVIRONMENTAL_VARIABLE, 80, true);
		return EXIT_FAILURE;
	}

	SGSharedPtr<SGPropertyNode> props = new SGPropertyNode;
	try {
		SGPath preferencesFile = fg_root;
		preferencesFile.append("defaults.xml");
		readProperties(preferencesFile.str(), props);
	} catch (...) {
		// In case of an error, at least make summer :)
		props->getNode("sim/startup/season", true)->setStringValue("summer");

		SG_LOG(SG_GENERAL, SG_ALERT,
				"Problems loading FlightGear preferences.\n" << "Probably FG_ROOT is not properly set.");
	}

	/// now set up the simgears required model stuff

	simgear::ResourceManager::instance()->addBasePath(fg_root,
			simgear::ResourceManager::PRIORITY_DEFAULT);
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
		SG_LOG(SG_GENERAL, SG_ALERT,
				"Problems loading FlightGear materials.\n" << "Probably FG_ROOT is not properly set.");
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
	options->setPluginStringData("SimGear::TERRASYNC_ROOT", fg_terrasync_root);

	// Here, all arguments are processed
	arguments.reportRemainingOptionsAsUnrecognized();
	arguments.writeErrorMessages(std::cerr);

	std::string dot = "";
	return processDirectory(options, dot);
}
