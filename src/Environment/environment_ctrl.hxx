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

#include "Navaids/positioned.hxx"
#include "Environment/environment.hxx"

// forward decls
class SGPropertyNode;
class FGAirport;
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
 * Environment controller using user-supplied parameters.
 */
class FGUserDefEnvironmentCtrl : public FGEnvironmentCtrl
{
public:
  FGUserDefEnvironmentCtrl ();
  virtual ~FGUserDefEnvironmentCtrl ();

  virtual void init ();
  virtual void update (double dt);

private:

  SGPropertyNode_ptr _base_wind_speed_node;
  SGPropertyNode_ptr _gust_wind_speed_node;

  double _current_wind_speed_kt;
  double _delta_wind_speed_kt;

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

    void read_table (const SGPropertyNode * node, std::vector<bucket *> &table);
    void do_interpolate (std::vector<bucket *> &table, double altitude_ft,
                         FGEnvironment * environment);

    FGEnvironment env1, env2;   // temporaries

    std::vector<bucket *> _boundary_table;
    std::vector<bucket *> _aloft_table;
};


// A convenience wrapper around FGMetar
struct FGMetarResult {
    std::string icao;
    FGMetar *m;
};



/**
 * Interplation controller using the FGMetar class
 */
class FGMetarEnvironmentCtrl : public FGEnvironmentCtrl
{
public:
    FGMetarEnvironmentCtrl ();
    virtual ~FGMetarEnvironmentCtrl ();

    virtual void init ();
    virtual void reinit ();
    virtual void update (double delta_time_sec);
    virtual void setEnvironment (FGEnvironment * environment);

private:
    FGInterpolateEnvironmentCtrl *env;

    std::string _icao;
    bool metar_loaded;
    float station_elevation_ft;
    float search_interval_sec;
    float same_station_interval_sec;
    float search_elapsed;
    float fetch_elapsed;
    float interpolate_elapsed;
    FGPositionedRef last_apt;
    SGPropertyNode_ptr proxy_host;
    SGPropertyNode_ptr proxy_port;
    SGPropertyNode_ptr proxy_auth;
    SGPropertyNode_ptr metar_max_age;

    FGMetarResult fetch_data( const string &icao );
    virtual void update_metar_properties( const FGMetar *m );
    void update_env_config();
    double interpolate_prop(const char * currentname, const char * requiredname, double dvalue);
    double interpolate_val(double currentval, double requiredval, double dvalue);
    const double EnvironmentUpdatePeriodSec;    // Seconds between interpolations
    const double MaxWindChangeKtsSec;           // Max wind change in kts/sec
    const double MaxVisChangePercentSec;        // Max visibility change in %/sec
    const double MaxPressureChangeInHgSec;      // Max pressure change in InHg/sec
    const double MaxCloudAltitudeChangeFtSec;   // Max cloud altitude change in ft/s
    const double MaxCloudThicknessChangeFtSec;  // Max cloud thickness change in ft/s
    const double MaxCloudInterpolationHeightFt; // Max distance from aircraft to
                                                // interpolate at. Any cloud
                                                // changes above this height
                                                // difference are not interpolated
    const double MaxCloudInterpolationDeltaFt;  // Max difference in altitude to 
                                                // interpolate. Any cloud changing height
                                                // by more than this value is not 
                                                // interpolated

private:

#if defined(ENABLE_THREADS)
    /**
     * FIFO queue which holds a pointer to the fetched metar data.
     */
    SGBlockingQueue <std::string > request_queue;

    /**
     * FIFO queue which holds a pointer to the fetched metar data.
     */
    SGLockedQueue < FGMetarResult > result_queue;
#else
    /**
     * FIFO queue which holds a pointer to the fetched metar data.
     */
    std::queue <std::string > request_queue;

    /**
     * FIFO queue which holds a pointer to the fetched metar data.
     */
    std::queue < FGMetarResult > result_queue;
#endif

#if defined(ENABLE_THREADS)
    /**
     * This class represents the thread of execution responsible for
     * fetching the metar data.
     */
    class MetarThread : public OpenThreads::Thread
    {
    public:
        MetarThread( FGMetarEnvironmentCtrl* f ) : fetcher(f) {}
        ~MetarThread() {}

        /**
         * Fetche the metar data from the NOAA.
         */
        void run();

    private:
        FGMetarEnvironmentCtrl *fetcher;

    private:
        // not implemented.
        MetarThread();
        MetarThread( const MetarThread& );
        MetarThread& operator=( const MetarThread& );
    };

    friend class MetarThread;

    /**
     * Metar data fetching thread.
     */
    MetarThread* thread;

    void thread_stop();
#endif // ENABLE_THREADS

    int _error_count;
    int _stale_count;
    double _dt;
    double _error_dt;
};

#endif // _ENVIRONMENT_CTRL_HXX
