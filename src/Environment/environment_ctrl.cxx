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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#include <simgear/debug/logstream.hxx>

#include <stdlib.h>
#include <algorithm>

#include <simgear/structure/commands.hxx>
#include <simgear/structure/exception.hxx>

#include <Airports/simple.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "environment_mgr.hxx"
#include "environment_ctrl.hxx"

SG_USING_STD(sort);



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
    sort(table.begin(), table.end());
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



////////////////////////////////////////////////////////////////////////
// Implementation of FGMetarEnvironmentCtrl.
////////////////////////////////////////////////////////////////////////

FGMetarEnvironmentCtrl::FGMetarEnvironmentCtrl ()
    : env( new FGInterpolateEnvironmentCtrl ),
      _icao( "" ),
      search_interval_sec( 60.0 ),        // 1 minute
      same_station_interval_sec( 900.0 ), // 15 minutes
      search_elapsed( 9999.0 ),
      fetch_elapsed( 9999.0 ),
      proxy_host( fgGetNode("/sim/presets/proxy/host", true) ),
      proxy_port( fgGetNode("/sim/presets/proxy/port", true) ),
      proxy_auth( fgGetNode("/sim/presets/proxy/authentication", true) ),
      metar_max_age( fgGetNode("/environment/params/metar-max-age-min", true) ),
      _error_count( 0 ),
      _dt( 0.0 ),
      _error_dt( 0.0 )
{
#if defined(ENABLE_THREADS) && ENABLE_THREADS
    thread = new MetarThread(this);
    thread->start( 1 );
#endif // ENABLE_THREADS
}

FGMetarEnvironmentCtrl::~FGMetarEnvironmentCtrl ()
{
#if defined(ENABLE_THREADS) && ENABLE_THREADS
   thread->cancel();
   thread->join();
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
    fgSetupWind( fgGetDouble("/environment/metar/base-wind-range-from"),
                 fgGetDouble("/environment/metar/base-wind-range-to"),
                 fgGetDouble("/environment/metar/base-wind-speed-kt"),
                 fgGetDouble("/environment/metar/gust-wind-speed-kt") );

    fgDefaultWeatherValue( "visibility-m",
                           fgGetDouble("/environment/metar/min-visibility-m") );
    set_temp_at_altitude( fgGetDouble("/environment/metar/temperature-degc"),
                          station_elevation_ft );
    set_dewpoint_at_altitude( fgGetDouble("/environment/metar/dewpoint-degc"),
                              station_elevation_ft );
    fgDefaultWeatherValue( "pressure-sea-level-inhg",
                           fgGetDouble("/environment/metar/pressure-inhg") );
}

void
FGMetarEnvironmentCtrl::init ()
{
    const SGPropertyNode *longitude
        = fgGetNode( "/position/longitude-deg", true );
    const SGPropertyNode *latitude
        = fgGetNode( "/position/latitude-deg", true );

    bool found_metar = false;
    long max_age = metar_max_age->getLongValue();
    // Don't check max age during init so that we don't loop over a lot
    // of airports metar if there is a problem.
    // The update() calls will find a correct metar if things went wrong here
    metar_max_age->setLongValue(60 * 24 * 7);

    while ( !found_metar && (_error_count < 3) ) {
        const FGAirport* a = globals->get_airports()
                   ->search( longitude->getDoubleValue(),
                             latitude->getDoubleValue(),
                             true );
        if ( a ) {  
            FGMetarResult result = fetch_data( a->getId() );
            if ( result.m != NULL ) {
                SG_LOG( SG_GENERAL, SG_INFO, "closest station w/ metar = "
                        << a->getId());
                last_apt = *a;
                _icao = a->getId();
                search_elapsed = 0.0;
                fetch_elapsed = 0.0;
                update_metar_properties( result.m );
                update_env_config();
                env->init();
                found_metar = true;
            } else {
                // mark as no metar so it doesn't show up in subsequent
                // searches.
                SG_LOG( SG_GENERAL, SG_INFO, "no metar at metar = "
                        << a->getId() );
                globals->get_airports()->no_metar( a->getId() );
            }
        }
    }
    metar_max_age->setLongValue(max_age);
}

void
FGMetarEnvironmentCtrl::reinit ()
{
    _error_count = 0;
    _error_dt = 0.0;

#if 0
    update_env_config();
#endif

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
    search_elapsed += delta_time_sec;
    fetch_elapsed += delta_time_sec;

    // if time for a new search request, push it onto the request
    // queue
    if ( search_elapsed > search_interval_sec ) {
        const FGAirport* a = globals->get_airports()
                   ->search( longitude->getDoubleValue(),
                             latitude->getDoubleValue(),
                             true );
        if ( a ) {
            if ( last_apt.getId() != a->getId()
                 || fetch_elapsed > same_station_interval_sec )
            {
                SG_LOG( SG_GENERAL, SG_INFO, "closest station w/ metar = "
                        << a->getId());
                request_queue.push( a->getId() );
                last_apt = *a;
                _icao = a->getId();
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
    }

#if defined(ENABLE_THREADS) && ENABLE_THREADS
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
            globals->get_airports()->no_metar( result.icao );
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
        if (max_age && result.m->getAge_min() > max_age) {
            SG_LOG( SG_GENERAL, SG_WARN, "METAR data too old");
            delete result.m;
            result.m = NULL;
        }

    } catch (const sg_io_exception& e) {
        SG_LOG( SG_GENERAL, SG_WARN, "Error fetching live weather data: "
                << e.getFormattedMessage().c_str() );
#if defined(ENABLE_THREADS) && ENABLE_THREADS
        if (_error_count++ >= 3) {
           SG_LOG( SG_GENERAL, SG_WARN, "Stop fetching data permanently.");
           thread->cancel();
           thread->join();
        }
#endif

        result.m = NULL;
    }

    _dt = 0;

    return result;
}


void
FGMetarEnvironmentCtrl::update_metar_properties( FGMetar *m )
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

    SGMetarVisibility *dirvis = m->getDirVisibility();
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
    vector<SGMetarCloud>::iterator cloud;

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


#if defined(ENABLE_THREADS) && ENABLE_THREADS
/**
 *
 */
void
FGMetarEnvironmentCtrl::MetarThread::run()
{
    pthread_cleanup_push( metar_cleanup_handler, fetcher );
    while ( true )
    {
        set_cancel( SGThread::CANCEL_DISABLE );

        string icao = fetcher->request_queue.pop();
        SG_LOG( SG_GENERAL, SG_INFO, "Thread: fetch metar data = " << icao );
        FGMetarResult result = fetcher->fetch_data( icao );

        set_cancel( SGThread::CANCEL_DEFERRED );

        fetcher->result_queue.push( result );
    }
    pthread_cleanup_pop(1);
}

/**
 * Ensure mutex is unlocked.
 */
void
metar_cleanup_handler( void* arg )
{
    FGMetarEnvironmentCtrl* fetcher = (FGMetarEnvironmentCtrl*) arg;
    fetcher->mutex.unlock();
}
#endif // ENABLE_THREADS


// end of environment_ctrl.cxx
