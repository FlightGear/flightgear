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
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//
#ifndef _FGCLOUDS_HXX
#define _FGCLOUDS_HXX

#ifdef HAVE_CONFIG
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/sound/soundmgr_openal.hxx>
#include <Main/fg_props.hxx>

#include STL_STRING

SG_USING_STD(string);

class SGNewCloud;
class SGCloudField;
class FGMetar;
class FGEnvironmentCtrl;

class FGClouds {

private:
	SGNewCloud *buildCloud(SGPropertyNode *cloud_def_root, const string& name);
	void buildLayer(SGCloudField *layer, const string& name, double alt, double coverage);

	void buildMETAR(void);

	void buildScenario( const string& scenario );

	void setLayer( int iLayer, float alt_m, const string& coverage, const string& layer_type );

	void update_metar_properties( FGMetar *m );

	void FGClouds::update_env_config ();

	int update_event;
	SGSoundSample *snd_lightning;
	FGEnvironmentCtrl * _controller;
	float station_elevation_ft;
    string last_scenario;
    SGPropertyNode *last_env_config, *last_env_clouds;

public:
	FGClouds(FGEnvironmentCtrl * controller);
	~FGClouds();

	void build(void);

	void init(void);

	int get_update_event(void) const;
	void set_update_event(int count);

};

#endif // _FGCLOUDS_HXX
