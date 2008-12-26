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

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/exception.hxx>

#include <Airports/simple.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "Environment/fgmetar.hxx"
#include "environment_mgr.hxx"
#include "environment_ctrl.hxx"

using std::sort;

class AirportWithMetar : public FGPositioned::Filter
{
public:
  virtual bool pass(FGPositioned* aPos) const
  {
    if ((aPos->type() < FGPositioned::AIRPORT) || (aPos->type() > FGPositioned::SEAPORT)) {
      return false;
    }
    
    FGAirport* apt = static_cast<FGAirport*>(aPos);
    return apt->getMetar();
  }
};

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
// Implementation of FGUserDefEnvironmentCtrl.
////////////////////////////////////////////////////////////////////////

FGUserDefEnvironmentCtrl::FGUserDefEnvironmentCtrl ()
  : _base_wind_speed_node(0),
    _gust_wind_speed_node(0),
    _current_wind_speed_kt(0),
    _delta_wind_speed_kt(0)
{
}

FGUserDefEnvironmentCtrl::~FGUserDefEnvironmentCtrl ()
{
}

void
FGUserDefEnvironmentCtrl::init ()
{
				// Fill in some defaults.
  if (!fgHasNode("/environment/params/base-wind-speed-kt"))
    fgSetDouble("/environment/params/base-wind-speed-kt",
		fgGetDouble("/environment/wind-speed-kt"));
  if (!fgHasNode("/environment/params/gust-wind-speed-kt"))
    fgSetDouble("/environment/params/gust-wind-speed-kt",
		fgGetDouble("/environment/params/base-wind-speed-kt"));

  _base_wind_speed_node =
    fgGetNode("/environment/params/base-wind-speed-kt", true);
  _gust_wind_speed_node =
    fgGetNode("/environment/params/gust-wind-speed-kt", true);

  _current_wind_speed_kt = _base_wind_speed_node->getDoubleValue();
  _delta_wind_speed_kt = 0.1;
}

void
FGUserDefEnvironmentCtrl::update (double dt)
{
  double base_wind_speed = _base_wind_speed_node->getDoubleValue();
  double gust_wind_speed = _gust_wind_speed_node->getDoubleValue();

  if (gust_wind_speed < base_wind_speed) {
      gust_wind_speed = base_wind_speed;
      _gust_wind_speed_node->setDoubleValue(gust_wind_speed);
  }

  if (base_wind_speed == gust_wind_speed) {
    _current_wind_speed_kt = base_wind_speed;
  } else {
    int rn = rand() % 128;
    int sign = (_delta_wind_speed_kt < 0 ? -1 : 1);
    double gust = _current_wind_speed_kt - base_wind_speed;
    double incr = gust / 50;

    if (rn == 0)
      _delta_wind_speed_kt = - _delta_wind_speed_kt;
    else if (rn < 4)
      _delta_wind_speed_kt -= incr * sign;
    else if (rn < 16)
      _delta_wind_speed_kt += incr * sign;

    _current_wind_speed_kt += _delta_wind_speed_kt;

    if (_current_wind_speed_kt < base_wind_speed) {
      _current_wind_speed_kt = base_wind_speed;
      _delta_wind_speed_kt = 0.01;
    } else if (_current_wind_speed_kt > gust_wind_speed) {
      _current_wind_speed_kt = gust_wind_speed;
      _delta_wind_speed_kt = -0.01;
    }
  }
  
  if (_environment != 0)
    _environment->set_wind_speed_kt(_current_wind_speed_kt);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInterpolateEnvironmentCtrl.
////////////////////////////////////////////////////////////////////////

FGInterpolateEnvironmentCtrl::FGInterpolateEnvironmentCtrl ()
{
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
    read_table(fgGetNode("/environment/config/boundary", true),
               _boundary_table);
    read_table(fgGetNode("/environment/config/aloft", true),
               _aloft_table);
}

void
FGInterpolateEnvironmentCtrl::reinit ()
{
    unsigned int i;
    for (i = 0; i < _boundary_table.size(); i++)
        delete _boundary_table[i];
    for (i = 0; i < _aloft_table.size(); i++)
        delete _aloft_table[i];
    _boundary_table.clear();
    _aloft_table.clear();
    init();
}

void
FGInterpolateEnvironmentCtrl::read_table (const SGPropertyNode * node,
                                          vector<bucket *> &table)
{
    for (int i = 0; i < node->nChildren(); i++) {
        const SGPropertyNode * child = node->getChild(i);
        if ( strcmp(child->getName(), "entry") == 0
	     && child->getStringValue("elevation-ft", "")[0] != '\0'
	     && ( child->getDoubleValue("elevation-ft") > 0.1 || i == 0 ) )
	{
            bucket * b = new bucket;
            if (i > 0)
                b->environment.copy(table[i-1]->environment);
            b->environment.read(child);
            b->altitude_ft = b->environment.get_elevation_ft();
            table.push_back(b);
        }
    }
    sort(table.begin(), table.end(), bucket::lessThan);
}

void
FGInterpolateEnvironmentCtrl::update (double delta_time_sec)
{
                                // FIXME
    double altitude_ft = fgGetDouble("/position/altitude-ft");
    double altitude_agl_ft = fgGetDouble("/position/altitude-agl-ft");
    double boundary_transition =
        fgGetDouble("/environment/config/boundary-transition-ft", 500);

    // double ground_elevation_ft = altitude_ft - altitude_agl_ft;

    int length = _boundary_table.size();

    if (length > 0) {
                                // boundary table
        double boundary_limit = _boundary_table[length-1]->altitude_ft;
        if (boundary_limit >= altitude_agl_ft) {
            do_interpolate(_boundary_table, altitude_agl_ft,
                           _environment);
            return;
        } else if ((boundary_limit + boundary_transition) >= altitude_agl_ft) {
                                // both tables
            do_interpolate(_boundary_table, altitude_agl_ft, &env1);
            do_interpolate(_aloft_table, altitude_ft, &env2);
            double fraction =
                (altitude_agl_ft - boundary_limit) / boundary_transition;
            interpolate(&env1, &env2, fraction, _environment);
            return;
        }
    }

                                // aloft table
    do_interpolate(_aloft_table, altitude_ft, _environment);
}

void
FGInterpolateEnvironmentCtrl::do_interpolate (vector<bucket *> &table,
                                              double altitude_ft,
                                              FGEnvironment * environment)
{
    int length = table.size();
    if (length == 0)
        return;

                                // Boundary conditions
    if ((length == 1) || (table[0]->altitude_ft >= altitude_ft)) {
        environment->copy(table[0]->environment);
        return;
    } else if (table[length-1]->altitude_ft <= altitude_ft) {
        environment->copy(table[length-1]->environment);
        return;
    }
        
                                // Search the interpolation table
    for (int i = 0; i < length - 1; i++) {
        if ((i == length - 1) || (table[i]->altitude_ft <= altitude_ft)) {
                FGEnvironment * env1 = &(table[i]->environment);
                FGEnvironment * env2 = &(table[i+1]->environment);
                double fraction;
                if (table[i]->altitude_ft == table[i+1]->altitude_ft)
                    fraction = 1.0;
                else
                    fraction =
                        ((altitude_ft - table[i]->altitude_ft) /
                         (table[i+1]->altitude_ft - table[i]->altitude_ft));
                interpolate(env1, env2, fraction, environment);

                return;
        }
    }
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
// Implementation of FGMetarEnvironmentCtrl.
////////////////////////////////////////////////////////////////////////

FGMetarEnvironmentCtrl::FGMetarEnvironmentCtrl ()
    : env( new FGInterpolateEnvironmentCtrl ),
      _icao( "" ),
      metar_loaded( false ),
      search_interval_sec( 60.0 ),        // 1 minute
      same_station_interval_sec( 900.0 ), // 15 minutes
      search_elapsed( 9999.0 ),
      fetch_elapsed( 9999.0 ),
      last_apt( 0 ),
      proxy_host( fgGetNode("/sim/presets/proxy/host", true) ),
      proxy_port( fgGetNode("/sim/presets/proxy/port", true) ),
      proxy_auth( fgGetNode("/sim/presets/proxy/authentication", true) ),
      metar_max_age( fgGetNode("/environment/params/metar-max-age-min", true) ),

      // Interpolation constant definitions.
      EnvironmentUpdatePeriodSec( 0.2 ),
      MaxWindChangeKtsSec( 0.2 ),
      MaxVisChangePercentSec( 0.05 ),
      MaxPressureChangeInHgSec( 0.0033 ),
      MaxCloudAltitudeChangeFtSec( 20.0 ),
      MaxCloudThicknessChangeFtSec( 50.0 ),
      MaxCloudInterpolationHeightFt( 5000.0 ),
      MaxCloudInterpolationDeltaFt( 4000.0 ),

      _error_count( 0 ),
      _stale_count( 0 ),
      _dt( 0.0 ),
      _error_dt( 0.0 )
{
#if defined(ENABLE_THREADS)
    thread = new MetarThread(this);
    thread->setProcessorAffinity(1);
    thread->start();
#endif // ENABLE_THREADS
}

FGMetarEnvironmentCtrl::~FGMetarEnvironmentCtrl ()
{
#if defined(ENABLE_THREADS)
   thread_stop();
#endif // ENABLE_THREADS

   delete env;
   env = NULL;
}


// use a "command" to set station temp at station elevation
static void set_temp_at_altitude( float temp_degc, float altitude_ft ) {
    SGPropertyNode args;
    SGPropertyNode *node = args.getNode("temp-degc", 0, true);
    node->setFloatValue( temp_degc );
    node = args.getNode("altitude-ft", 0, true);
    node->setFloatValue( altitude_ft );
    globals->get_commands()->execute("set-outside-air-temp-degc", &args);
}


static void set_dewpoint_at_altitude( float dewpoint_degc, float altitude_ft ) {
    SGPropertyNode args;
    SGPropertyNode *node = args.getNode("dewpoint-degc", 0, true);
    node->setFloatValue( dewpoint_degc );
    node = args.getNode("altitude-ft", 0, true);
    node->setFloatValue( altitude_ft );
    globals->get_commands()->execute("set-dewpoint-temp-degc", &args);
}


void
FGMetarEnvironmentCtrl::update_env_config ()
{
    double dir_from;
    double dir_to;
    double speed;
    double gust;
    double vis;
    double pressure;
    double temp;
    double dewpoint;
    
    // If we aren't in the METAR scenario, don't attempt to interpolate.
    if (strcmp(fgGetString("/environment/weather-scenario", "METAR"), "METAR")) return;

    if (metar_loaded) {
        // Generate interpolated values between the METAR and the current
        // configuration.

        // Pick up the METAR wind values and convert them into a vector.
        double metar[2];
        double metar_speed = fgGetDouble("/environment/metar/base-wind-speed-kt");
        double metar_heading = fgGetDouble("/environment/metar/base-wind-range-from");

        metar[0] = metar_speed * sin((metar_heading / 180.0) * M_PI);
        metar[1] = metar_speed * cos((metar_heading / 180.0) * M_PI);

        // Convert the current wind values and convert them into a vector
        double current[2];
        double current_speed =
                fgGetDouble("/environment/config/boundary/entry/wind-speed-kt");
        double current_heading = fgGetDouble(
                "/environment/config/boundary/entry/wind-from-heading-deg");

        current[0] = current_speed * sin((current_heading / 180.0) * M_PI);
        current[1] = current_speed * cos((current_heading / 180.0) * M_PI);

        // Determine the maximum component-wise value that the wind can change.
        // First we determine the fraction in the X and Y component, then
        // factor by the maximum wind change.
        double x = fabs(current[0] - metar[0]);
        double y = fabs(current[1] - metar[1]);
        double dx = x / (x + y);
        double dy = 1 - dx;

        double maxdx = dx * MaxWindChangeKtsSec;
        double maxdy = dy * MaxWindChangeKtsSec;

        // Interpolate each component separately.
        current[0] = interpolate_val(current[0], metar[0], maxdx);
        current[1] = interpolate_val(current[1], metar[1], maxdy);

        // Now convert back to polar coordinates.
        if ((current[0] == 0.0) && (current[1] == 0.0)) {
            // Special case where there is no wind (otherwise atan2 barfs)
            speed = 0.0;
            dir_from = current_heading;

        } else {
            // Some real wind to convert back from. Work out the speed
            // and direction value in degrees.
            speed = sqrt((current[0] * current[0]) + (current[1] * current[1]));
            dir_from = (atan2(current[0], current[1]) * 180.0 / M_PI);

            // Normalize the direction.
            if (dir_from < 0.0)
                dir_from += 360.0;

            SG_LOG( SG_GENERAL, SG_DEBUG, "Wind : " << dir_from << "@" << speed);
        }

        // Now handle the visibility. We convert both visibility values
        // to X-values, then interpolate from there, then back to real values.
        // The length_scale is fixed to 1000m, so the visibility changes by
        // by MaxVisChangePercentSec or 1000m X MaxVisChangePercentSec,
        // whichever is more.
        double currentvis =
                fgGetDouble("/environment/config/boundary/entry/visibility-m");
        double metarvis = fgGetDouble("/environment/metar/min-visibility-m");
        double currentxval = log(1000.0 + currentvis);
        double metarxval = log(1000.0 + metarvis);

        currentxval = interpolate_val(currentxval, metarxval, MaxVisChangePercentSec);

        // Now convert back from an X-value to a straightforward visibility.
        vis = exp(currentxval) - 1000.0;

        pressure = interpolate_prop(
                "/environment/config/boundary/entry/pressure-sea-level-inhg",
                "/environment/metar/pressure-inhg",
                MaxPressureChangeInHgSec);

        dir_to   = fgGetDouble("/environment/metar/base-wind-range-to");
        gust     = fgGetDouble("/environment/metar/gust-wind-speed-kt");
        temp     = fgGetDouble("/environment/metar/temperature-degc");
        dewpoint = fgGetDouble("/environment/metar/dewpoint-degc");

        // Set the cloud layers by interpolating over the METAR versions.
        SGPropertyNode * clouds = fgGetNode("/environment/metar/clouds");

        vector<SGPropertyNode_ptr> layers = clouds->getChildren("layer");
        vector<SGPropertyNode_ptr>::const_iterator layer;
        vector<SGPropertyNode_ptr>::const_iterator layers_end = layers.end();

        const char *cl = "/environment/clouds/layer[%i]";
        double aircraft_alt = fgGetDouble("/position/altitude-ft");
        char s[128];
        int i;

        for (i = 0, layer = layers.begin(); layer != layers_end; ++layer, i++) {
            double currentval;
            double requiredval;

            // In the case of clouds, we want to avoid writing if nothing has
            // changed, as these properties are tied to the renderer and will
            // cause the clouds to be updated, reseting the texture locations.

            // We don't interpolate the coverage values as no-matter how we
            // do it, it will be quite a sudden change of texture. Better to
            // have a single change than four or five.
            snprintf(s, 128, cl, i);
            strncat(s, "/coverage", 128);
            const char* coverage = (*layer)->getStringValue("coverage", "clear");
            if (strncmp(fgGetString(s), coverage, 128) != 0)
                fgSetString(s, coverage);

            snprintf(s, 128, cl, i);
            strncat(s, "/elevation-ft", 128);
            double current_alt = fgGetDouble(s);
            double required_alt = (*layer)->getDoubleValue("elevation-ft");

            if (current_alt < -9000 || required_alt < -9000 ||
                fabs(aircraft_alt - required_alt) > MaxCloudInterpolationHeightFt ||
                fabs(current_alt - required_alt) > MaxCloudInterpolationDeltaFt) {
                // We don't interpolate any layers that are
                //  - too far above us to be visible
                //  - too far below us to be visible
                //  - with too large a difference to make interpolation sensible
                //  - to or from -9999 (used as a placeholder)
                //  - any values that are too high above us,
                snprintf(s, 128, cl, i);
                strncat(s, "/elevation-ft", 128);
                if (current_alt != required_alt)
                    fgSetDouble(s, required_alt);

                snprintf(s, 128, cl, i);
                strncat(s, "/thickness-ft", 128);
                if (fgGetDouble(s) != (*layer)->getDoubleValue("thickness-ft"))
                    fgSetDouble(s, (*layer)->getDoubleValue("thickness-ft"));

            } else {
                // Interpolate the other values in the usual way
                if (current_alt != required_alt) {
                    current_alt = interpolate_val(current_alt,
                                                  required_alt,
                                                  MaxCloudAltitudeChangeFtSec);
                    fgSetDouble(s, current_alt);
                }

                snprintf(s, 128, cl, i);
                strncat(s, "/thickness-ft", 128);
                currentval = fgGetDouble(s);
                requiredval = (*layer)->getDoubleValue("thickness-ft");

                if (currentval != requiredval) {
                    currentval = interpolate_val(currentval,
                                                 requiredval,
                                                 MaxCloudThicknessChangeFtSec);
                    fgSetDouble(s, currentval);
                }
            }
        }

    } else {
        // We haven't already loaded a METAR, so apply it immediately.
        dir_from = fgGetDouble("/environment/metar/base-wind-range-from");
        dir_to   = fgGetDouble("/environment/metar/base-wind-range-to");
        speed    = fgGetDouble("/environment/metar/base-wind-speed-kt");
        gust     = fgGetDouble("/environment/metar/gust-wind-speed-kt");
        vis      = fgGetDouble("/environment/metar/min-visibility-m");
        pressure = fgGetDouble("/environment/metar/pressure-inhg");
        temp     = fgGetDouble("/environment/metar/temperature-degc");
        dewpoint = fgGetDouble("/environment/metar/dewpoint-degc");

        // Set the cloud layers by copying over the METAR versions.
        SGPropertyNode * clouds = fgGetNode("/environment/metar/clouds", true);

        vector<SGPropertyNode_ptr> layers = clouds->getChildren("layer");
        vector<SGPropertyNode_ptr>::const_iterator layer;
        vector<SGPropertyNode_ptr>::const_iterator layers_end = layers.end();

        const char *cl = "/environment/clouds/layer[%i]";
        char s[128];
        int i;

        for (i = 0, layer = layers.begin(); layer != layers_end; ++layer, i++) {
            snprintf(s, 128, cl, i);
            strncat(s, "/coverage", 128);
            fgSetString(s, (*layer)->getStringValue("coverage", "clear"));

            snprintf(s, 128, cl, i);
            strncat(s, "/elevation-ft", 128);
            fgSetDouble(s, (*layer)->getDoubleValue("elevation-ft"));

            snprintf(s, 128, cl, i);
            strncat(s, "/thickness-ft", 128);
            fgSetDouble(s, (*layer)->getDoubleValue("thickness-ft"));

            snprintf(s, 128, cl, i);
            strncat(s, "/span-m", 128);
            fgSetDouble(s, 40000.0);
        }

        // Force an update of the 3D clouds
        fgSetDouble("/environment/rebuild-layers", 1.0);
    }

    fgSetupWind(dir_from, dir_to, speed, gust);
    fgDefaultWeatherValue("visibility-m", vis);
    set_temp_at_altitude(temp, station_elevation_ft);
    set_dewpoint_at_altitude(dewpoint, station_elevation_ft);
    fgDefaultWeatherValue("pressure-sea-level-inhg", pressure);

    // We've now successfully loaded a METAR into the configuration
    metar_loaded = true;
}

double FGMetarEnvironmentCtrl::interpolate_prop(const char * currentname,
                                                const char * requiredname,
                                                double dt)
{
    double currentval = fgGetDouble(currentname);
    double requiredval = fgGetDouble(requiredname);
    return interpolate_val(currentval, requiredval, dt);
}

double FGMetarEnvironmentCtrl::interpolate_val(double currentval,
                                               double requiredval,
                                               double dt)
{
    double dval = EnvironmentUpdatePeriodSec * dt;

    if (fabs(currentval - requiredval) < dval) return requiredval;
    if (currentval < requiredval) return (currentval + dval);
    if (currentval > requiredval) return (currentval - dval);
    return requiredval;
}

void
FGMetarEnvironmentCtrl::init ()
{
    SGGeod pos = SGGeod::fromDeg(
      fgGetDouble("/position/longitude-deg", true), 
      fgGetDouble( "/position/latitude-deg", true));

    metar_loaded = false;
    bool found_metar = false;
    long max_age = metar_max_age->getLongValue();
    // Don't check max age during init so that we don't loop over a lot
    // of airports metar if there is a problem.
    // The update() calls will find a correct metar if things went wrong here
    metar_max_age->setLongValue(0);

    while ( !found_metar && (_error_count < 3) ) {
        AirportWithMetar filter;
        FGPositionedRef a = FGPositioned::findClosest(pos, 10000.0, &filter);
        if (!a) {
          break;
        }
        
        FGMetarResult result = fetch_data(a->ident());
        if ( result.m != NULL ) {
            SG_LOG( SG_GENERAL, SG_INFO, "closest station w/ metar = "
                    << a->ident());
            last_apt = a;
            _icao = a->ident();
            search_elapsed = 0.0;
            fetch_elapsed = 0.0;
            update_metar_properties( result.m );
            update_env_config();
            env->init();
            found_metar = true;
        } else {
            // mark as no metar so it doesn't show up in subsequent
            // searches.
            SG_LOG( SG_GENERAL, SG_INFO, "no metar at metar = " << a->ident() );
            static_cast<FGAirport*>(a.ptr())->setMetar(false);
        }
    } // of airprot-with-metar search iteration
    
    metar_max_age->setLongValue(max_age);
}

void
FGMetarEnvironmentCtrl::reinit ()
{
    _error_count = 0;
    _error_dt = 0.0;
    metar_loaded = false;

    env->reinit();
}

void
FGMetarEnvironmentCtrl::update(double delta_time_sec)
{
    _dt += delta_time_sec;
    if (_error_count >= 3)
       return;

    FGMetarResult result;

    static const SGPropertyNode *longitude
        = fgGetNode( "/position/longitude-deg", true );
    static const SGPropertyNode *latitude
        = fgGetNode( "/position/latitude-deg", true );
    SGGeod pos = SGGeod::fromDeg(longitude->getDoubleValue(), 
      latitude->getDoubleValue());
        
    search_elapsed += delta_time_sec;
    fetch_elapsed += delta_time_sec;
    interpolate_elapsed += delta_time_sec;

    // if time for a new search request, push it onto the request
    // queue
    if ( search_elapsed > search_interval_sec ) {
        AirportWithMetar filter;
        FGPositionedRef a = FGPositioned::findClosest(pos, 10000.0, &filter);
        if (a) {
          if ( !last_apt || last_apt->ident() != a->ident()
                 || fetch_elapsed > same_station_interval_sec )
            {
                SG_LOG( SG_GENERAL, SG_INFO, "closest station w/ metar = "
                        << a->ident());
                request_queue.push( a->ident() );
                last_apt = a;
                _icao = a->ident();
                search_elapsed = 0.0;
                fetch_elapsed = 0.0;
            } else {
                search_elapsed = 0.0;
                SG_LOG( SG_GENERAL, SG_INFO, "same station, waiting = "
                        << same_station_interval_sec - fetch_elapsed );
            }

        } else {
          SG_LOG( SG_GENERAL, SG_WARN,
                    "Unable to find any airports with metar" );
        }
    } else if ( interpolate_elapsed > EnvironmentUpdatePeriodSec ) {
        // Interpolate the current configuration closer to the actual METAR
        update_env_config();
        env->reinit();
        interpolate_elapsed = 0.0;
    }

#if !defined(ENABLE_THREADS)
    // No loader thread running so manually fetch the data
    string id = "";
    while ( !request_queue.empty() ) {
        id = request_queue.front();
        request_queue.pop();
    }

    if ( !id.empty() ) {
        SG_LOG( SG_GENERAL, SG_INFO, "inline fetching = " << id );
        result = fetch_data( id );
        result_queue.push( result );
    }
#endif // ENABLE_THREADS

    // process any results from the loader.
    while ( !result_queue.empty() ) {
        result = result_queue.front();
        result_queue.pop();
        if ( result.m != NULL ) {
            update_metar_properties( result.m );
            delete result.m;
            update_env_config();
            env->reinit();
        } else {
            // mark as no metar so it doesn't show up in subsequent
            // searches, and signal an immediate re-search.
            SG_LOG( SG_GENERAL, SG_WARN,
                    "no metar at station = " << result.icao );
            const FGAirport* apt = globals->get_airports()->search(result.icao);
            const_cast<FGAirport*>(apt)->setMetar(false);
            search_elapsed = 9999.0;
        }
    }

    env->update(delta_time_sec);
}


void
FGMetarEnvironmentCtrl::setEnvironment (FGEnvironment * environment)
{
    env->setEnvironment(environment);
}

FGMetarResult
FGMetarEnvironmentCtrl::fetch_data( const string &icao )
{
    FGMetarResult result;
    result.icao = icao;

    // if the last error was more than three seconds ago,
    // then pretent nothing happened.
    if (_error_dt < 3) {
        _error_dt += _dt;

    } else {
        _error_dt = 0.0;
        _error_count = 0;
    }

    // fetch station elevation if exists
    const FGAirport* a = globals->get_airports()->search( icao );
    if ( a ) {
        station_elevation_ft = a->getElevation();
    }

    // fetch current metar data
    try {
        string host = proxy_host->getStringValue();
        string auth = proxy_auth->getStringValue();
        string port = proxy_port->getStringValue();
        result.m = new FGMetar( icao, host, port, auth);

        long max_age = metar_max_age->getLongValue();
        long age = result.m->getAge_min();
        if (max_age &&  age > max_age) {
            SG_LOG( SG_GENERAL, SG_WARN, "METAR data too old (" << age << " min).");
            delete result.m;
            result.m = NULL;

            if (++_stale_count > 10) {
                _error_count = 1000;
                throw sg_io_exception("More than 10 stale METAR messages in a row."
                        " Check your system time!");
            }
        } else
            _stale_count = 0;

    } catch (const sg_io_exception& e) {
        SG_LOG( SG_GENERAL, SG_WARN, "Error fetching live weather data: "
                << e.getFormattedMessage().c_str() );
#if defined(ENABLE_THREADS)
        if (_error_count++ >= 3) {
           SG_LOG( SG_GENERAL, SG_WARN, "Stop fetching data permanently.");
           thread_stop();
        }
#endif

        result.m = NULL;
    }

    _dt = 0;

    return result;
}


void
FGMetarEnvironmentCtrl::update_metar_properties( const FGMetar *m )
{
    int i;
    double d;
    char s[128];

    fgSetString("/environment/metar/real-metar", m->getData());
	// don't update with real weather when we use a custom weather scenario
	const char *current_scenario = fgGetString("/environment/weather-scenario", "METAR");
	if( strcmp(current_scenario, "METAR") && strcmp(current_scenario, "none"))
		return;
    fgSetString("/environment/metar/last-metar", m->getData());
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

    const char *cl = "/environment/metar/clouds/layer[%i]";
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


#if defined(ENABLE_THREADS)
void
FGMetarEnvironmentCtrl::thread_stop()
{
    request_queue.push( string() );	// ask thread to terminate
    thread->join();
}

void
FGMetarEnvironmentCtrl::MetarThread::run()
{
    while ( true )
    {
        string icao = fetcher->request_queue.pop();
        if (icao.empty())
            return;
        SG_LOG( SG_GENERAL, SG_INFO, "Thread: fetch metar data = " << icao );
        FGMetarResult result = fetcher->fetch_data( icao );
        fetcher->result_queue.push( result );
    }
}
#endif // ENABLE_THREADS


// end of environment_ctrl.cxx
