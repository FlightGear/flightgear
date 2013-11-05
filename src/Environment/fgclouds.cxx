// Build a cloud layer based on metar
//
// Written by Harald JOHNSEN, started April 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
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
//
//

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "fgclouds.hxx"

#include <cstring>
#include <cstdio>
#include <Main/fg_props.hxx>

#include <simgear/constants.h>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/scene/sky/sky.hxx>
//#include <simgear/environment/visual_enviro.hxx>
#include <simgear/scene/sky/cloudfield.hxx>
#include <simgear/scene/sky/newcloud.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Viewer/renderer.hxx>
#include <Airports/airport.hxx>

// RNG seed to ensure cloud synchronization across multi-process
// deployments
static mt seed;

FGClouds::FGClouds() :
    clouds_3d_enabled(false),
    index(0)
{
	update_event = 0;
}

FGClouds::~FGClouds()
{
    globals->get_commands()->removeCommand("add-cloud");
	globals->get_commands()->removeCommand("del-cloud");
	globals->get_commands()->removeCommand("move-cloud");

}

int FGClouds::get_update_event(void) const {
	return update_event;
}

void FGClouds::set_update_event(int count) {
	update_event = count;
	buildCloudLayers();
}

void FGClouds::Init(void)
{
	mt_init_time_10(&seed);

	globals->get_commands()->addCommand("add-cloud", this, &FGClouds::add3DCloud);
	globals->get_commands()->addCommand("del-cloud", this, &FGClouds::delete3DCloud);
	globals->get_commands()->addCommand("move-cloud", this, &FGClouds::move3DCloud);
}

// Build an invidual cloud. Returns the extents of the cloud for coverage calculations
double FGClouds::buildCloud(SGPropertyNode *cloud_def_root, SGPropertyNode *box_def_root,
                            const std::string& name, double grid_z_rand, SGCloudField *layer)
{
	SGPropertyNode *box_def=NULL;
	SGPropertyNode *cld_def=NULL;
	double extent = 0.0;

	SGPath texture_root = globals->get_fg_root();
	texture_root.append("Textures");
	texture_root.append("Sky");

	box_def = box_def_root->getChild(name.c_str());

	string base_name = name.substr(0,2);
	if( !box_def ) {
		if( name[2] == '-' ) {
			box_def = box_def_root->getChild(base_name.c_str());
		}
		if( !box_def )
			return 0.0;
	}

	double x = mt_rand(&seed) * SGCloudField::fieldSize - (SGCloudField::fieldSize / 2.0);
	double y = mt_rand(&seed) * SGCloudField::fieldSize - (SGCloudField::fieldSize / 2.0);
	double z = grid_z_rand * (mt_rand(&seed) - 0.5);

	float lon = fgGetNode("/position/longitude-deg", false)->getFloatValue();
	float lat = fgGetNode("/position/latitude-deg", false)->getFloatValue();

	SGVec3f pos(x,y,z);

	for(int i = 0; i < box_def->nChildren() ; i++) {
		SGPropertyNode *abox = box_def->getChild(i);
		if( strcmp(abox->getName(), "box") == 0) {

			string type = abox->getStringValue("type", "cu-small");
			cld_def = cloud_def_root->getChild(type.c_str());
			if ( !cld_def ) return 0.0;

			double w = abox->getDoubleValue("width", 1000.0);
			double h = abox->getDoubleValue("height", 1000.0);
			int hdist = abox->getIntValue("hdist", 1);
			int vdist = abox->getIntValue("vdist", 1);

			double c = abox->getDoubleValue("count", 5);
			int count = (int) (c + (mt_rand(&seed) - 0.5) * c);

			extent = std::max(w*w, extent);

			for (int j = 0; j < count; j++)	{

				// Locate the clouds randomly in the defined space. The hdist and
				// vdist values control the horizontal and vertical distribution
				// by simply summing random components.
				double x = 0.0;
				double y = 0.0;
				double z = 0.0;

				for (int k = 0; k < hdist; k++)
				{
					x += (mt_rand(&seed) / hdist);
					y += (mt_rand(&seed) / hdist);
				}

				for (int k = 0; k < vdist; k++)
				{
					z += (mt_rand(&seed) / vdist);
				}

				x = w * (x - 0.5) + pos[0]; // N/S
				y = w * (y - 0.5) + pos[1]; // E/W
				z = h * z + pos[2]; // Up/Down. pos[2] is the cloudbase

				//SGVec3f newpos = SGVec3f(x, y, z);
				SGNewCloud cld(texture_root, cld_def, &seed);

				//layer->addCloud(newpos, cld.genCloud());
				layer->addCloud(lon, lat, z, x, y, index++, cld.genCloud());
			}
		}
	}

	// Return the maximum extent of the cloud
	return extent;
}

void FGClouds::buildLayer(int iLayer, const string& name, double coverage) {
	struct {
		string name;
		double count;
	} tCloudVariety[20];
	int CloudVarietyCount = 0;
	double totalCount = 0.0;

    SGSky* thesky = globals->get_renderer()->getSky();
    
	SGPropertyNode *cloud_def_root = fgGetNode("/environment/cloudlayers/clouds", false);
	SGPropertyNode *box_def_root   = fgGetNode("/environment/cloudlayers/boxes", false);
	SGPropertyNode *layer_def_root = fgGetNode("/environment/cloudlayers/layers", false);
	SGCloudField *layer = thesky->get_cloud_layer(iLayer)->get_layer3D();
	layer->clear();

	// If we don't have the required properties, then render the cloud in 2D
	if ((! clouds_3d_enabled) || coverage == 0.0 ||
		layer_def_root == NULL || cloud_def_root == NULL || box_def_root == NULL) {
			thesky->get_cloud_layer(iLayer)->set_enable3dClouds(false);
			return;
	}

	// If we can't find a definition for this cloud type, then render the cloud in 2D
	SGPropertyNode *layer_def=NULL;
	layer_def = layer_def_root->getChild(name.c_str());
	if( !layer_def ) {
		if( name[2] == '-' ) {
			string base_name = name.substr(0,2);
			layer_def = layer_def_root->getChild(base_name.c_str());
		}
		if( !layer_def ) {
			thesky->get_cloud_layer(iLayer)->set_enable3dClouds(false);
			return;
		}
	}

	// At this point, we know we've got some 3D clouds to generate.
	thesky->get_cloud_layer(iLayer)->set_enable3dClouds(true);

	double grid_z_rand = layer_def->getDoubleValue("grid-z-rand");

	for(int i = 0; i < layer_def->nChildren() ; i++) {
		SGPropertyNode *acloud = layer_def->getChild(i);
		if( strcmp(acloud->getName(), "cloud") == 0) {
			string cloud_name = acloud->getStringValue("name");
			tCloudVariety[CloudVarietyCount].name = cloud_name;
			double count = acloud->getDoubleValue("count", 1.0);
			tCloudVariety[CloudVarietyCount].count = count;
			int variety = 0;
			char variety_name[50];
			do {
				variety++;
				snprintf(variety_name, sizeof(variety_name) - 1, "%s-%d", cloud_name.c_str(), variety);
			} while( box_def_root->getChild(variety_name, 0, false) );

			totalCount += count;
			if( CloudVarietyCount < 20 )
				CloudVarietyCount++;
		}
	}
	totalCount = 1.0 / totalCount;

	// Determine how much cloud coverage we need in m^2.
	double cov = coverage * SGCloudField::fieldSize * SGCloudField::fieldSize;

	while (cov > 0.0f) {
		double choice = mt_rand(&seed);

		for(int i = 0; i < CloudVarietyCount ; i ++) {
			choice -= tCloudVariety[i].count * totalCount;
			if( choice <= 0.0 ) {
				cov -= buildCloud(cloud_def_root,
						box_def_root,
						tCloudVariety[i].name,
						grid_z_rand,
						layer);
				break;
			}
		}
	}

	// Now we've built any clouds, enable them and set the density (coverage)
	//layer->setCoverage(coverage);
	//layer->applyCoverage();
	thesky->get_cloud_layer(iLayer)->set_enable3dClouds(clouds_3d_enabled);
}

void FGClouds::buildCloudLayers(void) {
	SGPropertyNode *metar_root = fgGetNode("/environment", true);

	//double wind_speed_kt	 = metar_root->getDoubleValue("wind-speed-kt");
	double temperature_degc  = metar_root->getDoubleValue("temperature-sea-level-degc");
	double dewpoint_degc     = metar_root->getDoubleValue("dewpoint-sea-level-degc");
	double pressure_mb       = metar_root->getDoubleValue("pressure-sea-level-inhg") * SG_INHG_TO_PA / 100.0;
	double rel_humidity      = metar_root->getDoubleValue("relative-humidity");

	// formule d'Epsy, base d'un cumulus
	double cumulus_base = 122.0 * (temperature_degc - dewpoint_degc);
	double stratus_base = 100.0 * (100.0 - rel_humidity) * SG_FEET_TO_METER;

    SGSky* thesky = globals->get_renderer()->getSky();
	for(int iLayer = 0 ; iLayer < thesky->get_cloud_layer_count(); iLayer++) {
		SGPropertyNode *cloud_root = fgGetNode("/environment/clouds/layer", iLayer, true);

		double alt_ft = cloud_root->getDoubleValue("elevation-ft");
		double alt_m = alt_ft * SG_FEET_TO_METER;
		string coverage = cloud_root->getStringValue("coverage");

		double coverage_norm = 0.0;
		if( coverage == "few" )
			coverage_norm = 2.0/8.0;	// <1-2
		else if( coverage == "scattered" )
			coverage_norm = 4.0/8.0;	// 3-4
		else if( coverage == "broken" )
			coverage_norm = 6.0/8.0;	// 5-7
		else if( coverage == "overcast" )
			coverage_norm = 8.0/8.0;	// 8

		string layer_type = "nn";

		if( coverage == "cirrus" ) {
			layer_type = "ci";
		} else if( alt_ft > 16500 ) {
//			layer_type = "ci|cs|cc";
			layer_type = "ci";
		} else if( alt_ft > 6500 ) {
//			layer_type = "as|ac|ns";
			layer_type = "ac";
			if( pressure_mb < 1005.0 && coverage_norm >= 0.5 )
				layer_type = "ns";
		} else {
//			layer_type = "st|cu|cb|sc";
			if( cumulus_base * 0.80 < alt_m && cumulus_base * 1.20 > alt_m ) {
				// +/- 20% from cumulus probable base
				layer_type = "cu";
			} else if( stratus_base * 0.80 < alt_m && stratus_base * 1.40 > alt_m ) {
				// +/- 20% from stratus probable base
				layer_type = "st";
			} else {
				// above formulae is far from perfect
				if ( alt_ft < 2000 )
					layer_type = "st";
				else if( alt_ft < 4500 )
					layer_type = "cu";
				else
					layer_type = "sc";
			}
		}

		cloud_root->setStringValue("layer-type",layer_type);
		buildLayer(iLayer, layer_type, coverage_norm);
	}
}

void FGClouds::set_3dClouds(bool enable)
{
	if (enable != clouds_3d_enabled) {
		clouds_3d_enabled = enable;
		buildCloudLayers();
	}
}

bool FGClouds::get_3dClouds() const
{
	return clouds_3d_enabled;
}

/**
 * Adds a 3D cloud to a cloud layer.
 *
 * Property arguments
 * layer - the layer index to add this cloud to. (Defaults to 0)
 * index - the index for this cloud (to be used later)
 * lon/lat/alt - the position for the cloud
 * (Various) - cloud definition properties. See README.3DClouds
 *
 */
 bool FGClouds::add3DCloud(const SGPropertyNode *arg)
 {
   int l = arg->getIntValue("layer", 0);
   int index = arg->getIntValue("index", 0);

   SGPath texture_root = globals->get_fg_root();
	 texture_root.append("Textures");
	 texture_root.append("Sky");

	 float lon = arg->getFloatValue("lon-deg", 0.0f);
	 float lat = arg->getFloatValue("lat-deg", 0.0f);
	 float alt = arg->getFloatValue("alt-ft",  0.0f);
	 float x   = arg->getFloatValue("x-offset-m",  0.0f);
	 float y   = arg->getFloatValue("y-offset-m",  0.0f);

   SGSky* thesky = globals->get_renderer()->getSky();
   SGCloudField *layer = thesky->get_cloud_layer(l)->get_layer3D();
   SGNewCloud cld(texture_root, arg, &seed);
   bool success = layer->addCloud(lon, lat, alt, x, y, index, cld.genCloud());

   // Adding a 3D cloud immediately makes this layer 3D.
   thesky->get_cloud_layer(l)->set_enable3dClouds(true);

   return success;
 }

 /**
  * Removes a 3D cloud from a cloud layer
  *
  * Property arguments
  *
  * layer - the layer index to remove this cloud from. (defaults to 0)
  * index - the cloud index
  *
  */
 bool FGClouds::delete3DCloud(const SGPropertyNode *arg)
 {
   int l = arg->getIntValue("layer", 0);
   int i = arg->getIntValue("index", 0);

   SGSky* thesky = globals->get_renderer()->getSky();
   SGCloudField *layer = thesky->get_cloud_layer(l)->get_layer3D();
	 return layer->deleteCloud(i);
 }

/**
 * Move a cloud within a 3D layer
 *
 * Property arguments
 * layer - the layer index to add this cloud to. (Defaults to 0)
 * index - the cloud index to move.
 * lon/lat/alt - the position for the cloud
 *
 */
bool FGClouds::move3DCloud(const SGPropertyNode *arg)
 {
   int l = arg->getIntValue("layer", 0);
   int i = arg->getIntValue("index", 0);
      SGSky* thesky = globals->get_renderer()->getSky();
     
	 float lon = arg->getFloatValue("lon-deg", 0.0f);
	 float lat = arg->getFloatValue("lat-deg", 0.0f);
	 float alt = arg->getFloatValue("alt-ft",  0.0f);
	 float x   = arg->getFloatValue("x-offset-m",  0.0f);
	 float y   = arg->getFloatValue("y-offset-m",  0.0f);

   SGCloudField *layer = thesky->get_cloud_layer(l)->get_layer3D();
	 return layer->repositionCloud(i, lon, lat, alt, x, y);
 }
