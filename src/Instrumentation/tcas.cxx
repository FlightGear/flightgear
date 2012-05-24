// tcas.cxx -- Traffic Alert and Collision Avoidance System (TCAS) Emulation
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
//
///////////////////////////////////////////////////////////////////////////////

/* References:
 *
 *  [TCASII] Introduction to TCAS II Version 7, Federal Aviation Administration, November 2000
 *           http://www.arinc.com/downloads/tcas/tcas.pdf
 *
 *  [EUROACAS] Eurocontrol Airborne Collision Avoidance System (ACAS),
 *           http://www.eurocontrol.int/msa/public/standard_page/ACAS_Startpage.html
 *
 * Glossary:
 *
 *  ALIM: Altitude Limit
 *
 *  CPA: Closest point of approach as computed from a threat's range and range rate.
 *
 *  DMOD: Distance MODification
 *
 *  Intruder: A target that has satisfied the traffic detection criteria.
 *
 *  Proximity target: Any target that is less than 6 nmi in range and within +/-1200ft
 *      vertically, but that does not meet the intruder or threat criteria.
 *
 *  RA: Resolution advisory. An indication given by TCAS II to a flight crew that a
 *      vertical maneuver should, or in some cases should not, be performed to attain or
 *      maintain safe separation from a threat.
 *
 *  SL: Sensitivity Level. A value used in defining the size of the protected volume
 *      around the own aircraft.
 *
 *  TA: Traffic Advisory. An indication given by TCAS to the pilot when an aircraft has
 *      entered, or is projected to enter, the protected volume around the own aircraft.
 *
 *  Tau: Approximation of the time, in seconds, to CPA or to the aircraft being at the
 *       same altitude.
 *
 *  TCAS: Traffic alert and Collision Avoidance System
 */

/* Module properties:
 *
 *   serviceable             enable/disable TCAS processing
 *   
 *   voice/file-prefix       path (and optional prefix) for sound sample files
 *                           (only evaluated at start-up)  
 *
 *   inputs/mode             TCAS mode selection: 0=off,1=standby,2=TA only,3=auto(TA/RA)
 *   inputs/self-test        trigger self-test sequence
 *
 *   outputs/traffic-alert   intruder detected (true=TA-threat is active, includes RA-threats)
 *   outputs/advisory-alert  resolution advisory is issued (true=advisory is valid)
 *   outputs/vertical-speed  vertical speed required by advisory (+/-2000/1500/500/0)
 *
 *   speaker/max-dist        Max. distance where speaker is heard
 *   speaker/reference-dist  Distance to pilot
 *   speaker/volume          Volume at reference distance
 *
 *   debug/threat-trigger    trigger debugging test (in debug mode only)
 *   debug/threat-RA         debugging RA value (in debug mode only)
 *   debug/threat-level      debugging threat level (in debug mode only)
 */

#ifdef _MSC_VER
#  pragma warning( disable: 4355 )
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <string>
#include <sstream>

#include <simgear/constants.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/structure/exception.hxx>

using std::string;

#if defined( HAVE_VERSION_H ) && HAVE_VERSION_H
#  include <Include/version.h>
#else
#  include <Include/no_version.h>
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include "instrument_mgr.hxx"
#include "tcas.hxx"

///////////////////////////////////////////////////////////////////////////////
// debug switches /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//#define FEATURE_TCAS_DEBUG_ANNUNCIATOR
//#define FEATURE_TCAS_DEBUG_COORDINATOR
//#define FEATURE_TCAS_DEBUG_THREAT_DETECTOR
//#define FEATURE_TCAS_DEBUG_TRACKER
//#define FEATURE_TCAS_DEBUG_ADV_GENERATOR
//#define FEATURE_TCAS_DEBUG_PROPERTIES

///////////////////////////////////////////////////////////////////////////////
// constants //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/** Sensitivity Level Definition and Alarm Thresholds (TCASII, Version 7) 
 *  Max Own        |    |TA-level                |RA-level
 *  Altitude(ft)   | SL |Tau(s),DMOD(nm),ALIM(ft)|Tau(s),DMOD(nm),ALIM(ft) */
const TCAS::SensitivityLevel
TCAS::ThreatDetector::sensitivityLevels[] = {
   { 1000,           2,  {20,   0.30,     850},  {0,    0,        0  }}, 
   { 2350,           3,  {25,   0.33,     850},  {15,   0.20,     300}},
   { 5000,           4,  {30,   0.48,     850},  {20,   0.35,     300}},
   {10000,           5,  {40,   0.75,     850},  {25,   0.55,     350}},
   {20000,           6,  {45,   1.00,     850},  {30,   0.80,     400}},
   {42000,           7,  {48,   1.30,     850},  {35,   1.10,     600}},
   {0,               8,  {48,   1.30,    1200},  {35,   1.10,     700}}
};

///////////////////////////////////////////////////////////////////////////////
// helpers ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define ADD_VOICE(Var, Sample, SayTwice)    \
    { make_voice(&Var);                    \
      append(Var, Sample);                  \
      if (SayTwice) append(Var, Sample); }

#define AVAILABLE_RA(Options, Advisory) (Advisory == (Advisory & Options))

#ifdef FEATURE_TCAS_DEBUG_THREAT_DETECTOR
/** calculate relative angle in between two headings */
static float
relAngle(float Heading1, float Heading2)
{
    Heading1 -= Heading2;

    while (Heading1 >= 360.0)
        Heading1 -= 360.0;

    while (Heading1 < 0.0)
        Heading1 += 360;

    return Heading1;
}
#endif

/** calculate range and bearing of lat2/lon2 relative to lat1/lon1 */ 
static void
calcRangeBearing(double lat1, double lon1, double lat2, double lon2,
                 double &rangeNm, double &bearing)
{
    // calculate the bearing and range of the second pos from the first
    double az2, distanceM;
    geo_inverse_wgs_84(lat1, lon1, lat2, lon2, &bearing, &az2, &distanceM);
    rangeNm = distanceM * SG_METER_TO_NM;
}

///////////////////////////////////////////////////////////////////////////////
// VoicePlayer ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
TCAS::VoicePlayer::init(void)
{
    FGVoicePlayer::init();

    ADD_VOICE(Voices.pTrafficTraffic, "traffic",                 true);
    ADD_VOICE(Voices.pClear,          "clear",                   false);
    ADD_VOICE(Voices.pClimb,          "climb",                   true);
    ADD_VOICE(Voices.pClimbNow,       "climb_now",               true);
    ADD_VOICE(Voices.pClimbCrossing,  "climb_crossing",          true);
    ADD_VOICE(Voices.pClimbIncrease,  "increase_climb",          false);
    ADD_VOICE(Voices.pDescend,        "descend",                 true);
    ADD_VOICE(Voices.pDescendNow,     "descend_now",             true);
    ADD_VOICE(Voices.pDescendCrossing,"descend_crossing",        true);
    ADD_VOICE(Voices.pDescendIncrease,"increase_descent",        false);
    ADD_VOICE(Voices.pAdjustVSpeed,   "adjust_vertical_speed",   false);
    ADD_VOICE(Voices.pMaintVSpeed,    "maintain_vertical_speed", false);
    ADD_VOICE(Voices.pMonitorVSpeed,  "monitor_vertical_speed",  false);
    ADD_VOICE(Voices.pLevelOff,       "level_off",               false);
    ADD_VOICE(Voices.pTestOk,         "test_ok",                 false);
    ADD_VOICE(Voices.pTestFail,       "test_fail",               false);

    speaker.update_configuration();
}

/////////////////////////////////////////////////////////////////////////////
// TCAS::Annunciator ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

TCAS::Annunciator::Annunciator(TCAS* _tcas) :
    tcas(_tcas),
    pLastVoice(NULL),
    voicePlayer(_tcas)
{
    clear();
}

void TCAS::Annunciator::clear(void)
{
    previous.threatLevel = ThreatNone;
    previous.RA          = AdvisoryClear;
    previous.RAOption    = OptionNone;
    pLastVoice = NULL;
}

void
TCAS::Annunciator::bind(SGPropertyNode* node)
{
    voicePlayer.bind(node, "Sounds/tcas/female/");
}

void
TCAS::Annunciator::init(void)
{
    //TODO link to GPWS module/audio-on signal must be configurable
    nodeGpwsAlertOn = fgGetNode("/instrumentation/mk-viii/outputs/discretes/audio-on", true);
    voicePlayer.init();
}

void
TCAS::Annunciator::update(void)
{
    voicePlayer.update();
    
    /* [TCASII]: "The priority scheme gives ground proximity warning systems (GPWS)
     *   a higher annunciation priority than a TCAS alert. TCAS aural annunciation will
     *   be inhibited during the time that a GPWS alert is active." */
    if (nodeGpwsAlertOn->getBoolValue())
        voicePlayer.pause();
    else
        voicePlayer.resume();
}

/** Trigger voice sample for current alert. */
void
TCAS::Annunciator::trigger(const ResolutionAdvisory& current, bool revertedRA)
{
    int RA = current.RA;
    int RAOption = current.RAOption;

    if (RA == AdvisoryClear)
    {
        if (previous.RA != AdvisoryClear)
        {
            voicePlayer.play(voicePlayer.Voices.pClear, VoicePlayer::PLAY_NOW);
            previous = current;
        }
        pLastVoice = NULL;
        return;
    }

    if ((previous.RA == AdvisoryClear)||
        (tcas->tracker.newTraffic()))
    {
        voicePlayer.play(voicePlayer.Voices.pTrafficTraffic, VoicePlayer::PLAY_NOW);
    }

    // pick voice sample
    VoicePlayer::Voice* pVoice = NULL;
    switch(RA)
    {
        case AdvisoryClimb:
            if (revertedRA)
                pVoice = voicePlayer.Voices.pClimbNow;
            else
            if (AVAILABLE_RA(RAOption, OptionIncreaseClimb))
                pVoice = voicePlayer.Voices.pClimbIncrease;
            else
            if (AVAILABLE_RA(RAOption, OptionCrossingClimb))
                pVoice = voicePlayer.Voices.pClimbCrossing;
            else
                pVoice = voicePlayer.Voices.pClimb;
            break;

        case AdvisoryDescend:
            if (revertedRA)
                pVoice = voicePlayer.Voices.pDescendNow;
            else
            if (AVAILABLE_RA(RAOption, OptionIncreaseDescend))
                pVoice = voicePlayer.Voices.pDescendIncrease;
            else
            if (AVAILABLE_RA(RAOption, OptionCrossingDescent))
                pVoice = voicePlayer.Voices.pDescendCrossing;
            else
                pVoice = voicePlayer.Voices.pDescend;
            break;

        case AdvisoryAdjustVSpeed:
            pVoice = voicePlayer.Voices.pAdjustVSpeed;
            break;

        case AdvisoryMaintVSpeed:
            pVoice = voicePlayer.Voices.pMaintVSpeed;
            break;

        case AdvisoryMonitorVSpeed:
            pVoice = voicePlayer.Voices.pMonitorVSpeed;
            break;

        case AdvisoryLevelOff:
            pVoice = voicePlayer.Voices.pLevelOff;
            break;

        case AdvisoryIntrusion:
            break;

        default:
            RA = AdvisoryIntrusion;
            break;
    }

    previous = current;

    if ((pLastVoice == pVoice)&&
        (!tcas->tracker.newTraffic()))
    {
        // don't repeat annunciation
        return;
    }
    pLastVoice = pVoice;
    if (pVoice)
        voicePlayer.play(pVoice);

#ifdef FEATURE_TCAS_DEBUG_ANNUNCIATOR
    cout << "Annunciating TCAS RA " << RA << endl;
#endif
}

void
TCAS::Annunciator::test(bool testOk)
{
    if (testOk)
        voicePlayer.play(voicePlayer.Voices.pTestOk);
    else
        voicePlayer.play(voicePlayer.Voices.pTestFail);
}

/////////////////////////////////////////////////////////////////////////////
// TCAS::AdvisoryCoordinator ////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

TCAS::AdvisoryCoordinator::AdvisoryCoordinator(TCAS* _tcas) :
  tcas(_tcas),
  lastTATime(0)
{
    init();
}

void
TCAS::AdvisoryCoordinator::init(void)
{
    clear();
    previous = current;
}

void
TCAS::AdvisoryCoordinator::bind(SGPropertyNode* node)
{
    nodeTAWarning = node->getNode("outputs/traffic-alert", true);
    nodeTAWarning->setBoolValue(false);
}

void
TCAS::AdvisoryCoordinator::clear(void)
{
    current.threatLevel = ThreatNone;
    current.RA          = AdvisoryClear;
    current.RAOption    = OptionNone;
}

/** Add all suitable resolution advisories for a single threat. */
void
TCAS::AdvisoryCoordinator::add(const ResolutionAdvisory& newAdvisory)
{
    if ((newAdvisory.RA == AdvisoryClear)||
        (newAdvisory.threatLevel < current.threatLevel))
        return;

    if (current.threatLevel == newAdvisory.threatLevel)
    {
        // combine with other advisories so far
        current.RA &= newAdvisory.RA;
        // remember any advisory modifier
        current.RAOption |= newAdvisory.RAOption;
    }
    else
    {
        current = newAdvisory;
    }
}

/** Pick and trigger suitable resolution advisory. */
void
TCAS::AdvisoryCoordinator::update(int mode)
{
    bool revertedRA = false; // has advisory changed?
    double currentTime = globals->get_sim_time_sec();

    if (current.RA == AdvisoryClear)
    {
        // filter: wait 5 seconds after last TA/RA before announcing TA clearance
        if ((previous.RA != AdvisoryClear)&&
             (currentTime - lastTATime < 5.0))
            return;
    }
    else
    {
        // intruder detected

#ifdef FEATURE_TCAS_DEBUG_COORDINATOR
        cout << "TCAS::Annunciator::update: previous: " << previous.RA << ", new: " << current.RA << endl;
#endif

        lastTATime = currentTime;
        if ((previous.RA == AdvisoryClear)||
            (previous.RA == AdvisoryIntrusion)||
            ((current.RA & previous.RA) != previous.RA))
        {
            // no RA yet, or we can't keep previous RA: pick one - in order of priority

            if (AVAILABLE_RA(current.RA, AdvisoryMonitorVSpeed))
            {
                // prio 1: monitor vertical speed only
                current.RA = AdvisoryMonitorVSpeed;
            }
            else
            if (AVAILABLE_RA(current.RA, AdvisoryMaintVSpeed))
            {
                // prio 2: maintain vertical speed
                current.RA = AdvisoryMaintVSpeed;
            }
            else
            if (AVAILABLE_RA(current.RA, AdvisoryAdjustVSpeed))
            {
                // prio 3: adjust vertical speed (TCAS II 7.0 only)
                current.RA = AdvisoryAdjustVSpeed;
            }
            else
            if (AVAILABLE_RA(current.RA, AdvisoryLevelOff))
            {
                // prio 3: adjust vertical speed (TCAS II 7.1 only, [EUROACAS]: CP115)
                current.RA = AdvisoryLevelOff;
            }
            else
            if (AVAILABLE_RA(current.RA, AdvisoryClimb))
            {
                // prio 4: climb
                current.RA = AdvisoryClimb;
            }
            else
            if (AVAILABLE_RA(current.RA, AdvisoryDescend))
            {
                // prio 5: descend
                current.RA = AdvisoryDescend;
            }
            else
            {
                // no RA, issue a TA only
                current.RA = AdvisoryIntrusion;
            }

            // check if earlier advisory was reverted
            revertedRA = ((previous.RA != current.RA)&&
                          (previous.RA != 0)&&
                          (previous.RA != AdvisoryIntrusion));
        }
        else
        {
            // keep earlier RA
            current.RA = previous.RA;
        }
    }

    /* [TCASII]: "Aural annunciations are inhibited below 500+/-100 feet AGL." */
    if ((tcas->threatDetector.getRadarAlt() > 500)&&
        (mode >= SwitchTaOnly))
        tcas->annunciator.trigger(current, revertedRA);
    else
    if (current.RA == AdvisoryClear)
    {
        /* explicitly clear traffic alert (since aural annunciation disabled) */
        tcas->annunciator.clear();
    }

    previous = current;
    
    /* [TCASII] "[..] also performs the function of setting flags that control the displays.
     *   The traffic display, the RA display, [..] use these flags to alert the pilot to
     *   the presence of TAs and RAs." */
    nodeTAWarning->setBoolValue(current.RA != AdvisoryClear);
}

///////////////////////////////////////////////////////////////////////////////
// TCAS::ThreatDetector ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TCAS::ThreatDetector::ThreatDetector(TCAS* _tcas) :
    tcas(_tcas),
    checkCount(0),
    pAlarmThresholds(&sensitivityLevels[0])
{
    self.radarAltFt = 0.0;
    unitTest();
}

void
TCAS::ThreatDetector::init(void)
{
    nodeLat         = fgGetNode("/position/latitude-deg",          true);
    nodeLon         = fgGetNode("/position/longitude-deg",         true);
    nodePressureAlt = fgGetNode("/position/altitude-ft",           true);
    nodeRadarAlt    = fgGetNode("/position/altitude-agl-ft",       true);
    nodeHeading     = fgGetNode("/orientation/heading-deg",        true);
    nodeVelocity    = fgGetNode("/velocities/airspeed-kt",         true);
    nodeVerticalFps = fgGetNode("/velocities/vertical-speed-fps",  true);
    
    tcas->advisoryGenerator.init(&self,&currentThreat);
}

/** Update local position and threat sensitivity levels. */
void
TCAS::ThreatDetector::update(void)
{
    // update local position
    self.lat           = nodeLat->getDoubleValue();
    self.lon           = nodeLon->getDoubleValue();
    self.pressureAltFt = nodePressureAlt->getDoubleValue();
    self.heading       = nodeHeading->getDoubleValue();
    self.velocityKt    = nodeVelocity->getDoubleValue();
    self.verticalFps   = nodeVerticalFps->getDoubleValue();

    /* radar altimeter provides a lot of spikes due to uneven terrain
     * MK-VIII GPWS-spec requires smoothing the radar altitude with a
     * 10second moving average. Likely the TCAS spec requires the same.
     * => We use a cheap 10 second exponential average method.
     */
    const double SmoothingFactor = 0.3;
    self.radarAltFt = nodeRadarAlt->getDoubleValue()*SmoothingFactor +
            (1-SmoothingFactor)*self.radarAltFt;

#ifdef FEATURE_TCAS_DEBUG_THREAT_DETECTOR
    printf("TCAS::ThreatDetector::update: radarAlt = %f\n",self.radarAltFt);
    checkCount = 0;
#endif

    // determine current altitude's "Sensitivity Level Definition and Alarm Thresholds" 
    int sl=0;
    for (sl=0;((self.radarAltFt > sensitivityLevels[sl].maxAltitude)&&
               (sensitivityLevels[sl].maxAltitude));sl++);
    pAlarmThresholds = &sensitivityLevels[sl];
    tcas->advisoryGenerator.setAlarmThresholds(pAlarmThresholds);
}

/** Check if plane's transponder is enabled. */
bool
TCAS::ThreatDetector::checkTransponder(const SGPropertyNode* pModel, float velocityKt)
{
    const string name = pModel->getName();
    if (name != "multiplayer" && name != "aircraft")
    {
        // assume non-MP/non-AI planes (e.g. ships) have no transponder
        return false;
    }

    if (velocityKt < 40)
    {
        /* assume all pilots have their transponder switched off while taxiing/parking
         * (at low speed) */
        return false;
    }

    if ((name == "multiplayer")&&
        (pModel->getBoolValue("controls/invisible")))
    {
        // ignored MP plane: pretend transponder is switched off
        return false;
    }

    return true;
}

/** Check if plane is a threat. */
int
TCAS::ThreatDetector::checkThreat(int mode, const SGPropertyNode* pModel)
{
#ifdef FEATURE_TCAS_DEBUG_THREAT_DETECTOR
    checkCount++;
#endif
    
    float velocityKt  = pModel->getDoubleValue("velocities/true-airspeed-kt");

    if (!checkTransponder(pModel, velocityKt))
        return ThreatInvisible;

    int threatLevel = ThreatNone;
    float altFt = pModel->getDoubleValue("position/altitude-ft");
    currentThreat.relativeAltitudeFt = altFt - self.pressureAltFt;

    // save computation time: don't care when relative altitude is excessive
    if (fabs(currentThreat.relativeAltitudeFt) > 10000)
        return threatLevel;

    // position data of current intruder
    double lat        = pModel->getDoubleValue("position/latitude-deg");
    double lon        = pModel->getDoubleValue("position/longitude-deg");
    float heading     = pModel->getDoubleValue("orientation/true-heading-deg");

    double distanceNm, bearing;
    calcRangeBearing(self.lat, self.lon, lat, lon, distanceNm, bearing);

    // save computation time: don't care for excessive distances (also captures NaNs...)
    if ((distanceNm > 10)||(distanceNm < 0))
        return threatLevel;

    currentThreat.verticalFps = pModel->getDoubleValue("velocities/vertical-speed-fps");
    
    /* Detect proximity targets
     * [TCASII]: "Any target that is less than 6 nmi in range and within +/-1200ft
     *  vertically, but that does not meet the intruder or threat criteria." */
    if ((distanceNm < 6)&&
        (fabs(currentThreat.relativeAltitudeFt) < 1200))
    {
        // at least a proximity target
        threatLevel = ThreatProximity;
    }

    /* do not detect any threats when in standby or on ground and taxiing */
    if ((mode <= SwitchStandby)||
        ((self.radarAltFt < 360)&&(self.velocityKt < 40)))
    {
        return threatLevel;
    }

    if (tcas->tracker.active())
    {
        currentThreat.callsign = pModel->getStringValue("callsign");
        currentThreat.isTracked = tcas->tracker.isTracked(currentThreat.callsign);
    }
    else
        currentThreat.isTracked = false;
    
    // first stage: vertical movement
    checkVerticalThreat();

    // stop processing when no vertical threat
    if ((!currentThreat.verticalTA)&&
        (!currentThreat.isTracked))
        return threatLevel;

    // second stage: horizontal movement
    horizontalThreat(bearing, distanceNm, heading, velocityKt);

    if (!currentThreat.isTracked)
    {
        // no horizontal threat?
        if (!currentThreat.horizontalTA)
            return threatLevel;

        if ((currentThreat.horizontalTau < 0)||
            (currentThreat.verticalTau < 0))
        {
            // do not trigger new alerts when Tau is negative, but keep existing alerts
            int previousThreatLevel = pModel->getIntValue("tcas/threat-level", 0);
            if (previousThreatLevel == 0)
                return threatLevel;
        }
    }

#ifdef FEATURE_TCAS_DEBUG_THREAT_DETECTOR
    cout << "#" << checkCount << ": " << pModel->getStringValue("callsign") << endl;
#endif

    
    /* [TCASII]: "For either a TA or an RA to be issued, both the range and 
     *    vertical criteria, in terms of tau or the fixed thresholds, must be
     *    satisfied only one of the criteria is satisfied, TCAS will not issue
     *    an advisory." */
    if (currentThreat.horizontalTA && currentThreat.verticalTA)
        threatLevel = ThreatTA;
    if (currentThreat.horizontalRA && currentThreat.verticalRA)
        threatLevel = ThreatRA;

    if (!tcas->tracker.active())
        currentThreat.callsign = pModel->getStringValue("callsign");

    tcas->tracker.add(currentThreat.callsign, threatLevel);
    
    // check existing threat level
    if (currentThreat.isTracked)
    {
        int oldLevel = tcas->tracker.getThreatLevel(currentThreat.callsign);
        if (oldLevel > threatLevel)
            threatLevel = oldLevel;
    }

    // find all resolution options for this conflict
    threatLevel = tcas->advisoryGenerator.resolution(mode, threatLevel, distanceNm, altFt, heading, velocityKt);
   
    
#ifdef FEATURE_TCAS_DEBUG_THREAT_DETECTOR
    printf("  threat: distance: %4.1f, bearing: %4.1f, alt: %5.1f, velocity: %4.1f, heading: %4.1f, vspeed: %4.1f, "
           "own alt: %5.1f, own heading: %4.1f, own velocity: %4.1f, vertical tau: %3.2f"
           //", closing speed: %f"
           "\n",
           distanceNm, relAngle(bearing, self.heading), altFt, velocityKt, heading, currentThreat.verticalFps,
           self.altFt, self.heading, self.velocityKt
           //, currentThreat.closingSpeedKt
           ,currentThreat.verticalTau
           );
#endif

    return threatLevel;
}

/** Check if plane is a vertical threat. */
void
TCAS::ThreatDetector::checkVerticalThreat(void)
{
    // calculate relative vertical speed and altitude
    float dV = self.verticalFps - currentThreat.verticalFps;
    float dA = currentThreat.relativeAltitudeFt;

    currentThreat.verticalTA  = false;
    currentThreat.verticalRA  = false;
    currentThreat.verticalTau = 0;

    /* [TCASII]: "The vertical tau is equal to the altitude separation (feet)
     *   divided by the combined vertical speed of the two aircraft (feet/minute)
     *   times 60." */
    float tau = 0;
    if (fabs(dV) > 0.1)
        tau = dA/dV;

    /* [TCASII]: "When the combined vertical speed of the TCAS and the intruder aircraft
     *    is low, TCAS will use a fixed-altitude threshold to determine whether a TA or
     *    an RA should be issued." */
    if ((fabs(dV) < 3.0)||
        ((tau < 0) && (tau > -5)))
    {
        /* vertical closing speed is low (below 180fpm/3fps), check
         * fixed altitude range. */
        float abs_dA = fabs(dA);
        if (abs_dA < pAlarmThresholds->RA.ALIM)
        {
            // continuous intrusion at RA-level
            currentThreat.verticalTA = true;
            currentThreat.verticalRA = true;
        }
        else
        if (abs_dA < pAlarmThresholds->TA.ALIM)
        {
            // continuous intrusion: with TA-level, but no RA-threat
            currentThreat.verticalTA = true;
        }
        // else: no RA/TA threat
    }
    else
    {
        if ((tau < pAlarmThresholds->TA.Tau)&&
            (tau >= -5))
        {
            currentThreat.verticalTA = true;
            currentThreat.verticalRA = (tau < pAlarmThresholds->RA.Tau);
        }
    }
    currentThreat.verticalTau = tau;

#ifdef FEATURE_TCAS_DEBUG_THREAT_DETECTOR
    if (currentThreat.verticalTA)
        printf("  vertical dV=%f (%f-%f), dA=%f\n", dV, self.verticalFps, currentThreat.verticalFps, dA);
#endif
}

/** Check if plane is a horizontal threat. */
void
TCAS::ThreatDetector::horizontalThreat(float bearing, float distanceNm, float heading, float velocityKt)
{
    // calculate speed
    float vxKt = sin(heading*SGD_DEGREES_TO_RADIANS)*velocityKt - sin(self.heading*SGD_DEGREES_TO_RADIANS)*self.velocityKt;
    float vyKt = cos(heading*SGD_DEGREES_TO_RADIANS)*velocityKt - cos(self.heading*SGD_DEGREES_TO_RADIANS)*self.velocityKt;

    // calculate horizontal closing speed
    float closingSpeedKt2 = vxKt*vxKt+vyKt*vyKt; 
    float closingSpeedKt  = sqrt(closingSpeedKt2);

    /* [TCASII]: "The range tau is equal to the slant range (nmi) divided by the closing speed
     *    (knots) multiplied by 3600."
     * => calculate allowed slant range (nmi) based on known maximum tau */
    float TA_rangeNm = (pAlarmThresholds->TA.Tau*closingSpeedKt)/3600;
    float RA_rangeNm = (pAlarmThresholds->RA.Tau*closingSpeedKt)/3600;

    if (closingSpeedKt < 100)
    {
        /* [TCASII]: "In events where the rate of closure is very low, [..]
         *    an intruder aircraft can come very close in range without crossing the
         *    range tau boundaries [..]. To provide protection in these types of
         *    advisories, the range tau boundaries are modified [..] to use
         *    a fixed-range threshold to issue TAs and RAs in these slow closure
         *    encounters." */
        TA_rangeNm += (100.0-closingSpeedKt)*(pAlarmThresholds->TA.DMOD/100.0);
        RA_rangeNm += (100.0-closingSpeedKt)*(pAlarmThresholds->RA.DMOD/100.0);
    }
    if (TA_rangeNm < pAlarmThresholds->TA.DMOD)
        TA_rangeNm = pAlarmThresholds->TA.DMOD;
    if (RA_rangeNm < pAlarmThresholds->RA.DMOD)
        RA_rangeNm = pAlarmThresholds->RA.DMOD;

    currentThreat.horizontalTA   = (distanceNm < TA_rangeNm);
    currentThreat.horizontalRA   = (distanceNm < RA_rangeNm);
    currentThreat.horizontalTau  = -1;
    
    if ((currentThreat.horizontalRA)&&
        (currentThreat.verticalRA))
    {
        /* an RA will be issued. Prepare extra data for the
         * traffic resolution stage, i.e. calculate
         * exact time tau to horizontal CPA.
         */

        /* relative position of intruder is
         *   Sx(t) = sx + vx*t
         *   Sy(t) = sy + vy*t
         * horizontal distance to intruder is r(t)
         *   r(t) = sqrt( Sx(t)^2 + Sy(t)^2 )
         * => horizontal CPA at time t=tau, where r(t) has minimum
         * r2(t) := r^2(t) = Sx(t)^2 + Sy(t)^2
         * since r(t)>0 for all t => minimum of r(t) is also minimum of r2(t)
         * => (d/dt) r2(t) = r2'(t) is 0 for t=tau
         *    r2(t) = ((Sx(t)^2 + Sy(t))^2) = c + b*t + a*t^2
         * => r2'(t) = b + a*2*t
         * at t=tau:
         *    r2'(tau) = 0 = b + 2*a*tau
         * => tau = -b/(2*a) 
         */
        float sx = sin(bearing*SGD_DEGREES_TO_RADIANS)*distanceNm;
        float sy = cos(bearing*SGD_DEGREES_TO_RADIANS)*distanceNm;
        float vx = vxKt * (SG_KT_TO_MPS*SG_METER_TO_NM);
        float vy = vyKt * (SG_KT_TO_MPS*SG_METER_TO_NM);
        float a  = vx*vx + vy*vy;
        float b  = 2*(sx*vx + sy*vy);
        float tau = 0;
        if (a > 0.0001)
            tau = -b/(2*a);
#ifdef FEATURE_TCAS_DEBUG_THREAT_DETECTOR
        printf("  Time to horizontal CPA: %4.2f\n",tau);
#endif
        if (tau > pAlarmThresholds->RA.Tau)
            tau = pAlarmThresholds->RA.Tau;
        
        // remember time to horizontal CPA
        currentThreat.horizontalTau = tau;
    }
}

/** Test threat detection logic. */
void
TCAS::ThreatDetector::unitTest(void)
{
    pAlarmThresholds = &sensitivityLevels[1];
#if 0
    // vertical tests
    self.verticalFps = 0;
    self.altFt = 1000;
    cout << "identical altitude and vspeed " << endl;
    checkVerticalThreat(self.altFt, self.verticalFps);
    cout << "1000ft alt offset, dV=100 " << endl;
    checkVerticalThreat(self.altFt+1000, 100);
    cout << "-1000ft alt offset, dV=100 " << endl;
    checkVerticalThreat(self.altFt-1000, 100);
    cout << "3000ft alt offset, dV=10 " << endl;
    checkVerticalThreat(self.altFt+3000, 10);
    cout << "500ft alt offset, dV=100 " << endl;
    checkVerticalThreat(self.altFt+500, 100);
    cout << "500ft alt offset, dV=-100 " << endl;
    checkVerticalThreat(self.altFt+500, -100);

    // horizontal tests
    self.heading = 0;
    self.velocityKt = 0;
    cout << "10nm behind, overtaking with 1Nm/s" << endl;
    horizontalThreat(-180, 10, 0, 1/(SG_KT_TO_MPS*SG_METER_TO_NM));

    cout << "10nm ahead, departing with 1Nm/s" << endl;
    horizontalThreat(0, 20, 0, 1/(SG_KT_TO_MPS*SG_METER_TO_NM));

    self.heading = 90;
    self.velocityKt = 1/(SG_KT_TO_MPS*SG_METER_TO_NM);
    cout << "10nm behind, overtaking with 1Nm/s at 90 degrees" << endl;
    horizontalThreat(-90, 20, 90, 2/(SG_KT_TO_MPS*SG_METER_TO_NM));

    self.heading = 20;
    self.velocityKt = 1/(SG_KT_TO_MPS*SG_METER_TO_NM);
    cout << "10nm behind, overtaking with 1Nm/s at 20 degrees" << endl;
    horizontalThreat(200, 20, 20, 2/(SG_KT_TO_MPS*SG_METER_TO_NM));
#endif
}

///////////////////////////////////////////////////////////////////////////////
// TCAS::AdvisoryGenerator ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TCAS::AdvisoryGenerator::AdvisoryGenerator(TCAS* _tcas) :
    tcas(_tcas),
    pSelf(NULL),
    pCurrentThreat(NULL),
    pAlarmThresholds(NULL)
{
}

void
TCAS::AdvisoryGenerator::init(const LocalInfo* _pSelf, ThreatInfo* _pCurrentThreat)
{
    pCurrentThreat = _pCurrentThreat;
    pSelf = _pSelf;
}

void
TCAS::AdvisoryGenerator::setAlarmThresholds(const SensitivityLevel* _pAlarmThresholds)
{
    pAlarmThresholds = _pAlarmThresholds;
}

/** Calculate projected vertical separation at horizontal CPA. */
float
TCAS::AdvisoryGenerator::verticalSeparation(float newVerticalFps)
{
    // calculate relative vertical speed and altitude
    float dV = pCurrentThreat->verticalFps - newVerticalFps;
    float tau = pCurrentThreat->horizontalTau;
    // don't use negative tau to project future separation...
    if (tau < 0.5)
        tau = 0.5;
    return pCurrentThreat->relativeAltitudeFt + tau * dV;
}

/** Determine RA sense. */
void
TCAS::AdvisoryGenerator::determineRAsense(int& RASense, bool& isCrossing)
{
    /* [TCASII]: "[..] a two step process is used to select the appropriate RA for the encounter
             *   geometry. The first step in the process is to select the RA sense, i.e., upward or downward." */
    RASense = 0;
    isCrossing = false;
    
    /* [TCASII]: "Based on the range and altitude tracks of the intruder, the CAS logic models the
     *   intruder's flight path from its present position to CPA. The CAS logic then models upward
     *   and downward sense RAs for own aircraft [..] to determine which sense provides the most
     *   vertical separation at CPA." */
    float upSenseRelAltFt   = verticalSeparation(+2000/60.0);
    float downSenseRelAltFt = verticalSeparation(-2000/60.0);
    if (fabs(upSenseRelAltFt) >= fabs(downSenseRelAltFt))
        RASense = +1;  // upward
    else
        RASense = -1; // downward

    /* [TCASII]: "In encounters where either of the senses results in the TCAS aircraft crossing through
     *   the intruder's altitude, TCAS is designed to select the nonaltitude crossing sense if the
     *   noncrossing sense provides the desired vertical separation, known as ALIM, at CPA." */
    /* [TCASII]: "If ALIM cannot be obtained in the nonaltitude crossing sense, an altitude
     *   crossing RA will be issued." */
    if ((RASense > 0)&&
        (pCurrentThreat->relativeAltitudeFt > 200))
    {
        // threat is above and RA is crossing
        if (fabs(downSenseRelAltFt) > pAlarmThresholds->TA.ALIM)
        {
            // non-crossing descend is sufficient 
            RASense = -1;
        }
        else
        {
            // keep crossing climb RA
            isCrossing = true;
        }
    }
    else
    if ((RASense < 0)&&
        (pCurrentThreat->relativeAltitudeFt < -200))
    {
        // threat is below and RA is crossing
        if (fabs(upSenseRelAltFt) > pAlarmThresholds->TA.ALIM)
        {
            // non-crossing climb is sufficient 
            RASense = 1;
        }
        else
        {
            // keep crossing descent RA
            isCrossing = true;
        }
    }
    // else: threat is at same altitude, keep optimal RA sense (non-crossing)

    pCurrentThreat->RASense = RASense;

#ifdef FEATURE_TCAS_DEBUG_ADV_GENERATOR
        printf("  RASense: %i, crossing: %u, relAlt: %4.1f, upward separation: %4.1f, downward separation: %4.1f\n",
               RASense,isCrossing,
               pCurrentThreat->relativeAltitudeFt,
               upSenseRelAltFt,downSenseRelAltFt);
#endif
}

/** Determine suitable resolution advisories. */
int
TCAS::AdvisoryGenerator::resolution(int mode, int threatLevel, float rangeNm, float altFt,
                                    float heading, float velocityKt)
{
    int RAOption = OptionNone;
    int RA       = AdvisoryIntrusion;

    // RAs are disabled under certain conditions
    if (threatLevel == ThreatRA)
    {
        /* [TCASII]: "... less than 360 feet, TCAS considers the reporting aircraft
         *   to be on the ground. If TCAS determines the intruder to be on the ground, it
         *   inhibits the generation of advisories against this aircraft."*/
        if (altFt < 360)
            threatLevel = ThreatTA;

        /* [EUROACAS]: "Certain RAs are inhibited at altitudes based on inputs from the radio altimeter:
         *   [..] (c)1000ft (+/- 100ft) and below, all RAs are inhibited;" */
        if (pSelf->radarAltFt < 1000)
            threatLevel = ThreatTA;
        
        // RAs only issued in mode "Auto" (= "TA/RA" mode)
        if (mode != SwitchAuto)
            threatLevel = ThreatTA;
    }

    bool isCrossing = false; 
    int RASense = 0;
    // determine suitable RAs
    if (threatLevel == ThreatRA)
    {
        /* [TCASII]: "[..] a two step process is used to select the appropriate RA for the encounter
         *   geometry. The first step in the process is to select the RA sense, i.e., upward or downward." */
        determineRAsense(RASense, isCrossing);

        /* second step: determine required strength */
        if (RASense > 0)
        {
            // upward

            if ((pSelf->verticalFps < -1000/60.0)&&
                (!isCrossing))
            {
                // currently descending, see if reducing current descent is sufficient 
                float relAltFt = verticalSeparation(-500/60.0);
                if (relAltFt > pAlarmThresholds->TA.ALIM)
                    RA |= AdvisoryAdjustVSpeed;
            }
            RA |= AdvisoryClimb;
            if (isCrossing)
                RAOption |= OptionCrossingClimb;
        }

        if (RASense < 0)
        {
            // downward

            if ((pSelf->verticalFps > 1000/60.0)&&
                (!isCrossing))
            {
                // currently climbing, see if reducing current climb is sufficient 
                float relAltFt = verticalSeparation(500/60.0);
                if (relAltFt < -pAlarmThresholds->TA.ALIM)
                    RA |= AdvisoryAdjustVSpeed;
            }
            RA |= AdvisoryDescend;
            if (isCrossing)
                RAOption |= OptionCrossingDescent;
        }

        //TODO
        /* [TCASII]: "When two TCAS-equipped aircraft are converging vertically with opposite rates
         *   and are currently well separated in altitude, TCAS will first issue a vertical speed
         *   limit (Negative) RA to reinforce the pilots' likely intention to level off at adjacent
         *   flight levels." */
        
        //TODO
        /* [TCASII]: "[..] if the CAS logic determines that the response to a Positive RA has provided
         *   ALIM feet of vertical separation before CPA, the initial RA will be weakened to either a
         *   Do Not Descend RA (after an initial Climb RA) or a Do Not Climb RA (after an initial
         *   Descend RA)." */
        
        //TODO
        /* [TCASII]: "TCAS is designed to inhibit Increase Descent RAs below 1450 feet AGL; */
        
        /* [TCASII]: "Descend RAs below 1100 feet AGL;" (inhibited) */
        if (pSelf->radarAltFt < 1100)
        {
            RA &= ~AdvisoryDescend;
            //TODO Support "Do not descend" RA
            RA |= AdvisoryIntrusion;
        }
    }

#ifdef FEATURE_TCAS_DEBUG_ADV_GENERATOR
    cout << "  resolution advisory: " << RA << endl;
#endif

    ResolutionAdvisory newAdvisory;
    newAdvisory.RAOption    = RAOption;
    newAdvisory.RA          = RA;
    newAdvisory.threatLevel = threatLevel;
    tcas->advisoryCoordinator.add(newAdvisory);
    
    return threatLevel;
}

///////////////////////////////////////////////////////////////////////////////
// TCAS ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TCAS::TCAS(SGPropertyNode* pNode) :
    name("tcas"),
    num(0),
    nextUpdateTime(0),
    selfTestStep(0),
    properties_handler(this),
    threatDetector(this),
    tracker(this),
    advisoryCoordinator(this),
    advisoryGenerator(this),
    annunciator(this)
{
    for (int i = 0; i < pNode->nChildren(); ++i)
    {
        SGPropertyNode* pChild = pNode->getChild(i);
        string cname = pChild->getName();
        string cval = pChild->getStringValue();

        if (cname == "name")
            name = cval;
        else if (cname == "number")
            num = pChild->getIntValue();
        else
        {
            SG_LOG(SG_INSTR, SG_WARN, "Error in TCAS config logic");
            if (name.length())
                SG_LOG(SG_INSTR, SG_WARN, "Section = " << name);
        }
    }
}

void
TCAS::init(void)
{
    annunciator.init();
    advisoryCoordinator.init();
    threatDetector.init();
}

void
TCAS::bind(void)
{
    SGPropertyNode* node = fgGetNode(("/instrumentation/" + name).c_str(), num, true);

    nodeServiceable  = node->getNode("serviceable", true);

    // TCAS mode selection (0=off, 1=standby, 2=TA only, 3=auto(TA/RA) ) 
    nodeModeSwitch   = node->getNode("inputs/mode", true);
    // self-test button
    nodeSelfTest     = node->getNode("inputs/self-test", true);
    // default value
    nodeSelfTest->setBoolValue(false);

#ifdef FEATURE_TCAS_DEBUG_PROPERTIES
    SGPropertyNode* nodeDebug = node->getNode("debug", true);
    // debug triggers
    nodeDebugTrigger = nodeDebug->getNode("threat-trigger", true);
    nodeDebugRA      = nodeDebug->getNode("threat-RA",      true);
    nodeDebugThreat  = nodeDebug->getNode("threat-level",   true);
    // default values
    nodeDebugTrigger->setBoolValue(false);
    nodeDebugRA->setIntValue(3);
    nodeDebugThreat->setIntValue(1);
#endif

    annunciator.bind(node);
    advisoryCoordinator.bind(node);
}

void
TCAS::unbind(void)
{
    properties_handler.unbind();
}

/** Monitor traffic for safety threats. */
void
TCAS::update(double dt)
{
    if (!nodeServiceable->getBoolValue())
        return;
    int mode = nodeModeSwitch->getIntValue();
    if (mode == SwitchOff)
        return;

    nextUpdateTime -= dt;
    if (nextUpdateTime <= 0.0 )
    {
        nextUpdateTime = 1.0;

        // remove obsolete targets
        tracker.update();

        // get aircrafts current position/speed/heading
        threatDetector.update();

        // clear old threats
        advisoryCoordinator.clear();

        if (nodeSelfTest->getBoolValue())
        {
            if (threatDetector.getVelocityKt() >= 40)
            {
                // disable self-test when plane moves above taxiing speed
                nodeSelfTest->setBoolValue(false);
                selfTestStep = 0;
            }
            else
            {
                selfTest();
                // speed-up self test
                nextUpdateTime = 0;
                // no further TCAS processing during self-test
                return;
            }
        }

#ifdef FEATURE_TCAS_DEBUG_PROPERTIES
        if (nodeDebugTrigger->getBoolValue())
        {
            // debugging test
            ResolutionAdvisory debugAdvisory;
            debugAdvisory.RAOption    = OptionNone;
            debugAdvisory.RA          = nodeDebugRA->getIntValue();
            debugAdvisory.threatLevel = nodeDebugThreat->getIntValue();
            advisoryCoordinator.add(debugAdvisory);
        }
        else
#endif
        {
            SGPropertyNode* pAi = fgGetNode("/ai/models", true);

            // check all aircraft
            for (int i = pAi->nChildren() - 1; i >= -1; i--)
            {
                SGPropertyNode* pModel = pAi->getChild(i);
                if ((pModel)&&(pModel->nChildren()))
                {
                    int threatLevel = threatDetector.checkThreat(mode, pModel);
                    /* expose aircraft threat-level (to be used by other instruments,
                     * i.e. TCAS display) */
                    if (threatLevel==ThreatRA)
                        pModel->setIntValue("tcas/ra-sense", -threatDetector.getRASense());
                    pModel->setIntValue("tcas/threat-level", threatLevel);
                }
            }
        }
        advisoryCoordinator.update(mode);
    }
    annunciator.update();
}

/** Run a single self-test iteration. */
void
TCAS::selfTest(void)
{
    annunciator.update();
    if (annunciator.isPlaying())
    {
        return;
    }

    ResolutionAdvisory newAdvisory;
    newAdvisory.threatLevel = ThreatRA;
    newAdvisory.RA          = AdvisoryClear;
    newAdvisory.RAOption    = OptionNone;
    // TCAS audio is disabled below 500ft AGL
    threatDetector.setRadarAlt(501);

    // trigger various advisories
    switch(selfTestStep)
    {
        case 0:
            newAdvisory.RA = AdvisoryIntrusion;
            newAdvisory.threatLevel = ThreatTA;
            break;
        case 1:
            newAdvisory.RA = AdvisoryClimb;
            break;
        case 2:
            newAdvisory.RA = AdvisoryClimb;
            newAdvisory.RAOption = OptionIncreaseClimb;
            break;
        case 3:
            newAdvisory.RA = AdvisoryClimb;
            newAdvisory.RAOption = OptionCrossingClimb;
            break;
        case 4:
            newAdvisory.RA = AdvisoryDescend;
            break;
        case 5:
            newAdvisory.RA = AdvisoryDescend;
            newAdvisory.RAOption = OptionIncreaseDescend;
            break;
        case 6:
            newAdvisory.RA = AdvisoryDescend;
            newAdvisory.RAOption = OptionCrossingDescent;
            break;
        case 7:
            newAdvisory.RA = AdvisoryAdjustVSpeed;
            break;
        case 8:
            newAdvisory.RA = AdvisoryMaintVSpeed;
            break;
        case 9:
            newAdvisory.RA = AdvisoryMonitorVSpeed;
            break;
        case 10:
            newAdvisory.threatLevel = ThreatNone;
            newAdvisory.RA = AdvisoryClear;
            break;
        case 11:
            annunciator.test(true);
            selfTestStep+=2;
            return;
        default:
            nodeSelfTest->setBoolValue(false);
            selfTestStep = 0;
            return;
    }

    advisoryCoordinator.add(newAdvisory);
    advisoryCoordinator.update(SwitchAuto);

    selfTestStep++;
}

///////////////////////////////////////////////////////////////////////////////
// TCAS::Tracker //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TCAS::Tracker::Tracker(TCAS* _tcas) :
    tcas(_tcas),
    currentTime(0),
    haveTargets(false),
    newTargets(false)
{
    targets.clear();
}

void
TCAS::Tracker::update(void)
{
    currentTime = globals->get_sim_time_sec();
    newTargets = false;

    if (haveTargets)
    {
        // remove outdated targets
        TrackerTargets::iterator it = targets.begin();
        while (it != targets.end())
        {
            TrackerTarget* pTarget = it->second;
            if (currentTime - pTarget->TAtimestamp > 10.0)
            {
                TrackerTargets::iterator temp = it;
                ++it;
#ifdef FEATURE_TCAS_DEBUG_TRACKER
                printf("target %s no longer a TA threat.\n",temp->first.c_str());
#endif
                targets.erase(temp->first);
                delete pTarget;
                pTarget = NULL;
            }
            else
            {
                if ((pTarget->threatLevel == ThreatRA)&&
                    (currentTime - pTarget->RAtimestamp > 7.0))
                {
                    pTarget->threatLevel = ThreatTA;
#ifdef FEATURE_TCAS_DEBUG_TRACKER
                    printf("target %s no longer an RA threat.\n",it->first.c_str());
#endif
                }
                ++it;
            }
        }
        haveTargets = !targets.empty();
    }
}

void
TCAS::Tracker::add(const string callsign, int detectedLevel)
{
    TrackerTarget* pTarget = NULL;
    if (haveTargets)
    {
        TrackerTargets::iterator it = targets.find(callsign);
        if (it != targets.end())
        {
            pTarget = it->second;
        }
    }

    if (!pTarget)
    {
        pTarget = new TrackerTarget();
        pTarget->TAtimestamp = 0;
        pTarget->RAtimestamp = 0;
        pTarget->threatLevel = 0;
        newTargets = true;
        targets[callsign] = pTarget;
#ifdef FEATURE_TCAS_DEBUG_TRACKER
        printf("new target: %s, level: %i\n",callsign.c_str(),detectedLevel);
#endif
    }

    if (detectedLevel > pTarget->threatLevel)
        pTarget->threatLevel = detectedLevel;

    if (detectedLevel >= ThreatTA)
        pTarget->TAtimestamp = currentTime;

    if (detectedLevel >= ThreatRA)
        pTarget->RAtimestamp = currentTime;

    haveTargets = true;
}

bool
TCAS::Tracker::_isTracked(string callsign)
{
    return targets.find(callsign) != targets.end();
}

int
TCAS::Tracker::getThreatLevel(string callsign)
{
    TrackerTargets::iterator it = targets.find(callsign);
    if (it != targets.end())
        return it->second->threatLevel;
    else
        return 0;
}
