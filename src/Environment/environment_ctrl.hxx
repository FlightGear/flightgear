// environment-ctrl.hxx -- controller for environment information.
//
// Written by David Megginson, started May 2002.
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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
// $Id$

#ifndef _ENVIRONMENT_CTRL_HXX
#define _ENVIRONMENT_CTRL_HXX

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>

#if defined(ENABLE_THREADS)
# include <OpenThreads/Thread>
# include <simgear/threads/SGQueue.hxx>
#endif

#include <queue>
#include <vector>

#include <Navaids/positioned.hxx>
#include <Environment/environment.hxx>
#include "fgwind.hxx"

// forward decls
class SGPropertyNode;
class SGSampleGroup;
class FGMetar;

/**
 * Interface to control environment information for a specific location.
 */
class FGEnvironmentCtrl : public SGSubsystem
{

public:

	FGEnvironmentCtrl ();
	virtual ~FGEnvironmentCtrl ();

	virtual void setEnvironment (FGEnvironment * environment);

	virtual const FGEnvironment * getEnvironment () const { return _environment; }

	virtual void setLongitudeDeg (double lon_deg);
	virtual void setLatitudeDeg (double lat_deg);
	virtual void setElevationFt (double elev_ft);
	virtual void setPosition (double lon_deg, double lat_deg, double elev_ft);

	virtual double getLongitudeDeg () const { return _lon_deg; }
	virtual double getLatitudeDeg () const { return _lat_deg; }
	virtual double getElevationFt () const { return _elev_ft; }

protected:

	FGEnvironment * _environment;
	double _lon_deg;
	double _lat_deg;
	double _elev_ft;

};



/**
 * Interplation controller using user-supplied parameters.
 */
class FGInterpolateEnvironmentCtrl : public FGEnvironmentCtrl
{
public:
	FGInterpolateEnvironmentCtrl ();
	virtual ~FGInterpolateEnvironmentCtrl ();
	
	virtual void init ();
	virtual void reinit ();
	virtual void update (double delta_time_sec);

private:
	
	struct bucket {
		double altitude_ft;
		FGEnvironment environment;
		bool operator< (const bucket &b) const;
		// LessThan predicate for bucket pointers.
		static bool lessThan(bucket *a, bucket *b);
	};

	void read_table (const SGPropertyNode * node, std::vector<bucket *> &table, FGEnvironment * parent = NULL );
	void do_interpolate (std::vector<bucket *> &table, double altitude_ft,
						 FGEnvironment * environment);

	FGEnvironment env1, env2;	 // temporaries

	std::vector<bucket *> _boundary_table;
	std::vector<bucket *> _aloft_table;

	SGPropertyNode_ptr altitude_n;
	SGPropertyNode_ptr altitude_agl_n;
	SGPropertyNode_ptr boundary_transition_n;
	SGPropertyNode_ptr boundary_n;
	SGPropertyNode_ptr aloft_n;
};



/**
 * Interplation controller using the FGMetar class
 */

class FGMetarCtrl : public SGSubsystem
{
public:
	FGMetarCtrl (SGSubsystem * environmentCtrl);
	virtual ~FGMetarCtrl ();

	virtual void init ();
	virtual void reinit ();
	virtual void update (double delta_time_sec);

	void set_metar( const char * metar );
	const char * get_metar(void) const;
	bool get_valid(void) const { return metar_valid; }
	void set_enabled(bool _enabled) { enabled = _enabled; }
	bool get_enabled(void) const { return enabled; }
	void set_setup_winds_aloft(bool _setup_winds_aloft) { setup_winds_aloft = _setup_winds_aloft; }
	bool get_setup_winds_aloft(void) const { return setup_winds_aloft; }

private:
	void bind();
	void unbind();

	SGSharedPtr<FGWindModulator> windModulator;
	bool metar_valid;
	bool enabled;
	bool setup_winds_aloft;
	bool first_update;
	bool wind_interpolation_required;
	string metar;
	double metar_sealevel_temperature;
	double metar_sealevel_dewpoint;
	double interpolate_prop(const char * currentname, const char * requiredname, double dvalue);
	double interpolate_val(double currentval, double requiredval, double dvalue);
	const double MaxWindChangeKtsSec;			 // Max wind change in kts/sec
	const double MaxVisChangePercentSec;		// Max visibility change in %/sec
	const double MaxPressureChangeInHgSec;		// Max pressure change in InHg/sec
	const double MaxTemperatureChangeDegcSec;       // Max temperature change in degc/s
	const double MaxCloudAltitudeChangeFtSec;	 // Max cloud altitude change in ft/s
	const double MaxCloudThicknessChangeFtSec;	// Max cloud thickness change in ft/s
	const double MaxCloudInterpolationHeightFt; // Max distance from aircraft to
												// interpolate at. Any cloud
												// changes above this height
												// difference are not interpolated
	const double MaxCloudInterpolationDeltaFt;	// Max difference in altitude to 
												// interpolate. Any cloud changing height
												// by more than this value is not 
												// interpolated

	SGSubsystem * _environmentCtrl;

	SGPropertyNode_ptr metar_base_n;
	SGPropertyNode_ptr station_id_n;
	SGPropertyNode_ptr station_elevation_n;
	SGPropertyNode_ptr min_visibility_n;
	SGPropertyNode_ptr max_visibility_n;
	SGPropertyNode_ptr base_wind_range_from_n;
	SGPropertyNode_ptr base_wind_range_to_n;
	SGPropertyNode_ptr base_wind_dir_n;
	SGPropertyNode_ptr base_wind_speed_n;
	SGPropertyNode_ptr gust_wind_speed_n;
	SGPropertyNode_ptr temperature_n;
	SGPropertyNode_ptr dewpoint_n;
	SGPropertyNode_ptr humidity_n;
	SGPropertyNode_ptr pressure_n;
	SGPropertyNode_ptr clouds_n;
	SGPropertyNode_ptr environment_clouds_n;
	SGPropertyNode_ptr rain_n;
	SGPropertyNode_ptr hail_n;
	SGPropertyNode_ptr snow_n;
	SGPropertyNode_ptr snow_cover_n;
	SGPropertyNode_ptr ground_elevation_n;
	SGPropertyNode_ptr longitude_n;
	SGPropertyNode_ptr latitude_n;
	SGPropertyNode_ptr magnetic_variation_n;

	SGPropertyNode_ptr boundary_wind_speed_n;
	SGPropertyNode_ptr boundary_wind_from_heading_n;
	SGPropertyNode_ptr boundary_visibility_n;
	SGPropertyNode_ptr boundary_sea_level_pressure_n;
	SGPropertyNode_ptr boundary_sea_level_temperature_n;
	SGPropertyNode_ptr boundary_sea_level_dewpoint_n;
private:

};

/*
 * The subsyste to load real world weather
 */
class FGMetarFetcher : public SGSubsystem
{
public:
	FGMetarFetcher();
	virtual ~FGMetarFetcher();

	virtual void init ();
	virtual void reinit ();
	virtual void update (double delta_time_sec);

private:
	friend class MetarThread;
#if defined(ENABLE_THREADS)
	/**
	 * FIFO queue which holds a pointer to the metar requests.
	 */
	SGBlockingQueue <string> request_queue;

	OpenThreads::Thread * metar_thread;
#endif

	void fetch( const string & id );

	SGPropertyNode_ptr enable_n;

	SGPropertyNode_ptr longitude_n;
	SGPropertyNode_ptr latitude_n;

	SGPropertyNode_ptr proxy_host_n;
	SGPropertyNode_ptr proxy_port_n;
	SGPropertyNode_ptr proxy_auth_n;
	SGPropertyNode_ptr max_age_n;

	SGPropertyNode_ptr output_n;

	string current_airport_id;
	double fetch_timer;
	double search_timer;
	double error_timer;

	long _stale_count;
	long _error_count;
	bool enabled;
};


#endif // _ENVIRONMENT_CTRL_HXX
