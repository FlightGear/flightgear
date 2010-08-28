// environment_ctrl.cxx -- manager for natural environment information.
//
// Written by David Megginson, started February 2002.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <algorithm>
#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/exception.hxx>

#include <Airports/simple.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "atmosphere.hxx"
#include "fgmetar.hxx"
#include "environment_ctrl.hxx"

using std::sort;

class AirportWithMetar : public FGAirport::AirportFilter {
public:
	virtual bool passAirport(FGAirport* aApt) const {
		return aApt->getMetar();
	}
  
  // permit heliports and seaports too
  virtual FGPositioned::Type maxType() const
  { return FGPositioned::SEAPORT; }
};

static AirportWithMetar airportWithMetarFilter;

////////////////////////////////////////////////////////////////////////
// Implementation of FGEnvironmentCtrl abstract base class.
////////////////////////////////////////////////////////////////////////

FGEnvironmentCtrl::FGEnvironmentCtrl ()
	: _environment(0),
	_lon_deg(0),
	_lat_deg(0),
	_elev_ft(0)
{
}

FGEnvironmentCtrl::~FGEnvironmentCtrl ()
{
}

void
FGEnvironmentCtrl::setEnvironment (FGEnvironment * environment)
{
	_environment = environment;
}

void
FGEnvironmentCtrl::setLongitudeDeg (double lon_deg)
{
	_lon_deg = lon_deg;
}

void
FGEnvironmentCtrl::setLatitudeDeg (double lat_deg)
{
	_lat_deg = lat_deg;
}

void
FGEnvironmentCtrl::setElevationFt (double elev_ft)
{
	_elev_ft = elev_ft;
}

void
FGEnvironmentCtrl::setPosition (double lon_deg, double lat_deg, double elev_ft)
{
	_lon_deg = lon_deg;
	_lat_deg = lat_deg;
	_elev_ft = elev_ft;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInterpolateEnvironmentCtrl.
////////////////////////////////////////////////////////////////////////


FGInterpolateEnvironmentCtrl::FGInterpolateEnvironmentCtrl ()
{
	altitude_n = fgGetNode("/position/altitude-ft", true);
	altitude_agl_n = fgGetNode("/position/altitude-agl-ft", true);
	boundary_transition_n = fgGetNode("/environment/config/boundary-transition-ft", false );
	boundary_n = fgGetNode("/environment/config/boundary", true );
	aloft_n = fgGetNode("/environment/config/aloft", true );
}

FGInterpolateEnvironmentCtrl::~FGInterpolateEnvironmentCtrl ()
{
	unsigned int i;
	for (i = 0; i < _boundary_table.size(); i++)
		delete _boundary_table[i];
	for (i = 0; i < _aloft_table.size(); i++)
		delete _aloft_table[i];
}



void
FGInterpolateEnvironmentCtrl::init ()
{
	read_table( boundary_n, _boundary_table);
	// pass in a pointer to the environment of the last bondary layer as
	// a starting point
	read_table( aloft_n, _aloft_table, 
		_boundary_table.size() > 0 ?  
		&(*(_boundary_table.end()-1))->environment : NULL );
}

void
FGInterpolateEnvironmentCtrl::reinit ()
{
	init();
}

void
FGInterpolateEnvironmentCtrl::read_table (const SGPropertyNode * node, vector<bucket *> &table, FGEnvironment * parent )
{
	double last_altitude_ft = 0.0;
	double sort_required = false;
	size_t i;

	for (i = 0; i < (size_t)node->nChildren(); i++) {
		const SGPropertyNode * child = node->getChild(i);
		if ( strcmp(child->getName(), "entry") == 0
		 && child->getStringValue("elevation-ft", "")[0] != '\0'
		 && ( child->getDoubleValue("elevation-ft") > 0.1 || i == 0 ) )
	{
			bucket * b;
			if( i < table.size() ) {
				// recycle existing bucket
				b = table[i];
			} else {
				// more nodes than buckets in table, add a new one
				b = new bucket;
				table.push_back(b);
			}
			if (i == 0 && parent != NULL )
				b->environment.copy( *parent );
			if (i > 0)
				b->environment.copy(table[i-1]->environment);
			
			b->environment.read(child);
			b->altitude_ft = b->environment.get_elevation_ft();

			// check, if altitudes are in ascending order
			if( b->altitude_ft < last_altitude_ft )
				sort_required = true;
			last_altitude_ft = b->altitude_ft;
		}
	}
	// remove leftover buckets
	while( table.size() > i ) {
		bucket * b = *(table.end() - 1);
		delete b;
		table.pop_back();
	}

	if( sort_required )
		sort(table.begin(), table.end(), bucket::lessThan);

	// cleanup entries with (almost)same altitude
	for( vector<bucket *>::size_type n = 1; n < table.size(); n++ ) {
		if( fabs(table[n]->altitude_ft - table[n-1]->altitude_ft ) < 1 ) {
			SG_LOG( SG_GENERAL, SG_ALERT, "Removing duplicate altitude entry in environment config for altitude " << table[n]->altitude_ft );
			table.erase( table.begin() + n );
		}
	}
}

void
FGInterpolateEnvironmentCtrl::update (double delta_time_sec)
{
	double altitude_ft = altitude_n->getDoubleValue();
	double altitude_agl_ft = altitude_agl_n->getDoubleValue();
	double boundary_transition = 
		boundary_transition_n == NULL ? 500 : boundary_transition_n->getDoubleValue();

	int length = _boundary_table.size();

	if (length > 0) {
		// boundary table
		double boundary_limit = _boundary_table[length-1]->altitude_ft;
		if (boundary_limit >= altitude_agl_ft) {
			do_interpolate(_boundary_table, altitude_agl_ft, _environment);
			return;
		} else if ((boundary_limit + boundary_transition) >= altitude_agl_ft) {
			//TODO: this is 500ft above the top altitude of boundary layer
			//shouldn't this be +/-250 ft off of the top altitude?
			// both tables
			do_interpolate(_boundary_table, altitude_agl_ft, &env1);
			do_interpolate(_aloft_table, altitude_ft, &env2);
			double fraction = boundary_transition > SGLimitsd::min() ?
				(altitude_agl_ft - boundary_limit) / boundary_transition : 1.0;
			interpolate(&env1, &env2, fraction, _environment);
			return;
		}
	}
	// aloft table
	do_interpolate(_aloft_table, altitude_ft, _environment);
}

void
FGInterpolateEnvironmentCtrl::do_interpolate (vector<bucket *> &table, double altitude_ft, FGEnvironment * environment)
{
	int length = table.size();
	if (length == 0)
		return;

	// Boundary conditions
	if ((length == 1) || (table[0]->altitude_ft >= altitude_ft)) {
		environment->copy(table[0]->environment); // below bottom of table
		return;
	} else if (table[length-1]->altitude_ft <= altitude_ft) {
		environment->copy(table[length-1]->environment); // above top of table
		return;
	} 

	// Search the interpolation table
	int layer;
	for ( layer = 1; // can't be below bottom layer, handled above
	      layer < length && table[layer]->altitude_ft <= altitude_ft;
 	      layer++);
	FGEnvironment * env1 = &(table[layer-1]->environment);
	FGEnvironment * env2 = &(table[layer]->environment);
	// two layers of same altitude were sorted out in read_table
	double fraction = ((altitude_ft - table[layer-1]->altitude_ft) /
	                  (table[layer]->altitude_ft - table[layer-1]->altitude_ft));
	interpolate(env1, env2, fraction, environment);
}

bool
FGInterpolateEnvironmentCtrl::bucket::operator< (const bucket &b) const
{
	return (altitude_ft < b.altitude_ft);
}

bool
FGInterpolateEnvironmentCtrl::bucket::lessThan(bucket *a, bucket *b)
{
	return (a->altitude_ft) < (b->altitude_ft);
}


////////////////////////////////////////////////////////////////////////
// Implementation of FGMetarCtrl.
////////////////////////////////////////////////////////////////////////

FGMetarCtrl::FGMetarCtrl( SGSubsystem * environmentCtrl )
	:
	metar_valid(false),
	setup_winds_aloft(true),
	wind_interpolation_required(true),
	metar_sealevel_temperature(15.0),
	metar_sealevel_dewpoint(5.0),
	// Interpolation constant definitions.
	MaxWindChangeKtsSec( 0.2 ),
	MaxVisChangePercentSec( 0.05 ),
	MaxPressureChangeInHgSec( 0.0005 ), // approx 1hpa/min
	MaxTemperatureChangeDegcSec(10.0/60.0), // approx 10degc/min
	MaxCloudAltitudeChangeFtSec( 20.0 ),
	MaxCloudThicknessChangeFtSec( 50.0 ),
	MaxCloudInterpolationHeightFt( 5000.0 ),
	MaxCloudInterpolationDeltaFt( 4000.0 ),
	_environmentCtrl(environmentCtrl)
{
	windModulator = new FGBasicWindModulator();

	metar_base_n = fgGetNode( "/environment/metar", true );
	station_id_n = metar_base_n->getNode("station-id", true );
	station_elevation_n = metar_base_n->getNode("station-elevation-ft", true );
	min_visibility_n = metar_base_n->getNode("min-visibility-m", true );
	max_visibility_n = metar_base_n->getNode("max-visibility-m", true );
	base_wind_range_from_n = metar_base_n->getNode("base-wind-range-from", true );
	base_wind_range_to_n = metar_base_n->getNode("base-wind-range-to", true );
	base_wind_speed_n = metar_base_n->getNode("base-wind-speed-kt", true );
	base_wind_dir_n = metar_base_n->getNode("base-wind-dir-deg", true );
	gust_wind_speed_n = metar_base_n->getNode("gust-wind-speed-kt", true );
	temperature_n = metar_base_n->getNode("temperature-degc", true );
	dewpoint_n = metar_base_n->getNode("dewpoint-degc", true );
	humidity_n = metar_base_n->getNode("rel-humidity-norm", true );
	pressure_n = metar_base_n->getNode("pressure-inhg", true );
	clouds_n = metar_base_n->getNode("clouds", true );
	rain_n = metar_base_n->getNode("rain-norm", true );
	hail_n = metar_base_n->getNode("hail-norm", true );
	snow_n = metar_base_n->getNode("snow-norm", true );
	snow_cover_n = metar_base_n->getNode("snow-cover", true );
	magnetic_variation_n = fgGetNode( "/environment/magnetic-variation-deg", true );
	ground_elevation_n = fgGetNode( "/position/ground-elev-m", true );
	longitude_n = fgGetNode( "/position/longitude-deg", true );
	latitude_n = fgGetNode( "/position/latitude-deg", true );
	environment_clouds_n = fgGetNode("/environment/clouds");

	boundary_wind_speed_n = fgGetNode("/environment/config/boundary/entry/wind-speed-kt", true );
	boundary_wind_from_heading_n = fgGetNode("/environment/config/boundary/entry/wind-from-heading-deg", true );
	boundary_visibility_n = fgGetNode("/environment/config/boundary/entry/visibility-m", true );
	boundary_sea_level_pressure_n = fgGetNode("/environment/config/boundary/entry/pressure-sea-level-inhg", true );
	boundary_sea_level_temperature_n = fgGetNode("/environment/config/boundary/entry/temperature-sea-level-degc", true );
	boundary_sea_level_dewpoint_n = fgGetNode("/environment/config/boundary/entry/dewpoint-sea-level-degc", true );
}

FGMetarCtrl::~FGMetarCtrl ()
{
}

void FGMetarCtrl::bind ()
{
	fgTie("/environment/metar/valid", this, &FGMetarCtrl::get_valid );
	fgTie("/environment/params/metar-updates-environment", this, &FGMetarCtrl::get_enabled, &FGMetarCtrl::set_enabled );
	fgTie("/environment/params/metar-updates-winds-aloft", this, &FGMetarCtrl::get_setup_winds_aloft, &FGMetarCtrl::set_setup_winds_aloft );
}

void FGMetarCtrl::unbind ()
{
	fgUntie("/environment/metar/valid");
	fgUntie("/environment/params/metar-updates-environment");
	fgUntie("/environment/params/metar-updates-winds-aloft");
}

// use a "command" to set station temp at station elevation
static void set_temp_at_altitude( double temp_degc, double altitude_ft ) {
	SGPropertyNode args;
	SGPropertyNode *node = args.getNode("temp-degc", 0, true);
	node->setDoubleValue( temp_degc );
	node = args.getNode("altitude-ft", 0, true);
	node->setDoubleValue( altitude_ft );
	globals->get_commands()->execute( altitude_ft == 0.0 ? 
		"set-sea-level-air-temp-degc" : 
		"set-outside-air-temp-degc", &args);
}

static void set_dewpoint_at_altitude( double dewpoint_degc, double altitude_ft ) {
	SGPropertyNode args;
	SGPropertyNode *node = args.getNode("dewpoint-degc", 0, true);
	node->setDoubleValue( dewpoint_degc );
	node = args.getNode("altitude-ft", 0, true);
	node->setDoubleValue( altitude_ft );
	globals->get_commands()->execute( altitude_ft == 0.0 ?
		"set-dewpoint-sea-level-air-temp-degc" :
		"set-dewpoint-temp-degc", &args);
}

/*
 Setup the wind nodes for a branch in the /environment/config/<branchName>/entry nodes

 Output properties:
 wind-from-heading-deg
 wind-speed-kt
 turbulence/magnitude-norm

 Input properties:
 wind-heading-change-deg       how many degrees does the wind direction change at this level
 wind-speed-change-rel         relative change of wind speed at this level 
 turbulence/factor             factor for the calculated turbulence magnitude at this level
 */
static void setupWindBranch( string branchName, double dir, double speed, double gust )
{
	SGPropertyNode_ptr branch = fgGetNode("/environment/config", true)->getNode(branchName,true);
	vector<SGPropertyNode_ptr> entries = branch->getChildren("entry");
	for ( vector<SGPropertyNode_ptr>::iterator it = entries.begin(); it != entries.end(); it++) {

		// change wind direction as configured
		double layer_dir = dir + (*it)->getDoubleValue("wind-heading-change-deg", 0.0 );
		if( layer_dir >= 360.0 ) layer_dir -= 360.0;
		if( layer_dir < 0.0 ) layer_dir += 360.0;
		(*it)->setDoubleValue("wind-from-heading-deg", layer_dir);

		double layer_speed = speed*(1 + (*it)->getDoubleValue("wind-speed-change-rel", 0.0 ));
		(*it)->setDoubleValue("wind-speed-kt", layer_speed );

		// add some turbulence
		SGPropertyNode_ptr turbulence = (*it)->getNode("turbulence",true);

		double turbulence_norm = speed/50;
		if( gust > speed ) {
			turbulence_norm += (gust-speed)/25;
		}
		if( turbulence_norm > 1.0 ) turbulence_norm = 1.0;

		turbulence_norm *= turbulence->getDoubleValue("factor", 0.0 );
		turbulence->setDoubleValue( "magnitude-norm", turbulence_norm );
	}
}

static void setupWind( bool setup_aloft, double dir, double speed, double gust )
{
	setupWindBranch( "boundary", dir, speed, gust );
	if( setup_aloft )
		setupWindBranch( "aloft", dir, speed, gust );
}

double FGMetarCtrl::interpolate_val(double currentval, double requiredval, double dval )
{
	if (fabs(currentval - requiredval) < dval) return requiredval;
	if (currentval < requiredval) return (currentval + dval);
	if (currentval > requiredval) return (currentval - dval);
	return requiredval;
}

void
FGMetarCtrl::init ()
{
	first_update = true;
	wind_interpolation_required = true;
}

void
FGMetarCtrl::reinit ()
{
	init();
}

static inline double convert_to_360( double d )
{
	if( d < 0.0 ) return d + 360.0;
	if( d >= 360.0 ) return d - 360.0;
	return d;
}

static inline double convert_to_180( double d )
{
	return d > 180.0 ? d - 360.0 : d;
}

// Return the sea level pressure for a metar observation, in inHg.
// This is different from QNH because it accounts for the current
// temperature at the observation point.
// metarPressure in inHg
// fieldHt in ft
// fieldTemp in C

static double reducePressureSl(double metarPressure, double fieldHt,
                               double fieldTemp)
{
	double elev = fieldHt * SG_FEET_TO_METER;
	double fieldPressure
		= FGAtmo::fieldPressure(elev, metarPressure * atmodel::inHg);
	double slPressure = P_layer(0, elev, fieldPressure,
		fieldTemp + atmodel::freezing, atmodel::ISA::lam0);
	return slPressure / atmodel::inHg;
}

void
FGMetarCtrl::update(double dt)
{
	if( dt <= 0 || !metar_valid ||!enabled)
		return;

	windModulator->update(dt);
	// Interpolate the current configuration closer to the actual METAR

	bool reinit_required = false;
	bool layer_rebuild_required = false;
	double station_elevation_ft = station_elevation_n->getDoubleValue();

	if (first_update) {
		double dir = base_wind_dir_n->getDoubleValue()+magnetic_variation_n->getDoubleValue();
		double speed = base_wind_speed_n->getDoubleValue();
		double gust = gust_wind_speed_n->getDoubleValue();
		setupWind(setup_winds_aloft, dir, speed, gust);

		double metarvis = min_visibility_n->getDoubleValue();
		fgDefaultWeatherValue("visibility-m", metarvis);

		set_temp_at_altitude(temperature_n->getDoubleValue(), station_elevation_ft);
		set_dewpoint_at_altitude(dewpoint_n->getDoubleValue(), station_elevation_ft);

		double metarpressure = pressure_n->getDoubleValue();
		fgDefaultWeatherValue("pressure-sea-level-inhg",
			reducePressureSl(metarpressure,
			station_elevation_ft,
			temperature_n->getDoubleValue()));

		// We haven't already loaded a METAR, so apply it immediately.
		vector<SGPropertyNode_ptr> layers = clouds_n->getChildren("layer");
		vector<SGPropertyNode_ptr>::const_iterator layer;
		vector<SGPropertyNode_ptr>::const_iterator layers_end = layers.end();

		int i;
		for (i = 0, layer = layers.begin(); layer != layers_end; ++layer, i++) {
			SGPropertyNode *target = environment_clouds_n->getChild("layer", i, true);

			target->setStringValue("coverage",
					(*layer)->getStringValue("coverage", "clear"));
			target->setDoubleValue("elevation-ft",
					(*layer)->getDoubleValue("elevation-ft"));
			target->setDoubleValue("thickness-ft",
					(*layer)->getDoubleValue("thickness-ft"));
			target->setDoubleValue("span-m", 40000.0);
		}

		first_update = false;
		reinit_required = true;
		layer_rebuild_required = true;

	} else {
		if( wind_interpolation_required ) {
			// Generate interpolated values between the METAR and the current
			// configuration.

			// Pick up the METAR wind values and convert them into a vector.
			double metar[2];
			double metar_speed = base_wind_speed_n->getDoubleValue();
			double metar_heading = base_wind_dir_n->getDoubleValue()+magnetic_variation_n->getDoubleValue();

			metar[0] = metar_speed * sin(metar_heading * SG_DEGREES_TO_RADIANS );
			metar[1] = metar_speed * cos(metar_heading * SG_DEGREES_TO_RADIANS);

			// Convert the current wind values and convert them into a vector
			double current[2];
			double speed = boundary_wind_speed_n->getDoubleValue();
			double dir_from = boundary_wind_from_heading_n->getDoubleValue();;

			current[0] = speed * sin(dir_from * SG_DEGREES_TO_RADIANS );
			current[1] = speed * cos(dir_from * SG_DEGREES_TO_RADIANS );

			// Determine the maximum component-wise value that the wind can change.
			// First we determine the fraction in the X and Y component, then
			// factor by the maximum wind change.
			double x = fabs(current[0] - metar[0]);
			double y = fabs(current[1] - metar[1]);

			// only interpolate if we have a difference
			if (x + y > 0.01 ) {
				double dx = x / (x + y);
				double dy = 1 - dx;

				double maxdx = dx * MaxWindChangeKtsSec;
				double maxdy = dy * MaxWindChangeKtsSec;

				// Interpolate each component separately.
				current[0] = interpolate_val(current[0], metar[0], maxdx*dt);
				current[1] = interpolate_val(current[1], metar[1], maxdy*dt);

				// Now convert back to polar coordinates.
				if ((fabs(current[0]) > 0.1) || (fabs(current[1]) > 0.1)) {
					// Some real wind to convert back from. Work out the speed
					// and direction value in degrees.
					speed = sqrt((current[0] * current[0]) + (current[1] * current[1]));
					dir_from = (atan2(current[0], current[1]) * SG_RADIANS_TO_DEGREES );

					// Normalize the direction.
					if (dir_from < 0.0)
						dir_from += 360.0;

					SG_LOG( SG_GENERAL, SG_DEBUG, "Wind : " << dir_from << "@" << speed);
				} else {
					// Special case where there is no wind (otherwise atan2 barfs)
					speed = 0.0;
				}
				double gust = gust_wind_speed_n->getDoubleValue();
				setupWind(setup_winds_aloft, dir_from, speed, gust);
				reinit_required = true;
			} else { 
				wind_interpolation_required = false;
			}
		} else { // if(wind_interpolation_required)
			// interpolation of wind vector is finished, apply wind
			// variations and gusts for the boundary layer only


			bool wind_modulated = false;

			// start with the main wind direction
			double wind_dir = base_wind_dir_n->getDoubleValue()+magnetic_variation_n->getDoubleValue();
			double min = convert_to_180(base_wind_range_from_n->getDoubleValue()+magnetic_variation_n->getDoubleValue());
			double max = convert_to_180(base_wind_range_to_n->getDoubleValue()+magnetic_variation_n->getDoubleValue());
			if( max > min ) {
				// if variable winds configured, modulate the wind direction
				double f = windModulator->get_direction_offset_norm();
				wind_dir = min+(max-min)*f;
				double old = convert_to_180(boundary_wind_from_heading_n->getDoubleValue());
				wind_dir = convert_to_360(fgGetLowPass(old, wind_dir, dt ));
				wind_modulated = true;
			}
			
			// start with main wind speed
			double wind_speed = base_wind_speed_n->getDoubleValue();
			max = gust_wind_speed_n->getDoubleValue();
			if( max > wind_speed ) {
				// if gusts are configured, modulate wind magnitude
				double f = windModulator->get_magnitude_factor_norm();
				wind_speed = wind_speed+(max-wind_speed)*f;
				wind_speed = fgGetLowPass(boundary_wind_speed_n->getDoubleValue(), wind_speed, dt );
				wind_modulated = true;
			}
			if( wind_modulated ) {
				setupWind(false, wind_dir, wind_speed, max);
				reinit_required = true;
			}
		}

		// Now handle the visibility. We convert both visibility values
		// to X-values, then interpolate from there, then back to real values.
		// The length_scale is fixed to 1000m, so the visibility changes by
		// by MaxVisChangePercentSec or 1000m X MaxVisChangePercentSec,
		// whichever is more.
		double vis = boundary_visibility_n->getDoubleValue();;
		double metarvis = min_visibility_n->getDoubleValue();
		if( vis != metarvis ) {
			double currentxval = log(1000.0 + vis);
			double metarxval = log(1000.0 + metarvis);

			currentxval = interpolate_val(currentxval, metarxval, MaxVisChangePercentSec*dt);

			// Now convert back from an X-value to a straightforward visibility.
			vis = exp(currentxval) - 1000.0;
			fgDefaultWeatherValue("visibility-m", vis);
			reinit_required = true;
		}

		double pressure = boundary_sea_level_pressure_n->getDoubleValue();
		double metarpressure = pressure_n->getDoubleValue();
		double newpressure = reducePressureSl(metarpressure,
			station_elevation_ft,
			temperature_n->getDoubleValue());
		if( pressure != newpressure ) {
			pressure = interpolate_val( pressure, newpressure, MaxPressureChangeInHgSec*dt );
			fgDefaultWeatherValue("pressure-sea-level-inhg", pressure);
			reinit_required = true;
		}

		{
			double temperature = boundary_sea_level_temperature_n->getDoubleValue();
			double dewpoint = boundary_sea_level_dewpoint_n->getDoubleValue();
			if( metar_sealevel_temperature != temperature ) {
				temperature = interpolate_val( temperature, metar_sealevel_temperature, MaxTemperatureChangeDegcSec*dt );
				set_temp_at_altitude( temperature, 0.0 );
			}
			if( metar_sealevel_dewpoint != dewpoint ) {
				dewpoint = interpolate_val( dewpoint, metar_sealevel_dewpoint, MaxTemperatureChangeDegcSec*dt );
				set_dewpoint_at_altitude( dewpoint, 0.0 );
			}
		}

		// Set the cloud layers by interpolating over the METAR versions.
		vector<SGPropertyNode_ptr> layers = clouds_n->getChildren("layer");
		vector<SGPropertyNode_ptr>::const_iterator layer;
		vector<SGPropertyNode_ptr>::const_iterator layers_end = layers.end();

		double aircraft_alt = fgGetDouble("/position/altitude-ft");
		int i;

		for (i = 0, layer = layers.begin(); layer != layers_end; ++layer, i++) {
			SGPropertyNode *target = environment_clouds_n->getChild("layer", i, true);

			// In the case of clouds, we want to avoid writing if nothing has
			// changed, as these properties are tied to the renderer and will
			// cause the clouds to be updated, reseting the texture locations.

			// We don't interpolate the coverage values as no-matter how we
			// do it, it will be quite a sudden change of texture. Better to
			// have a single change than four or five.
			const char *coverage = (*layer)->getStringValue("coverage", "clear");
			SGPropertyNode *cov = target->getNode("coverage", true);
			if (strcmp(cov->getStringValue(), coverage) != 0) {
				cov->setStringValue(coverage);
				layer_rebuild_required = true;
			}

			double required_alt = (*layer)->getDoubleValue("elevation-ft");
			double current_alt = target->getDoubleValue("elevation-ft");
			double required_thickness = (*layer)->getDoubleValue("thickness-ft");
			SGPropertyNode *thickness = target->getNode("thickness-ft", true);

			if (current_alt < -9000 || required_alt < -9000 ||
				fabs(aircraft_alt - required_alt) > MaxCloudInterpolationHeightFt ||
				fabs(current_alt - required_alt) > MaxCloudInterpolationDeltaFt) {
				// We don't interpolate any layers that are
				//  - too far above us to be visible
				//  - too far below us to be visible
				//  - with too large a difference to make interpolation sensible
				//  - to or from -9999 (used as a placeholder)
				//  - any values that are too high above us,
				if (current_alt != required_alt)
					target->setDoubleValue("elevation-ft", required_alt);

				if (thickness->getDoubleValue() != required_thickness)
					thickness->setDoubleValue(required_thickness);

			} else {
				// Interpolate the other values in the usual way
				if (current_alt != required_alt) {
					current_alt = interpolate_val(current_alt, required_alt, MaxCloudAltitudeChangeFtSec*dt);
					target->setDoubleValue("elevation-ft", current_alt);
				}

				double current_thickness = thickness->getDoubleValue();

				if (current_thickness != required_thickness) {
					current_thickness = interpolate_val(current_thickness,
												 required_thickness,
												 MaxCloudThicknessChangeFtSec*dt);
					thickness->setDoubleValue(current_thickness);
				}
			}
		}
	}

	// Force an update of the 3D clouds
	if( layer_rebuild_required )
		fgSetInt("/environment/rebuild-layers", 1 );

	// Reinitializing of the environment controller required
	if( reinit_required )
		_environmentCtrl->reinit();
}

const char * FGMetarCtrl::get_metar(void) const
{
	return metar.c_str();
}

static const char *coverage_string[] = { "clear", "few", "scattered", "broken", "overcast" };
static const double thickness_value[] = { 0, 65, 600, 750, 1000 };

void FGMetarCtrl::set_metar( const char * metar_string )
{
	int i;

	metar = metar_string;

	SGSharedPtr<FGMetar> m;
	try {
		m = new FGMetar( metar_string );
	}
	catch( sg_io_exception ) {
		SG_LOG( SG_GENERAL, SG_WARN, "Can't get metar: " << metar_string );
		metar_valid = false;
		return;
	}

	wind_interpolation_required = true;

	min_visibility_n->setDoubleValue( m->getMinVisibility().getVisibility_m() );
	max_visibility_n->setDoubleValue( m->getMaxVisibility().getVisibility_m() );

	const SGMetarVisibility *dirvis = m->getDirVisibility();
	for (i = 0; i < 8; i++, dirvis++) {
		SGPropertyNode *vis = metar_base_n->getChild("visibility", i, true);
		double v = dirvis->getVisibility_m();

		vis->setDoubleValue("min-m", v);
		vis->setDoubleValue("max-m", v);
	}

	base_wind_dir_n->setIntValue( m->getWindDir() );
	base_wind_range_from_n->setIntValue( m->getWindRangeFrom() );
	base_wind_range_to_n->setIntValue( m->getWindRangeTo() );
	base_wind_speed_n->setDoubleValue( m->getWindSpeed_kt() );
	gust_wind_speed_n->setDoubleValue( m->getGustSpeed_kt() );
	temperature_n->setDoubleValue( m->getTemperature_C() );
	dewpoint_n->setDoubleValue( m->getDewpoint_C() );
	humidity_n->setDoubleValue( m->getRelHumidity() );
	pressure_n->setDoubleValue( m->getPressure_inHg() );


	// get station elevation to compute cloud base
	double station_elevation_ft = 0;
	{
		// 1. check the id given in the metar
		FGAirport* a = FGAirport::findByIdent(m->getId());

		// 2. if unknown, find closest airport with metar to current position
		if( a == NULL ) {
			SGGeod pos = SGGeod::fromDeg(longitude_n->getDoubleValue(), latitude_n->getDoubleValue());
			a = FGAirport::findClosest(pos, 10000.0, &airportWithMetarFilter);
		}

		// 3. otherwise use ground elevation
		if( a != NULL ) {
			station_elevation_ft = a->getElevation();
			station_id_n->setStringValue( a->ident());
		} else {
			station_elevation_ft = ground_elevation_n->getDoubleValue() * SG_METER_TO_FEET;
			station_id_n->setStringValue( m->getId());
		}
	}

	station_elevation_n->setDoubleValue( station_elevation_ft );

	{	// calculate sea level temperature and dewpoint
		FGEnvironment dummy; // instantiate a dummy so we can leech a method
		dummy.set_elevation_ft( station_elevation_ft );
		dummy.set_temperature_degc( temperature_n->getDoubleValue() );
		dummy.set_dewpoint_degc( dewpoint_n->getDoubleValue() );
		metar_sealevel_temperature = dummy.get_temperature_sea_level_degc();
		metar_sealevel_dewpoint = dummy.get_dewpoint_sea_level_degc();
	}

	vector<SGMetarCloud> cv = m->getClouds();
	vector<SGMetarCloud>::const_iterator cloud, cloud_end = cv.end();

	int layer_cnt = environment_clouds_n->getChildren("layer").size();
	for (i = 0, cloud = cv.begin(); i < layer_cnt; i++) {


		const char *coverage = "clear";
		double elevation = -9999.0;
		double thickness = 0.0;
		const double span = 40000.0;

		if (cloud != cloud_end) {
			int c = cloud->getCoverage();
			coverage = coverage_string[c];
			elevation = cloud->getAltitude_ft() + station_elevation_ft;
			thickness = thickness_value[c];
			++cloud;
		}

		SGPropertyNode *layer = clouds_n->getChild("layer", i, true );

		// if the coverage has changed, a rebuild of the layer is needed
		if( strcmp(layer->getStringValue("coverage"), coverage ) ) {
			layer->setStringValue("coverage", coverage);
		}
		layer->setDoubleValue("elevation-ft", elevation);
		layer->setDoubleValue("thickness-ft", thickness);
		layer->setDoubleValue("span-m", span);
	}

	rain_n->setDoubleValue(m->getRain());
	hail_n->setDoubleValue(m->getHail());
	snow_n->setDoubleValue(m->getSnow());
	snow_cover_n->setBoolValue(m->getSnowCover());
	metar_valid = true;
}

#if defined(ENABLE_THREADS)
/**
 * This class represents the thread of execution responsible for
 * fetching the metar data.
 */
class MetarThread : public OpenThreads::Thread {
public:
	MetarThread( FGMetarFetcher * f ) : metar_fetcher(f) {}
	~MetarThread() {}

	/**
	 * Fetche the metar data from the NOAA.
	 */
	void run();

private:
	FGMetarFetcher * metar_fetcher;
};

void MetarThread::run()
{
	for( ;; ) {
		string airport_id = metar_fetcher->request_queue.pop();

		if( airport_id.size() == 0 )
			break;

		if( metar_fetcher->_error_count > 3 ) {
			SG_LOG( SG_GENERAL, SG_WARN, "Too many erros fetching METAR, thread stopped permanently.");
			break;
		}

		metar_fetcher->fetch( airport_id );
	}
}
#endif

FGMetarFetcher::FGMetarFetcher() : 
#if defined(ENABLE_THREADS)
	metar_thread(NULL),
#endif
	fetch_timer(0.0),
	search_timer(0.0),
	error_timer(0.0),
	_stale_count(0),
	_error_count(0),
	enabled(false)
{
	longitude_n = fgGetNode( "/position/longitude-deg", true );
	latitude_n  = fgGetNode( "/position/latitude-deg", true );
	enable_n    = fgGetNode( "/environment/params/real-world-weather-fetch", true );

	proxy_host_n = fgGetNode("/sim/presets/proxy/host", true);
	proxy_port_n = fgGetNode("/sim/presets/proxy/port", true);
	proxy_auth_n = fgGetNode("/sim/presets/proxy/authentication", true);
	max_age_n    = fgGetNode("/environment/params/metar-max-age-min", true);

	output_n	 = fgGetNode("/environment/metar/data", true );
#if defined(ENABLE_THREADS)
	metar_thread = new MetarThread(this);
// FIXME: do we really need setProcessorAffinity()?
//	metar_thread->setProcessorAffinity(1);
	metar_thread->start();
#endif // ENABLE_THREADS
}


FGMetarFetcher::~FGMetarFetcher()
{
#if defined(ENABLE_THREADS)
	request_queue.push("");
	metar_thread->join();
	delete metar_thread;
#endif // ENABLE_THREADS
}

void FGMetarFetcher::init ()
{
	fetch_timer = 0.0;
	search_timer = 0.0;
	error_timer = 0.0;
	_stale_count = 0;
	_error_count = 0;
	current_airport_id.clear();
	/* Torsten Dreyer:
	   hack to stop startup.nas complaining if metar arrives after nasal-dir-initialized
	   is fired. Immediately fetch and wait for the METAR before continuing. This gets the
	   /environment/metar/xxx properties filled before nasal-dir is initialized.
	   Maybe the runway selection should happen here to make startup.nas obsolete?
	*/
	const char * startup_airport = fgGetString("/sim/startup/options/airport");
	if( *startup_airport ) {
		FGAirport * a = FGAirport::getByIdent( startup_airport );
		if( a ) {
			SGGeod pos = SGGeod::fromDeg(a->getLongitude(), a->getLatitude());
			a = FGAirport::findClosest(pos, 10000.0, &airportWithMetarFilter);
			current_airport_id = a->getId();
			fetch( current_airport_id );
		}
	}
}

void FGMetarFetcher::reinit ()
{
	init();
}

/* search for closest airport with metar every xx seconds */
static const int search_interval_sec = 60;

/* fetch metar for airport, even if airport has not changed every xx seconds */
static const int fetch_interval_sec = 900;

/* reset error counter after xxx seconds */
static const int error_timer_sec = 3;

void FGMetarFetcher::update (double delta_time_sec)
{
	fetch_timer -= delta_time_sec;
	search_timer -= delta_time_sec;
	error_timer -= delta_time_sec;

	if( error_timer <= 0.0 ) {
		error_timer = error_timer_sec;
		_error_count = 0;
	}

	if( enable_n->getBoolValue() == false ) {
		enabled = false;
		return;
	}

	// we were just enabled, reset all timers to 
	// trigger immediate metar fetch
	if( !enabled ) {
		search_timer = 0.0;
		fetch_timer = 0.0;
		error_timer = error_timer_sec;
		enabled = true;
	}

	FGAirport * a = NULL;

	if( search_timer <= 0.0 ) {
		// search timer expired, search closest airport with metar
		SGGeod pos = SGGeod::fromDeg(longitude_n->getDoubleValue(), latitude_n->getDoubleValue());
		a = FGAirport::findClosest(pos, 10000.0, &airportWithMetarFilter);
		search_timer = search_interval_sec;
	}

	if( a == NULL )
		return;


	if( a->ident() != current_airport_id || fetch_timer <= 0 ) {
		// fetch timer expired or airport has changed, schedule a fetch
		current_airport_id = a->ident();
		fetch_timer = fetch_interval_sec;
#if defined(ENABLE_THREADS)
		// push this airport id into the queue for the worker thread
		request_queue.push( current_airport_id );
#else
		// if there is no worker thread, immediately fetch the data
		fetch( current_airport_id );
#endif
	}
}

void FGMetarFetcher::fetch( const string & id )
{
	if( enable_n->getBoolValue() == false ) 
		return;

	SGSharedPtr<FGMetar> result = NULL;

	// fetch current metar data
	try {
		string host = proxy_host_n->getStringValue();
		string auth = proxy_auth_n->getStringValue();
		string port = proxy_port_n->getStringValue();

		result = new FGMetar( id, host, port, auth);

		long max_age = max_age_n->getLongValue();
		long age = result->getAge_min();

		if (max_age && age > max_age) {
			SG_LOG( SG_GENERAL, SG_WARN, "METAR data too old (" << age << " min).");
			if (++_stale_count > 10) {
				_error_count = 1000;
				throw sg_io_exception("More than 10 stale METAR messages in a row." " Check your system time!");
			}
		} else {
			_stale_count = 0;
		}

	} catch (const sg_io_exception& e) {
		SG_LOG( SG_GENERAL, SG_WARN, "Error fetching live weather data: " << e.getFormattedMessage().c_str() );
		result = NULL;
		// remove METAR flag from the airport
		FGAirport * a = FGAirport::findByIdent( id );
		if( a ) a->setMetar( false );
		// immediately schedule a new search
		search_timer = 0.0;
	}

	// write the metar to the property node, the rest is done by the methods tied to this property
	// don't write the metar data, if real-weather-fetch has been disabled in the meantime
	if( result != NULL && enable_n->getBoolValue() == true ) 
		output_n->setStringValue( result->getData() );
}

// end of environment_ctrl.cxx

