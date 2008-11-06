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

#include <Main/fg_props.hxx>

#include <simgear/constants.h>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/environment/visual_enviro.hxx>
#include <simgear/scene/sky/cloudfield.hxx>
#include <simgear/scene/sky/newcloud.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/props/props_io.hxx>

#include <Main/globals.hxx>
#include <Airports/simple.hxx>
#include <Main/util.hxx>

#include "environment_ctrl.hxx"
#include "environment_mgr.hxx"
#include "fgmetar.hxx"
#include "fgclouds.hxx"

extern SGSky *thesky;


FGClouds::FGClouds(FGEnvironmentCtrl * controller) :
	station_elevation_ft(0.0),
	_controller( controller ),
	snd_lightning(NULL),
        rebuild_required(true),
    last_scenario( "none" ),
    last_env_config( new SGPropertyNode() ),
    last_env_clouds( new SGPropertyNode() )
{
	update_event = 0;
	fgSetString("/environment/weather-scenario", last_scenario.c_str());
}
FGClouds::~FGClouds() {
}

int FGClouds::get_update_event(void) const {
	return update_event;
}
void FGClouds::set_update_event(int count) {
	update_event = count;
	build();
}

void FGClouds::init(void) {
	if( snd_lightning == NULL ) {
		snd_lightning = new SGSoundSample(globals->get_fg_root().c_str(), "Sounds/thunder.wav");
		snd_lightning->set_max_dist(7000.0f);
		snd_lightning->set_reference_dist(3000.0f);
		SGSoundMgr *soundMgr = globals->get_soundmgr();
		soundMgr->add( snd_lightning, "thunder" );
		sgEnviro.set_soundMgr( soundMgr );
	}
        
        rebuild_required = true;
}

void FGClouds::buildCloud(SGPropertyNode *cloud_def_root, SGPropertyNode  *box_def_root, const string& name, sgVec3 pos, SGCloudField *layer) {
	SGPropertyNode *box_def=NULL;
        SGPropertyNode *cld_def=NULL;
        
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
			return;
	}
        
	for(int i = 0; i < box_def->nChildren() ; i++) {
		SGPropertyNode *abox = box_def->getChild(i);
		if( strcmp(abox->getName(), "box") == 0) {
			double x = abox->getDoubleValue("x") + pos[0];
			double y = abox->getDoubleValue("y") + pos[1];
			double z = abox->getDoubleValue("z") + pos[2];
                        SGVec3f newpos = SGVec3f(x, z, y);
                        
                        string type = abox->getStringValue("type", "cu-small");
                               
                        cld_def = cloud_def_root->getChild(type.c_str());
                        if ( !cld_def ) return;
                        
                        double min_width = cld_def->getDoubleValue("min-cloud-width-m", 500.0);
                        double max_width = cld_def->getDoubleValue("max-cloud-width-m", 1000.0);
                        double min_height = cld_def->getDoubleValue("min-cloud-height-m", min_width);
                        double max_height = cld_def->getDoubleValue("max-cloud-height-m", max_width);
                        double min_sprite_width = cld_def->getDoubleValue("min-sprite-width-m", 200.0);
                        double max_sprite_width = cld_def->getDoubleValue("max-sprite-width-m", min_sprite_width);
                        double min_sprite_height = cld_def->getDoubleValue("min-sprite-height-m", min_sprite_width);
                        double max_sprite_height = cld_def->getDoubleValue("max-sprite-height-m", max_sprite_width);
                        int num_sprites = cld_def->getIntValue("num-sprites", 20);
                        int num_textures_x = cld_def->getIntValue("num-textures-x", 1);
                        int num_textures_y = cld_def->getIntValue("num-textures-y", 1);
                        double bottom_shade = cld_def->getDoubleValue("bottom-shade", 1.0);
                        string texture = cld_def->getStringValue("texture", "cl_cumulus.rgb");
          
                        SGNewCloud *cld = 
                                new SGNewCloud(texture_root, 
                                               texture, 
                                               min_width, 
                                               max_width, 
                                               min_height,
                                               max_height,
                                               min_sprite_width,
                                               max_sprite_width,
                                               min_sprite_height,
                                               max_sprite_height,
                                               bottom_shade,
                                               num_sprites,
                                               num_textures_x,
                                               num_textures_y);
                        layer->addCloud(newpos, cld);
		}
	}
}

void FGClouds::buildLayer(int iLayer, const string& name, double alt, double coverage) {
	struct {
		string name;
		double count;
	} tCloudVariety[20];
	int CloudVarietyCount = 0;
	double totalCount = 0.0;
        
        if (! clouds_3d_enabled) return;
        
	SGPropertyNode *cloud_def_root = fgGetNode("/environment/cloudlayers/clouds", false);
	SGPropertyNode *box_def_root   = fgGetNode("/environment/cloudlayers/boxes", false);
	SGPropertyNode *layer_def_root = fgGetNode("/environment/cloudlayers/layers", false);
        SGCloudField *layer = thesky->get_cloud_layer(iLayer)->get_layer3D();

	layer->clear();
	// when we don't generate clouds the layer is rendered in 2D
	if( coverage == 0.0 )
		return;
	if( layer_def_root == NULL || cloud_def_root == NULL || box_def_root == NULL)
		return;
	
	SGPropertyNode *layer_def=NULL;

	layer_def = layer_def_root->getChild(name.c_str());
	if( !layer_def ) {
		if( name[2] == '-' ) {
			string base_name = name.substr(0,2);
			layer_def = layer_def_root->getChild(base_name.c_str());
		}
		if( !layer_def )
			return;
	}
        
	double grid_x_size = layer_def->getDoubleValue("grid-x-size", 1000.0);
	double grid_y_size = layer_def->getDoubleValue("grid-y-size", 1000.0);
	double grid_x_rand = layer_def->getDoubleValue("grid-x-rand", grid_x_size);
	double grid_y_rand = layer_def->getDoubleValue("grid-y-rand", grid_y_size);
	double grid_z_rand = layer_def->getDoubleValue("grid-z-rand");

	for(int i = 0; i < layer_def->nChildren() ; i++) {
		SGPropertyNode *acloud = layer_def->getChild(i);
		if( strcmp(acloud->getName(), "cloud") == 0) {
			string cloud_name = acloud->getStringValue("name");
			tCloudVariety[CloudVarietyCount].name = cloud_name;
			double count = acloud->getDoubleValue("count", 1.0);
			tCloudVariety[CloudVarietyCount].count = count;
			int variety = 0;
			cloud_name = cloud_name + "-%d";
			char variety_name[50];
			do {
				variety++;
				snprintf(variety_name, sizeof(variety_name), cloud_name.c_str(), variety);
			} while( box_def_root->getChild(variety_name, 0, false) );

			totalCount += count;
			if( CloudVarietyCount < 20 )
				CloudVarietyCount++;
		}
	}
	totalCount = 1.0 / totalCount;
	double currCoverage = 0.0;

	for(double px = 0.0; px < SGCloudField::fieldSize; px += grid_x_size) {
		for(double py = 0.0; py < SGCloudField::fieldSize; py += grid_y_size) {
			double x = px + grid_x_rand * (sg_random() - 0.5) - (SGCloudField::fieldSize / 2.0);
			double y = py + grid_y_rand * (sg_random() - 0.5) - (SGCloudField::fieldSize / 2.0);
			double z = alt + grid_z_rand * (sg_random() - 0.5);
			double choice = sg_random();
			currCoverage += coverage;
			if( currCoverage < 1.0 )
				continue;
			currCoverage -= 1.0;

			for(int i = 0; i < CloudVarietyCount ; i ++) {
				choice -= tCloudVariety[i].count * totalCount;
				if( choice <= 0.0 ) {
					sgVec3 pos={x,z,y};

                                        buildCloud(cloud_def_root, box_def_root, tCloudVariety[i].name, pos, layer);
					break;
				}
			}
		}
	}

        // Now we've built any clouds, enable them
        thesky->get_cloud_layer(iLayer)->set_enable3dClouds(clouds_3d_enabled);
}

// TODO:call this after real metar updates
void FGClouds::buildMETAR(void) {
	SGPropertyNode *metar_root = fgGetNode("/environment", true);
        
	double wind_speed_kt	 = metar_root->getDoubleValue("wind-speed-kt");
	double temperature_degc  = metar_root->getDoubleValue("temperature-sea-level-degc");
	double dewpoint_degc	 = metar_root->getDoubleValue("dewpoint-sea-level-degc");
	double pressure_mb		= metar_root->getDoubleValue("pressure-sea-level-inhg") * SG_INHG_TO_PA / 100.0;

	double dewp = pow(10.0, 7.5 * dewpoint_degc / (237.7 + dewpoint_degc));
	double temp = pow(10.0, 7.5 * temperature_degc / (237.7 + temperature_degc));
	double rel_humidity = dewp * 100 / temp;

	// formule d'Epsy, base d'un cumulus
	double cumulus_base = 122.0 * (temperature_degc - dewpoint_degc);
	double stratus_base = 100.0 * (100.0 - rel_humidity) * SG_FEET_TO_METER;

	bool cu_seen = false;

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
			if( pressure_mb < 1005.0 && coverage_norm >= 5.5 )
				layer_type = "ns";
		} else {
//			layer_type = "st|cu|cb|sc";
			// +/- 20% from stratus probable base
			if( stratus_base * 0.80 < alt_m && stratus_base * 1.40 > alt_m )
				layer_type = "st";
			// +/- 20% from cumulus probable base
			else if( cumulus_base * 0.80 < alt_m && cumulus_base * 1.20 > alt_m )
				layer_type = "cu";
			else {
				// above formulae is far from perfect
				if ( alt_ft < 2000 )
					layer_type = "st";
				else if( alt_ft < 4500 )
					layer_type = "cu";
				else
					layer_type = "sc";
			}
		}

		buildLayer(iLayer, layer_type, alt_m, coverage_norm);
	}
}

// copy from FGMetarEnvironmentCtrl until better
void
FGClouds::update_metar_properties( const FGMetar *m )
{
    int i;
    double d;
    char s[128];

    fgSetString("/environment/metar/station-id", m->getId());
    fgSetDouble("/environment/metar/min-visibility-m",
                m->getMinVisibility().getVisibility_m() );
    fgSetDouble("/environment/metar/max-visibility-m",
                m->getMaxVisibility().getVisibility_m() );

    const SGMetarVisibility *dirvis = m->getDirVisibility();
    for (i = 0; i < 8; i++, dirvis++) {
        const char *min = "/environment/metar/visibility[%d]/min-m";
        const char *max = "/environment/metar/visibility[%d]/max-m";

        d = dirvis->getVisibility_m();

        snprintf(s, 128, min, i);
        fgSetDouble(s, d);
        snprintf(s, 128, max, i);
        fgSetDouble(s, d);
    }

    fgSetInt("/environment/metar/base-wind-range-from",
             m->getWindRangeFrom() );
    fgSetInt("/environment/metar/base-wind-range-to",
             m->getWindRangeTo() );
    fgSetDouble("/environment/metar/base-wind-speed-kt",
                m->getWindSpeed_kt() );
    fgSetDouble("/environment/metar/gust-wind-speed-kt",
                m->getGustSpeed_kt() );
    fgSetDouble("/environment/metar/temperature-degc",
                m->getTemperature_C() );
    fgSetDouble("/environment/metar/dewpoint-degc",
                m->getDewpoint_C() );
    fgSetDouble("/environment/metar/rel-humidity-norm",
                m->getRelHumidity() );
    fgSetDouble("/environment/metar/pressure-inhg",
                m->getPressure_inHg() );

    vector<SGMetarCloud> cv = m->getClouds();
    vector<SGMetarCloud>::const_iterator cloud;

    const char *cl = "/environment/clouds/layer[%i]";
    for (i = 0, cloud = cv.begin(); cloud != cv.end(); cloud++, i++) {
        const char *coverage_string[5] = 
            { "clear", "few", "scattered", "broken", "overcast" };
        const double thickness[5] = { 0, 65, 600,750, 1000};
        int q;

        snprintf(s, 128, cl, i);
        strncat(s, "/coverage", 128);
        q = cloud->getCoverage();
        fgSetString(s, coverage_string[q] );

        snprintf(s, 128, cl, i);
        strncat(s, "/elevation-ft", 128);
        fgSetDouble(s, cloud->getAltitude_ft() + station_elevation_ft);

        snprintf(s, 128, cl, i);
        strncat(s, "/thickness-ft", 128);
        fgSetDouble(s, thickness[q]);

        snprintf(s, 128, cl, i);
        strncat(s, "/span-m", 128);
        fgSetDouble(s, 40000.0);
    }

    for (; i < FGEnvironmentMgr::MAX_CLOUD_LAYERS; i++) {
        snprintf(s, 128, cl, i);
        strncat(s, "/coverage", 128);
        fgSetString(s, "clear");

        snprintf(s, 128, cl, i);
        strncat(s, "/elevation-ft", 128);
        fgSetDouble(s, -9999);

        snprintf(s, 128, cl, i);
        strncat(s, "/thickness-ft", 128);
        fgSetDouble(s, 0);

        snprintf(s, 128, cl, i);
        strncat(s, "/span-m", 128);
        fgSetDouble(s, 40000.0);
    }

    fgSetDouble("/environment/metar/rain-norm", m->getRain());
    fgSetDouble("/environment/metar/hail-norm", m->getHail());
    fgSetDouble("/environment/metar/snow-norm", m->getSnow());
    fgSetBool("/environment/metar/snow-cover", m->getSnowCover());
}

void
FGClouds::update_env_config ()
{
    fgSetupWind( fgGetDouble("/environment/metar/base-wind-range-from"),
                 fgGetDouble("/environment/metar/base-wind-range-to"),
                 fgGetDouble("/environment/metar/base-wind-speed-kt"),
                 fgGetDouble("/environment/metar/gust-wind-speed-kt") );

    fgDefaultWeatherValue( "visibility-m",
                           fgGetDouble("/environment/metar/min-visibility-m") );
#if 0
    set_temp_at_altitude( fgGetDouble("/environment/metar/temperature-degc"),
                          station_elevation_ft );
    set_dewpoint_at_altitude( fgGetDouble("/environment/metar/dewpoint-degc"),
                              station_elevation_ft );
#endif
    fgDefaultWeatherValue( "pressure-sea-level-inhg",
                           fgGetDouble("/environment/metar/pressure-inhg") );
}


void FGClouds::setLayer( int iLayer, float alt_ft, const string& coverage, const string& layer_type ) {
	double coverage_norm = 0.0;
	if( coverage == "few" )
		coverage_norm = 2.0/8.0;	// <1-2
	else if( coverage == "scattered" )
		coverage_norm = 4.0/8.0;	// 3-4
	else if( coverage == "broken" )
		coverage_norm = 6.0/8.0;	// 5-7
	else if( coverage == "overcast" )
		coverage_norm = 8.0/8.0;	// 8

	buildLayer(iLayer, layer_type, station_elevation_ft + alt_ft * SG_FEET_TO_METER, coverage_norm);
}

void FGClouds::buildScenario( const string& scenario ) {
	string fakeMetar="";
	string station = fgGetString("/environment/metar/station-id", "XXXX");

	// fetch station elevation if exists
    if( station == "XXXX" )
        station_elevation_ft = fgGetDouble("/position/ground-elev-m", 0.0);
    else {
        const FGAirport* a = globals->get_airports()->search( station );
        station_elevation_ft = (a ? a->getElevation() : 0.0);
    }

	for(int iLayer = 0 ; iLayer < thesky->get_cloud_layer_count(); iLayer++) {
            thesky->get_cloud_layer(iLayer)
                ->setCoverage(SGCloudLayer::SG_CLOUD_CLEAR);
            thesky->get_cloud_layer(iLayer)->get_layer3D()->clear();
	}

	station += " 011000Z ";
	if( scenario == "Fair weather" ) {
		fakeMetar = "15003KT 12SM SCT033 FEW200 20/08 Q1015 NOSIG";
		setLayer(0, 3300.0, "scattered", "cu");
	} else if( scenario == "Thunderstorm" ) {
		fakeMetar = "15012KT 08SM TSRA SCT040 BKN070 20/12 Q0995";
		setLayer(0, 4000.0, "scattered", "cb");
		setLayer(1, 7000.0, "scattered", "ns");
	} else 
		return;
	FGMetar *m = new FGMetar( station + fakeMetar );
	update_metar_properties( m );
	update_env_config();
	// propagate aloft tables
	_controller->reinit();

	fgSetString("/environment/metar/last-metar", m->getData());
	// TODO:desactivate real metar updates
	if( scenario == "Fair weather" ) {
		fgSetString("/environment/clouds/layer[1]/coverage", "cirrus");
	}
}


void FGClouds::build() {
	string scenario = fgGetString("/environment/weather-scenario", "METAR");

        if(!rebuild_required && (scenario == last_scenario))
            return;
        
        
        
        if( last_scenario == "none" ) {
        // save clouds and weather conditions
            SGPropertyNode *param = fgGetNode("/environment/config", true);
            copyProperties( param, last_env_config );
            param = fgGetNode("/environment/clouds", true);
            copyProperties( param, last_env_clouds );
        }
        if( scenario == "METAR" ) {
            string realMetar = fgGetString("/environment/metar/real-metar", "");
            if( realMetar != "" ) {
                fgSetString("/environment/metar/last-metar", realMetar.c_str());
                FGMetar *m = new FGMetar( realMetar );
                update_metar_properties( m );
                update_env_config();
			// propagate aloft tables
                _controller->reinit();
            }
            buildMETAR();
        }
        else if( scenario == "none" ) {
        // restore clouds and weather conditions
            SGPropertyNode *param = fgGetNode("/environment/config", true);
            copyProperties( last_env_config, param );
            param = fgGetNode("/environment/clouds", true);
            copyProperties( last_env_clouds, param );
            fgSetDouble("/environment/metar/rain-norm", 0.0);
            fgSetDouble("/environment/metar/snow-norm", 0.0);
//	    update_env_config();
	    // propagate aloft tables
            _controller->reinit();
            buildMETAR();
        }
        else
            buildScenario( scenario );
        
        last_scenario = scenario;
        rebuild_required = false;

	// ...
        if( snd_lightning == NULL )
            init();
}

void FGClouds::set_3dClouds(bool enable)
{
    if (enable != clouds_3d_enabled)
    {
        clouds_3d_enabled = enable;
        
        for(int iLayer = 0 ; iLayer < thesky->get_cloud_layer_count(); iLayer++) {
            thesky->get_cloud_layer(iLayer)->set_enable3dClouds(enable);
            
            if (!enable) {            
                thesky->get_cloud_layer(iLayer)->get_layer3D()->clear();        
            }
        }
        
        if (enable)
        {
            rebuild_required = true;
            build();
        }
    }
}
    
bool FGClouds::get_3dClouds() const {
    return clouds_3d_enabled;
}
  
