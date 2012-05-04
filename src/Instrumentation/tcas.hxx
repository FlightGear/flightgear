// tcas.hxx -- Traffic Alert and Collision Avoidance System (TCAS)
//
// Written by Thorsten Brehm, started December 2010.
//
// Copyright (C) 2010 Thorsten Brehm - brehmt (at) gmail com
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef __INSTRUMENTS_TCAS_HXX
#define __INSTRUMENTS_TCAS_HXX

#include <assert.h>

#include <vector>
#include <deque>
#include <map>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <Sound/voiceplayer.hxx>

using std::vector;
using std::deque;
using std::map;

class SGSampleGroup;

#include <Main/globals.hxx>

#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable: 4355 )
#endif

///////////////////////////////////////////////////////////////////////////////
// TCAS  //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class TCAS : public SGSubsystem
{

    typedef enum
    {
        AdvisoryClear         = 0,                          /*< Clear of traffic */
        AdvisoryIntrusion     = 1,                          /*< Intrusion flag */
        AdvisoryClimb         = AdvisoryIntrusion|(1 << 1), /*< RA climb */
        AdvisoryDescend       = AdvisoryIntrusion|(1 << 2), /*< RA descend */
        AdvisoryAdjustVSpeed  = AdvisoryIntrusion|(1 << 3), /*< RA adjust vertical speed (TCAS II 7.0 only) */
        AdvisoryMaintVSpeed   = AdvisoryIntrusion|(1 << 4), /*< RA maintain vertical speed */
        AdvisoryMonitorVSpeed = AdvisoryIntrusion|(1 << 5), /*< RA monitor vertical speed */
        AdvisoryLevelOff      = AdvisoryIntrusion|(1 << 6)  /*< RA level off (TCAS II 7.1 only) */
    } EnumAdvisory;

    typedef enum
    {
        OptionNone            = 0,        /*< no option modifier */
        OptionIncreaseClimb   = (1 << 0), /*< increase climb */
        OptionIncreaseDescend = (1 << 1), /*< increase descend */
        OptionCrossingClimb   = (1 << 2), /*< crossing climb */
        OptionCrossingDescent = (1 << 3)  /*< crossing descent */
    } EnumAdvisoryOption;

    typedef enum
    {
        SwitchOff        = 0, /*< TCAS switched off */
        SwitchStandby    = 1, /*< TCAS standby (no TA/RA) */
        SwitchTaOnly     = 2, /*< TCAS in TA-only mode (no RA) */
        SwitchAuto       = 3  /*< TCAS in TA/RA mode */
    } EnumModeSwitch;

    typedef enum
    {
        ThreatInvisible    = -1,/*< Traffic is invisible to TCAS (i.e. no transponder) */
        ThreatNone         = 0, /*< Traffic is visible but no threat. */
        ThreatProximity    = 1, /*< Proximity intruder traffic (no threat). */
        ThreatTA           = 2, /*< TA-level threat traffic. */
        ThreatRA           = 3  /*< RA-level threat traffic. */
    } EnumThreatLevel;

    typedef struct
    {
      int  threatLevel;  /*< intruder threat level: 0=clear, 1=proximity,
                             2=intruder, 3=proximity intruder */
      int  RA;           /*< resolution advisory */
      int  RAOption;     /*< option flags for advisory */
    } ResolutionAdvisory;

    typedef struct
    {
        float Tau;       /*< vertical/horizontal protection range in seconds */ 
        float DMOD;      /*< horizontal protection range in nm */
        float ALIM;      /*< vertical protection range in ft */
    } Thresholds;

    typedef struct
    {
        double    maxAltitude; /*< max altitude for this sensitivity level */
        int        sl;         /*< sensitivity level */
        Thresholds TA;         /*< thresholds for TA-level threats */
        Thresholds RA;         /*< thresholds for RA-level threats */
    } SensitivityLevel;

    typedef struct
    {
        string callsign;
        bool   verticalTA;
        bool   verticalRA;
        bool   horizontalTA;
        bool   horizontalRA;
        bool   isTracked;
        float  horizontalTau;
        float  verticalTau;
        float  relativeAltitudeFt;
        float  verticalFps;
        int    RASense;
    } ThreatInfo;

    typedef struct
    {
        int    threatLevel;
        double TAtimestamp;
        double RAtimestamp;
    } TrackerTarget;

    typedef map<string,TrackerTarget*> TrackerTargets;

    typedef struct
    {
        double lat;
        double lon;
        float  pressureAltFt;
        float  radarAltFt;
        float  heading;
        float  velocityKt;
        float  verticalFps;
    } LocalInfo; /*< info structure for local aircraft */

    /////////////////////////////////////////////////////////////////////////////
    // TCAS::PropertiesHandler ///////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    class PropertiesHandler : public FGVoicePlayer::PropertiesHandler
    {
      TCAS *tcas;

    public:
      PropertiesHandler (TCAS *device) :
        FGVoicePlayer::PropertiesHandler(), tcas(device) {}

      PropertiesHandler (void) : FGVoicePlayer::PropertiesHandler() {}
    };

    /////////////////////////////////////////////////////////////////////////////
    // TCAS::VoicePlayer ////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    class VoicePlayer :
        public FGVoicePlayer
    {
    public:
        VoicePlayer (TCAS* tcas) :
          FGVoicePlayer(&tcas->properties_handler, "tcas") {}

        ~VoicePlayer (void) {}

        void init    (void);

        struct
        {
          Voice* pTrafficTraffic;
          Voice* pClimb;
          Voice* pClimbNow;
          Voice* pClimbCrossing;
          Voice* pClimbIncrease;
          Voice* pDescend;
          Voice* pDescendNow;
          Voice* pDescendCrossing;
          Voice* pDescendIncrease;
          Voice* pClear;
          Voice* pAdjustVSpeed;
          Voice* pMaintVSpeed;
          Voice* pMonitorVSpeed;
          Voice* pLevelOff;
          Voice* pTestOk;
          Voice* pTestFail;
        } Voices;
    private:
        SGPropertyNode_ptr nodeSoundFilePrefix;
    };

    /////////////////////////////////////////////////////////////////////////////
    // TCAS::Annunciator ////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    class Annunciator
    {
    public:
        Annunciator    (TCAS* tcas);
        ~Annunciator   (void) {}
        void bind      (SGPropertyNode* node);
        void init      (void);
        void update    (void);

        void trigger   (const ResolutionAdvisory& newAdvisory, bool revertedRA);
        void test      (bool testOk);
        void clear     (void);
        bool isPlaying (void) { return voicePlayer.is_playing();}

    private:
        TCAS* tcas;
        ResolutionAdvisory previous;
        FGVoicePlayer::Voice* pLastVoice;
        VoicePlayer voicePlayer;
        SGPropertyNode_ptr nodeGpwsAlertOn;
    };

    /////////////////////////////////////////////////////////////////////////////
    // TCAS::AdvisoryCoordinator ////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    class AdvisoryCoordinator
    {
    public:
        AdvisoryCoordinator  (TCAS* _tcas);
        ~AdvisoryCoordinator (void) {}

        void bind            (SGPropertyNode* node);
        void init            (void);
        void update          (int mode);

        void clear           (void);
        void add             (const ResolutionAdvisory& newAdvisory);

    private:
        TCAS* tcas;
        double lastTATime;
        ResolutionAdvisory current;
        ResolutionAdvisory previous;
        SGPropertyNode_ptr nodeTAWarning;
    };

    /////////////////////////////////////////////////////////////////////////////
    // TCAS::Tracker ////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    class Tracker
    {
    public:
        Tracker  (TCAS* _tcas);
        ~Tracker (void) {}

        void update          (void);

        void add             (const string callsign, int detectedLevel);
        bool active          (void) { return haveTargets;}
        bool newTraffic      (void) { return newTargets;}
        bool isTracked       (string callsign) { if (!haveTargets) return false;else return _isTracked(callsign);}
        bool _isTracked      (string callsign);
        int  getThreatLevel  (string callsign);

    private:
        TCAS*  tcas;
        double currentTime;
        bool   haveTargets;
        bool   newTargets;
        TrackerTargets targets;
    };
    
    /////////////////////////////////////////////////////////////////////////////
    // TCAS::AdvisoryGenerator //////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    class AdvisoryGenerator
    {
    public:
        AdvisoryGenerator        (TCAS* _tcas);
        ~AdvisoryGenerator       (void) {}

        void init                (const LocalInfo* _pSelf, ThreatInfo* _pCurrentThreat);

        void setAlarmThresholds  (const SensitivityLevel* _pAlarmThresholds);

        int   resolution         (int mode, int threatLevel, float distanceNm,
                                  float altFt, float heading, float velocityKt);

    private:
        float verticalSeparation (float newVerticalFps);
        void  determineRAsense   (int& RASense, bool& isCrossing);

    private:
        TCAS*             tcas;
        const LocalInfo*  pSelf;          /*< info structure for local aircraft */
        ThreatInfo* pCurrentThreat; /*< info structure on current intruder/threat */
        const SensitivityLevel* pAlarmThresholds;
    };

    /////////////////////////////////////////////////////////////////////////////
    // TCAS::ThreatDetector /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    class ThreatDetector
    {
    public:
        ThreatDetector            (TCAS* _tcas);
        ~ThreatDetector           (void) {}

        void  init                (void);
        void  update              (void);

        bool  checkTransponder    (const SGPropertyNode* pModel, float velocityKt);
        int   checkThreat         (int mode, const SGPropertyNode* pModel);
        void  checkVerticalThreat (void);
        void  horizontalThreat    (float bearing, float distanceNm, float heading,
                                   float velocityKt);

        void  setPressureAlt      (float altFt) { self.pressureAltFt = altFt;}
        float getPressureAlt      (void)        { return self.pressureAltFt;}

        void  setRadarAlt         (float altFt) { self.radarAltFt = altFt;}
        float getRadarAlt         (void)        { return self.radarAltFt;}

        float getVelocityKt       (void)        { return self.velocityKt;}
        int   getRASense          (void)        { return currentThreat.RASense;}

    private:
        void  unitTest            (void);

    private:
        static const SensitivityLevel sensitivityLevels[];

        TCAS*              tcas;
        int                checkCount;

        SGPropertyNode_ptr nodeLat;
        SGPropertyNode_ptr nodeLon;
        SGPropertyNode_ptr nodePressureAlt;
        SGPropertyNode_ptr nodeRadarAlt;
        SGPropertyNode_ptr nodeHeading;
        SGPropertyNode_ptr nodeVelocity;
        SGPropertyNode_ptr nodeVerticalFps;

        LocalInfo          self;          /*< info structure for local aircraft */
        ThreatInfo         currentThreat; /*< info structure on current intruder/threat */
        const SensitivityLevel* pAlarmThresholds;
    };

private:
    string              name;
    int                 num;
    double              nextUpdateTime;
    int                 selfTestStep;

    SGPropertyNode_ptr  nodeModeSwitch;
    SGPropertyNode_ptr  nodeServiceable;
    SGPropertyNode_ptr  nodeSelfTest;
    SGPropertyNode_ptr  nodeDebugTrigger;
    SGPropertyNode_ptr  nodeDebugRA;
    SGPropertyNode_ptr  nodeDebugThreat;

    PropertiesHandler   properties_handler;
    ThreatDetector      threatDetector;
    Tracker             tracker;
    AdvisoryCoordinator advisoryCoordinator;
    AdvisoryGenerator   advisoryGenerator;
    Annunciator         annunciator;

private:
    void selfTest       (void);

public:
    TCAS (SGPropertyNode* node);

    virtual void bind   (void);
    virtual void unbind (void);
    virtual void init   (void);
    virtual void update (double dt);
};

#ifdef _MSC_VER
#  pragma warning( pop )
#endif

#endif // __INSTRUMENTS_TCAS_HXX
