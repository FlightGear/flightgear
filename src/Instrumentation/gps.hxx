// gps.hxx - distance-measuring equipment.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENTS_GPS_HXX
#define __INSTRUMENTS_GPS_HXX 1

#include <cassert>
#include <memory>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>

#include <Navaids/positioned.hxx>
#include <Instrumentation/rnav_waypt_controller.hxx>

// forward decls
class SGRoute;
class FGRouteMgr;
class FGAirport;
class GPSListener;

class SGGeodProperty
{
public:
    SGGeodProperty()
    {
    }

    void init(SGPropertyNode* base, const char* lonStr, const char* latStr, const char* altStr = NULL);
    void init(const char* lonStr, const char* latStr, const char* altStr = NULL);
    void clear();
    void operator=(const SGGeod& geod);
    SGGeod get() const;

private:
    SGPropertyNode_ptr _lon, _lat, _alt;
};

/**
 * Model a GPS radio.
 *
 * Input properties:
 *
 * /position/longitude-deg
 * /position/latitude-deg
 * /position/altitude-ft
 * /environment/magnetic-variation-deg
 * /systems/electrical/outputs/gps
 * /instrumentation/gps/serviceable
 * 
 *
 * Output properties:
 *
 * /instrumentation/gps/indicated-longitude-deg
 * /instrumentation/gps/indicated-latitude-deg
 * /instrumentation/gps/indicated-altitude-ft
 * /instrumentation/gps/indicated-vertical-speed-fpm
 * /instrumentation/gps/indicated-track-true-deg
 * /instrumentation/gps/indicated-track-magnetic-deg
 * /instrumentation/gps/indicated-ground-speed-kt
 *
 * /instrumentation/gps/wp-distance-nm
 * /instrumentation/gps/wp-bearing-deg
 * /instrumentation/gps/wp-bearing-mag-deg
 * /instrumentation/gps/TTW
 * /instrumentation/gps/course-deviation-deg
 * /instrumentation/gps/course-error-nm
 * /instrumentation/gps/to-flag
 * /instrumentation/gps/odometer
 * /instrumentation/gps/trip-odometer
 * /instrumentation/gps/true-bug-error-deg
 * /instrumentation/gps/magnetic-bug-error-deg
 */
class GPS : public SGSubsystem, public flightgear::RNAV
{
public:
    GPS (SGPropertyNode *node);
    GPS ();
    virtual ~GPS ();

  // SGSubsystem interface
    virtual void init ();
    virtual void update (double delta_time_sec);

    virtual void bind();
    virtual void unbind();

  // RNAV interface
    virtual SGGeod position();
    virtual double trackDeg();
    virtual double groundSpeedKts();
    virtual double vspeedFPM();
    virtual double magvarDeg();
    virtual double selectedMagCourse();
    virtual double overflightArmDistanceM();

private:
    friend class GPSListener;
    friend class SearchFilter;

    /**
     * Configuration manager, track data relating to aircraft installation
     */
    class Config
    {
    public:
      Config();

      void bind(GPS* aOwner, SGPropertyNode* aCfg);

      bool turnAnticipationEnabled() const { return _enableTurnAnticipation; }

      /**
       * Desired turn rate in degrees/second. From this we derive the turn
       * radius and hence how early we need to anticipate it.
       */
      double turnRateDegSec() const        { return _turnRate; }

      /**
       * Distance at which we arm overflight sequencing. Once inside this
       * distance, a change of the wp1 'TO' flag to false will be considered
       * overlight of the wp.
       */
      double overflightArmDistanceNm() const { return _overflightArmDistance; }

      /**
       * Time before the next WP to activate an external annunciator
       */
      double waypointAlertTime() const     { return _waypointAlertTime; }

      bool requireHardSurface() const      { return _requireHardSurface; }

      bool cdiDeflectionIsAngular() const  { return (_cdiMaxDeflectionNm <= 0.0); }

      double cdiDeflectionLinearPeg() const
      {
        assert(_cdiMaxDeflectionNm > 0.0);
        return _cdiMaxDeflectionNm;
      }

      bool driveAutopilot() const          { return _driveAutopilot; }

      bool courseSelectable() const        { return _courseSelectable; }

    private:
      bool _enableTurnAnticipation;

      // desired turn rate in degrees per second
      double _turnRate;

      // distance from waypoint to arm overflight sequencing (in nm)
      double _overflightArmDistance;

      // time before reaching a waypoint to trigger annunciator light/sound
      // (in seconds)
      double _waypointAlertTime;

      // minimum runway length to require when filtering
      double _minRunwayLengthFt;

      // should we require a hard-surfaced runway when filtering?
      bool _requireHardSurface;

      double _cdiMaxDeflectionNm;

      // should we drive the autopilot directly or not?
      bool _driveAutopilot;

      // is selected-course-deg read to set desired-course or not?
      bool _courseSelectable;
    };

    class SearchFilter : public FGPositioned::Filter
    {
    public:
      virtual bool pass(FGPositioned* aPos) const;

      virtual FGPositioned::Type minType() const;
      virtual FGPositioned::Type maxType() const;
    };

    /** reset all output properties to default / non-service values */
    void clearOutput();

    void updateBasicData(double dt);

    void updateTrackingBug();
    void updateReferenceNavaid(double dt);
    void referenceNavaidSet(const std::string& aNavaid);
    void updateRouteData();
    void driveAutopilot();
    
    void routeActivated();
    void routeManagerSequenced();
    void routeEdited();
    void routeFinished();

    void updateTurn();
    void updateOverflight();
    void beginTurn();
    void endTurn();

    double computeTurnProgress(double aBearing) const;
    void computeTurnData();
    void updateTurnData();
    double computeTurnRadiusNm(double aGroundSpeedKts) const;

    /** Update one-shot things when WP1 / leg data change */
    void wp1Changed();

// scratch maintenance utilities
    void setScratchFromPositioned(FGPositioned* aPos, int aIndex);
    void setScratchFromCachedSearchResult();
    void setScratchFromRouteWaypoint(int aIndex);

    /** Add airport-specific information to a scratch result */
    void addAirportToScratch(FGAirport* aAirport);
  
    void clearScratch();

    /** Predicate, determine if the lon/lat position in the scratch is
     * valid or not. */
    bool isScratchPositionValid() const;

    FGPositioned::Filter* createFilter(FGPositioned::Type aTy);
  
   /** Search kernel - called each time we step through a result */
    void performSearch();

// command handlers
    void selectLegMode();
    void selectOBSMode();
    void directTo();
    void loadRouteWaypoint();
    void loadNearest();
    void search();
    void nextResult();
    void previousResult();
    void defineWaypoint();
    void insertWaypointAtIndex(int aIndex);
    void removeWaypointAtIndex(int aIndex);

// tied-property getter/setters
    void setCommand(const char* aCmd);
    const char* getCommand() const { return ""; }

    const char* getMode() const { return _mode.c_str(); }

    bool getScratchValid() const { return _scratchValid; }
    double getScratchDistance() const;
    double getScratchMagBearing() const;
    double getScratchTrueBearing() const;
    bool getScratchHasNext() const;

    double getSelectedCourse() const { return _selectedCourse; }
    void setSelectedCourse(double crs);
    double getDesiredCourse() const { return _desiredCourse; }

    double getCDIDeflection() const;

    double getLegDistance() const;
    double getLegCourse() const;
    double getLegMagCourse() const;

    double getTrueTrack() const { return _last_true_track; }
    double getMagTrack() const;
    double getGroundspeedKts() const { return _last_speed_kts; }
    double getVerticalSpeed() const { return _last_vertical_speed; }

    //bool getLegMode() const { return _mode == "leg"; }
    //bool getObsMode() const { return _mode == "obs"; }

    const char* getWP0Ident() const;
    const char* getWP0Name() const;

    const char* getWP1Ident() const;
    const char* getWP1Name() const;

    double getWP1Distance() const;
    double getWP1TTW() const;
    const char* getWP1TTWString() const;
    double getWP1Bearing() const;
    double getWP1MagBearing() const;
    double getWP1CourseDeviation() const;
    double getWP1CourseErrorNm() const;
    bool getWP1ToFlag() const;
    bool getWP1FromFlag() const;

    // true-bearing-error and mag-bearing-error


    /**
     * Tied-properties helper, record nodes which are tied for easy un-tie-ing
     */
    template <typename T>
    void tie(SGPropertyNode* aNode, const char* aRelPath, const SGRawValue<T>& aRawValue)
    {
        _tiedProperties.Tie(aNode->getNode(aRelPath, true), aRawValue);
    }

    /** helper, tie the lat/lon/elev of a SGGeod to the named children of aNode */
    void tieSGGeod(SGPropertyNode* aNode, SGGeod& aRef,
                   const char* lonStr, const char* latStr, const char* altStr);
  
    /** helper, tie a SGGeod to proeprties, but read-only */
    void tieSGGeodReadOnly(SGPropertyNode* aNode, SGGeod& aRef,
                           const char* lonStr, const char* latStr, const char* altStr);

// members
    SGPropertyNode_ptr _gpsNode;
    SGPropertyNode_ptr _currentWayptNode;
    SGPropertyNode_ptr _magvar_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _tracking_bug_node;
    SGPropertyNode_ptr _raim_node;

    SGPropertyNode_ptr _odometer_node;
    SGPropertyNode_ptr _trip_odometer_node;
    SGPropertyNode_ptr _true_bug_error_node;
    SGPropertyNode_ptr _magnetic_bug_error_node;
    SGPropertyNode_ptr _eastWestVelocity;
    SGPropertyNode_ptr _northSouthVelocity;

    SGPropertyNode_ptr _ref_navaid_id_node;
    SGPropertyNode_ptr _ref_navaid_bearing_node;
    SGPropertyNode_ptr _ref_navaid_distance_node;
    SGPropertyNode_ptr _ref_navaid_mag_bearing_node;
    SGPropertyNode_ptr _ref_navaid_frequency_node;
    SGPropertyNode_ptr _ref_navaid_name_node;

    SGPropertyNode_ptr _route_active_node;
    SGPropertyNode_ptr _route_current_wp_node;
    SGPropertyNode_ptr _routeDistanceNm;
    SGPropertyNode_ptr _routeETE;
    SGPropertyNode_ptr _routeEditedSignal;
    SGPropertyNode_ptr _routeFinishedSignal;
    SGPropertyNode_ptr _desiredCourseNode;

    double _selectedCourse;
    double _desiredCourse;

    bool _dataValid;
    SGGeod _last_pos;
    bool _lastPosValid;
    double _last_speed_kts;
    double _last_true_track;
    double _last_vertical_speed;
    double _lastEWVelocity;
    double _lastNSVelocity;

    std::string _mode;
    GPSListener* _listener;
    Config _config;
    FGRouteMgr* _routeMgr;

    bool _ref_navaid_set;
    double _ref_navaid_elapsed;
    FGPositionedRef _ref_navaid;

    std::string _name;
    int _num;

    SGGeodProperty _position;
    SGGeod _wp0_position;
    SGGeod _indicated_pos;
    double _legDistanceNm;

// scratch data
    SGGeod _scratchPos;
    SGPropertyNode_ptr _scratchNode;
    bool _scratchValid;

// search data
    int _searchResultIndex;
    std::string _searchQuery;
    FGPositioned::Type _searchType;
    bool _searchExact;
    FGPositioned::List _searchResults;
    bool _searchIsRoute; ///< set if 'search' is actually the current route
    bool _searchHasNext; ///< is there a result after this one?
    bool _searchNames; ///< set if we're searching names instead of idents

// turn data
    bool _computeTurnData; ///< do we need to update the turn data?
    bool _anticipateTurn; ///< are we anticipating the next turn or not?
    bool _inTurn; // is a turn in progress?
    bool _turnSequenced; // have we sequenced the new leg?
    double _turnAngle; // angle to turn through, in degrees
    double _turnStartBearing; // bearing of inbound leg
    double _turnRadius; // radius of turn in nm
    SGGeod _turnPt;
    SGGeod _turnCentre;

    std::auto_ptr<flightgear::WayptController> _wayptController;

    SGPropertyNode_ptr _realismSimpleGps; ///< should the GPS be simple or realistic?
    flightgear::WayptRef _prevWaypt;
    flightgear::WayptRef _currentWaypt;

// autopilot drive properties
    SGPropertyNode_ptr _apDrivingFlag;
    SGPropertyNode_ptr _apTrueHeading;
    SGPropertyNode_ptr _apTargetAltitudeFt;
    SGPropertyNode_ptr _apAltitudeLock;

    simgear::TiedPropertyList _tiedProperties;

};

#endif // __INSTRUMENTS_GPS_HXX
