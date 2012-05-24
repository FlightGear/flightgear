// mk_viii.cxx -- Honeywell MK VIII EGPWS emulation
//
// Written by Jean-Yves Lefort, started September 2005.
//
// Copyright (C) 2005, 2006  Jean-Yves Lefort - jylefort@FreeBSD.org
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
//
// References:
//
//	[PILOT]		Honeywell International Inc., "MK VI and MK VIII,
//			Enhanced Ground Proximity Warning System (EGPWS),
//			Pilot's Guide", May 2004
//
//			http://www.egpws.com/engineering_support/documents/pilot_guides/060-4314-000.pdf
//
//	[SPEC]		Honeywell International Inc., "Product Specification
//			for the MK VI and MK VIII Enhanced Ground Proximity
//			Warning System (EGPWS)", June 2004
//
//			http://www.egpws.com/engineering_support/documents/specs/965-1180-601.pdf
//
//	[INSTALL]	Honeywell Inc., "MK VI, MK VIII, Enhanced Ground
//			Proximity Warning System (Class A TAWS), Installation
//			Design Guide", July 2003
//
//			http://www.egpws.com/engineering_support/documents/install/060-4314-150.pdf
//
// Notes:
//
//	[1]		[SPEC] does not specify the "must be airborne"
//			condition; we use it to make sure that the alert
//			will not trigger when on the ground, since it would
//			make no sense.

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
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/structure/exception.hxx>

using std::string;

#include <Airports/runways.hxx>
#include <Airports/simple.hxx>

#if defined( HAVE_VERSION_H ) && HAVE_VERSION_H
#  include <Include/version.h>
#else
#  include <Include/no_version.h>
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include "instrument_mgr.hxx"
#include "mk_viii.hxx"

///////////////////////////////////////////////////////////////////////////////
// constants //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GLIDESLOPE_DOTS_TO_DDM		0.0875		// [INSTALL] 6.2.30
#define GLIDESLOPE_DDM_TO_DOTS		(1 / GLIDESLOPE_DOTS_TO_DDM)

#define LOCALIZER_DOTS_TO_DDM		0.0775		// [INSTALL] 6.2.33
#define LOCALIZER_DDM_TO_DOTS		(1 / LOCALIZER_DOTS_TO_DDM)

///////////////////////////////////////////////////////////////////////////////
// helpers ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define assert_not_reached()	assert(false)
#define n_elements(_array)	(sizeof(_array) / sizeof((_array)[0]))
#define test_bits(_bits, _test)	(((_bits) & (_test)) != 0)

#define mk_node(_name)		(mk->properties_handler.external_properties._name)

#define mk_dinput_feed(_name)	(mk->io_handler.input_feeders.discretes._name)
#define mk_dinput(_name)	(mk->io_handler.inputs.discretes._name)
#define mk_ainput_feed(_name)	(mk->io_handler.input_feeders.arinc429._name)
#define mk_ainput(_name)	(mk->io_handler.inputs.arinc429._name)
#define mk_doutput(_name)	(mk->io_handler.outputs.discretes._name)
#define mk_aoutput(_name)	(mk->io_handler.outputs.arinc429._name)
#define mk_data(_name)		(mk->io_handler.data._name)

#define mk_voice(_name)		(mk->voice_player.voices._name)
#define mk_altitude_voice(_i)	(mk->voice_player.voices.altitude_callouts[(_i)])

#define mk_alert(_name)		(AlertHandler::ALERT_ ## _name)
#define mk_alert_flag(_name)	(AlertHandler::ALERT_FLAG_ ## _name)
#define mk_set_alerts		(mk->alert_handler.set_alerts)
#define mk_unset_alerts		(mk->alert_handler.unset_alerts)
#define mk_repeat_alert		(mk->alert_handler.repeat_alert)
#define mk_test_alert(_name)	(mk_test_alerts(mk_alert(_name)))
#define mk_test_alerts(_test)	(test_bits(mk->alert_handler.alerts, (_test)))

const double MK_VIII::TCFHandler::k = 0.25;

static double
modify_amplitude (double amplitude, double dB)
{
  return amplitude * pow(10.0, dB / 20.0);
}

static double
heading_add (double h1, double h2)
{
  double result = h1 + h2;
  if (result >= 360)
    result -= 360;
  return result;
}

static double
heading_substract (double h1, double h2)
{
  double result = h1 - h2;
  if (result < 0)
    result += 360;
  return result;
}

static double
get_heading_difference (double h1, double h2)
{
  double diff = h1 - h2;

  if (diff < -180)
    diff += 360;
  else if (diff > 180)
    diff -= 360;

  return fabs(diff);
}

static double
get_reciprocal_heading (double h)
{
  return heading_add(h, 180);
}

///////////////////////////////////////////////////////////////////////////////
// PropertiesHandler //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
MK_VIII::PropertiesHandler::init ()
{
  mk_node(ai_caged) = fgGetNode("/instrumentation/attitude-indicator/caged-flag", true);
  mk_node(ai_roll) = fgGetNode("/instrumentation/attitude-indicator/indicated-roll-deg", true);
  mk_node(ai_serviceable) = fgGetNode("/instrumentation/attitude-indicator/serviceable", true);
  mk_node(altimeter_altitude) = fgGetNode("/instrumentation/altimeter/indicated-altitude-ft", true);
  mk_node(altimeter_serviceable) = fgGetNode("/instrumentation/altimeter/serviceable", true);
  mk_node(altitude) = fgGetNode("/position/altitude-ft", true);
  mk_node(altitude_agl) = fgGetNode("/position/altitude-agl-ft", true);
  mk_node(altitude_gear_agl) = fgGetNode("/position/gear-agl-ft", true);
  mk_node(altitude_radar_agl) = fgGetNode("/instrumentation/radar-altimeter/radar-altitude-ft", true);
  mk_node(orientation_roll) = fgGetNode("/orientation/roll-deg", true);
  mk_node(asi_serviceable) = fgGetNode("/instrumentation/airspeed-indicator/serviceable", true);
  mk_node(asi_speed) = fgGetNode("/instrumentation/airspeed-indicator/indicated-speed-kt", true);
  mk_node(autopilot_heading_lock) = fgGetNode("/autopilot/locks/heading", true);
  mk_node(flaps) = fgGetNode("/controls/flight/flaps", true);
  mk_node(gear_down) = fgGetNode("/controls/gear/gear-down", true);
  mk_node(latitude) = fgGetNode("/position/latitude-deg", true);
  mk_node(longitude) = fgGetNode("/position/longitude-deg", true);
  mk_node(nav0_cdi_serviceable) = fgGetNode("/instrumentation/nav/cdi/serviceable", true);
  mk_node(nav0_gs_distance) = fgGetNode("/instrumentation/nav/gs-distance", true);
  mk_node(nav0_gs_needle_deflection) = fgGetNode("/instrumentation/nav/gs-needle-deflection", true);
  mk_node(nav0_gs_serviceable) = fgGetNode("/instrumentation/nav/gs/serviceable", true);
  mk_node(nav0_has_gs) = fgGetNode("/instrumentation/nav/has-gs", true);
  mk_node(nav0_heading_needle_deflection) = fgGetNode("/instrumentation/nav/heading-needle-deflection", true);
  mk_node(nav0_in_range) = fgGetNode("/instrumentation/nav/in-range", true);
  mk_node(nav0_nav_loc) = fgGetNode("/instrumentation/nav/nav-loc", true);
  mk_node(nav0_serviceable) = fgGetNode("/instrumentation/nav/serviceable", true);
  mk_node(power) = fgGetNode(("/systems/electrical/outputs/" + mk->name).c_str(), mk->num, true);
  mk_node(replay_state) = fgGetNode("/sim/freeze/replay-state", true);
  mk_node(vs) = fgGetNode("/velocities/vertical-speed-fps", true);
}

///////////////////////////////////////////////////////////////////////////////
// PowerHandler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
MK_VIII::PowerHandler::bind (SGPropertyNode *node)
{
  mk->properties_handler.tie(node, "serviceable", SGRawValuePointer<bool>(&serviceable));
}

bool
MK_VIII::PowerHandler::handle_abnormal_voltage (bool abnormal,
						Timer *timer,
						double max_duration)
{
  if (abnormal)
    {
      if (timer->start_or_elapsed() >= max_duration)
	return true;		// power loss
    }
  else
    timer->stop();

  return false;
}

void
MK_VIII::PowerHandler::update ()
{
  double voltage = mk_node(power)->getDoubleValue();
  bool now_powered = true;

  // [SPEC] 3.2.1

  if (! serviceable)
    now_powered = false;
  if (voltage < 15)
    now_powered = false;
  if (handle_abnormal_voltage(voltage < 20.5, &low_surge_timer, 0.03))
    now_powered = false;
  if (handle_abnormal_voltage(voltage < 22.0 || voltage > 30.3, &abnormal_timer, 300))
    now_powered = false;
  if (handle_abnormal_voltage(voltage > 32.2, &high_surge_timer, 1))
    now_powered = false;
  if (handle_abnormal_voltage(voltage > 37.8, &very_high_surge_timer, 0.1))
    now_powered = false;
  if (voltage > 46.3)
    now_powered = false;

  if (powered)
    {
      // [SPEC] 3.5.2

      if (now_powered)
	power_loss_timer.stop();
      else
	{
	  if (power_loss_timer.start_or_elapsed() >= 0.2)
	    power_off();
	}
    }
  else
    {
      if (now_powered)
	power_on();
    }
}

void
MK_VIII::PowerHandler::power_on ()
{
  powered = true;
  mk->system_handler.power_on();
}

void
MK_VIII::PowerHandler::power_off ()
{
  powered = false;
  mk->system_handler.power_off();
  mk->voice_player.stop(VoicePlayer::STOP_NOW);
  mk->self_test_handler.power_off(); // run before IOHandler::power_off()
  mk->io_handler.power_off();
  mk->mode2_handler.power_off();
  mk->mode6_handler.power_off();
}

///////////////////////////////////////////////////////////////////////////////
// SystemHandler //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
MK_VIII::SystemHandler::power_on ()
{
  state = STATE_BOOTING;

  // [SPEC] 3.5.2 mentions a 20 seconds maximum boot delay. We use a
  // random delay between 3 and 5 seconds.

  boot_delay = 3 + sg_random() * 2;
  boot_timer.start();
}

void
MK_VIII::SystemHandler::power_off ()
{
  state = STATE_OFF;

  boot_timer.stop();
}

void
MK_VIII::SystemHandler::update ()
{
  if (state == STATE_BOOTING)
    {
      if (boot_timer.elapsed() >= boot_delay)
	{
	  last_replay_state = mk_node(replay_state)->getIntValue();

	  mk->configuration_module.boot();

	  mk->io_handler.boot();
	  mk->fault_handler.boot();
	  mk->alert_handler.boot();

	  // inputs are needed by the following boot() routines
	  mk->io_handler.update_inputs();

	  mk->mode2_handler.boot();
	  mk->mode6_handler.boot();

	  state = STATE_ON;

	  mk->io_handler.post_boot();
	}
    }
  else if (state != STATE_OFF && mk->configuration_module.state == ConfigurationModule::STATE_OK)
    {
      // If the replay state changes, switch to reposition mode for 3
      // seconds ([SPEC] 6.0.5) to avoid spurious alerts.

      int replay_state = mk_node(replay_state)->getIntValue();
      if (replay_state != last_replay_state)
	{
	  mk->alert_handler.reposition();
	  mk->io_handler.reposition();

	  last_replay_state = replay_state;
	  state = STATE_REPOSITION;
	  reposition_timer.start();
	}

      if (state == STATE_REPOSITION && reposition_timer.elapsed() >= 3)
	{
	  // inputs are needed by StateHandler::post_reposition()
	  mk->io_handler.update_inputs();

	  mk->state_handler.post_reposition();

	  state = STATE_ON;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
// ConfigurationModule ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

MK_VIII::ConfigurationModule::ConfigurationModule (MK_VIII *device)
  : mk(device)
{
  // arbitrary defaults
  categories[CATEGORY_AIRCRAFT_MODE_TYPE_SELECT] = 0;
  categories[CATEGORY_AIR_DATA_INPUT_SELECT] = 1;
  categories[CATEGORY_POSITION_INPUT_SELECT] = 2;
  categories[CATEGORY_ALTITUDE_CALLOUTS] = 13;
  categories[CATEGORY_AUDIO_MENU_SELECT] = 0;
  categories[CATEGORY_TERRAIN_DISPLAY_SELECT] = 1;
  categories[CATEGORY_OPTIONS_SELECT_GROUP_1] = 124;
  categories[CATEGORY_RADIO_ALTITUDE_INPUT_SELECT] = 2;
  categories[CATEGORY_NAVIGATION_INPUT_SELECT] = 3;
  categories[CATEGORY_ATTITUDE_INPUT_SELECT] = 6;
  categories[CATEGORY_HEADING_INPUT_SELECT] = 2;
  categories[CATEGORY_WINDSHEAR_INPUT_SELECT] = 0;
  categories[CATEGORY_INPUT_OUTPUT_DISCRETE_TYPE_SELECT] = 7;
  categories[CATEGORY_AUDIO_OUTPUT_LEVEL] = 0;
  categories[CATEGORY_UNDEFINED_INPUT_SELECT_1] = 0;
  categories[CATEGORY_UNDEFINED_INPUT_SELECT_2] = 0;
  categories[CATEGORY_UNDEFINED_INPUT_SELECT_3] = 0;
}

static double m1_t1_min_agl1 (double vs) { return -1620 - 1.1133 * vs; }
static double m1_t1_min_agl2 (double vs) { return -400 - 0.4 * vs; }
static double m1_t4_min_agl1 (double vs) { return -1625.47 - 1.1167 * vs; }
static double m1_t4_min_agl2 (double vs) { return -0.1958 * vs; }

static int m3_t1_max_agl (bool flap_override) { return 1500; }
static double m3_t1_max_alt_loss (bool flap_override, double agl) { return 5.4 + 0.092 * agl; }
static int m3_t2_max_agl (bool flap_override) { return flap_override ? 815 : 925; }
static double m3_t2_max_alt_loss (bool flap_override, double agl)
{
  int bias = agl > 700 ? 5 : 0;

  if (flap_override)
    return (9.0 + 0.184 * agl) + bias;
  else
    return (5.4 + 0.092 * agl) + bias;
}

static double m4_t1_min_agl2 (double airspeed) { return -1083 + 8.333 * airspeed; }
static double m4_t568_a_min_agl2 (double airspeed) { return -1523 + 11.36 * airspeed; }
static double m4_t79_a_min_agl2 (double airspeed) { return -1182 + 11.36 * airspeed; }
static double m4_t568_b_min_agl2 (double airspeed) { return -1570 + 11.6 * airspeed; }
static double m4_t79_b_min_agl2 (double airspeed) { return -1222 + 11.6 * airspeed; }

bool
MK_VIII::ConfigurationModule::m6_t2_is_bank_angle (Parameter<double> *agl,
						   double abs_roll_deg,
						   bool ap_engaged)
{
  if (ap_engaged)
    {
      if (agl->ncd || agl->get() > 122)
	return abs_roll_deg > 33;
    }
  else
    {
      if (agl->ncd || agl->get() > 2450)
	return abs_roll_deg > 55;
      else if (agl->get() > 150)
	return agl->get() < 153.33333 * abs_roll_deg - 5983.3333;
    }

  if (agl->get() > 30)
    return agl->get() < 4 * abs_roll_deg - 10;
  else if (agl->get() > 5)
    return abs_roll_deg > 10;

  return false;
}

bool
MK_VIII::ConfigurationModule::m6_t4_is_bank_angle (Parameter<double> *agl,
						   double abs_roll_deg,
						   bool ap_engaged)
{
  if (ap_engaged)
    {
      if (agl->ncd || agl->get() > 156)
	return abs_roll_deg > 33;
    }
  else
    {
      if (agl->ncd || agl->get() > 210)
	return abs_roll_deg > 50;
    }

  if (agl->get() > 10)
    return agl->get() < 5.7142857 * abs_roll_deg - 75.714286;

  return false;
}

bool
MK_VIII::ConfigurationModule::read_aircraft_mode_type_select (int value)
{
  static const Mode1Handler::EnvelopesConfiguration m1_t1 =
    { false, 10, m1_t1_min_agl1, 284, m1_t1_min_agl2, 2450 };
  static const Mode1Handler::EnvelopesConfiguration m1_t4 =
    { true, 50, m1_t4_min_agl1, 346, m1_t4_min_agl2, 1958 };

  static const Mode2Handler::Configuration m2_t1 = { 190, 280 };
  static const Mode2Handler::Configuration m2_t4 = { 220, 310 };
  static const Mode2Handler::Configuration m2_t5 = { 220, 310 };

  static const Mode3Handler::Configuration m3_t1 =
    { 30, m3_t1_max_agl, m3_t1_max_alt_loss };
  static const Mode3Handler::Configuration m3_t2 = 
    { 50, m3_t2_max_agl, m3_t2_max_alt_loss };

  static const Mode4Handler::EnvelopesConfiguration m4_t1_ac =
    { 190, 250, 500, m4_t1_min_agl2, 1000 };
  static const Mode4Handler::EnvelopesConfiguration m4_t5_ac =
    { 178, 200, 500, m4_t568_a_min_agl2, 1000 };
  static const Mode4Handler::EnvelopesConfiguration m4_t68_ac =
    { 178, 200, 500, m4_t568_a_min_agl2, 750 };
  static const Mode4Handler::EnvelopesConfiguration m4_t79_ac =
    { 148, 170, 500, m4_t79_a_min_agl2, 750 };

  static const Mode4Handler::EnvelopesConfiguration m4_t1_b =
    { 159, 250, 245, m4_t1_min_agl2, 1000 };
  static const Mode4Handler::EnvelopesConfiguration m4_t5_b =
    { 148, 200, 200, m4_t568_b_min_agl2, 1000 };
  static const Mode4Handler::EnvelopesConfiguration m4_t6_b =
    { 150, 200, 170, m4_t568_b_min_agl2, 750 };
  static const Mode4Handler::EnvelopesConfiguration m4_t7_b =
    { 120, 170, 170, m4_t79_b_min_agl2, 750 };
  static const Mode4Handler::EnvelopesConfiguration m4_t8_b =
    { 148, 200, 150, m4_t568_b_min_agl2, 750 };
  static const Mode4Handler::EnvelopesConfiguration m4_t9_b =
    { 118, 170, 150, m4_t79_b_min_agl2, 750 };

  static const Mode4Handler::ModesConfiguration m4_t1 = { &m4_t1_ac, &m4_t1_b };
  static const Mode4Handler::ModesConfiguration m4_t5 = { &m4_t5_ac, &m4_t5_b };
  static const Mode4Handler::ModesConfiguration m4_t6 = { &m4_t68_ac, &m4_t6_b };
  static const Mode4Handler::ModesConfiguration m4_t7 = { &m4_t79_ac, &m4_t7_b };
  static const Mode4Handler::ModesConfiguration m4_t8 = { &m4_t68_ac, &m4_t8_b };
  static const Mode4Handler::ModesConfiguration m4_t9 = { &m4_t79_ac, &m4_t9_b };

  static Mode6Handler::BankAnglePredicate m6_t2 = m6_t2_is_bank_angle;
  static Mode6Handler::BankAnglePredicate m6_t4 = m6_t4_is_bank_angle;

  static const IOHandler::FaultsConfiguration f_slow = { 180, 200 };
  static const IOHandler::FaultsConfiguration f_fast = { 250, 290 };

  static const struct
  {
    int						type;
    const Mode1Handler::EnvelopesConfiguration	*m1;
    const Mode2Handler::Configuration		*m2;
    const Mode3Handler::Configuration		*m3;
    const Mode4Handler::ModesConfiguration	*m4;
    Mode6Handler::BankAnglePredicate		m6;
    const IOHandler::FaultsConfiguration	*f;
    int						runway_database;
  } aircraft_types[] = {
    { 0,	&m1_t4, &m2_t4, &m3_t2, &m4_t6, m6_t4, &f_fast, 2000 },
    { 1,	&m1_t4, &m2_t4, &m3_t2, &m4_t8, m6_t4, &f_fast, 2000 },
    { 2,	&m1_t4, &m2_t4, &m3_t2, &m4_t5, m6_t4, &f_fast, 2000 },
    { 3,	&m1_t4, &m2_t5, &m3_t2, &m4_t7, m6_t4, &f_slow, 2000 },
    { 4,	&m1_t4, &m2_t5, &m3_t2, &m4_t9, m6_t4, &f_slow, 2000 },
    { 254,	&m1_t1, &m2_t1, &m3_t1, &m4_t1, m6_t2, &f_fast, 3500 },
    { 255,	&m1_t1, &m2_t1, &m3_t1, &m4_t1, m6_t2, &f_fast, 2000 }
  };

  for (size_t i = 0; i < n_elements(aircraft_types); i++)
    if (aircraft_types[i].type == value)
      {
	mk->mode1_handler.conf.envelopes = aircraft_types[i].m1;
	mk->mode2_handler.conf = aircraft_types[i].m2;
	mk->mode3_handler.conf = aircraft_types[i].m3;
	mk->mode4_handler.conf.modes = aircraft_types[i].m4;
	mk->mode6_handler.conf.is_bank_angle = aircraft_types[i].m6;
	mk->io_handler.conf.faults = aircraft_types[i].f;
	mk->conf.runway_database = aircraft_types[i].runway_database;
	return true;
      }

  state = STATE_INVALID_AIRCRAFT_TYPE;
  return false;
}

bool
MK_VIII::ConfigurationModule::read_air_data_input_select (int value)
{
  // unimplemented
  return (value >= 0 && value <= 6) || (value >= 10 && value <= 13) || value == 255;
}

bool
MK_VIII::ConfigurationModule::read_position_input_select (int value)
{
  if (value == 2)
    mk->io_handler.conf.use_internal_gps = true;
  else if ((value >= 0 && value <= 5)
	   || (value >= 10 && value <= 13)
	   || (value == 253)
	   || (value == 255))
    mk->io_handler.conf.use_internal_gps = false;
  else
    return false;

  return true;
}

bool
MK_VIII::ConfigurationModule::read_altitude_callouts (int value)
{
  enum
  {
    MINIMUMS		= -1,
    SMART_500		= -2,
    FIELD_500		= -3,
    FIELD_500_ABOVE	= -4
  };

  static const struct
  {
    int	id;
    int callouts[13];
  } values[] = {
    { 0, { MINIMUMS, SMART_500, 200, 100, 50, 40, 30, 20, 10, 0 } },
    { 1, { MINIMUMS, SMART_500, 200, 0 } },
    { 2, { MINIMUMS, SMART_500, 100, 50, 40, 30, 20, 10, 0 } },
    { 3, { MINIMUMS, SMART_500, 0 } },
    { 4, { MINIMUMS, 200, 100, 50, 40, 30, 20, 10, 0 } },
    { 5, { MINIMUMS, 200, 0 } },
    { 6, { MINIMUMS, 100, 50, 40, 30, 20, 10, 0 } },
    { 7, { 0 } },
    { 8, { MINIMUMS, 0 } },
    { 9, { MINIMUMS, 500, 200, 100, 50, 40, 30, 20, 10, 0 } },
    { 10, { MINIMUMS, 500, 200, 0 } },
    { 11, { MINIMUMS, 500, 100, 50, 40, 30, 20, 10, 0 } },
    { 12, { MINIMUMS, 500, 0 } },
    { 13, { MINIMUMS, 1000, 500, 400, 300, 200, 100, 50, 40, 30, 20, 10, 0 } },
    { 14, { MINIMUMS, 100, 0 } },
    { 15, { MINIMUMS, 200, 100, 0 } },
    { 100, { FIELD_500, 0 } },
    { 101, { FIELD_500_ABOVE, 0 } }
  };

  unsigned i;

  mk->mode6_handler.conf.minimums_enabled = false;
  mk->mode6_handler.conf.smart_500_enabled = false;
  mk->mode6_handler.conf.above_field_voice = NULL;
  for (i = 0; i < n_altitude_callouts; i++)
    mk->mode6_handler.conf.altitude_callouts_enabled[i] = false;

  for (i = 0; i < n_elements(values); i++)
    if (values[i].id == value)
      {
	for (int j = 0; values[i].callouts[j] != 0; j++)
	  switch (values[i].callouts[j])
	    {
	    case MINIMUMS:
	      mk->mode6_handler.conf.minimums_enabled = true;
	      break;

	    case SMART_500:
	      mk->mode6_handler.conf.smart_500_enabled = true;
	      break;

	    case FIELD_500:
	      mk->mode6_handler.conf.above_field_voice = mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_500);
	      break;

	    case FIELD_500_ABOVE:
	      mk->mode6_handler.conf.above_field_voice = mk_voice(five_hundred_above);
	      break;

	    default:
	      for (unsigned k = 0; k < n_altitude_callouts; k++)
		if (mk->mode6_handler.altitude_callout_definitions[k] == values[i].callouts[j])
		  mk->mode6_handler.conf.altitude_callouts_enabled[k] = true;
	      break;
	    }

	return true;
      }

  return false;
}

bool
MK_VIII::ConfigurationModule::read_audio_menu_select (int value)
{
  if (value == 0 || value == 1)
    mk->mode4_handler.conf.voice_too_low_gear = mk_voice(too_low_gear);
  else if (value == 2 || value == 3)
    mk->mode4_handler.conf.voice_too_low_gear = mk_voice(too_low_flaps);
  else
    return false;

  return true;
}

bool
MK_VIII::ConfigurationModule::read_terrain_display_select (int value)
{
  if (value == 2)
    mk->tcf_handler.conf.enabled = false;
  else if (value == 0 || value == 1 || (value >= 3 && value <= 15)
	   || (value >= 18 && value <= 20) || (value >= 235 && value <= 255))
    mk->tcf_handler.conf.enabled = true;
  else
    return false;

  return true;
}

bool
MK_VIII::ConfigurationModule::read_options_select_group_1 (int value)
{
  if (value >= 0 && value < 128)
    {
      mk->io_handler.conf.flap_reversal = test_bits(value, 1 << 1);
      mk->mode6_handler.conf.bank_angle_enabled = test_bits(value, 1 << 2);
      mk->io_handler.conf.steep_approach_enabled = test_bits(value, 1 << 6);
      return true;
    }
  else
    return false;
}

bool
MK_VIII::ConfigurationModule::read_radio_altitude_input_select (int value)
{
  mk->io_handler.conf.altitude_source = value;
  return (value >= 0 && value <= 4) || (value >= 251 && value <= 255);
}

bool
MK_VIII::ConfigurationModule::read_navigation_input_select (int value)
{
  if (value >= 0 && value <= 2)
    mk->io_handler.conf.localizer_enabled = false;
  else if ((value >= 3 && value <= 5) || (value >= 250 && value <= 255))
    mk->io_handler.conf.localizer_enabled = true;
  else
    return false;

  return true;
}

bool
MK_VIII::ConfigurationModule::read_attitude_input_select (int value)
{
  if (value == 2)
    mk->io_handler.conf.use_attitude_indicator=true;
  else
    mk->io_handler.conf.use_attitude_indicator=false;
  return (value >= 0 && value <= 6) || value == 253 || value == 255;
}

bool
MK_VIII::ConfigurationModule::read_heading_input_select (int value)
{
  // unimplemented
  return (value >= 0 && value <= 3) || value == 254 || value == 255;
}

bool
MK_VIII::ConfigurationModule::read_windshear_input_select (int value)
{
  // unimplemented
  return value == 0 || (value >= 253 && value <= 255);
}

bool
MK_VIII::ConfigurationModule::read_input_output_discrete_type_select (int value)
{
  static const struct
  {
    int					type;
    IOHandler::LampConfiguration	lamp_conf;
    bool				gpws_inhibit_enabled;
    bool				momentary_flap_override_enabled;
    bool				alternate_steep_approach;
  } io_types[] = {
    { 0,	{ false, false }, false, true, true },
    { 1,	{ true, false }, false, true, true },
    { 2,	{ false, false }, true, true, true },
    { 3,	{ true, false }, true, true, true },
    { 4,	{ false, true }, true, true, true },
    { 5,	{ true, true }, true, true, true },
    { 6,	{ false, false }, true, true, false },
    { 7,	{ true, false }, true, true, false },
    { 254,	{ false, false }, true, false, true },
    { 255,	{ false, false }, true, false, true }
  };

  for (size_t i = 0; i < n_elements(io_types); i++)
    if (io_types[i].type == value)
      {
	mk->io_handler.conf.lamp = &io_types[i].lamp_conf;
	mk->io_handler.conf.gpws_inhibit_enabled = io_types[i].gpws_inhibit_enabled;
	mk->io_handler.conf.momentary_flap_override_enabled = io_types[i].momentary_flap_override_enabled;
	mk->io_handler.conf.alternate_steep_approach = io_types[i].alternate_steep_approach;
	return true;
      }

  return false;
}

bool
MK_VIII::ConfigurationModule::read_audio_output_level (int value)
{
  static const struct
  {
    int id;
    int relative_dB;
  } values[] = {
    { 0, 0 },
    { 1, -6 },
    { 2, -12 },
    { 3, -18 },
    { 4, -24 }
  };

  for (size_t i = 0; i < n_elements(values); i++)
    if (values[i].id == value)
      {
	mk->voice_player.set_volume(mk->voice_player.conf.volume = modify_amplitude(1.0, values[i].relative_dB));
	return true;
      }

  // The self test needs the voice player even when the configuration
  // is invalid, so set a default value.
  mk->voice_player.set_volume(mk->voice_player.conf.volume = 1.0);
  return false;
}

bool
MK_VIII::ConfigurationModule::read_undefined_input_select (int value)
{
  // unimplemented
  return value == 0;
}

void
MK_VIII::ConfigurationModule::boot ()
{
  bool (MK_VIII::ConfigurationModule::*readers[N_CATEGORIES]) (int) = {
    &MK_VIII::ConfigurationModule::read_aircraft_mode_type_select,
    &MK_VIII::ConfigurationModule::read_air_data_input_select,
    &MK_VIII::ConfigurationModule::read_position_input_select,
    &MK_VIII::ConfigurationModule::read_altitude_callouts,
    &MK_VIII::ConfigurationModule::read_audio_menu_select,
    &MK_VIII::ConfigurationModule::read_terrain_display_select,
    &MK_VIII::ConfigurationModule::read_options_select_group_1,
    &MK_VIII::ConfigurationModule::read_radio_altitude_input_select,
    &MK_VIII::ConfigurationModule::read_navigation_input_select,
    &MK_VIII::ConfigurationModule::read_attitude_input_select,
    &MK_VIII::ConfigurationModule::read_heading_input_select,
    &MK_VIII::ConfigurationModule::read_windshear_input_select,
    &MK_VIII::ConfigurationModule::read_input_output_discrete_type_select,
    &MK_VIII::ConfigurationModule::read_audio_output_level,
    &MK_VIII::ConfigurationModule::read_undefined_input_select,
    &MK_VIII::ConfigurationModule::read_undefined_input_select,
    &MK_VIII::ConfigurationModule::read_undefined_input_select
  };

  memcpy(effective_categories, categories, sizeof(categories));
  state = STATE_OK;

  for (int i = 0; i < N_CATEGORIES; i++)
    if (! (this->*readers[i])(effective_categories[i]))
      {
	SG_LOG(SG_INSTR, SG_ALERT, "MK VIII EGPWS configuration category " << i + 1 << ": invalid value " << effective_categories[i]);

	if (state == STATE_OK)
	  state = STATE_INVALID_DATABASE;

	mk_doutput(gpws_inop) = true;
	mk_doutput(tad_inop) = true;

	return;
      }

  // handle conflicts

  if (mk->mode6_handler.conf.above_field_voice && ! mk->tcf_handler.conf.enabled)
    {
      // [INSTALL] 3.6
      SG_LOG(SG_INSTR, SG_ALERT, "MK VIII EGPWS configuration module: when category 4 is set to 100 or 101, category 6 must not be set to 2");
      state = STATE_INVALID_DATABASE;
    }
}

void
MK_VIII::ConfigurationModule::bind (SGPropertyNode *node)
{
  for (int i = 0; i < N_CATEGORIES; i++)
    {
      std::ostringstream name;
      name << "configuration-module/category-" << i + 1;
      mk->properties_handler.tie(node, name.str().c_str(), SGRawValuePointer<int>(&categories[i]));
    }
}

///////////////////////////////////////////////////////////////////////////////
// FaultHandler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// [INSTALL] only specifies that the faults cause a GPWS INOP
// indication. We arbitrarily set a TAD INOP when it makes sense.
const unsigned int MK_VIII::FaultHandler::fault_inops[] = {
  INOP_GPWS | INOP_TAD,		// [INSTALL] 3.15.1.3
  INOP_GPWS,			// [INSTALL] appendix E 6.6.2
  INOP_GPWS,			// [INSTALL] appendix E 6.6.4
  INOP_GPWS,			// [INSTALL] appendix E 6.6.6
  INOP_GPWS | INOP_TAD,		// [INSTALL] appendix E 6.6.7
  INOP_GPWS,			// [INSTALL] appendix E 6.6.13
  INOP_GPWS,			// [INSTALL] appendix E 6.6.25
  INOP_GPWS,			// [INSTALL] appendix E 6.6.27
  INOP_TAD,			// unspecified
  INOP_GPWS,			// unspecified
  INOP_GPWS,			// unspecified
  // [INSTALL] 2.3.10.1 specifies that GPWS INOP is "activated during
  // any detected partial or total failure of the GPWS modes 1-5", so
  // do not set any INOP for mode 6 and bank angle.
  0,				// unspecified
  0,				// unspecified
  INOP_TAD			// unspecified
};

bool
MK_VIII::FaultHandler::has_faults () const
{
  for (int i = 0; i < N_FAULTS; i++)
    if (faults[i])
      return true;

  return false;
}

bool
MK_VIII::FaultHandler::has_faults (unsigned int inop)
{
  for (int i = 0; i < N_FAULTS; i++)
    if (faults[i] && test_bits(fault_inops[i], inop))
      return true;

  return false;
}

void
MK_VIII::FaultHandler::boot ()
{
  memset(faults, 0, sizeof(faults));
}

void
MK_VIII::FaultHandler::set_fault (Fault fault)
{
  if (! faults[fault])
    {
      faults[fault] = true;

      mk->self_test_handler.set_inop();

      if (test_bits(fault_inops[fault], INOP_GPWS))
	{
	  mk_unset_alerts(~mk_alert(TCF_TOO_LOW_TERRAIN));
	  mk_doutput(gpws_inop) = true;
	}
      if (test_bits(fault_inops[fault], INOP_TAD))
	{
	  mk_unset_alerts(mk_alert(TCF_TOO_LOW_TERRAIN));
	  mk_doutput(tad_inop) = true;
	}
    }
}

void
MK_VIII::FaultHandler::unset_fault (Fault fault)
{
  if (faults[fault])
    {
      faults[fault] = false;
      if (! has_faults(INOP_GPWS))
	mk_doutput(gpws_inop) = false;
      if (! has_faults(INOP_TAD))
	mk_doutput(tad_inop) = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
// IOHandler //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

double
MK_VIII::IOHandler::TerrainClearanceFilter::update (double agl)
{
  // [PILOT] page 20 specifies that the terrain clearance is equal to
  // 75% of the radio altitude, averaged over the previous 15 seconds.

  // no updates when simulation is paused (dt=0.0), and add 5 samples/second only 
  if (globals->get_sim_time_sec() - last_update < 0.2)
      return value;
  last_update = globals->get_sim_time_sec();

  samples_type::iterator iter;

  // remove samples older than 15 seconds
  for (iter = samples.begin(); iter != samples.end() && globals->get_sim_time_sec() - (*iter).timestamp >= 15; iter = samples.begin())
    samples.erase(iter);

  // append new sample
  samples.push_back(Sample<double>(agl));

  // calculate average
  double new_value = 0;
  if (samples.size() > 0)
    {
      // time consuming loop => queue limited to 75 samples
      // (= 15seconds * 5samples/second)
      for (iter = samples.begin(); iter != samples.end(); iter++)
        new_value += (*iter).value;
      new_value /= samples.size();
    }
  new_value *= 0.75;

  if (new_value > value)
    value = new_value;

  return value;
}

void
MK_VIII::IOHandler::TerrainClearanceFilter::reset ()
{
  samples.clear();
  value = 0;
  last_update = -1.0;
}

MK_VIII::IOHandler::IOHandler (MK_VIII *device)
  : mk(device), _lamp(LAMP_NONE)
{
  memset(&input_feeders, 0, sizeof(input_feeders));
  memset(&inputs.discretes, 0, sizeof(inputs.discretes));
  memset(&outputs, 0, sizeof(outputs));
  memset(&power_saved, 0, sizeof(power_saved));

  mk_dinput_feed(landing_gear) = true;
  mk_dinput_feed(landing_flaps) = true;
  mk_dinput_feed(glideslope_inhibit) = true;
  mk_dinput_feed(decision_height) = true;
  mk_dinput_feed(autopilot_engaged) = true;
  mk_ainput_feed(uncorrected_barometric_altitude) = true;
  mk_ainput_feed(barometric_altitude_rate) = true;
  mk_ainput_feed(radio_altitude) = true;
  mk_ainput_feed(glideslope_deviation) = true;
  mk_ainput_feed(roll_angle) = true;
  mk_ainput_feed(localizer_deviation) = true;
  mk_ainput_feed(computed_airspeed) = true;

  // will be unset on power on
  mk_doutput(gpws_inop) = true;
  mk_doutput(tad_inop) = true;
}

void
MK_VIII::IOHandler::boot ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  mk_doutput(gpws_inop) = false;
  mk_doutput(tad_inop) = false;

  mk_doutput(glideslope_cancel) = power_saved.glideslope_cancel;

  altitude_samples.clear();
  reset_terrain_clearance();
}

void
MK_VIII::IOHandler::post_boot ()
{
  if (momentary_steep_approach_enabled())
    {
      last_landing_gear = mk_dinput(landing_gear);
      last_real_flaps_down = real_flaps_down();
    }

  // read externally-latching input discretes
  update_alternate_discrete_input(&mk_dinput(mode6_low_volume));
  update_alternate_discrete_input(&mk_dinput(audio_inhibit));
}

void
MK_VIII::IOHandler::power_off ()
{
  power_saved.glideslope_cancel = mk_doutput(glideslope_cancel); // [SPEC] 6.2.5

  memset(&outputs, 0, sizeof(outputs));

  audio_inhibit_fault_timer.stop();
  landing_gear_fault_timer.stop();
  flaps_down_fault_timer.stop();
  momentary_flap_override_fault_timer.stop();
  self_test_fault_timer.stop();
  glideslope_cancel_fault_timer.stop();
  steep_approach_fault_timer.stop();
  gpws_inhibit_fault_timer.stop();
  ta_tcf_inhibit_fault_timer.stop();

  // [SPEC] 6.9.3.5
  mk_doutput(gpws_inop) = true;
  mk_doutput(tad_inop) = true;
}

void
MK_VIII::IOHandler::enter_ground ()
{
  reset_terrain_clearance();

  if (conf.momentary_flap_override_enabled)
    mk_doutput(flap_override) = false;	// [INSTALL] 3.15.1.6
}

void
MK_VIII::IOHandler::enter_takeoff ()
{
  reset_terrain_clearance();

  if (momentary_steep_approach_enabled())
    // landing or go-around, disable steep approach as per [SPEC] 6.2.1
    mk_doutput(steep_approach) = false;
}

void
MK_VIII::IOHandler::update_inputs ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  // 1. process input feeders

  if (mk_dinput_feed(landing_gear))
    mk_dinput(landing_gear) = mk_node(gear_down)->getBoolValue();
  if (mk_dinput_feed(landing_flaps))
    mk_dinput(landing_flaps) = mk_node(flaps)->getDoubleValue() < 1;
  if (mk_dinput_feed(glideslope_inhibit))
    mk_dinput(glideslope_inhibit) = mk_node(nav0_gs_distance)->getDoubleValue() < 0;
  if (mk_dinput_feed(autopilot_engaged))
    {
      const char *mode;

      mode = mk_node(autopilot_heading_lock)->getStringValue();
      mk_dinput(autopilot_engaged) = mode && *mode;
    }

  if (mk_ainput_feed(uncorrected_barometric_altitude))
    {
      if (mk_node(altimeter_serviceable)->getBoolValue())
	mk_ainput(uncorrected_barometric_altitude).set(mk_node(altimeter_altitude)->getDoubleValue());
      else
	mk_ainput(uncorrected_barometric_altitude).unset();
    }
  if (mk_ainput_feed(barometric_altitude_rate))
    // Do not use the vsi, because it is pressure-based only, and
    // therefore too laggy for GPWS alerting purposes. I guess that
    // a real ADC combines pressure-based and inerta-based altitude
    // rates to provide a non-laggy rate. That non-laggy rate is
    // best emulated by /velocities/vertical-speed-fps * 60.
    mk_ainput(barometric_altitude_rate).set(mk_node(vs)->getDoubleValue() * 60);
  if (mk_ainput_feed(radio_altitude))
    {
      double agl;
      switch (conf.altitude_source)
      {
          case 3:
              agl = mk_node(altitude_gear_agl)->getDoubleValue();
              break;
          case 4:
              agl = mk_node(altitude_radar_agl)->getDoubleValue();
              break;
          default: // 0,1,2 (and any currently unsupported values)
              agl = mk_node(altitude_agl)->getDoubleValue();
              break;
      }
      // Some flight models may return negative values when on the
      // ground or after a crash; do not allow them.
      mk_ainput(radio_altitude).set(SG_MAX2(0.0, agl));
    }
  if (mk_ainput_feed(glideslope_deviation))
    {
      if (mk_node(nav0_serviceable)->getBoolValue()
	  && mk_node(nav0_gs_serviceable)->getBoolValue()
	  && mk_node(nav0_in_range)->getBoolValue()
	  && mk_node(nav0_has_gs)->getBoolValue())
	// gs-needle-deflection is expressed in degrees, and 1 dot =
	// 0.32 degrees (according to
	// http://www.ntsb.gov/Recs/letters/2000/A00_41_45.pdf)
	mk_ainput(glideslope_deviation).set(mk_node(nav0_gs_needle_deflection)->getDoubleValue() / 0.32 * GLIDESLOPE_DOTS_TO_DDM);
      else
	mk_ainput(glideslope_deviation).unset();
    }
  if (mk_ainput_feed(roll_angle))
    {
      if (conf.use_attitude_indicator)
      {
        // read data from attitude indicator instrument (requires vacuum system to work)
      if (mk_node(ai_serviceable)->getBoolValue() && ! mk_node(ai_caged)->getBoolValue())
	mk_ainput(roll_angle).set(mk_node(ai_roll)->getDoubleValue());
      else
	mk_ainput(roll_angle).unset();
    }
      else
      {
        // use simulator source
        mk_ainput(roll_angle).set(mk_node(orientation_roll)->getDoubleValue());
      }
    }
  if (mk_ainput_feed(localizer_deviation))
    {
      if (mk_node(nav0_serviceable)->getBoolValue()
	  && mk_node(nav0_cdi_serviceable)->getBoolValue()
	  && mk_node(nav0_in_range)->getBoolValue()
	  && mk_node(nav0_nav_loc)->getBoolValue())
	// heading-needle-deflection is expressed in degrees, and 1
	// dot = 2 degrees (0.5 degrees for ILS, but navradio.cxx
	// performs the conversion)
	mk_ainput(localizer_deviation).set(mk_node(nav0_heading_needle_deflection)->getDoubleValue() / 2 * LOCALIZER_DOTS_TO_DDM);
      else
	mk_ainput(localizer_deviation).unset();
    }
  if (mk_ainput_feed(computed_airspeed))
    {
      if (mk_node(asi_serviceable)->getBoolValue())
	mk_ainput(computed_airspeed).set(mk_node(asi_speed)->getDoubleValue());
      else
	mk_ainput(computed_airspeed).unset();
    }

  // 2. compute data

  mk_data(decision_height).set(&mk_ainput(decision_height));
  mk_data(radio_altitude).set(&mk_ainput(radio_altitude));
  mk_data(roll_angle).set(&mk_ainput(roll_angle));

  // update barometric_altitude_rate as per [SPEC] 6.2.1: "During
  // normal conditions, the system will base Mode 1 computations upon
  // barometric rate from the ADC. If this computed data is not valid
  // or available then the system will use internally computed
  // barometric altitude rate."

  if (! mk_ainput(barometric_altitude_rate).ncd)
    // the altitude rate input is valid, use it
    mk_data(barometric_altitude_rate).set(mk_ainput(barometric_altitude_rate).get());
  else
    {
      bool has = false;

      // The altitude rate input is invalid. We can compute an
      // altitude rate if all the following conditions are true:
      //
      //   - the altitude input is valid
      //   - there is an altitude sample with age >= 1 second
      //   - that altitude sample is valid

      if (! mk_ainput(uncorrected_barometric_altitude).ncd)
	{
	  if (! altitude_samples.empty() && ! altitude_samples.front().value.ncd)
	    {
	      double elapsed = globals->get_sim_time_sec() - altitude_samples.front().timestamp;
	      if (elapsed >= 1)
		{
		  has = true;
		  mk_data(barometric_altitude_rate).set((mk_ainput(uncorrected_barometric_altitude).get() - altitude_samples.front().value.get()) / elapsed * 60);
		}
	    }
	}

      if (! has)
	mk_data(barometric_altitude_rate).unset();
    }

  altitude_samples.push_back(Sample< Parameter<double> >(mk_ainput(uncorrected_barometric_altitude)));

  // Erase everything from the beginning of the list up to the sample
  // preceding the most recent sample whose age is >= 1 second.

  altitude_samples_type::iterator erase_last = altitude_samples.begin();
  altitude_samples_type::iterator iter;

  for (iter = altitude_samples.begin(); iter != altitude_samples.end(); iter++)
    if (globals->get_sim_time_sec() - (*iter).timestamp >= 1)
      erase_last = iter;
    else
      break;

  if (erase_last != altitude_samples.begin())
    altitude_samples.erase(altitude_samples.begin(), erase_last);

  // update GPS data

  if (conf.use_internal_gps)
    {
      mk_data(gps_altitude).set(mk_node(altitude)->getDoubleValue());
      mk_data(gps_latitude).set(mk_node(latitude)->getDoubleValue());
      mk_data(gps_longitude).set(mk_node(longitude)->getDoubleValue());
      mk_data(gps_vertical_figure_of_merit).set(0.0);
    }
  else
    {
      mk_data(gps_altitude).set(&mk_ainput(gps_altitude));
      mk_data(gps_latitude).set(&mk_ainput(gps_latitude));
      mk_data(gps_longitude).set(&mk_ainput(gps_longitude));
      mk_data(gps_vertical_figure_of_merit).set(&mk_ainput(gps_vertical_figure_of_merit));
    }

  // update glideslope and localizer

  mk_data(glideslope_deviation_dots).set(&mk_ainput(glideslope_deviation), GLIDESLOPE_DDM_TO_DOTS);
  mk_data(localizer_deviation_dots).set(&mk_ainput(localizer_deviation), LOCALIZER_DDM_TO_DOTS);

  // Update geometric altitude; [SPEC] 6.7.8 provides an overview of a
  // complex algorithm which combines several input sources to
  // calculate the geometric altitude. Since the exact algorithm is
  // not given, we fallback to a much simpler method.

  if (! mk_data(gps_altitude).ncd)
    mk_data(geometric_altitude).set(mk_data(gps_altitude).get());
  else if (! mk_ainput(uncorrected_barometric_altitude).ncd)
    mk_data(geometric_altitude).set(mk_ainput(uncorrected_barometric_altitude).get());
  else
    mk_data(geometric_altitude).unset();

  // update terrain clearance

  update_terrain_clearance();

  // 3. perform sanity checks

  if (! mk_data(radio_altitude).ncd && mk_data(radio_altitude).get() < 0)
    mk_data(radio_altitude).unset();

  if (! mk_data(decision_height).ncd && mk_data(decision_height).get() < 0)
    mk_data(decision_height).unset();

  if (! mk_data(gps_latitude).ncd
      && (mk_data(gps_latitude).get() < -90
	  || mk_data(gps_latitude).get() > 90))
    mk_data(gps_latitude).unset();

  if (! mk_data(gps_longitude).ncd
      && (mk_data(gps_longitude).get() < -180
	  || mk_data(gps_longitude).get() > 180))
    mk_data(gps_longitude).unset();

  if (! mk_data(roll_angle).ncd
      && ((mk_data(roll_angle).get() < -180)
	  || (mk_data(roll_angle).get() > 180)))
    mk_data(roll_angle).unset();

  // 4. process input feeders requiring data computed above

  if (mk_dinput_feed(decision_height))
    mk_dinput(decision_height) = ! mk_data(radio_altitude).ncd
      && ! mk_data(decision_height).ncd
      && mk_data(radio_altitude).get() <= mk_data(decision_height).get();
}

void
MK_VIII::IOHandler::update_terrain_clearance ()
{
  if (! mk_data(radio_altitude).ncd)
    mk_data(terrain_clearance).set(terrain_clearance_filter.update(mk_data(radio_altitude).get()));
  else
    mk_data(terrain_clearance).unset();
}

void
MK_VIII::IOHandler::reset_terrain_clearance ()
{
  terrain_clearance_filter.reset();
  update_terrain_clearance();
}

void
MK_VIII::IOHandler::reposition ()
{
  reset_terrain_clearance();
}

void
MK_VIII::IOHandler::handle_input_fault (bool test, FaultHandler::Fault fault)
{
  if (test)
    {
      if (! mk->fault_handler.faults[fault])
	mk->fault_handler.set_fault(fault);
    }
  else
    mk->fault_handler.unset_fault(fault);
}

void
MK_VIII::IOHandler::handle_input_fault (bool test,
					Timer *timer,
					double max_duration,
					FaultHandler::Fault fault)
{
  if (test)
    {
      if (! mk->fault_handler.faults[fault])
	{
	  if (timer->start_or_elapsed() >= max_duration)
	    mk->fault_handler.set_fault(fault);
	}
    }
  else
    {
      mk->fault_handler.unset_fault(fault);
      timer->stop();
    }
}

void
MK_VIII::IOHandler::update_input_faults ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  // [INSTALL] 3.15.1.3
  handle_input_fault(mk_dinput(audio_inhibit),
		     &audio_inhibit_fault_timer,
		     60,
		     FaultHandler::FAULT_ALL_MODES_INHIBIT);

  // [INSTALL] appendix E 6.6.2
  handle_input_fault(mk_dinput(landing_gear)
		     && ! mk_ainput(computed_airspeed).ncd
		     && mk_ainput(computed_airspeed).get() > conf.faults->max_gear_down_airspeed,
		     &landing_gear_fault_timer,
		     60,
		     FaultHandler::FAULT_GEAR_SWITCH);

  // [INSTALL] appendix E 6.6.4
  handle_input_fault(flaps_down()
		     && ! mk_ainput(computed_airspeed).ncd
		     && mk_ainput(computed_airspeed).get() > conf.faults->max_flaps_down_airspeed,
		     &flaps_down_fault_timer,
		     60,
		     FaultHandler::FAULT_FLAPS_SWITCH);

  // [INSTALL] appendix E 6.6.6
  if (conf.momentary_flap_override_enabled)
    handle_input_fault(mk_dinput(momentary_flap_override),
		       &momentary_flap_override_fault_timer,
		       15,
		       FaultHandler::FAULT_MOMENTARY_FLAP_OVERRIDE_INVALID);

  // [INSTALL] appendix E 6.6.7
  handle_input_fault(mk_dinput(self_test),
		     &self_test_fault_timer,
		     60,
		     FaultHandler::FAULT_SELF_TEST_INVALID);

  // [INSTALL] appendix E 6.6.13
  handle_input_fault(mk_dinput(glideslope_cancel),
		     &glideslope_cancel_fault_timer,
		     15,
		     FaultHandler::FAULT_GLIDESLOPE_CANCEL_INVALID);

  // [INSTALL] appendix E 6.6.25
  if (momentary_steep_approach_enabled())
    handle_input_fault(mk_dinput(steep_approach),
		       &steep_approach_fault_timer,
		       15,
		       FaultHandler::FAULT_STEEP_APPROACH_INVALID);

  // [INSTALL] appendix E 6.6.27
  if (conf.gpws_inhibit_enabled)
    handle_input_fault(mk_dinput(gpws_inhibit),
		       &gpws_inhibit_fault_timer,
		       5,
		       FaultHandler::FAULT_GPWS_INHIBIT);

  // [INSTALL] does not specify a fault for this one, but it makes
  // sense to have it behave like GPWS inhibit
  handle_input_fault(mk_dinput(ta_tcf_inhibit),
		     &ta_tcf_inhibit_fault_timer,
		     5,
		     FaultHandler::FAULT_TA_TCF_INHIBIT);

  // [PILOT] page 48: "In the event that required data for a
  // particular function is not available, then that function is
  // automatically inhibited and annunciated"

  handle_input_fault(mk_data(radio_altitude).ncd
		     || mk_ainput(uncorrected_barometric_altitude).ncd
		     || mk_data(barometric_altitude_rate).ncd
		     || mk_ainput(computed_airspeed).ncd
		     || mk_data(terrain_clearance).ncd,
		     FaultHandler::FAULT_MODES14_INPUTS_INVALID);

  if (! mk_dinput(glideslope_inhibit))
    handle_input_fault(mk_data(radio_altitude).ncd,
		       FaultHandler::FAULT_MODE5_INPUTS_INVALID);

  if (mk->mode6_handler.altitude_callouts_enabled())
    handle_input_fault(mk->mode6_handler.conf.above_field_voice
		       ? (mk_data(gps_latitude).ncd
			  || mk_data(gps_longitude).ncd
			  || mk_data(geometric_altitude).ncd)
		       : mk_data(radio_altitude).ncd,
		       FaultHandler::FAULT_MODE6_INPUTS_INVALID);

  if (mk->mode6_handler.conf.bank_angle_enabled)
    handle_input_fault(mk_data(roll_angle).ncd,
		       FaultHandler::FAULT_BANK_ANGLE_INPUTS_INVALID);

  if (mk->tcf_handler.conf.enabled)
    handle_input_fault(mk_data(radio_altitude).ncd
		       || mk_data(geometric_altitude).ncd
		       || mk_data(gps_latitude).ncd
		       || mk_data(gps_longitude).ncd
		       || mk_data(gps_vertical_figure_of_merit).ncd,
		       FaultHandler::FAULT_TCF_INPUTS_INVALID);
}

void
MK_VIII::IOHandler::update_alternate_discrete_input (bool *ptr)
{
  assert(mk->system_handler.state == SystemHandler::STATE_ON);

  if (ptr == &mk_dinput(mode6_low_volume))
    {
      if (mk->configuration_module.state == ConfigurationModule::STATE_OK
	  && mk->self_test_handler.state == SelfTestHandler::STATE_NONE)
	mk->mode6_handler.set_volume(*ptr ? modify_amplitude(1.0, -6) : 1.0);
    }
  else if (ptr == &mk_dinput(audio_inhibit))
    {
      if (mk->configuration_module.state == ConfigurationModule::STATE_OK
	  && mk->self_test_handler.state == SelfTestHandler::STATE_NONE)
	mk->voice_player.set_volume(*ptr ? 0.0 : mk->voice_player.conf.volume);
    }
}

void
MK_VIII::IOHandler::update_internal_latches ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  // [SPEC] 6.2.1
  if (conf.momentary_flap_override_enabled
      && mk_doutput(flap_override)
      && ! mk->state_handler.takeoff
      && (mk_data(radio_altitude).ncd || mk_data(radio_altitude).get() <= 50))
    mk_doutput(flap_override) = false;

  // [SPEC] 6.2.1
  if (momentary_steep_approach_enabled())
    {
      if (mk_doutput(steep_approach)
	  && ! mk->state_handler.takeoff
	  && ((last_landing_gear && ! mk_dinput(landing_gear))
	      || (last_real_flaps_down && ! real_flaps_down())))
	// gear or flaps raised during approach: it's a go-around
	mk_doutput(steep_approach) = false;

      last_landing_gear = mk_dinput(landing_gear);
      last_real_flaps_down = real_flaps_down();
    }

  // [SPEC] 6.2.5
  if (mk_doutput(glideslope_cancel)
      && (mk_data(glideslope_deviation_dots).ncd
	  || mk_data(radio_altitude).ncd
	  || mk_data(radio_altitude).get() > 2000
	  || mk_data(radio_altitude).get() < 30))
    mk_doutput(glideslope_cancel) = false;
}

void
MK_VIII::IOHandler::update_egpws_alert_discrete_1 ()
{
  if (mk->voice_player.voice)
    {
      const struct
      {
	int			bit;
	VoicePlayer::Voice	*voice;
      } voices[] = {
	{ 11, mk_voice(sink_rate_pause_sink_rate) },
	{ 12, mk_voice(pull_up) },
	{ 13, mk_voice(terrain) },
	{ 13, mk_voice(terrain_pause_terrain) },
	{ 14, mk_voice(dont_sink_pause_dont_sink) },
	{ 15, mk_voice(too_low_gear) },
	{ 16, mk_voice(too_low_flaps) },
	{ 17, mk_voice(too_low_terrain) },
	{ 18, mk_voice(soft_glideslope) },
	{ 18, mk_voice(hard_glideslope) },
	{ 19, mk_voice(minimums_minimums) }
      };

      for (size_t i = 0; i < n_elements(voices); i++)
	if (voices[i].voice == mk->voice_player.voice)
	  {
	    mk_aoutput(egpws_alert_discrete_1) = 1 << voices[i].bit;
	    return;
	  }
    }

  mk_aoutput(egpws_alert_discrete_1) = 0;
}

void
MK_VIII::IOHandler::update_egpwc_logic_discretes ()
{
  mk_aoutput(egpwc_logic_discretes) = 0;

  if (mk->state_handler.takeoff)
    mk_aoutput(egpwc_logic_discretes) |= 1 << 18;

  static const struct
  {
    int			bit;
    unsigned int	alerts;
  } logic[] = {
    { 13, mk_alert(TCF_TOO_LOW_TERRAIN) },
    { 19, mk_alert(MODE1_SINK_RATE) },
    { 20, mk_alert(MODE1_PULL_UP) },
    { 21, mk_alert(MODE2A_PREFACE) | mk_alert(MODE2B_PREFACE) | mk_alert(MODE2B_LANDING_MODE) | mk_alert(MODE2A_ALTITUDE_GAIN_TERRAIN_CLOSING) },
    { 22, mk_alert(MODE2A) | mk_alert(MODE2B) },
    { 23, mk_alert(MODE3) },
    { 24, mk_alert(MODE4_TOO_LOW_FLAPS) | mk_alert(MODE4_TOO_LOW_GEAR) | mk_alert(MODE4AB_TOO_LOW_TERRAIN) | mk_alert(MODE4C_TOO_LOW_TERRAIN) },
    { 25, mk_alert(MODE5_SOFT) | mk_alert(MODE5_HARD) }
  };

  for (size_t i = 0; i < n_elements(logic); i++)
    if (mk_test_alerts(logic[i].alerts))
      mk_aoutput(egpwc_logic_discretes) |= 1 << logic[i].bit;
}

void
MK_VIII::IOHandler::update_mode6_callouts_discrete_1 ()
{
  if (mk->voice_player.voice)
    {
      const struct
      {
	int			bit;
	VoicePlayer::Voice	*voice;
      } voices[] = {
	{ 11, mk_voice(minimums_minimums) },
	{ 16, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_10) },
	{ 17, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_20) },
	{ 18, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_30) },
	{ 19, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_40) },
	{ 20, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_50) },
	{ 23, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_100) },
	{ 24, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_200) },
	{ 25, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_300) }
      };

      for (size_t i = 0; i < n_elements(voices); i++)
	if (voices[i].voice == mk->voice_player.voice)
	  {
	    mk_aoutput(mode6_callouts_discrete_1) = 1 << voices[i].bit;
	    return;
	  }
    }

  mk_aoutput(mode6_callouts_discrete_1) = 0;
}

void
MK_VIII::IOHandler::update_mode6_callouts_discrete_2 ()
{
  if (mk->voice_player.voice)
    {
      const struct
      {
	int			bit;
	VoicePlayer::Voice	*voice;
      } voices[] = {
	{ 11, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_400) },
	{ 12, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_500) },
	{ 13, mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_1000) },
	{ 18, mk_voice(bank_angle_pause_bank_angle) },
	{ 18, mk_voice(bank_angle_pause_bank_angle_3) },
	{ 23, mk_voice(five_hundred_above) }
      };

      for (size_t i = 0; i < n_elements(voices); i++)
	if (voices[i].voice == mk->voice_player.voice)
	  {
	    mk_aoutput(mode6_callouts_discrete_2) = 1 << voices[i].bit;
	    return;
	  }
    }

  mk_aoutput(mode6_callouts_discrete_2) = 0;
}

void
MK_VIII::IOHandler::update_egpws_alert_discrete_2 ()
{
  mk_aoutput(egpws_alert_discrete_2) = 1 << 27; // Terrain NA

  if (mk_doutput(glideslope_cancel))
    mk_aoutput(egpws_alert_discrete_2) |= 1 << 11;
  if (mk_doutput(gpws_alert))
    mk_aoutput(egpws_alert_discrete_2) |= 1 << 12;
  if (mk_doutput(gpws_warning))
    mk_aoutput(egpws_alert_discrete_2) |= 1 << 13;
  if (mk_doutput(gpws_inop))
    mk_aoutput(egpws_alert_discrete_2) |= 1 << 14;
  if (mk_doutput(audio_on))
    mk_aoutput(egpws_alert_discrete_2) |= 1 << 16;
  if (mk_doutput(tad_inop))
    mk_aoutput(egpws_alert_discrete_2) |= 1 << 24;
  if (mk->fault_handler.has_faults())
    mk_aoutput(egpws_alert_discrete_2) |= 1 << 25;
}

void
MK_VIII::IOHandler::update_egpwc_alert_discrete_3 ()
{
  mk_aoutput(egpwc_alert_discrete_3) = 1 << 17 | 1 << 18;

  if (mk->fault_handler.faults[FaultHandler::FAULT_MODES14_INPUTS_INVALID])
    mk_aoutput(egpwc_alert_discrete_3) |= 1 << 11;
  if (mk->fault_handler.faults[FaultHandler::FAULT_MODE5_INPUTS_INVALID])
    mk_aoutput(egpwc_alert_discrete_3) |= 1 << 12;
  if (mk->fault_handler.faults[FaultHandler::FAULT_MODE6_INPUTS_INVALID])
    mk_aoutput(egpwc_alert_discrete_3) |= 1 << 13;
  if (mk->fault_handler.faults[FaultHandler::FAULT_BANK_ANGLE_INPUTS_INVALID])
    mk_aoutput(egpwc_alert_discrete_3) |= 1 << 14;
  if (mk_doutput(tad_inop))
    mk_aoutput(egpwc_alert_discrete_3) |= 1 << 16;
}

void
MK_VIII::IOHandler::update_outputs ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  mk_doutput(audio_on) = ! mk_dinput(audio_inhibit)
    && mk->voice_player.voice
    && ! mk->voice_player.voice->element->silence;

  update_egpws_alert_discrete_1();
  update_egpwc_logic_discretes();
  update_mode6_callouts_discrete_1();
  update_mode6_callouts_discrete_2();
  update_egpws_alert_discrete_2();
  update_egpwc_alert_discrete_3();
}

bool *
MK_VIII::IOHandler::get_lamp_output (Lamp lamp)
{
  switch (lamp)
    {
    case LAMP_GLIDESLOPE:
      return &mk_doutput(gpws_alert);

    case LAMP_CAUTION:
      return conf.lamp->format2 ? &mk_doutput(gpws_alert) : &mk_doutput(gpws_warning);

    case LAMP_WARNING:
      return &mk_doutput(gpws_warning);

    default:
      assert_not_reached();
      return NULL;
    }
}

void
MK_VIII::IOHandler::update_lamps ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  if (_lamp != LAMP_NONE && conf.lamp->flashing)
    {
      // [SPEC] 6.9.3: 70 cycles per minute
      if (lamp_timer.elapsed() >= 60.0 / 70.0 / 2.0)
	{
	  bool *output = get_lamp_output(_lamp);
	  *output = ! *output;
	  lamp_timer.start();
	}
    }
}

void
MK_VIII::IOHandler::set_lamp (Lamp lamp)
{
  if (lamp == _lamp)
    return;

  _lamp = lamp;

  mk_doutput(gpws_warning) = false;
  mk_doutput(gpws_alert) = false;

  if (lamp != LAMP_NONE)
    {
      *get_lamp_output(lamp) = true;
      lamp_timer.start();
    }
}

bool
MK_VIII::IOHandler::gpws_inhibit () const
{
  return conf.gpws_inhibit_enabled && mk_dinput(gpws_inhibit);
}

bool
MK_VIII::IOHandler::real_flaps_down () const
{
  return conf.flap_reversal ? mk_dinput(landing_flaps) : ! mk_dinput(landing_flaps);
}

bool
MK_VIII::IOHandler::flaps_down () const
{
  return flap_override() ? true : real_flaps_down();
}

bool
MK_VIII::IOHandler::flap_override () const
{
  return conf.momentary_flap_override_enabled ? mk_doutput(flap_override) : false;
}

bool
MK_VIII::IOHandler::steep_approach () const
{
  if (conf.steep_approach_enabled)
    // If alternate action was configured in category 13, we return
    // the value of the input discrete (which should be connected to a
    // latching steep approach cockpit switch). Otherwise, we return
    // the value of the output discrete.
    return conf.alternate_steep_approach ? mk_dinput(steep_approach) : mk_doutput(steep_approach);
  else
    return false;
}

bool
MK_VIII::IOHandler::momentary_steep_approach_enabled () const
{
  return conf.steep_approach_enabled && ! conf.alternate_steep_approach;
}

void
MK_VIII::IOHandler::tie_input (SGPropertyNode *node,
			       const char *name,
			       bool *input,
			       bool *feed)
{
  mk->properties_handler.tie(node, (string("inputs/discretes/") + name).c_str(),
          FGVoicePlayer::RawValueMethodsData<MK_VIII::IOHandler, bool, bool *>(*this, input, &MK_VIII::IOHandler::get_discrete_input, &MK_VIII::IOHandler::set_discrete_input));
  if (feed)
    mk->properties_handler.tie(node, (string("input-feeders/discretes/") + name).c_str(), SGRawValuePointer<bool>(feed));
}

void
MK_VIII::IOHandler::tie_input (SGPropertyNode *node,
			       const char *name,
			       Parameter<double> *input,
			       bool *feed)
{
  mk->properties_handler.tie(node, (string("inputs/arinc429/") + name).c_str(), SGRawValuePointer<double>(input->get_pointer()));
  mk->properties_handler.tie(node, (string("inputs/arinc429/") + name + "-ncd").c_str(), SGRawValuePointer<bool>(&input->ncd));
  if (feed)
    mk->properties_handler.tie(node, (string("input-feeders/arinc429/") + name).c_str(), SGRawValuePointer<bool>(feed));
}

void
MK_VIII::IOHandler::tie_output (SGPropertyNode *node,
				const char *name,
				bool *output)
{
  SGPropertyNode *child = node->getNode((string("outputs/discretes/") + name).c_str(), true);

  mk->properties_handler.tie(child, SGRawValuePointer<bool>(output));
  child->setAttribute(SGPropertyNode::WRITE, false);
}

void
MK_VIII::IOHandler::tie_output (SGPropertyNode *node,
				const char *name,
				int *output)
{
  SGPropertyNode *child = node->getNode((string("outputs/arinc429/") + name).c_str(), true);

  mk->properties_handler.tie(child, SGRawValuePointer<int>(output));
  child->setAttribute(SGPropertyNode::WRITE, false);
}

void
MK_VIII::IOHandler::bind (SGPropertyNode *node)
{
  mk->properties_handler.tie(node, "inputs/rs-232/present-status", SGRawValueMethods<MK_VIII::IOHandler, bool>(*this, &MK_VIII::IOHandler::get_present_status, &MK_VIII::IOHandler::set_present_status));

  tie_input(node, "landing-gear", &mk_dinput(landing_gear), &mk_dinput_feed(landing_gear));
  tie_input(node, "landing-flaps", &mk_dinput(landing_flaps), &mk_dinput_feed(landing_flaps));
  tie_input(node, "momentary-flap-override", &mk_dinput(momentary_flap_override));
  tie_input(node, "self-test", &mk_dinput(self_test));
  tie_input(node, "glideslope-inhibit", &mk_dinput(glideslope_inhibit), &mk_dinput_feed(glideslope_inhibit));
  tie_input(node, "glideslope-cancel", &mk_dinput(glideslope_cancel));
  tie_input(node, "decision-height", &mk_dinput(decision_height), &mk_dinput_feed(decision_height));
  tie_input(node, "mode6-low-volume", &mk_dinput(mode6_low_volume));
  tie_input(node, "audio-inhibit", &mk_dinput(audio_inhibit));
  tie_input(node, "ta-tcf-inhibit", &mk_dinput(ta_tcf_inhibit));
  tie_input(node, "autopilot-engaged", &mk_dinput(autopilot_engaged), &mk_dinput_feed(autopilot_engaged));
  tie_input(node, "steep-approach", &mk_dinput(steep_approach));
  tie_input(node, "gpws-inhibit", &mk_dinput(gpws_inhibit));

  tie_input(node, "uncorrected-barometric-altitude", &mk_ainput(uncorrected_barometric_altitude), &mk_ainput_feed(uncorrected_barometric_altitude));
  tie_input(node, "barometric-altitude-rate", &mk_ainput(barometric_altitude_rate), &mk_ainput_feed(barometric_altitude_rate));
  tie_input(node, "gps-altitude", &mk_ainput(gps_altitude));
  tie_input(node, "gps-latitude", &mk_ainput(gps_latitude));
  tie_input(node, "gps-longitude", &mk_ainput(gps_longitude));
  tie_input(node, "gps-vertical-figure-of-merit", &mk_ainput(gps_vertical_figure_of_merit));
  tie_input(node, "radio-altitude", &mk_ainput(radio_altitude), &mk_ainput_feed(radio_altitude));
  tie_input(node, "glideslope-deviation", &mk_ainput(glideslope_deviation), &mk_ainput_feed(glideslope_deviation));
  tie_input(node, "roll-angle", &mk_ainput(roll_angle), &mk_ainput_feed(roll_angle));
  tie_input(node, "localizer-deviation", &mk_ainput(localizer_deviation), &mk_ainput_feed(localizer_deviation));
  tie_input(node, "computed-airspeed", &mk_ainput(computed_airspeed), &mk_ainput_feed(computed_airspeed));
  tie_input(node, "decision-height", &mk_ainput(decision_height), &mk_ainput_feed(decision_height));

  tie_output(node, "gpws-warning", &mk_doutput(gpws_warning));
  tie_output(node, "gpws-alert", &mk_doutput(gpws_alert));
  tie_output(node, "audio-on", &mk_doutput(audio_on));
  tie_output(node, "gpws-inop", &mk_doutput(gpws_inop));
  tie_output(node, "tad-inop", &mk_doutput(tad_inop));
  tie_output(node, "flap-override", &mk_doutput(flap_override));
  tie_output(node, "glideslope-cancel", &mk_doutput(glideslope_cancel));
  tie_output(node, "steep-approach", &mk_doutput(steep_approach));

  tie_output(node, "egpws-alert-discrete-1", &mk_aoutput(egpws_alert_discrete_1));
  tie_output(node, "egpwc-logic-discretes", &mk_aoutput(egpwc_logic_discretes));
  tie_output(node, "mode6-callouts-discrete-1", &mk_aoutput(mode6_callouts_discrete_1));
  tie_output(node, "mode6-callouts-discrete-2", &mk_aoutput(mode6_callouts_discrete_2));
  tie_output(node, "egpws-alert-discrete-2", &mk_aoutput(egpws_alert_discrete_2));
  tie_output(node, "egpwc-alert-discrete-3", &mk_aoutput(egpwc_alert_discrete_3));
}

bool
MK_VIII::IOHandler::get_discrete_input (bool *ptr) const
{
   return *ptr;
}

void
MK_VIII::IOHandler::set_discrete_input (bool *ptr, bool value)
{
  if (value == *ptr)
    return;

  if (mk->system_handler.state == SystemHandler::STATE_ON)
    {
      if (ptr == &mk_dinput(momentary_flap_override))
	{
	  if (mk->configuration_module.state == ConfigurationModule::STATE_OK
	      && mk->self_test_handler.state == SelfTestHandler::STATE_NONE
	      && conf.momentary_flap_override_enabled
	      && value)
	    mk_doutput(flap_override) = ! mk_doutput(flap_override);
	}
      else if (ptr == &mk_dinput(self_test))
	mk->self_test_handler.handle_button_event(value);
      else if (ptr == &mk_dinput(glideslope_cancel))
	{
	  if (mk->configuration_module.state == ConfigurationModule::STATE_OK
	      && mk->self_test_handler.state == SelfTestHandler::STATE_NONE
	      && value)
	    {
	      if (! mk_doutput(glideslope_cancel))
		{
		  // [SPEC] 6.2.5

		  // Although we are not called from update(), we are
		  // sure that the inputs we use here are defined,
		  // since state is STATE_ON.

		  if (! mk_data(glideslope_deviation_dots).ncd
		      && ! mk_data(radio_altitude).ncd
		      && mk_data(radio_altitude).get() <= 2000
		      && mk_data(radio_altitude).get() >= 30)
		    mk_doutput(glideslope_cancel) = true;
		}
	      // else do nothing ([PILOT] page 22: "Glideslope Cancel
	      // can not be deselected (reset) by again pressing the
	      // Glideslope Cancel switch.")
	    }
	}
      else if (ptr == &mk_dinput(steep_approach))
	{
	  if (mk->configuration_module.state == ConfigurationModule::STATE_OK
	      && mk->self_test_handler.state == SelfTestHandler::STATE_NONE
	      && momentary_steep_approach_enabled()
	      && value)
	    mk_doutput(steep_approach) = ! mk_doutput(steep_approach);
	}
    }

  *ptr = value;

  if (mk->system_handler.state == SystemHandler::STATE_ON)
    update_alternate_discrete_input(ptr);
}

void
MK_VIII::IOHandler::present_status_section (const char *name)
{
  printf("%s\n", name);
}

void
MK_VIII::IOHandler::present_status_item (const char *name, const char *value)
{
  if (value)
    printf("\t%-32s %s\n", name, value);
  else
    printf("\t%s\n", name);
}

void
MK_VIII::IOHandler::present_status_subitem (const char *name)
{
  printf("\t\t%s\n", name);
}

void
MK_VIII::IOHandler::present_status ()
{
  // [SPEC] 6.10.13

  if (mk->system_handler.state != SystemHandler::STATE_ON)
    return;

  present_status_section("EGPWC CONFIGURATION");
  present_status_item("PART NUMBER:", "965-1220-000"); // [SPEC] 1.1
  present_status_item("MOD STATUS:", "N/A");
  present_status_item("SERIAL NUMBER:", "N/A");
  printf("\n");
  present_status_item("APPLICATION S/W VERSION:", FLIGHTGEAR_VERSION);
  present_status_item("TERRAIN DATABASE VERSION:", FLIGHTGEAR_VERSION);
  present_status_item("ENVELOPE MOD DATABASE VERSION:", "N/A");
  present_status_item("BOOT CODE VERSION:", FLIGHTGEAR_VERSION);
  printf("\n");

  present_status_section("CURRENT FAULTS");
  if (mk->configuration_module.state == ConfigurationModule::STATE_OK && ! mk->fault_handler.has_faults())
    present_status_item("NO FAULTS");
  else
    {
      if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
	{
	  present_status_item("GPWS COMPUTER FAULTS:");
	  switch (mk->configuration_module.state)
	    {
	    case ConfigurationModule::STATE_INVALID_DATABASE:
	      present_status_subitem("APPLICATION DATABASE FAILED");
	      break;

	    case ConfigurationModule::STATE_INVALID_AIRCRAFT_TYPE:
	      present_status_subitem("CONFIGURATION TYPE INVALID");
	      break;

	    default:
	      assert_not_reached();
	      break;
	    }
	}
      else
	{
	  present_status_item("GPWS COMPUTER OK");
	  present_status_item("GPWS EXTERNAL FAULTS:");

	  static const char *fault_names[] = {
	    "ALL MODES INHIBIT",
	    "GEAR SWITCH",
	    "FLAPS SWITCH",
	    "MOMENTARY FLAP OVERRIDE INVALID",
	    "SELF TEST INVALID",
	    "GLIDESLOPE CANCEL INVALID",
	    "STEEP APPROACH INVALID",
	    "GPWS INHIBIT",
	    "TA & TCF INHIBIT",
	    "MODES 1-4 INPUTS INVALID",
	    "MODE 5 INPUTS INVALID",
	    "MODE 6 INPUTS INVALID",
	    "BANK ANGLE INPUTS INVALID",
	    "TCF INPUTS INVALID"
	  };

	  for (size_t i = 0; i < n_elements(fault_names); i++)
	    if (mk->fault_handler.faults[i])
	      present_status_subitem(fault_names[i]);
	}
    }
  printf("\n");

  present_status_section("CONFIGURATION:");

  static const char *category_names[] = {
    "AIRCRAFT TYPE",
    "AIR DATA TYPE",
    "POSITION INPUT TYPE",
    "CALLOUTS OPTION TYPE",
    "AUDIO MENU TYPE",
    "TERRAIN DISPLAY TYPE",
    "OPTIONS 1 TYPE",
    "RADIO ALTITUDE TYPE",
    "NAVIGATION INPUT TYPE",
    "ATTITUDE TYPE",
    "MAGNETIC HEADING TYPE",
    "WINDSHEAR INPUT TYPE",
    "IO DISCRETE TYPE",
    "VOLUME SELECT"
  };

  for (size_t i = 0; i < n_elements(category_names); i++)
    {
      std::ostringstream value;
      value << "= " << mk->configuration_module.effective_categories[i];
      present_status_item(category_names[i], value.str().c_str());
    }
}

bool
MK_VIII::IOHandler::get_present_status () const
{
   return false;
}

void
MK_VIII::IOHandler::set_present_status (bool value)
{ 
   if (value) present_status();
}


///////////////////////////////////////////////////////////////////////////////
// MK_VIII::VoicePlayer ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
MK_VIII::VoicePlayer::init ()
{
    FGVoicePlayer::init();

#define STDPAUSE 0.75   // [SPEC] 6.4.4: "the standard 0.75 second delay"
    make_voice(&voices.application_data_base_failed, "application-data-base-failed");
    make_voice(&voices.bank_angle, "bank-angle");
    make_voice(&voices.bank_angle_bank_angle, "bank-angle", "bank-angle");
    make_voice(&voices.bank_angle_bank_angle_3, "bank-angle", "bank-angle", 3.0);
    make_voice(&voices.bank_angle_inop, "bank-angle-inop");
    make_voice(&voices.bank_angle_pause_bank_angle, "bank-angle", STDPAUSE, "bank-angle");
    make_voice(&voices.bank_angle_pause_bank_angle_3, "bank-angle", STDPAUSE, "bank-angle", 3.0);
    make_voice(&voices.callouts_inop, "callouts-inop");
    make_voice(&voices.configuration_type_invalid, "configuration-type-invalid");
    make_voice(&voices.dont_sink, "dont-sink");
    make_voice(&voices.dont_sink_pause_dont_sink, "dont-sink", STDPAUSE, "dont-sink");
    make_voice(&voices.five_hundred_above, "500-above");
    make_voice(&voices.glideslope, "glideslope");
    make_voice(&voices.glideslope_inop, "glideslope-inop");
    make_voice(&voices.gpws_inop, "gpws-inop");
    make_voice(&voices.hard_glideslope, "glideslope", "glideslope", 3.0);
    make_voice(&voices.minimums, "minimums");
    make_voice(&voices.minimums_minimums, "minimums", "minimums");
    make_voice(&voices.pull_up, "pull-up");
    make_voice(&voices.sink_rate, "sink-rate");
    make_voice(&voices.sink_rate_pause_sink_rate, "sink-rate", STDPAUSE, "sink-rate");
    make_voice(&voices.soft_glideslope, new Voice::SampleElement(get_sample("glideslope"), modify_amplitude(1.0, -6)));
    make_voice(&voices.terrain, "terrain");
    make_voice(&voices.terrain_pause_terrain, "terrain", STDPAUSE, "terrain");
    make_voice(&voices.too_low_flaps, "too-low-flaps");
    make_voice(&voices.too_low_gear, "too-low-gear");
    make_voice(&voices.too_low_terrain, "too-low-terrain");

    for (unsigned i = 0; i < n_altitude_callouts; i++)
      {
        std::ostringstream name;
        name << "altitude-" << MK_VIII::Mode6Handler::altitude_callout_definitions[i];
        make_voice(&voices.altitude_callouts[i], name.str().c_str());
      }
    speaker.update_configuration();
}

///////////////////////////////////////////////////////////////////////////////
// SelfTestHandler ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool
MK_VIII::SelfTestHandler::_was_here (int position)
{
  if (position > current)
    {
      current = position;
      return false;
    }
  else
    return true;
}

MK_VIII::SelfTestHandler::Action
MK_VIII::SelfTestHandler::sleep (double duration)
{
  Action _action = { ACTION_SLEEP, duration, NULL };
  return _action;
}

MK_VIII::SelfTestHandler::Action
MK_VIII::SelfTestHandler::play (VoicePlayer::Voice *voice)
{
  mk->voice_player.play(voice);
  Action _action = { ACTION_VOICE, 0, NULL };
  return _action;
}

MK_VIII::SelfTestHandler::Action
MK_VIII::SelfTestHandler::discrete_on (bool *discrete, double duration)
{
  *discrete = true;
  return sleep(duration);
}

MK_VIII::SelfTestHandler::Action
MK_VIII::SelfTestHandler::discrete_on_off (bool *discrete, double duration)
{
  *discrete = true;
  Action _action = { ACTION_SLEEP | ACTION_DISCRETE_ON_OFF, duration, discrete };
  return _action;
}

MK_VIII::SelfTestHandler::Action
MK_VIII::SelfTestHandler::discrete_on_off (bool *discrete, VoicePlayer::Voice *voice)
{
  *discrete = true;
  mk->voice_player.play(voice);
  Action _action = { ACTION_VOICE | ACTION_DISCRETE_ON_OFF, 0, discrete };
  return _action;
}

MK_VIII::SelfTestHandler::Action
MK_VIII::SelfTestHandler::done ()
{
  Action _action = { ACTION_DONE, 0, NULL };
  return _action;
}

MK_VIII::SelfTestHandler::Action
MK_VIII::SelfTestHandler::run ()
{
  // Note that "Terrain INOP" and "Terrain NA" are or'ed to the same
  // output discrete (see [SPEC] 6.9.3.5).

#define was_here()		was_here_offset(0)
#define was_here_offset(offset)	_was_here(__LINE__ * 20 + (offset))

  if (! was_here())
    {
      if (mk->configuration_module.state == ConfigurationModule::STATE_INVALID_DATABASE)
	return play(mk_voice(application_data_base_failed));
      else if (mk->configuration_module.state == ConfigurationModule::STATE_INVALID_AIRCRAFT_TYPE)
	return play(mk_voice(configuration_type_invalid));
    }
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return done();

  if (! was_here())
    return discrete_on(&mk_doutput(gpws_inop), 0.7);
  if (! was_here())
    return sleep(0.7);		// W/S INOP
  if (! was_here())
    return discrete_on(&mk_doutput(tad_inop), 0.7);
  if (! was_here())
    {
      mk_doutput(gpws_inop) = false;
      // do not disable tad_inop since we must enable Terrain NA
      // do not disable W/S INOP since we do not emulate it
      return sleep(0.7);	// Terrain NA
    }
  if (! was_here())
    {
      mk_doutput(tad_inop) = false; // disable Terrain NA
      if (mk->io_handler.conf.momentary_flap_override_enabled)
	return discrete_on_off(&mk_doutput(flap_override), 1.0);
    }
  if (! was_here())
    return discrete_on_off(&mk_doutput(audio_on), 1.0);
  if (! was_here())
    {
      if (mk->io_handler.momentary_steep_approach_enabled())
	return discrete_on_off(&mk_doutput(steep_approach), 1.0);
    }

  if (! was_here())
    {
      if (mk_dinput(glideslope_inhibit))
	goto glideslope_end;
      else
	{
	  if (mk->fault_handler.faults[FaultHandler::FAULT_MODE5_INPUTS_INVALID])
	    goto glideslope_inop;
	}
    }
  if (! was_here())
    return discrete_on_off(&mk_doutput(gpws_alert), mk_voice(glideslope));
  if (! was_here())
    return discrete_on_off(&mk_doutput(glideslope_cancel), 0.7);
  goto glideslope_end;
 glideslope_inop:
  if (! was_here())
    return play(mk_voice(glideslope_inop));
 glideslope_end:

  if (! was_here())
    {
      if (mk->fault_handler.faults[FaultHandler::FAULT_MODES14_INPUTS_INVALID])
	goto gpws_inop;
    }
  if (! was_here())
    return discrete_on_off(&mk_doutput(gpws_warning), mk_voice(pull_up));
  goto gpws_end;
 gpws_inop:
  if (! was_here())
    return play(mk_voice(gpws_inop));
 gpws_end:

  if (! was_here())
    {
      if (mk_dinput(self_test))	// proceed to long self test
	goto long_test;
    }
  if (! was_here())
    {
      if (mk->mode6_handler.conf.bank_angle_enabled
	  && mk->fault_handler.faults[FaultHandler::FAULT_BANK_ANGLE_INPUTS_INVALID])
	return play(mk_voice(bank_angle_inop));
    }
  if (! was_here())
    {
      if (mk->mode6_handler.altitude_callouts_enabled()
	  && mk->fault_handler.faults[FaultHandler::FAULT_MODE6_INPUTS_INVALID])
	return play(mk_voice(callouts_inop));
    }
  if (! was_here())
    return done();

 long_test:
  if (! was_here())
    {
      mk_doutput(gpws_inop) = true;
      // do not enable W/S INOP, since we do not emulate it
      mk_doutput(tad_inop) = true; // Terrain INOP, Terrain NA

      return play(mk_voice(sink_rate));
    }
  if (! was_here())
    return play(mk_voice(pull_up));
  if (! was_here())
    return play(mk_voice(terrain));
  if (! was_here())
    return play(mk_voice(pull_up));
  if (! was_here())
    return play(mk_voice(dont_sink));
  if (! was_here())
    return play(mk_voice(too_low_terrain));
  if (! was_here())
    return play(mk->mode4_handler.conf.voice_too_low_gear);
  if (! was_here())
    return play(mk_voice(too_low_flaps));
  if (! was_here())
    return play(mk_voice(too_low_terrain));
  if (! was_here())
    return play(mk_voice(glideslope));
  if (! was_here())
    {
      if (mk->mode6_handler.conf.bank_angle_enabled)
	return play(mk_voice(bank_angle));
    }

  if (! was_here())
    {
      if (! mk->mode6_handler.altitude_callouts_enabled())
	goto callouts_disabled;
    }
  if (! was_here())
    {
      if (mk->mode6_handler.conf.minimums_enabled)
	return play(mk_voice(minimums));
    }
  if (! was_here())
    {
      if (mk->mode6_handler.conf.above_field_voice)
	return play(mk->mode6_handler.conf.above_field_voice);
    }
  for (unsigned i = 0; i < n_altitude_callouts; i++)
    if (! was_here_offset(i))
      {
	if (mk->mode6_handler.conf.altitude_callouts_enabled[i])
	  return play(mk_altitude_voice(i));
      }
  if (! was_here())
    {
      if (mk->mode6_handler.conf.smart_500_enabled)
	return play(mk_altitude_voice(Mode6Handler::ALTITUDE_CALLOUT_500));
    }
  goto callouts_end;
 callouts_disabled:
  if (! was_here())
    return play(mk_voice(minimums_minimums));
 callouts_end:

  if (! was_here())
    {
      if (mk->tcf_handler.conf.enabled)
	return play(mk_voice(too_low_terrain));
    }

  return done();
}

void
MK_VIII::SelfTestHandler::start ()
{
  assert(state == STATE_START);

  memcpy(&saved_outputs, &mk->io_handler.outputs, sizeof(mk->io_handler.outputs));
  memset(&mk->io_handler.outputs, 0, sizeof(mk->io_handler.outputs));

  // [SPEC] 6.10.6: "The self-test results are annunciated, at 6db
  // lower than the normal audio level selected for the aircraft"
  mk->voice_player.set_volume(modify_amplitude(mk->voice_player.conf.volume, -6));

  if (mk_dinput(mode6_low_volume))
    mk->mode6_handler.set_volume(1.0);

  state = STATE_RUNNING;
  cancel = CANCEL_NONE;
  memset(&action, 0, sizeof(action));
  current = 0;
}

void
MK_VIII::SelfTestHandler::stop ()
{
  if (state != STATE_NONE)
    {
      if (state != STATE_START)
	{
	  mk->voice_player.stop(VoicePlayer::STOP_NOW);
	  mk->voice_player.set_volume(mk_dinput(audio_inhibit) ? 0.0 : mk->voice_player.conf.volume);

	  if (mk_dinput(mode6_low_volume))
	    mk->mode6_handler.set_volume(modify_amplitude(1.0, -6));

	  memcpy(&mk->io_handler.outputs, &saved_outputs, sizeof(mk->io_handler.outputs));
	}

      button_pressed = false;
      state = STATE_NONE;
      // reset self-test handler position
      current=0;
    }
}

void
MK_VIII::SelfTestHandler::handle_button_event (bool value)
{
  if (state == STATE_NONE)
    {
      if (value)
	state = STATE_START;
    }
  else if (state == STATE_START)
    {
      if (value)
	state = STATE_NONE;	// cancel the not-yet-started test
    }
  else if (cancel == CANCEL_NONE)
    {
      if (value)
	{
	  assert(! button_pressed);
	  button_pressed = true;
	  button_press_timestamp = globals->get_sim_time_sec();
	}
      else
	{
	  if (button_pressed)
	    {
	      if (globals->get_sim_time_sec() - button_press_timestamp < 2)
		cancel = CANCEL_SHORT;
	      else
		cancel = CANCEL_LONG;
	    }
	}
    }
}

bool
MK_VIII::SelfTestHandler::update ()
{
  if (state == STATE_NONE)
    return false;
  else if (state == STATE_START)
    {
      if (mk->state_handler.ground && ! mk->io_handler.steep_approach())
	start();
      else
	{
	  state = STATE_NONE;
	  return false;
	}
    }
  else
    {
      if (button_pressed && cancel == CANCEL_NONE)
	{
	  if (globals->get_sim_time_sec() - button_press_timestamp >= 2)
	    cancel = CANCEL_LONG;
	}
    }

  if (! mk->state_handler.ground || cancel != CANCEL_NONE)
    {
      stop();
      return false;
    }

  if (test_bits(action.flags, ACTION_SLEEP))
    {
      if (globals->get_sim_time_sec() - sleep_start < action.sleep_duration)
	return true;
    }
  if (test_bits(action.flags, ACTION_VOICE))
    {
      if (mk->voice_player.voice)
	return true;
    }
  if (test_bits(action.flags, ACTION_DISCRETE_ON_OFF))
    *action.discrete = false;

  action = run();

  if (test_bits(action.flags, ACTION_SLEEP))
    sleep_start = globals->get_sim_time_sec();
  if (test_bits(action.flags, ACTION_DONE))
    {
      stop();
      return false;
    }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// AlertHandler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool
MK_VIII::AlertHandler::select_voice_alerts (unsigned int test)
{
  if (has_alerts(test))
    {
      voice_alerts = alerts & test;
      return true;
    }
  else
    {
      voice_alerts &= ~test;
      if (voice_alerts == 0)
	mk->voice_player.stop();

      return false;
    }
}

void
MK_VIII::AlertHandler::boot ()
{
  reset();
}

void
MK_VIII::AlertHandler::reposition ()
{
  reset();

  mk->io_handler.set_lamp(IOHandler::LAMP_NONE);
  mk->voice_player.stop(VoicePlayer::STOP_NOW);
}

void
MK_VIII::AlertHandler::reset ()
{
  alerts = 0;
  old_alerts = 0;
  voice_alerts = 0;
  repeated_alerts = 0;
  altitude_callout_voice = NULL;
}

void
MK_VIII::AlertHandler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  // Lamps and voices are prioritized according to [SPEC] 6.9.2.
  //
  // Voices which can interrupt other voices (VoicePlayer::PLAY_NOW)
  // are specified by [INSTALL] appendix E 5.3.5.
  //
  // When a voice is overriden by a higher priority voice and the
  // overriding voice stops, we restore the previous voice if it was
  // looped (this is not specified by [SPEC] but seems to make sense).

  // update lamp

  if (has_alerts(ALERT_MODE1_PULL_UP | ALERT_MODE2A | ALERT_MODE2B))
    mk->io_handler.set_lamp(IOHandler::LAMP_WARNING);
  else if (has_alerts(ALERT_MODE1_SINK_RATE
		      | ALERT_MODE2A_PREFACE
		      | ALERT_MODE2B_PREFACE
		      | ALERT_MODE2A_ALTITUDE_GAIN_TERRAIN_CLOSING
		      | ALERT_MODE2A_ALTITUDE_GAIN
		      | ALERT_MODE2B_LANDING_MODE
		      | ALERT_MODE3
		      | ALERT_MODE4_TOO_LOW_FLAPS
		      | ALERT_MODE4_TOO_LOW_GEAR
		      | ALERT_MODE4AB_TOO_LOW_TERRAIN
		      | ALERT_MODE4C_TOO_LOW_TERRAIN
		      | ALERT_TCF_TOO_LOW_TERRAIN))
    mk->io_handler.set_lamp(IOHandler::LAMP_CAUTION);
  else if (has_alerts(ALERT_MODE5_SOFT | ALERT_MODE5_HARD))
    mk->io_handler.set_lamp(IOHandler::LAMP_GLIDESLOPE);
  else
    mk->io_handler.set_lamp(IOHandler::LAMP_NONE);

  // update voice

  if (select_voice_alerts(ALERT_MODE1_PULL_UP))
    {
      if (! has_old_alerts(ALERT_MODE1_PULL_UP))
	{
	  if (mk->voice_player.voice != mk_voice(sink_rate_pause_sink_rate))
	    mk->voice_player.play(mk_voice(sink_rate), VoicePlayer::PLAY_NOW);
	  mk->voice_player.play(mk_voice(pull_up), VoicePlayer::PLAY_LOOPED);
	}
    }
  else if (select_voice_alerts(ALERT_MODE2A_PREFACE | ALERT_MODE2B_PREFACE))
    {
      if (! has_old_alerts(ALERT_MODE2A_PREFACE | ALERT_MODE2B_PREFACE))
	mk->voice_player.play(mk_voice(terrain_pause_terrain), VoicePlayer::PLAY_NOW);
    }
  else if (select_voice_alerts(ALERT_MODE2A | ALERT_MODE2B))
    {
      if (mk->voice_player.voice != mk_voice(pull_up))
	mk->voice_player.play(mk_voice(pull_up), VoicePlayer::PLAY_NOW | VoicePlayer::PLAY_LOOPED);
    }
  else if (select_voice_alerts(ALERT_MODE2A_ALTITUDE_GAIN_TERRAIN_CLOSING | ALERT_MODE2B_LANDING_MODE))
    {
      if (mk->voice_player.voice != mk_voice(terrain))
	mk->voice_player.play(mk_voice(terrain), VoicePlayer::PLAY_LOOPED);
    }
  else if (select_voice_alerts(ALERT_MODE6_MINIMUMS))
    {
      if (! has_old_alerts(ALERT_MODE6_MINIMUMS))
	mk->voice_player.play(mk_voice(minimums_minimums));
    }
  else if (select_voice_alerts(ALERT_MODE4AB_TOO_LOW_TERRAIN | ALERT_MODE4C_TOO_LOW_TERRAIN))
    {
      if (must_play_voice(ALERT_MODE4AB_TOO_LOW_TERRAIN | ALERT_MODE4C_TOO_LOW_TERRAIN))
	mk->voice_player.play(mk_voice(too_low_terrain));
    }
  else if (select_voice_alerts(ALERT_TCF_TOO_LOW_TERRAIN))
    {
      if (must_play_voice(ALERT_TCF_TOO_LOW_TERRAIN))
	mk->voice_player.play(mk_voice(too_low_terrain));
    }
  else if (select_voice_alerts(ALERT_MODE6_ALTITUDE_CALLOUT))
    {
      if (! has_old_alerts(ALERT_MODE6_ALTITUDE_CALLOUT))
	{
	  assert(altitude_callout_voice != NULL);
	  mk->voice_player.play(altitude_callout_voice);
	  altitude_callout_voice = NULL;
	}
    }
  else if (select_voice_alerts(ALERT_MODE4_TOO_LOW_GEAR))
    {
      if (must_play_voice(ALERT_MODE4_TOO_LOW_GEAR))
	mk->voice_player.play(mk->mode4_handler.conf.voice_too_low_gear);
    }
  else if (select_voice_alerts(ALERT_MODE4_TOO_LOW_FLAPS))
    {
      if (must_play_voice(ALERT_MODE4_TOO_LOW_FLAPS))
	mk->voice_player.play(mk_voice(too_low_flaps));
    }
  else if (select_voice_alerts(ALERT_MODE1_SINK_RATE))
    {
      if (must_play_voice(ALERT_MODE1_SINK_RATE))
	mk->voice_player.play(mk_voice(sink_rate_pause_sink_rate));
      // [SPEC] 6.2.1: "During the time that the voice message for the
      // outer envelope is inhibited and the caution/warning lamp is
      // on, the Mode 5 alert message is allowed to annunciate for
      // excessive glideslope deviation conditions."
      else if (mk->voice_player.voice != mk_voice(sink_rate_pause_sink_rate)
	       && mk->voice_player.next_voice != mk_voice(sink_rate_pause_sink_rate))
	{
	  if (has_alerts(ALERT_MODE5_HARD))
	    {
	      voice_alerts |= ALERT_MODE5_HARD;
	      if (mk->voice_player.voice != mk_voice(hard_glideslope))
		mk->voice_player.play(mk_voice(hard_glideslope), VoicePlayer::PLAY_LOOPED);
	    }
	  else if (has_alerts(ALERT_MODE5_SOFT))
	    {
	      voice_alerts |= ALERT_MODE5_SOFT;
	      if (must_play_voice(ALERT_MODE5_SOFT))
		mk->voice_player.play(mk_voice(soft_glideslope));
	    }
	}
    }
  else if (select_voice_alerts(ALERT_MODE3))
    {
      if (must_play_voice(ALERT_MODE3))
	mk->voice_player.play(mk_voice(dont_sink_pause_dont_sink));
    }
  else if (select_voice_alerts(ALERT_MODE5_HARD))
    {
      if (mk->voice_player.voice != mk_voice(hard_glideslope))
	mk->voice_player.play(mk_voice(hard_glideslope), VoicePlayer::PLAY_LOOPED);
    }
  else if (select_voice_alerts(ALERT_MODE5_SOFT))
    {
      if (must_play_voice(ALERT_MODE5_SOFT))
	mk->voice_player.play(mk_voice(soft_glideslope));
    }
  else if (select_voice_alerts(ALERT_MODE6_LOW_BANK_ANGLE_3))
    {
      if (mk->voice_player.voice != mk_voice(bank_angle_bank_angle_3))
	mk->voice_player.play(mk_voice(bank_angle_bank_angle_3), VoicePlayer::PLAY_LOOPED);
    }
  else if (select_voice_alerts(ALERT_MODE6_HIGH_BANK_ANGLE_3))
    {
      if (mk->voice_player.voice != mk_voice(bank_angle_pause_bank_angle_3))
	mk->voice_player.play(mk_voice(bank_angle_pause_bank_angle_3), VoicePlayer::PLAY_LOOPED);
    }
  else if (select_voice_alerts(ALERT_MODE6_LOW_BANK_ANGLE_2))
    {
      if (! has_old_alerts(ALERT_MODE6_LOW_BANK_ANGLE_2 | ALERT_MODE6_HIGH_BANK_ANGLE_2))
	mk->voice_player.play(mk_voice(bank_angle_bank_angle));
    }
  else if (select_voice_alerts(ALERT_MODE6_HIGH_BANK_ANGLE_2))
    {
      if (! has_old_alerts(ALERT_MODE6_LOW_BANK_ANGLE_2 | ALERT_MODE6_HIGH_BANK_ANGLE_2))
	mk->voice_player.play(mk_voice(bank_angle_pause_bank_angle));
    }
  else if (select_voice_alerts(ALERT_MODE6_LOW_BANK_ANGLE_1))
    {
      if (! has_old_alerts(ALERT_MODE6_LOW_BANK_ANGLE_1 | ALERT_MODE6_HIGH_BANK_ANGLE_1))
	mk->voice_player.play(mk_voice(bank_angle_bank_angle));
    }
  else if (select_voice_alerts(ALERT_MODE6_HIGH_BANK_ANGLE_1))
    {
      if (! has_old_alerts(ALERT_MODE6_LOW_BANK_ANGLE_1 | ALERT_MODE6_HIGH_BANK_ANGLE_1))
	mk->voice_player.play(mk_voice(bank_angle_pause_bank_angle));
    }

  // remember all alerts voiced so far...
  old_alerts |= voice_alerts;
  // ... forget those no longer active
  old_alerts &= alerts;
  repeated_alerts = 0;
}

void
MK_VIII::AlertHandler::set_alerts (unsigned int _alerts,
				   unsigned int flags,
				   VoicePlayer::Voice *_altitude_callout_voice)
{
  alerts |= _alerts;
  if (test_bits(flags, ALERT_FLAG_REPEAT))
    repeated_alerts |= _alerts;
  if (_altitude_callout_voice)
    altitude_callout_voice = _altitude_callout_voice;
}

void
MK_VIII::AlertHandler::unset_alerts (unsigned int _alerts)
{
  alerts &= ~_alerts;
  repeated_alerts &= ~_alerts;
  if (_alerts & ALERT_MODE6_ALTITUDE_CALLOUT)
    altitude_callout_voice = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// StateHandler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
MK_VIII::StateHandler::update_ground ()
{
  if (ground)
    {
      if (! mk_ainput(computed_airspeed).ncd && mk_ainput(computed_airspeed).get() > 60
	  && ! mk_data(radio_altitude).ncd && mk_data(radio_altitude).get() > 30)
	{
	  if (potentially_airborne_timer.start_or_elapsed() > 1)
	    leave_ground();
	}
      else
	potentially_airborne_timer.stop();
    }
  else
    {
      if (! mk_ainput(computed_airspeed).ncd && mk_ainput(computed_airspeed).get() < 40
	  && ! mk_data(radio_altitude).ncd && mk_data(radio_altitude).get() < 30)
	enter_ground();
    }
}

void
MK_VIII::StateHandler::enter_ground ()
{
  ground = true;
  mk->io_handler.enter_ground();

  // [SPEC] 6.0.1 does not specify this, but it seems to be an
  // omission; at this point, we must make sure that we are in takeoff
  // mode (otherwise the next takeoff might happen in approach mode).
  if (! takeoff)
    enter_takeoff();
}

void
MK_VIII::StateHandler::leave_ground ()
{
  ground = false;
  mk->mode2_handler.leave_ground();
}

void
MK_VIII::StateHandler::update_takeoff ()
{
  if (takeoff)
    {
      // [SPEC] 6.0.2 is somewhat confusing: it mentions hardcoded
      // terrain clearance (500, 750) and airspeed (178, 200) values,
      // but it also mentions the mode 4A boundary, which varies as a
      // function of aircraft type. We consider that the mentioned
      // values are examples, and that we should use the mode 4A upper
      // boundary.

      if (! mk_data(terrain_clearance).ncd
	  && ! mk_ainput(computed_airspeed).ncd
	  && mk_data(terrain_clearance).get() > mk->mode4_handler.get_upper_agl(mk->mode4_handler.conf.modes->ac))
	leave_takeoff();
    }
  else
    {
      if (! mk_data(radio_altitude).ncd
	  && mk_data(radio_altitude).get() < mk->mode4_handler.conf.modes->b->min_agl1
	  && mk->io_handler.flaps_down()
	  && mk_dinput(landing_gear))
	enter_takeoff();
    }
}

void
MK_VIII::StateHandler::enter_takeoff ()
{
  takeoff = true;
  mk->io_handler.enter_takeoff();
  mk->mode2_handler.enter_takeoff();
  mk->mode3_handler.enter_takeoff();
  mk->mode6_handler.enter_takeoff();
}

void
MK_VIII::StateHandler::leave_takeoff ()
{
  takeoff = false;
  mk->mode6_handler.leave_takeoff();
}

void
MK_VIII::StateHandler::post_reposition ()
{
  // Synchronize the state with the current situation.

  bool _ground = !
    (! mk_ainput(computed_airspeed).ncd && mk_ainput(computed_airspeed).get() > 60
     && ! mk_data(radio_altitude).ncd && mk_data(radio_altitude).get() > 30);

  bool _takeoff = _ground;

  if (ground && ! _ground)
    leave_ground();
  else if (! ground && _ground)
    enter_ground();

  if (takeoff && ! _takeoff)
    leave_takeoff();
  else if (! takeoff && _takeoff)
    enter_takeoff();
}

void
MK_VIII::StateHandler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  update_ground();
  update_takeoff();
}

///////////////////////////////////////////////////////////////////////////////
// Mode1Handler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

double
MK_VIII::Mode1Handler::get_pull_up_bias ()
{
  // [PILOT] page 54: "Steep Approach has priority over Flap Override
  // if selected simultaneously."

  if (mk->io_handler.steep_approach())
    return 200;
  else if (conf.envelopes->flap_override_bias && mk->io_handler.flap_override())
    return 300;
  else
    return 0;
}

bool
MK_VIII::Mode1Handler::is_pull_up ()
{
  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && ! mk_data(radio_altitude).ncd
      && ! mk_data(barometric_altitude_rate).ncd
      && mk_data(radio_altitude).get() > conf.envelopes->min_agl)
    {
      if (mk_data(radio_altitude).get() < conf.envelopes->pull_up_max_agl1)
	{
	  if (mk_data(radio_altitude).get() < conf.envelopes->pull_up_min_agl1(mk_data(barometric_altitude_rate).get() + get_pull_up_bias()))
	    return true;
	}
      else if (mk_data(radio_altitude).get() < conf.envelopes->pull_up_max_agl2)
	{
	  if (mk_data(radio_altitude).get() < conf.envelopes->pull_up_min_agl2(mk_data(barometric_altitude_rate).get() + get_pull_up_bias()))
	    return true;
	}
    }

  return false;
}

void
MK_VIII::Mode1Handler::update_pull_up ()
{
  if (is_pull_up())
    {
      if (! mk_test_alert(MODE1_PULL_UP))
	{
	  // [SPEC] 6.2.1: at least one sink rate must be issued
	  // before switching to pull up; if no sink rate has
	  // occurred, a 0.2 second delay is used.
	  if (mk->voice_player.voice == mk_voice(sink_rate_pause_sink_rate)
	      || pull_up_timer.start_or_elapsed() >= 0.2)
	    mk_set_alerts(mk_alert(MODE1_PULL_UP));
	}
    }
  else
    {
      pull_up_timer.stop();
      mk_unset_alerts(mk_alert(MODE1_PULL_UP));
    }
}

double
MK_VIII::Mode1Handler::get_sink_rate_bias ()
{
  // [PILOT] page 54: "Steep Approach has priority over Flap Override
  // if selected simultaneously."

  if (mk->io_handler.steep_approach())
    return 500;
  else if (conf.envelopes->flap_override_bias && mk->io_handler.flap_override())
    return 300;
  else if (! mk_data(glideslope_deviation_dots).ncd)
    {
      double bias = 0;

      if (mk_data(glideslope_deviation_dots).get() <= -2)
	bias = 300;
      else if (mk_data(glideslope_deviation_dots).get() < 0)
	bias = -150 * mk_data(glideslope_deviation_dots).get();

      if (mk_data(radio_altitude).get() < 100)
	bias *= 0.01 * mk_data(radio_altitude).get();

      return bias;
    }
  else
    return 0;
}

bool
MK_VIII::Mode1Handler::is_sink_rate ()
{
  return ! mk->io_handler.gpws_inhibit()
    && ! mk->state_handler.ground // [1]
    && ! mk_data(radio_altitude).ncd
    && ! mk_data(barometric_altitude_rate).ncd
    && mk_data(radio_altitude).get() > conf.envelopes->min_agl
    && mk_data(radio_altitude).get() < 2450
    && mk_data(radio_altitude).get() < -572 - 0.6035 * (mk_data(barometric_altitude_rate).get() + get_sink_rate_bias());
}

double
MK_VIII::Mode1Handler::get_sink_rate_tti ()
{
  return mk_data(radio_altitude).get() / fabs(mk_data(barometric_altitude_rate).get());
}

void
MK_VIII::Mode1Handler::update_sink_rate ()
{
  if (is_sink_rate())
    {
      if (mk_test_alert(MODE1_SINK_RATE))
	{
	  double tti = get_sink_rate_tti();
	  if (tti <= sink_rate_tti * 0.8)
	    {
	      // ~20% degradation, warn again and store the new reference tti
	      sink_rate_tti = tti;
	      mk_repeat_alert(mk_alert(MODE1_SINK_RATE));
	    }
	}
      else
	{
	  if (sink_rate_timer.start_or_elapsed() >= 0.8)
	    {
	      mk_set_alerts(mk_alert(MODE1_SINK_RATE));
	      sink_rate_tti = get_sink_rate_tti();
	    }
	}
    }
  else
    {
      sink_rate_timer.stop();
      mk_unset_alerts(mk_alert(MODE1_SINK_RATE));
    }
}

void
MK_VIII::Mode1Handler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  update_pull_up();
  update_sink_rate();
}

///////////////////////////////////////////////////////////////////////////////
// Mode2Handler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

double
MK_VIII::Mode2Handler::ClosureRateFilter::limit_radio_altitude_rate (double r)
{
  // Limit radio altitude rate according to aircraft configuration,
  // allowing maximum sensitivity during cruise while providing
  // progressively less sensitivity during the landing phases of
  // flight.

  if (! mk_data(glideslope_deviation_dots).ncd && fabs(mk_data(glideslope_deviation_dots).get()) <= 2)
    {				// gs deviation <= +- 2 dots
      if (mk_dinput(landing_gear) && mk->io_handler.flaps_down())
	SG_CLAMP_RANGE(r, -1000.0, 3000.0);
      else if (mk_dinput(landing_gear) || mk->io_handler.flaps_down())
	SG_CLAMP_RANGE(r, 0.0, 4000.0);
      else
	SG_CLAMP_RANGE(r, 1000.0, 5000.0);
    }
  else
    {				// no ILS, or gs deviation > +- 2 dots
      if (mk_dinput(landing_gear) && mk->io_handler.flaps_down())
	SG_CLAMP_RANGE(r, 0.0, 4000.0);
      else if (mk_dinput(landing_gear) || mk->io_handler.flaps_down())
	SG_CLAMP_RANGE(r, 1000.0, 5000.0);
      // else no clamp
    }

  return r;
}

void
MK_VIII::Mode2Handler::ClosureRateFilter::init ()
{
  timer.stop();
  last_ra.set(&mk_data(radio_altitude));
  last_ba.set(&mk_ainput(uncorrected_barometric_altitude));
  ra_filter.reset();
  ba_filter.reset();
  output.unset();
}

void
MK_VIII::Mode2Handler::ClosureRateFilter::update ()
{
  double elapsed = timer.start_or_elapsed();
  if (elapsed < 1)
    return;

  if (! mk_data(radio_altitude).ncd && ! mk_ainput(uncorrected_barometric_altitude).ncd)
    {
      if (! last_ra.ncd && ! last_ba.ncd)
	{
	  // compute radio and barometric altitude rates (positive value = descent)
	  double ra_rate = -(mk_data(radio_altitude).get() - last_ra.get()) / elapsed * 60;
	  double ba_rate = -(mk_ainput(uncorrected_barometric_altitude).get() - last_ba.get()) / elapsed * 60;

	  // limit radio altitude rate according to aircraft configuration
	  ra_rate = limit_radio_altitude_rate(ra_rate);

	  // apply a low-pass filter to the radio altitude rate
	  ra_rate = ra_filter.filter(ra_rate);
	  // apply a high-pass filter to the barometric altitude rate
	  ba_rate = ba_filter.filter(ba_rate);

	  // combine both rates to obtain a closure rate
	  output.set(ra_rate + ba_rate);
	}
      else
	output.unset();
    }
  else
    {
      ra_filter.reset();
      ba_filter.reset();
      output.unset();
    }

  timer.start();
  last_ra.set(&mk_data(radio_altitude));
  last_ba.set(&mk_ainput(uncorrected_barometric_altitude));
}

bool
MK_VIII::Mode2Handler::b_conditions ()
{
  return mk->io_handler.flaps_down()
    || (! mk_data(glideslope_deviation_dots).ncd && fabs(mk_data(glideslope_deviation_dots).get()) < 2)
    || takeoff_timer.running;
}

bool
MK_VIII::Mode2Handler::is_a ()
{
  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && ! mk_data(radio_altitude).ncd
      && ! mk_ainput(computed_airspeed).ncd
      && ! closure_rate_filter.output.ncd
      && ! b_conditions())
    {
      if (mk_data(radio_altitude).get() < 1220)
	{
	  if (mk_data(radio_altitude).get() < -1579 + 0.7895 * closure_rate_filter.output.get())
	    return true;
	}
      else
	{
	  double upper_limit;

	  if (mk_ainput(computed_airspeed).get() <= conf->airspeed1)
	    upper_limit = 1650;
	  else if (mk_ainput(computed_airspeed).get() >= conf->airspeed2)
	    upper_limit = 2450;
	  else
	    upper_limit = 1650 + 8.9 * (mk_ainput(computed_airspeed).get() - conf->airspeed1);

	  if (mk_data(radio_altitude).get() < upper_limit)
	    {
	      if (mk_data(radio_altitude).get() < 522 + 0.1968 * closure_rate_filter.output.get())
		return true;
	    }
	}
    }

  return false;
}

bool
MK_VIII::Mode2Handler::is_b ()
{
  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && ! mk_data(radio_altitude).ncd
      && ! mk_data(barometric_altitude_rate).ncd
      && ! closure_rate_filter.output.ncd
      && b_conditions()
      && mk_data(radio_altitude).get() < 789)
    {
      double lower_limit;

      if (mk->io_handler.flaps_down())
	{
	  if (mk_data(barometric_altitude_rate).get() > -400)
	    lower_limit = 200;
	  else if (mk_data(barometric_altitude_rate).get() < -1000)
	    lower_limit = 600;
	  else
	    lower_limit = -66.777 - 0.667 * mk_data(barometric_altitude_rate).get();
	}
      else
	lower_limit = 30;

      if (mk_data(radio_altitude).get() > lower_limit)
	{
	  if (mk_data(radio_altitude).get() < -1579 + 0.7895 * closure_rate_filter.output.get())
	    return true;
	}
    }

  return false;
}

void
MK_VIII::Mode2Handler::check_pull_up (unsigned int preface_alert,
				      unsigned int alert)
{
  if (pull_up_timer.running)
    {
      if (pull_up_timer.elapsed() >= 1)
	{
	  mk_unset_alerts(preface_alert);
	  mk_set_alerts(alert);
	}
    }
  else
    {
      if (! mk->voice_player.voice)
	pull_up_timer.start();
    }
}

void
MK_VIII::Mode2Handler::update_a ()
{
  if (is_a())
    {
      if (mk_test_alert(MODE2A_PREFACE))
	check_pull_up(mk_alert(MODE2A_PREFACE), mk_alert(MODE2A));
      else if (! mk_test_alert(MODE2A))
	{
	  mk_unset_alerts(mk_alert(MODE2A_ALTITUDE_GAIN) | mk_alert(MODE2A_ALTITUDE_GAIN_TERRAIN_CLOSING));
	  mk_set_alerts(mk_alert(MODE2A_PREFACE));
	  a_start_time = globals->get_sim_time_sec();
	  pull_up_timer.stop();
	}
    }
  else
    {
      if (mk_test_alert(MODE2A_ALTITUDE_GAIN))
	{
	  if (mk->io_handler.gpws_inhibit()
	      || mk->state_handler.ground // [1]
	      || a_altitude_gain_timer.elapsed() >= 45
	      || mk_data(radio_altitude).ncd
	      || mk_ainput(uncorrected_barometric_altitude).ncd
	      || mk_ainput(uncorrected_barometric_altitude).get() - a_altitude_gain_alt >= 300
	      // [PILOT] page 12: "the visual alert will remain on
	      // until [...] landing flaps or the flap override switch
	      // is activated"
	      || mk->io_handler.flaps_down())
	    {
	      // exit altitude gain mode
	      a_altitude_gain_timer.stop();
	      mk_unset_alerts(mk_alert(MODE2A_ALTITUDE_GAIN) | mk_alert(MODE2A_ALTITUDE_GAIN_TERRAIN_CLOSING));
	    }
	  else
	    {
	      // [SPEC] 6.2.2.2: "If the terrain starts to fall away
	      // during this altitude gain time, the voice will shut
	      // off"
	      if (closure_rate_filter.output.get() < 0)
		mk_unset_alerts(mk_alert(MODE2A_ALTITUDE_GAIN_TERRAIN_CLOSING));
	    }
	}
      else if (mk_test_alerts(mk_alert(MODE2A_PREFACE) | mk_alert(MODE2A)))
	{
	  mk_unset_alerts(mk_alert(MODE2A_PREFACE) | mk_alert(MODE2A));

	  if (! mk->io_handler.gpws_inhibit()
	      && ! mk->state_handler.ground // [1]
	      && globals->get_sim_time_sec() - a_start_time > 3
	      && ! mk->io_handler.flaps_down()
	      && ! mk_data(radio_altitude).ncd
	      && ! mk_ainput(uncorrected_barometric_altitude).ncd)
	    {
	      // [SPEC] 6.2.2.2: mode 2A envelope violated for more
	      // than 3 seconds, enable altitude gain feature

	      a_altitude_gain_timer.start();
	      a_altitude_gain_alt = mk_ainput(uncorrected_barometric_altitude).get();

	      unsigned int alerts = mk_alert(MODE2A_ALTITUDE_GAIN);
	      if (closure_rate_filter.output.get() > 0)
		alerts |= mk_alert(MODE2A_ALTITUDE_GAIN_TERRAIN_CLOSING);

	      mk_set_alerts(alerts);
	    }
	}
    }
}

void
MK_VIII::Mode2Handler::update_b ()
{
  bool b = is_b();

  // handle normal mode

  if (b && (! mk_dinput(landing_gear) || ! mk->io_handler.flaps_down()))
    {
      if (mk_test_alert(MODE2B_PREFACE))
	check_pull_up(mk_alert(MODE2B_PREFACE), mk_alert(MODE2B));
      else if (! mk_test_alert(MODE2B))
	{
	  mk_set_alerts(mk_alert(MODE2B_PREFACE));
	  pull_up_timer.stop();
	}
    }
  else
    mk_unset_alerts(mk_alert(MODE2B_PREFACE) | mk_alert(MODE2B));

  // handle landing mode ([SPEC] 6.2.2.3: "If both landing gear and
  // flaps are in the landing configuration, then the message will be
  // Terrain")

  if (b && mk_dinput(landing_gear) && mk->io_handler.flaps_down())
    mk_set_alerts(mk_alert(MODE2B_LANDING_MODE));
  else
    mk_unset_alerts(mk_alert(MODE2B_LANDING_MODE));
}

void
MK_VIII::Mode2Handler::boot ()
{
  closure_rate_filter.init();
}

void
MK_VIII::Mode2Handler::power_off ()
{
  // [SPEC] 6.0.4: "This latching function is not power saved and a
  // system reset will force it false."
  takeoff_timer.stop();
}

void
MK_VIII::Mode2Handler::leave_ground ()
{
  // takeoff, reset timer
  takeoff_timer.start();
}

void
MK_VIII::Mode2Handler::enter_takeoff ()
{
  // reset timer, in case it's a go-around
  takeoff_timer.start();
}

void
MK_VIII::Mode2Handler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  closure_rate_filter.update();

  if (takeoff_timer.running && takeoff_timer.elapsed() >= 60)
    takeoff_timer.stop();

  update_a();
  update_b();
}

///////////////////////////////////////////////////////////////////////////////
// Mode3Handler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

double
MK_VIII::Mode3Handler::max_alt_loss (double _bias)
{
  return conf->max_alt_loss(mk->io_handler.flap_override(), mk_data(radio_altitude).get()) + mk_data(radio_altitude).get() * _bias;
}

double
MK_VIII::Mode3Handler::get_bias (double initial_bias, double alt_loss)
{
  // do not repeat altitude-loss alerts below 30ft agl
  if (mk_data(radio_altitude).get() > 30)
  {
    if (initial_bias < 0.0) // sanity check
      initial_bias = 0.0;
    // mk-viii spec: repeat alerts whenever losing 20% of initial altitude
    while ((alt_loss > max_alt_loss(initial_bias))&&
           (initial_bias < 1.0))
      initial_bias += 0.2;
  }

  return initial_bias;
}

bool
MK_VIII::Mode3Handler::is (double *alt_loss)
{
  if (has_descent_alt)
    {
      int max_agl = conf->max_agl(mk->io_handler.flap_override());

      if (mk_ainput(uncorrected_barometric_altitude).ncd
	  || mk_ainput(uncorrected_barometric_altitude).get() > descent_alt
	  || mk_data(radio_altitude).ncd
	  || mk_data(radio_altitude).get() > max_agl)
	{
	  armed = false;
	  has_descent_alt = false;
	}
      else
	{
	  // gear/flaps: [SPEC] 1.3.1.3
	  if (! mk->io_handler.gpws_inhibit()
	      && ! mk->state_handler.ground // [1]
	      && (! mk_dinput(landing_gear) || ! mk->io_handler.flaps_down())
	      && ! mk_data(barometric_altitude_rate).ncd
	      && ! mk_ainput(uncorrected_barometric_altitude).ncd
	      && ! mk_data(radio_altitude).ncd
	      && mk_data(barometric_altitude_rate).get() < 0)
	    {
	      double _alt_loss = descent_alt - mk_ainput(uncorrected_barometric_altitude).get();
	      if (armed || (mk_data(radio_altitude).get() > conf->min_agl
			    && mk_data(radio_altitude).get() < max_agl
			    && _alt_loss > max_alt_loss(0)))
		{
		  *alt_loss = _alt_loss;
		  return true;
		}
	    }
	}
    }
  else
    {
      if (! mk_data(barometric_altitude_rate).ncd
	  && ! mk_ainput(uncorrected_barometric_altitude).ncd
	  && mk_data(barometric_altitude_rate).get() < 0)
	{
	  has_descent_alt = true;
	  descent_alt = mk_ainput(uncorrected_barometric_altitude).get();
	}
    }

  return false;
}

void
MK_VIII::Mode3Handler::enter_takeoff ()
{
  armed = false;
  has_descent_alt = false;
}

void
MK_VIII::Mode3Handler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  if (mk->state_handler.takeoff)
    {
      double alt_loss;

      if (! mk->state_handler.ground /* [1] */ && is(&alt_loss))
	{
	  if (mk_test_alert(MODE3))
	    {
	      double new_bias = get_bias(bias, alt_loss);
	      if (new_bias > bias)
		{
		  bias = new_bias;
		  mk_repeat_alert(mk_alert(MODE3));
		}
	    }
	  else
	    {
	      armed = true;
	      bias = get_bias(0.2, alt_loss);
	      mk_set_alerts(mk_alert(MODE3));
	    }

	  return;
	}
    }

  mk_unset_alerts(mk_alert(MODE3));
}

///////////////////////////////////////////////////////////////////////////////
// Mode4Handler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// FIXME: for turbofans, the upper limit should be reduced from 1000
// to 800 ft if a sudden change in radio altitude is detected, in
// order to reduce nuisance alerts caused by overflying another
// aircraft (see [PILOT] page 16).

double
MK_VIII::Mode4Handler::get_upper_agl (const EnvelopesConfiguration *c)
{
  if (mk_ainput(computed_airspeed).get() >= c->airspeed2)
    return c->min_agl3;
  else if (mk_ainput(computed_airspeed).get() >= c->airspeed1)
    return c->min_agl2(mk_ainput(computed_airspeed).get());
  else
    return c->min_agl1;
}

const MK_VIII::Mode4Handler::EnvelopesConfiguration *
MK_VIII::Mode4Handler::get_ab_envelope ()
{
  // [SPEC] 6.2.4.1: "The Mode 4B envelope can also be selected by
  // setting flaps to landing configuration or by selecting flap
  // override."
  return mk_dinput(landing_gear) || mk->io_handler.flaps_down()
    ? conf.modes->b
    : conf.modes->ac;
}

double
MK_VIII::Mode4Handler::get_bias (double initial_bias, double min_agl)
{
  // do not repeat terrain/gear/flap alerts below 30ft agl
  if (mk_data(radio_altitude).get() > 30.0)
  {
    if (initial_bias < 0.0) // sanity check
      initial_bias = 0.0;
    while ((mk_data(radio_altitude).get() < min_agl - min_agl * initial_bias)&&
           (initial_bias < 1.0))
      initial_bias += 0.2;
  }

  return initial_bias;
}

void
MK_VIII::Mode4Handler::handle_alert (unsigned int alert,
				     double min_agl,
				     double *bias)
{
  if (mk_test_alerts(alert))
    {
      double new_bias = get_bias(*bias, min_agl);
      if (new_bias > *bias)
	{
	  *bias = new_bias;
	  mk_repeat_alert(alert);
	}
    }
  else
    {
      *bias = get_bias(0.2, min_agl);
      mk_set_alerts(alert);
    }
}

void
MK_VIII::Mode4Handler::update_ab ()
{
  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground
      && ! mk->state_handler.takeoff
      && ! mk_data(radio_altitude).ncd
      && ! mk_ainput(computed_airspeed).ncd
      && mk_data(radio_altitude).get() > 30)
    {
      const EnvelopesConfiguration *c = get_ab_envelope();
      if (mk_ainput(computed_airspeed).get() < c->airspeed1)
	{
	  if (mk_dinput(landing_gear))
	    {
	      if (! mk->io_handler.flaps_down() && mk_data(radio_altitude).get() < c->min_agl1)
		{
		  handle_alert(mk_alert(MODE4_TOO_LOW_FLAPS), c->min_agl1, &ab_bias);
		  return;
		}
	    }
	  else
	    {
	      if (mk_data(radio_altitude).get() < c->min_agl1)
		{
		  handle_alert(mk_alert(MODE4_TOO_LOW_GEAR), c->min_agl1, &ab_bias);
		  return;
		}
	    }
	}
    }

  mk_unset_alerts(mk_alert(MODE4_TOO_LOW_FLAPS) | mk_alert(MODE4_TOO_LOW_GEAR));
  ab_bias=0.0;
}

void
MK_VIII::Mode4Handler::update_ab_expanded ()
{
  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground
      && ! mk->state_handler.takeoff
      && ! mk_data(radio_altitude).ncd
      && ! mk_ainput(computed_airspeed).ncd
      && mk_data(radio_altitude).get() > 30)
    {
      const EnvelopesConfiguration *c = get_ab_envelope();
      if (mk_ainput(computed_airspeed).get() >= c->airspeed1)
	{
	  double min_agl = get_upper_agl(c);
	  if (mk_data(radio_altitude).get() < min_agl)
	    {
	      handle_alert(mk_alert(MODE4AB_TOO_LOW_TERRAIN), min_agl, &ab_expanded_bias);
	      return;
	    }
	}
    }

  mk_unset_alerts(mk_alert(MODE4AB_TOO_LOW_TERRAIN));
  ab_expanded_bias=0.0;
}

void
MK_VIII::Mode4Handler::update_c ()
{
  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && mk->state_handler.takeoff
      && ! mk_data(radio_altitude).ncd
      && ! mk_data(terrain_clearance).ncd
      && mk_data(radio_altitude).get() > 30
      && (! mk_dinput(landing_gear) || ! mk->io_handler.flaps_down())
      && mk_data(radio_altitude).get() < get_upper_agl(conf.modes->ac)
      && mk_data(radio_altitude).get() < mk_data(terrain_clearance).get())
    handle_alert(mk_alert(MODE4C_TOO_LOW_TERRAIN), mk_data(terrain_clearance).get(), &c_bias);
  else
  {
    mk_unset_alerts(mk_alert(MODE4C_TOO_LOW_TERRAIN));
    c_bias=0.0;
  }
}

void
MK_VIII::Mode4Handler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  update_ab();
  update_ab_expanded();
  update_c();
}

///////////////////////////////////////////////////////////////////////////////
// Mode5Handler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool
MK_VIII::Mode5Handler::is_hard ()
{
  if (mk_data(radio_altitude).get() > 30
      && mk_data(radio_altitude).get() < 300
      && mk_data(glideslope_deviation_dots).get() > 2)
    {
      if (mk_data(radio_altitude).get() < 150)
	{
	  if (mk_data(radio_altitude).get() > 293 - 71.43 * mk_data(glideslope_deviation_dots).get())
	    return true;
	}
      else
	return true;
    }

  return false;
}

bool
MK_VIII::Mode5Handler::is_soft (double bias)
{
  // do not repeat glide-slope alerts below 30ft agl
  if (mk_data(radio_altitude).get() > 30)
    {
      double bias_dots = 1.3 * bias;
      if (mk_data(glideslope_deviation_dots).get() > 1.3 + bias_dots)
	{
	  if (mk_data(radio_altitude).get() < 150)
	    {
	      if (mk_data(radio_altitude).get() > 243 - 71.43 * (mk_data(glideslope_deviation_dots).get() - bias_dots))
		return true;
	    }
	  else
	    {
	      double upper_limit;

	      if (mk_data(barometric_altitude_rate).ncd)
		upper_limit = 1000;
	      else
		{
		  if (mk_data(barometric_altitude_rate).get() >= 0)
		    upper_limit = 500;
		  else if (mk_data(barometric_altitude_rate).get() < -500)
		    upper_limit = 1000;
		  else
		    upper_limit = -mk_data(barometric_altitude_rate).get() + 500;
		}

	      if (mk_data(radio_altitude).get() < upper_limit)
		return true;
	    }
	}
    }

  return false;
}

double
MK_VIII::Mode5Handler::get_soft_bias (double initial_bias)
{
  if (initial_bias < 0.0) // sanity check
    initial_bias = 0.0;
  while ((is_soft(initial_bias))&&
         (initial_bias < 1.0))
    initial_bias += 0.2;

  return initial_bias;
}

void
MK_VIII::Mode5Handler::update_hard (bool is)
{
  if (is)
    {
      if (! mk_test_alert(MODE5_HARD))
	{
	  if (hard_timer.start_or_elapsed() >= 0.8)
	    mk_set_alerts(mk_alert(MODE5_HARD));
	}
    }
  else
    {
      hard_timer.stop();
      mk_unset_alerts(mk_alert(MODE5_HARD));
    }
}

void
MK_VIII::Mode5Handler::update_soft (bool is)
{
  if (is)
    {
      if (! mk_test_alert(MODE5_SOFT))
	{
	  double new_bias = get_soft_bias(soft_bias);
	  if (new_bias > soft_bias)
	    {
	      soft_bias = new_bias;
	      mk_repeat_alert(mk_alert(MODE5_SOFT));
	    }
	}
      else
	{
	  if (soft_timer.start_or_elapsed() >= 0.8)
	    {
	      soft_bias = get_soft_bias(0.2);
	      mk_set_alerts(mk_alert(MODE5_SOFT));
	    }
	}
    }
  else
    {
      soft_timer.stop();
      mk_unset_alerts(mk_alert(MODE5_SOFT));
    }
}

void
MK_VIII::Mode5Handler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && ! mk_dinput(glideslope_inhibit) // not on backcourse
      && ! mk_data(radio_altitude).ncd
      && ! mk_data(glideslope_deviation_dots).ncd
      && (! mk->io_handler.conf.localizer_enabled
	  || mk_data(localizer_deviation_dots).ncd
	  || mk_data(radio_altitude).get() < 500
	  || fabs(mk_data(localizer_deviation_dots).get()) < 2)
      && (! mk->state_handler.takeoff || mk->io_handler.flaps_down())
      && mk_dinput(landing_gear)
      && ! mk_doutput(glideslope_cancel))
    {
      update_hard(is_hard());
      update_soft(is_soft(0));
    }
  else
    {
      update_hard(false);
      update_soft(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Mode6Handler ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// keep sorted in descending order
const int MK_VIII::Mode6Handler::altitude_callout_definitions[] = 
  { 1000, 500, 400, 300, 200, 100, 50, 40, 30, 20, 10 };

void
MK_VIII::Mode6Handler::reset_minimums ()
{
  minimums_issued = false;
}

void
MK_VIII::Mode6Handler::reset_altitude_callouts ()
{
  for (unsigned i = 0; i < n_altitude_callouts; i++)
    altitude_callouts_issued[i] = false;
}

bool
MK_VIII::Mode6Handler::is_playing_altitude_callout ()
{
  for (unsigned i = 0; i < n_altitude_callouts; i++)
    if (mk->voice_player.voice == mk_altitude_voice(i)
	|| mk->voice_player.next_voice == mk_altitude_voice(i))
      return true;

  return false;
}

bool
MK_VIII::Mode6Handler::is_near_minimums (double callout)
{
  // [SPEC] 6.4.2

  if (! mk_data(decision_height).ncd)
    {
      double diff = callout - mk_data(decision_height).get();

      if (mk_data(radio_altitude).get() >= 200)
	{
	  if (fabs(diff) <= 30)
	    return true;
	}
      else
	{
	  if (diff >= -3 && diff <= 6)
	    return true;
	}
    }

  return false;
}

bool
MK_VIII::Mode6Handler::is_outside_band (double elevation, double callout)
{
  // [SPEC] 6.4.2
  return elevation < callout - (elevation > 150 ? 20 : 10);
}

bool
MK_VIII::Mode6Handler::inhibit_smart_500 ()
{
  // [SPEC] 6.4.3

  if (! mk_data(glideslope_deviation_dots).ncd
      && ! mk_dinput(glideslope_inhibit) // backcourse
      && ! mk_doutput(glideslope_cancel))
    {
      if (mk->io_handler.conf.localizer_enabled
	  && ! mk_data(localizer_deviation_dots).ncd)
	{
	  if (fabs(mk_data(localizer_deviation_dots).get()) <= 2)
	    return true;
	}
      else
	{
	  if (fabs(mk_data(glideslope_deviation_dots).get()) <= 2)
	    return true;
	}
    }

  return false;
}

void
MK_VIII::Mode6Handler::boot ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  last_decision_height = mk_dinput(decision_height);
  last_radio_altitude.set(&mk_data(radio_altitude));

  // [SPEC] 6.4.2
  for (unsigned i = 0; i < n_altitude_callouts; i++)
    altitude_callouts_issued[i] = ! mk_data(radio_altitude).ncd
      && mk_data(radio_altitude).get() <= altitude_callout_definitions[i];

  // extrapolated from [SPEC] 6.4.2
  minimums_issued = mk_dinput(decision_height);

  if (conf.above_field_voice)
    {
      update_runway();
      get_altitude_above_field(&last_altitude_above_field);
      // extrapolated from [SPEC] 6.4.2
      above_field_issued = ! last_altitude_above_field.ncd
	&& last_altitude_above_field.get() < 550;
    }
}

void
MK_VIII::Mode6Handler::power_off ()
{
  runway_timer.stop();
}

void
MK_VIII::Mode6Handler::enter_takeoff ()
{
  reset_altitude_callouts();	// [SPEC] 6.4.2
  reset_minimums();		// omitted by [SPEC]; common sense
}

void
MK_VIII::Mode6Handler::leave_takeoff ()
{
  reset_altitude_callouts();	// [SPEC] 6.0.2
  reset_minimums();		// [SPEC] 6.0.2
}

void
MK_VIII::Mode6Handler::set_volume (float volume)
{
  mk_voice(minimums_minimums)->set_volume(volume);
  mk_voice(five_hundred_above)->set_volume(volume);
  for (unsigned i = 0; i < n_altitude_callouts; i++)
    mk_altitude_voice(i)->set_volume(volume);
}

bool
MK_VIII::Mode6Handler::altitude_callouts_enabled ()
{
  if (conf.minimums_enabled || conf.smart_500_enabled || conf.above_field_voice)
    return true;

  for (unsigned i = 0; i < n_altitude_callouts; i++)
    if (conf.altitude_callouts_enabled[i])
      return true;

  return false;
}

void
MK_VIII::Mode6Handler::update_minimums ()
{
  if (! mk->io_handler.gpws_inhibit()
      && (mk->voice_player.voice == mk_voice(minimums_minimums)
	  || mk->voice_player.next_voice == mk_voice(minimums_minimums)))
    goto end;

  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && conf.minimums_enabled
      && ! minimums_issued
      && mk_dinput(landing_gear)
      && mk_dinput(decision_height)
      && ! last_decision_height)
    {
      minimums_issued = true;

      // If an altitude callout is playing, stop it now so that the
      // minimums callout can be played (note that proper connection
      // and synchronization of the radio-altitude ARINC 529 input,
      // decision-height discrete and decision-height ARINC 529 input
      // will prevent an altitude callout from being played near the
      // decision height).

      if (is_playing_altitude_callout())
	mk->voice_player.stop(VoicePlayer::STOP_NOW);

      mk_set_alerts(mk_alert(MODE6_MINIMUMS));
    }
  else
    mk_unset_alerts(mk_alert(MODE6_MINIMUMS));

 end:
  last_decision_height = mk_dinput(decision_height);
}

void
MK_VIII::Mode6Handler::update_altitude_callouts ()
{
  if (! mk->io_handler.gpws_inhibit() && is_playing_altitude_callout())
    goto end;

  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && ! mk_data(radio_altitude).ncd)
    for (unsigned i = 0; i < n_altitude_callouts && mk_data(radio_altitude).get() <= altitude_callout_definitions[i]; i++)
      if ((conf.altitude_callouts_enabled[i]
	   || (altitude_callout_definitions[i] == 500
	       && conf.smart_500_enabled))
	  && ! altitude_callouts_issued[i]
	  && (last_radio_altitude.ncd
	      || last_radio_altitude.get() > altitude_callout_definitions[i]))
	{
	  // lock out all callouts superior or equal to this one
	  for (unsigned j = 0; j <= i; j++)
	    altitude_callouts_issued[j] = true;

	  altitude_callouts_issued[i] = true;
	  if (! is_near_minimums(altitude_callout_definitions[i])
	      && ! is_outside_band(mk_data(radio_altitude).get(), altitude_callout_definitions[i])
	      && (! conf.smart_500_enabled
		  || altitude_callout_definitions[i] != 500
		  || ! inhibit_smart_500()))
	    {
	      mk->alert_handler.set_altitude_callout_alert(mk_altitude_voice(i));
	      goto end;
	    }
	}

  mk_unset_alerts(mk_alert(MODE6_ALTITUDE_CALLOUT));

 end:
  last_radio_altitude.set(&mk_data(radio_altitude));
}

bool
MK_VIII::Mode6Handler::test_runway (const FGRunway *_runway)
{
  if (_runway->lengthFt() < mk->conf.runway_database)
    return false;

  SGGeod pos(
    SGGeod::fromDeg(mk_data(gps_longitude).get(), mk_data(gps_latitude).get()));
  
  // get distance to threshold
  double distance, az1, az2;
  SGGeodesy::inverse(pos, _runway->threshold(), az1, az2, distance);
  return distance * SG_METER_TO_NM <= 5;
}

bool
MK_VIII::Mode6Handler::test_airport (const FGAirport *airport)
{
  for (unsigned int r=0; r<airport->numRunways(); ++r) {
    FGRunway* rwy(airport->getRunwayByIndex(r));
    
    if (test_runway(rwy)) return true;
  }

  return false;
}

bool MK_VIII::Mode6Handler::AirportFilter::passAirport(FGAirport* a) const
{
  bool ok = self->test_airport(a);
  return ok;
}

void
MK_VIII::Mode6Handler::update_runway ()
{
 
  if (mk_data(gps_latitude).ncd || mk_data(gps_longitude).ncd) {
     has_runway.unset();
     return;
  }

  // Search for the closest runway threshold in range 5
  // nm. Passing 30nm to
  // get_closest_airport() provides enough margin for large
  // airports, which may have a runway located far away from the
  // airport's reference point.
  AirportFilter filter(this);
  FGPositionedRef apt = FGPositioned::findClosest(
    SGGeod::fromDeg(mk_data(gps_longitude).get(), mk_data(gps_latitude).get()),
    30.0, &filter);
  if (apt) {
    runway.elevation = apt->elevation();
  }
  
  has_runway.set(apt != NULL);
}

void
MK_VIII::Mode6Handler::get_altitude_above_field (Parameter<double> *parameter)
{
  if (! has_runway.ncd && has_runway.get() && ! mk_data(geometric_altitude).ncd)
    parameter->set(mk_data(geometric_altitude).get() - runway.elevation);
  else
    parameter->unset();
}

void
MK_VIII::Mode6Handler::update_above_field_callout ()
{
  if (! conf.above_field_voice)
    return;

  // update_runway() has to iterate over the whole runway database
  // (which contains thousands of runways), so it would be unwise to
  // run it every frame. Run it every 3 seconds. Note that the first
  // iteration is run in boot().
  if (runway_timer.start_or_elapsed() >= 3)
    {
      update_runway();
      runway_timer.start();
    }

  Parameter<double> altitude_above_field;
  get_altitude_above_field(&altitude_above_field);

  if (! mk->io_handler.gpws_inhibit()
      && (mk->voice_player.voice == conf.above_field_voice
	  || mk->voice_player.next_voice == conf.above_field_voice))
    goto end;

  // handle reset term
  if (above_field_issued)
    {
      if ((! has_runway.ncd && ! has_runway.get())
	  || (! altitude_above_field.ncd && altitude_above_field.get() > 700))
	above_field_issued = false;
    }

  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && ! above_field_issued
      && ! altitude_above_field.ncd
      && altitude_above_field.get() < 550
      && (last_altitude_above_field.ncd
	  || last_altitude_above_field.get() >= 550))
    {
      above_field_issued = true;

      if (! is_outside_band(altitude_above_field.get(), 500))
	{
	  mk->alert_handler.set_altitude_callout_alert(conf.above_field_voice);
	  goto end;
	}
    }

  mk_unset_alerts(mk_alert(MODE6_ALTITUDE_CALLOUT));

 end:
  last_altitude_above_field.set(&altitude_above_field);
}

bool
MK_VIII::Mode6Handler::is_bank_angle (double abs_roll_angle, double bias)
{
  return conf.is_bank_angle(&mk_data(radio_altitude),
			    abs_roll_angle - abs_roll_angle * bias,
			    mk_dinput(autopilot_engaged));
}

bool
MK_VIII::Mode6Handler::is_high_bank_angle ()
{
  return mk_data(radio_altitude).ncd || mk_data(radio_altitude).get() >= 210;
}

unsigned int
MK_VIII::Mode6Handler::get_bank_angle_alerts ()
{
  if (! mk->io_handler.gpws_inhibit()
      && ! mk->state_handler.ground // [1]
      && ! mk_data(roll_angle).ncd)
    {
      double abs_roll_angle = fabs(mk_data(roll_angle).get());

      if (is_bank_angle(abs_roll_angle, 0.4))
	return is_high_bank_angle()
	  ? (mk_alert(MODE6_HIGH_BANK_ANGLE_1) | mk_alert(MODE6_HIGH_BANK_ANGLE_2) | mk_alert(MODE6_HIGH_BANK_ANGLE_3))
	  : (mk_alert(MODE6_LOW_BANK_ANGLE_1) | mk_alert(MODE6_LOW_BANK_ANGLE_2) | mk_alert(MODE6_LOW_BANK_ANGLE_3));
      else if (is_bank_angle(abs_roll_angle, 0.2))
	return is_high_bank_angle()
	  ? (mk_alert(MODE6_HIGH_BANK_ANGLE_1) | mk_alert(MODE6_HIGH_BANK_ANGLE_2))
	  : (mk_alert(MODE6_LOW_BANK_ANGLE_1) | mk_alert(MODE6_LOW_BANK_ANGLE_2));
      else if (is_bank_angle(abs_roll_angle, 0))
	return is_high_bank_angle()
	  ? mk_alert(MODE6_HIGH_BANK_ANGLE_1)
	  : mk_alert(MODE6_LOW_BANK_ANGLE_1);
    }

  return 0;
}

void
MK_VIII::Mode6Handler::update_bank_angle ()
{
  if (! conf.bank_angle_enabled)
    return;

  unsigned int alerts = get_bank_angle_alerts();

  // [SPEC] 6.4.4 specifies that the non-continuous alerts
  // (MODE6_*_BANK_ANGLE_1 and MODE6_*_BANK_ANGLE_2) are locked out
  // until the initial envelope is exited.

  if (! test_bits(alerts, mk_alert(MODE6_LOW_BANK_ANGLE_3)))
    mk_unset_alerts(mk_alert(MODE6_LOW_BANK_ANGLE_3));
  if (! test_bits(alerts, mk_alert(MODE6_HIGH_BANK_ANGLE_3)))
    mk_unset_alerts(mk_alert(MODE6_HIGH_BANK_ANGLE_3));

  if (alerts != 0)
    mk_set_alerts(alerts);
  else
    mk_unset_alerts(mk_alert(MODE6_LOW_BANK_ANGLE_1)
		    | mk_alert(MODE6_HIGH_BANK_ANGLE_1)
		    | mk_alert(MODE6_LOW_BANK_ANGLE_2)
		    | mk_alert(MODE6_HIGH_BANK_ANGLE_2));
}

void
MK_VIII::Mode6Handler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK)
    return;

  if (! mk->state_handler.takeoff
      && ! mk_data(radio_altitude).ncd
      && mk_data(radio_altitude).get() > 1000)
    {
      reset_altitude_callouts();	// [SPEC] 6.4.2
      reset_minimums();			// common sense
    }

  update_minimums();
  update_altitude_callouts();
  update_above_field_callout();
  update_bank_angle();
}

///////////////////////////////////////////////////////////////////////////////
// TCFHandler /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Gets the difference between the azimuth from @from_lat,@from_lon to
// @to_lat,@to_lon, and @to_heading, in degrees.
double
MK_VIII::TCFHandler::get_azimuth_difference (double from_lat,
					     double from_lon,
					     double to_lat,
					     double to_lon,
					     double to_heading)
{
  double az1, az2, distance;
  geo_inverse_wgs_84(0, from_lat, from_lon, to_lat, to_lon, &az1, &az2, &distance);
  return get_heading_difference(az1, to_heading);
}

// Gets the difference between the azimuth from the current GPS
// position to the center of @_runway, and the heading of @_runway, in
// degrees.
double
MK_VIII::TCFHandler::get_azimuth_difference (const FGRunway *_runway)
{
  return get_azimuth_difference(mk_data(gps_latitude).get(),
				mk_data(gps_longitude).get(),
				_runway->latitude(),
				_runway->longitude(),
				_runway->headingDeg());
}

// Selects the most likely intended destination runway of @airport,
// and returns it in @_runway. For each runway, the difference between
// the azimuth from the current GPS position to the center of the
// runway and its heading is computed. The runway having the smallest
// difference wins.
//
// This selection algorithm is not specified in [SPEC], but
// http://www.egpws.com/general_information/description/runway_select.htm
// talks about automatic runway selection.
FGRunway*
MK_VIII::TCFHandler::select_runway (const FGAirport *airport)
{
  FGRunway* _runway = 0;
  double min_diff = 360;
  
  for (unsigned int r=0; r<airport->numRunways(); ++r) {
    FGRunway* rwy(airport->getRunwayByIndex(r));
    double diff = get_azimuth_difference(rwy);
    if (diff < min_diff)
	  {
      min_diff = diff;
      _runway = rwy;
    }
  } // of airport runways iteration
  return _runway;
}

bool MK_VIII::TCFHandler::AirportFilter::passAirport(FGAirport* aApt) const
{
  return aApt->hasHardRunwayOfLengthFt(mk->conf.runway_database);
}
   
void
MK_VIII::TCFHandler::update_runway ()
{
  has_runway = false;
  if (mk_data(gps_latitude).ncd || mk_data(gps_longitude).ncd) {
    return;
  }

  // Search for the intended destination runway of the closest
  // airport in range 15 nm. Passing 30nm to get_closest_airport() 
  // provides enough margin for
  // large airports, which may have a runway located far away from
  // the airport's reference point.
  AirportFilter filter(mk);
  FGAirport* apt = FGAirport::findClosest(
    SGGeod::fromDeg(mk_data(gps_longitude).get(), mk_data(gps_latitude).get()),
    30.0, &filter);
      
  if (!apt) return;
  
	  FGRunway* _runway = select_runway(apt);
    
  if (!_runway) return;

	  has_runway = true;

	  runway.center.latitude = _runway->latitude();
	  runway.center.longitude = _runway->longitude();

	  runway.elevation = apt->elevation();

	  double half_length_m = _runway->lengthM() * 0.5;
	  runway.half_length = half_length_m * SG_METER_TO_NM;

	  //        b3 ________________ b0
	  //          |                |
	  //    h1>>> |  e1<<<<<<<<e0  | <<<h0
	  //          |________________|
	  //        b2                  b1

	  // get heading to runway threshold (h0) and end (h1)
	  runway.edges[0].heading = _runway->headingDeg();
	  runway.edges[1].heading = get_reciprocal_heading(_runway->headingDeg());

	  double az;

	  // get position of runway threshold (e0)
	  geo_direct_wgs_84(0,
			    runway.center.latitude,
			    runway.center.longitude,
			    runway.edges[1].heading,
			    half_length_m,
			    &runway.edges[0].position.latitude,
			    &runway.edges[0].position.longitude,
			    &az);

	  // get position of runway end (e1)
	  geo_direct_wgs_84(0,
			    runway.center.latitude,
			    runway.center.longitude,
			    runway.edges[0].heading,
			    half_length_m,
			    &runway.edges[1].position.latitude,
			    &runway.edges[1].position.longitude,
			    &az);

	  double half_width_m = _runway->widthM() * 0.5;

	  // get position of threshold bias area edges (b0 and b1)
	  get_bias_area_edges(&runway.edges[0].position,
			      runway.edges[1].heading,
			      half_width_m,
			      &runway.bias_area[0],
			      &runway.bias_area[1]);

	  // get position of end bias area edges (b2 and b3)
	  get_bias_area_edges(&runway.edges[1].position,
			      runway.edges[0].heading,
			      half_width_m,
			      &runway.bias_area[2],
			      &runway.bias_area[3]);
}

void
MK_VIII::TCFHandler::get_bias_area_edges (Position *edge,
					  double reciprocal,
					  double half_width_m,
					  Position *bias_edge1,
					  Position *bias_edge2)
{
  double half_bias_width_m = k * SG_NM_TO_METER + half_width_m;
  double tmp_latitude = 0.0, tmp_longitude = 0.0, az = 0.0;

  geo_direct_wgs_84(0,
		    edge->latitude,
		    edge->longitude,
		    reciprocal,
		    k * SG_NM_TO_METER,
		    &tmp_latitude,
		    &tmp_longitude,
		    &az);
  geo_direct_wgs_84(0,
		    tmp_latitude,
		    tmp_longitude,
		    heading_substract(reciprocal, 90),
		    half_bias_width_m,
		    &bias_edge1->latitude,
		    &bias_edge1->longitude,
		    &az);
  geo_direct_wgs_84(0,
		    tmp_latitude,
		    tmp_longitude,
		    heading_add(reciprocal, 90),
		    half_bias_width_m,
		    &bias_edge2->latitude,
		    &bias_edge2->longitude,
		    &az);
}

// Returns true if the current GPS position is inside the edge
// triangle of @edge. The edge triangle, which is specified and
// represented in [SPEC] 6.3.1, is defined as an isocele right
// triangle of infinite height, whose right angle is located at the
// position of @edge, and whose height is parallel to the heading of
// @edge.
bool
MK_VIII::TCFHandler::is_inside_edge_triangle (RunwayEdge *edge)
{
  return get_azimuth_difference(mk_data(gps_latitude).get(),
				mk_data(gps_longitude).get(),
				edge->position.latitude,
				edge->position.longitude,
				edge->heading) <= 45;
}

// Returns true if the current GPS position is inside the bias area of
// the currently selected runway.
bool
MK_VIII::TCFHandler::is_inside_bias_area ()
{
  double az1[4];
  double angles_sum = 0;

  for (int i = 0; i < 4; i++)
    {
      double az2, distance;
      geo_inverse_wgs_84(0,
			 mk_data(gps_latitude).get(),
			 mk_data(gps_longitude).get(),
			 runway.bias_area[i].latitude,
			 runway.bias_area[i].longitude,
			 &az1[i], &az2, &distance);
    }

  for (int i = 0; i < 4; i++)
    {
      double angle = az1[i == 3 ? 0 : i + 1] - az1[i];
      if (angle < -180)
	angle += 360;

      angles_sum += angle;
    }

  return angles_sum > 180;
}

bool
MK_VIII::TCFHandler::is_tcf ()
{
  if (mk_data(radio_altitude).get() > 10)
    {
      if (has_runway)
	{
	  double distance, az1, az2;

	  geo_inverse_wgs_84(0,
			     mk_data(gps_latitude).get(),
			     mk_data(gps_longitude).get(),
			     runway.center.latitude,
			     runway.center.longitude,
			     &az1, &az2, &distance);

	  distance *= SG_METER_TO_NM;

	  // distance to the inner envelope edge
	  double edge_distance = distance - runway.half_length - k;

	  if (edge_distance >= 15)
	    {
	      if (mk_data(radio_altitude).get() < 700)
		return true;
	    }
	  else if (edge_distance >= 12)
	    {
	      if (mk_data(radio_altitude).get() < 100 * edge_distance - 800)
		return true;
	    }
	  else if (edge_distance >= 4)
	    {
	      if (mk_data(radio_altitude).get() < 400)
		return true;
	    }
	  else if (edge_distance >= 2.45)
	    {
	      if (mk_data(radio_altitude).get() < 66.666667 * edge_distance + 133.33333)
		return true;
	    }
	  else
	    {
	      if (is_inside_edge_triangle(&runway.edges[0]) || is_inside_edge_triangle(&runway.edges[1]))
		{
		  if (edge_distance >= 1)
		    {
		      if (mk_data(radio_altitude).get() < 66.666667 * edge_distance + 133.33333)
			return true;
		    }
		  else if (edge_distance >= 0.05)
		    {
		      if (mk_data(radio_altitude).get() < 200 * edge_distance)
			return true;
		    }
		}
	      else
		{
		  if (! is_inside_bias_area())
		    {
		      if (mk_data(radio_altitude).get() < 245)
			return true;
		    }
		}
	    }
	}
      else
	{
	  if (mk_data(radio_altitude).get() < 700)
	    return true;
	}
    }

  return false;
}

bool
MK_VIII::TCFHandler::is_rfcf ()
{
  if (has_runway)
    {
      double distance, az1, az2;
      geo_inverse_wgs_84(0,
			 mk_data(gps_latitude).get(),
			 mk_data(gps_longitude).get(),
			 runway.center.latitude,
			 runway.center.longitude,
			 &az1, &az2, &distance);

      double krf = k + mk_data(gps_vertical_figure_of_merit).get() / 200;
      distance = distance * SG_METER_TO_NM - runway.half_length - krf;

      if (distance <= 5)
	{
	  double altitude_above_field = mk_data(geometric_altitude).get() - runway.elevation;

	  if (distance >= 1.5)
	    {
	      if (altitude_above_field < 300)
		return true;
	    }
	  else if (distance >= 0)
	    {
	      if (altitude_above_field < 200 * distance)
		return true;
	    }
	}
    }

  return false;
}

void
MK_VIII::TCFHandler::update ()
{
  if (mk->configuration_module.state != ConfigurationModule::STATE_OK || ! conf.enabled)
    return;

  // update_runway() has to iterate over the whole runway database
  // (which contains thousands of runways), so it would be unwise to
  // run it every frame. Run it every 3 seconds.
  if (! runway_timer.running || runway_timer.elapsed() >= 3)
    {
      update_runway();
      runway_timer.start();
    }

  if (! mk_dinput(ta_tcf_inhibit)
      && ! mk->state_handler.ground // [1]
      && ! mk_data(gps_latitude).ncd
      && ! mk_data(gps_longitude).ncd
      && ! mk_data(radio_altitude).ncd
      && ! mk_data(geometric_altitude).ncd
      && ! mk_data(gps_vertical_figure_of_merit).ncd)
    {
      double *_reference;

      if (is_tcf())
	_reference = mk_data(radio_altitude).get_pointer();
      else if (is_rfcf())
	_reference = mk_data(geometric_altitude).get_pointer();
      else
	_reference = NULL;

      if (_reference)
	{
	  if (mk_test_alert(TCF_TOO_LOW_TERRAIN))
	    {
	      double new_bias = bias;
	      // do not repeat terrain alerts below 30ft agl
	      if (mk_data(radio_altitude).get() > 30)
	      {
	        if (new_bias < 0.0) // sanity check
	          new_bias = 0.0;
	        while ((*reference < initial_value - initial_value * new_bias)&&
	               (new_bias < 1.0))
	          new_bias += 0.2;
	      }

	      if (new_bias > bias)
		{
		  bias = new_bias;
		  mk_repeat_alert(mk_alert(TCF_TOO_LOW_TERRAIN));
		}
	    }
	  else
	    {
	      bias = 0.2;
	      reference = _reference;
	      initial_value = *reference;
	      mk_set_alerts(mk_alert(TCF_TOO_LOW_TERRAIN));
	    }

	  return;
	}
    }

  mk_unset_alerts(mk_alert(TCF_TOO_LOW_TERRAIN));
}

///////////////////////////////////////////////////////////////////////////////
// MK_VIII ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

MK_VIII::MK_VIII (SGPropertyNode *node)
  : properties_handler(this),
    name("mk-viii"),
    num(0),
    power_handler(this),
    system_handler(this),
    configuration_module(this),
    fault_handler(this),
    io_handler(this),
    voice_player(this),
    self_test_handler(this),
    alert_handler(this),
    state_handler(this),
    mode1_handler(this),
    mode2_handler(this),
    mode3_handler(this),
    mode4_handler(this),
    mode5_handler(this),
    mode6_handler(this),
    tcf_handler(this)
{
  for (int i = 0; i < node->nChildren(); ++i)
    {
      SGPropertyNode *child = node->getChild(i);
      string cname = child->getName();
      string cval = child->getStringValue();

      if (cname == "name")
	name = cval;
      else if (cname == "number")
	num = child->getIntValue();
      else
	{
	  SG_LOG(SG_INSTR, SG_WARN, "Error in mk-viii config logic");
	  if (name.length())
	    SG_LOG(SG_INSTR, SG_WARN, "Section = " << name);
	}
    }
}

void
MK_VIII::init ()
{
  properties_handler.init();
  voice_player.init();
}

void
MK_VIII::bind ()
{
  SGPropertyNode *node = fgGetNode(("/instrumentation/" + name).c_str(), num, true);

  configuration_module.bind(node);
  power_handler.bind(node);
  io_handler.bind(node);
  voice_player.bind(node, "Sounds/mk-viii/");
}

void
MK_VIII::unbind ()
{
  properties_handler.unbind();
}

void
MK_VIII::update (double dt)
{
  power_handler.update();
  system_handler.update();

  if (system_handler.state != SystemHandler::STATE_ON)
    return;

  io_handler.update_inputs();
  io_handler.update_input_faults();

  voice_player.update();
  state_handler.update();

  if (self_test_handler.update())
    return;

  io_handler.update_internal_latches();
  io_handler.update_lamps();

  mode1_handler.update();
  mode2_handler.update();
  mode3_handler.update();
  mode4_handler.update();
  mode5_handler.update();
  mode6_handler.update();
  tcf_handler.update();

  alert_handler.update();
  io_handler.update_outputs();
}
