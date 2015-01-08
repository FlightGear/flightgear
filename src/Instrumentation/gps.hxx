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
#include <Navaids/FlightPlan.hxx>
#include <Instrumentation/rnav_waypt_controller.hxx>

#define FG_210_COMPAT 1

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
class GPS : public SGSubsystem,
            public flightgear::RNAV,
            public flightgear::FlightPlan::Delegate
{
public:
    GPS (SGPropertyNode *node, bool defaultGPSMode = false);
    GPS ();
    virtual ~GPS ();

  // SGSubsystem interface
    virtual void init ();
    virtual void reinit ();
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
    virtual double overflightDistanceM();
    virtual double overflightArmDistanceM();
    virtual double overflightArmAngleDeg();
    virtual SGGeod previousLegWaypointPosition(bool& isValid);

private:
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
       * Distance at which we switch to next waypoint.
       */
      double overflightDistanceNm() const { return _overflightDistance; }
      /**
       * Distance at which we arm overflight sequencing. Once inside this
       * distance, a change of the wp1 'TO' flag to false will be considered
       * overlight of the wp.
       */
      double overflightArmDistanceNm() const { return _overflightArmDistance; }
      /**
		 * abs angle at which we arm overflight sequencing.
		 */
      double overflightArmAngleDeg() const { return _overflightArmAngle; }

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

        /**
         * Select whether we fly the leg track between waypoints, or
         * use a direct course from the turn end. Since this is likely confusing,
         * look at: http://fgfs.goneabitbursar.com//screenshots/FlyByType-LegType.svg
         * For fly-by waypoints, there is no difference. For fly-over waypoints,
         * this selects if we fly TF or DF mode.
         */
        bool followLegTrackToFix() const      { return _followLegTrackToFix; }

    private:
      bool _enableTurnAnticipation;

      // desired turn rate in degrees per second
      double _turnRate;

      // distance from waypoint to arm overflight sequencing (in nm)
      double _overflightDistance;

      // distance from waypoint to arm overflight sequencing (in nm)
      double _overflightArmDistance;

      //abs angle from course to waypoint to arm overflight sequencing (in deg)
      double _overflightArmAngle;

      // time before reaching a waypoint to trigger annunciator light/sound
      // (in seconds)
      double _waypointAlertTime;

      // should we require a hard-surfaced runway when filtering?
      bool _requireHardSurface;

      double _cdiMaxDeflectionNm;

      // should we drive the autopilot directly or not?
      bool _driveAutopilot;

      // is selected-course-deg read to set desired-course or not?
      bool _courseSelectable;

        // do we fly direct to fixes, or follow the leg track closely?
        bool _followLegTrackToFix;
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
    void updateRouteData();
    void driveAutopilot();

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

    void clearScratch();

    /** Predicate, determine if the lon/lat position in the scratch is
     * valid or not. */
    bool isScratchPositionValid() const;
    FGPositionedRef positionedFromScratch() const;
    
#if FG_210_COMPAT
    void setScratchFromPositioned(FGPositioned* aPos, int aIndex);
    void setScratchFromCachedSearchResult();
    void setScratchFromRouteWaypoint(int aIndex);
    
    /** Add airport-specific information to a scratch result */
    void addAirportToScratch(FGAirport* aAirport);
    
    FGPositioned::Filter* createFilter(FGPositioned::Type aTy);
    
    /** Search kernel - called each time we step through a result */
    void performSearch();
    
    // command handlers
    void loadRouteWaypoint();
    void loadNearest();
    void search();
    void nextResult();
    void previousResult();
    void defineWaypoint();
    void insertWaypointAtIndex(int aIndex);
    void removeWaypointAtIndex(int aIndex);
    
    // tied-property getter/setters
    double getScratchDistance() const;
    double getScratchMagBearing() const;
    double getScratchTrueBearing() const;
    bool getScratchHasNext() const;

#endif
    
// command handlers
    void selectLegMode();
    void selectOBSMode(flightgear::Waypt* waypt);
    void directTo();

// tied-property getter/setters
    void setCommand(const char* aCmd);
    const char* getCommand() const { return ""; }

    const char* getMode() const { return _mode.c_str(); }
    bool getScratchValid() const { return _scratchValid; }

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

    const char* getWP0Ident() const;
    const char* getWP0Name() const;

    bool getWP1IValid() const;
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

// FlightPlan::Delegate
    virtual void currentWaypointChanged();
    virtual void waypointsChanged();
    virtual void cleared();
    virtual void endOfFlightPlan();
    
    void sequence();
    void routeManagerFlightPlanChanged(SGPropertyNode*);
    void routeActivated(SGPropertyNode*);
    
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
    
  //  SGPropertyNode_ptr _route_active_node;
    SGPropertyNode_ptr _route_current_wp_node;
    SGPropertyNode_ptr _routeDistanceNm;
    SGPropertyNode_ptr _routeETE;
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

    /**
     * the instrument manager creates a default instance of us,
     * if no explicit GPS is specific in the aircraft's instruments.xml file.
     * This allows default route-following to work with the generic autopilot.
     * This flag is set in that case, to inform us we're a 'fake' installation,
     * and not to worry about electrical power or similar.
     */
    bool _defaultGPSMode;
    
    std::string _mode;
    Config _config;
    std::string _name;
    int _num;

    SGGeod _wp0_position;
    SGGeod _indicated_pos;
    double _legDistanceNm;

// scratch data
    SGGeod _scratchPos;
    SGPropertyNode_ptr _scratchNode;
    bool _scratchValid;
#if FG_210_COMPAT
// search data
    int _searchResultIndex;
    std::string _searchQuery;
    FGPositioned::Type _searchType;
    bool _searchExact;
    FGPositionedList _searchResults;
    bool _searchIsRoute; ///< set if 'search' is actually the current route
    bool _searchHasNext; ///< is there a result after this one?
    bool _searchNames; ///< set if we're searching names instead of idents
#endif
    
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

    flightgear::WayptRef _prevWaypt;
    flightgear::WayptRef _currentWaypt;

// autopilot drive properties
    SGPropertyNode_ptr _apDrivingFlag;
    SGPropertyNode_ptr _apTrueHeading;
    
    simgear::TiedPropertyList _tiedProperties;

    flightgear::FlightPlanRef _route;
    
    SGPropertyChangeCallback<GPS> _callbackFlightPlanChanged;
    SGPropertyChangeCallback<GPS> _callbackRouteActivated;
};

#endif // __INSTRUMENTS_GPS_HXX
