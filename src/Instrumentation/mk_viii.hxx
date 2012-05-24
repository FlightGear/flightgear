// mk_viii.hxx -- Honeywell MK VIII EGPWS emulation
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


#ifndef __INSTRUMENTS_MK_VIII_HXX
#define __INSTRUMENTS_MK_VIII_HXX

#include <assert.h>

#include <vector>
#include <deque>
#include <map>

#include <simgear/props/props.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
using std::vector;
using std::deque;
using std::map;

class SGSampleGroup;

#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Main/globals.hxx>
#include <Sound/voiceplayer.hxx>

#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable: 4355 )
#endif


///////////////////////////////////////////////////////////////////////////////
// MK_VIII ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class MK_VIII : public SGSubsystem
{
  // keep in sync with Mode6Handler::altitude_callout_definitions[]
  static const unsigned n_altitude_callouts = 11;

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Parameter ///////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <class T>
  class Parameter
  {
    T _value;

  public:
    bool ncd;

    inline Parameter ()
      : _value(0), ncd(true) {}

    inline T get () const { assert(! ncd); return _value; }
    inline T *get_pointer () { return &_value; }
    inline void set (T value) { ncd = false; _value = value; }
    inline void unset () { ncd = true; }

    inline void set (const Parameter<T> *parameter)
    {
      if (parameter->ncd)
	unset();
      else
	set(parameter->get());
    }

    inline void set (const Parameter<double> *parameter, double factor)
    {
      if (parameter->ncd)
	unset();
      else
	set(parameter->get() * factor);
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Sample //////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <class T>
  class Sample
  {
  public:
    double	timestamp;
    T		value;

    inline Sample (T _value)
      : timestamp(globals->get_sim_time_sec()), value(_value) {}
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Timer ///////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Timer
  {
    double	start_time;

  public:
    bool	running;

    inline Timer ()
      : running(false) {}

    inline void start () { running = true; start_time = globals->get_sim_time_sec(); }
    inline void stop () { running = false; }
    inline double elapsed () const { assert(running); return globals->get_sim_time_sec() - start_time; }
    inline double start_or_elapsed ()
    {
      if (running)
	return elapsed();
      else
	{
	  start();
	  return 0;
	}
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::PropertiesHandler ///////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class PropertiesHandler : public FGVoicePlayer::PropertiesHandler
  {
    MK_VIII *mk;

  public:
    struct
    {
      SGPropertyNode_ptr ai_caged;
      SGPropertyNode_ptr ai_roll;
      SGPropertyNode_ptr ai_serviceable;
      SGPropertyNode_ptr altimeter_altitude;
      SGPropertyNode_ptr altimeter_serviceable;
      SGPropertyNode_ptr altitude;
      SGPropertyNode_ptr altitude_agl;
      SGPropertyNode_ptr altitude_gear_agl;
      SGPropertyNode_ptr altitude_radar_agl;
      SGPropertyNode_ptr orientation_roll;
      SGPropertyNode_ptr asi_serviceable;
      SGPropertyNode_ptr asi_speed;
      SGPropertyNode_ptr autopilot_heading_lock;
      SGPropertyNode_ptr flaps;
      SGPropertyNode_ptr gear_down;
      SGPropertyNode_ptr latitude;
      SGPropertyNode_ptr longitude;
      SGPropertyNode_ptr nav0_cdi_serviceable;
      SGPropertyNode_ptr nav0_gs_distance;
      SGPropertyNode_ptr nav0_gs_needle_deflection;
      SGPropertyNode_ptr nav0_gs_serviceable;
      SGPropertyNode_ptr nav0_has_gs;
      SGPropertyNode_ptr nav0_heading_needle_deflection;
      SGPropertyNode_ptr nav0_in_range;
      SGPropertyNode_ptr nav0_nav_loc;
      SGPropertyNode_ptr nav0_serviceable;
      SGPropertyNode_ptr power;
      SGPropertyNode_ptr replay_state;
      SGPropertyNode_ptr vs;
    } external_properties;

    inline PropertiesHandler (MK_VIII *device)
      : FGVoicePlayer::PropertiesHandler(), mk(device) {}

    PropertiesHandler() : FGVoicePlayer::PropertiesHandler() {}

    void init ();
  };

public:
  PropertiesHandler     properties_handler;

private:
  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::PowerHandler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class PowerHandler
  {
    MK_VIII *mk;

    bool serviceable;
    bool powered;

    Timer power_loss_timer;
    Timer abnormal_timer;
    Timer low_surge_timer;
    Timer high_surge_timer;
    Timer very_high_surge_timer;

    bool handle_abnormal_voltage (bool abnormal,
				  Timer *timer,
				  double max_duration);

    void power_on ();
    void power_off ();

  public:
    inline PowerHandler (MK_VIII *device)
      : mk(device), serviceable(false), powered(false) {}

    void bind (SGPropertyNode *node);
    void update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::SystemHandler ///////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class SystemHandler
  {
    MK_VIII *mk;

    double	boot_delay;
    Timer	boot_timer;

    int		last_replay_state;
    Timer	reposition_timer;

  public:
    typedef enum
    {
      STATE_OFF,
      STATE_BOOTING,
      STATE_ON,
      STATE_REPOSITION
    } State;

    State state;

    inline SystemHandler (MK_VIII *device)
      : mk(device), state(STATE_OFF) {}

    void power_on ();
    void power_off ();
    void update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::ConfigurationModule /////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class ConfigurationModule
  {
  public:
    // keep in sync with IOHandler::present_status()
    typedef enum 
    {
      CATEGORY_AIRCRAFT_MODE_TYPE_SELECT,
      CATEGORY_AIR_DATA_INPUT_SELECT,
      CATEGORY_POSITION_INPUT_SELECT,
      CATEGORY_ALTITUDE_CALLOUTS,
      CATEGORY_AUDIO_MENU_SELECT,
      CATEGORY_TERRAIN_DISPLAY_SELECT,
      CATEGORY_OPTIONS_SELECT_GROUP_1,
      CATEGORY_RADIO_ALTITUDE_INPUT_SELECT,
      CATEGORY_NAVIGATION_INPUT_SELECT,
      CATEGORY_ATTITUDE_INPUT_SELECT,
      CATEGORY_HEADING_INPUT_SELECT,
      CATEGORY_WINDSHEAR_INPUT_SELECT,
      CATEGORY_INPUT_OUTPUT_DISCRETE_TYPE_SELECT,
      CATEGORY_AUDIO_OUTPUT_LEVEL,
      CATEGORY_UNDEFINED_INPUT_SELECT_1,
      CATEGORY_UNDEFINED_INPUT_SELECT_2,
      CATEGORY_UNDEFINED_INPUT_SELECT_3,
      N_CATEGORIES
    } Category;

    typedef enum
    {
      STATE_OK,
      STATE_INVALID_DATABASE,
      STATE_INVALID_AIRCRAFT_TYPE
    } State;
    State state;

    int effective_categories[N_CATEGORIES];

    ConfigurationModule (MK_VIII *device);

    void boot ();
    void bind (SGPropertyNode *node);

  private:
    MK_VIII *mk;

    int categories[N_CATEGORIES];

    bool read_aircraft_mode_type_select (int value);
    bool read_air_data_input_select (int value);
    bool read_position_input_select (int value);
    bool read_altitude_callouts (int value);
    bool read_audio_menu_select (int value);
    bool read_terrain_display_select (int value);
    bool read_options_select_group_1 (int value);
    bool read_radio_altitude_input_select (int value);
    bool read_navigation_input_select (int value);
    bool read_attitude_input_select (int value);
    bool read_heading_input_select (int value);
    bool read_windshear_input_select (int value);
    bool read_input_output_discrete_type_select (int value);
    bool read_audio_output_level (int value);
    bool read_undefined_input_select (int value);

    static bool m6_t2_is_bank_angle (Parameter<double> *agl,
				     double abs_roll_deg,
				     bool ap_engaged);
    static bool m6_t4_is_bank_angle (Parameter<double> *agl,
				     double abs_roll_deg,
				     bool ap_engaged);
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::FaultHandler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class FaultHandler
  {
    enum
    {
      INOP_GPWS		= 1 << 0,
      INOP_TAD		= 1 << 1
    };

    MK_VIII *mk;

    static const unsigned int fault_inops[];

    bool has_faults (unsigned int inop);

  public:
    // keep in sync with IOHandler::present_status()
    typedef enum
    {
      FAULT_ALL_MODES_INHIBIT,
      FAULT_GEAR_SWITCH,
      FAULT_FLAPS_SWITCH,
      FAULT_MOMENTARY_FLAP_OVERRIDE_INVALID,
      FAULT_SELF_TEST_INVALID,
      FAULT_GLIDESLOPE_CANCEL_INVALID,
      FAULT_STEEP_APPROACH_INVALID,
      FAULT_GPWS_INHIBIT,
      FAULT_TA_TCF_INHIBIT,
      FAULT_MODES14_INPUTS_INVALID,
      FAULT_MODE5_INPUTS_INVALID,
      FAULT_MODE6_INPUTS_INVALID,
      FAULT_BANK_ANGLE_INPUTS_INVALID,
      FAULT_TCF_INPUTS_INVALID,
      N_FAULTS
    } Fault;

    bool faults[N_FAULTS];

    inline FaultHandler (MK_VIII *device)
      : mk(device) {}

    void boot ();

    void set_fault (Fault fault);
    void unset_fault (Fault fault);

    bool has_faults () const;
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::IOHandler ///////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

public:
  class IOHandler
  {
  public:
    enum Lamp
    {
      LAMP_NONE,
      LAMP_GLIDESLOPE,
      LAMP_CAUTION,
      LAMP_WARNING
    };

    struct LampConfiguration
    {
      bool format2;
      bool flashing;
    };

    struct FaultsConfiguration
    {
      double max_flaps_down_airspeed;
      double max_gear_down_airspeed;
    };

    struct _s_Conf
    {
      const LampConfiguration	*lamp;
      const FaultsConfiguration	*faults;
      bool			flap_reversal;
      bool			steep_approach_enabled;
      bool			gpws_inhibit_enabled;
      bool			momentary_flap_override_enabled;
      bool			alternate_steep_approach;
      bool			use_internal_gps;
      bool			localizer_enabled;
      int			altitude_source;
      bool			use_attitude_indicator;
    } conf;

    struct _s_input_feeders
    {
      struct _s_discretes
      {
	bool landing_gear;
	bool landing_flaps;
	bool glideslope_inhibit;
	bool decision_height;
	bool autopilot_engaged;
      } discretes;

      struct _s_arinc429
      {
	bool uncorrected_barometric_altitude;
	bool barometric_altitude_rate;
	bool radio_altitude;
	bool glideslope_deviation;
	bool roll_angle;
	bool localizer_deviation;
	bool computed_airspeed;
	bool decision_height;
      } arinc429;
    } input_feeders;

    struct _s_inputs
    {
      struct _s_discretes
      {
	bool landing_gear;		// appendix E 6.6.2, 3.15.1.4
	bool landing_flaps;		// appendix E 6.6.4, 3.15.1.2
	bool momentary_flap_override;	// appendix E 6.6.6, 3.15.1.6
	bool self_test;			// appendix E 6.6.7, 3.15.1.10
	bool glideslope_inhibit;	// appendix E 6.6.11, 3.15.1.1
	bool glideslope_cancel;		// appendix E 6.6.13, 3.15.1.5
	bool decision_height;		// appendix E 6.6.14, 3.10.2
	bool mode6_low_volume;		// appendix E 6.6.15, 3.15.1.7
	bool audio_inhibit;		// appendix E 6.6.16, 3.15.1.3
	bool ta_tcf_inhibit;		// appendix E 6.6.20, 3.15.1.9
	bool autopilot_engaged;		// appendix E 6.6.21, 3.15.1.8
	bool steep_approach;		// appendix E 6.6.25, 3.15.1.11
	bool gpws_inhibit;		// appendix E 6.6.27, 3.15.1.12
      } discretes;

      struct _s_arinc429
      {
	Parameter<double> uncorrected_barometric_altitude;	// appendix E 6.2.1
	Parameter<double> barometric_altitude_rate;		// appendix E 6.2.2
	Parameter<double> gps_altitude;				// appendix E 6.2.4
	Parameter<double> gps_latitude;				// appendix E 6.2.7
	Parameter<double> gps_longitude;			// appendix E 6.2.8
	Parameter<double> gps_vertical_figure_of_merit;		// appendix E 6.2.13
	Parameter<double> radio_altitude;			// appendix E 6.2.29
	Parameter<double> glideslope_deviation;			// appendix E 6.2.30
	Parameter<double> roll_angle;				// appendix E 6.2.31
	Parameter<double> localizer_deviation;			// appendix E 6.2.33
	Parameter<double> computed_airspeed;			// appendix E 6.2.39
	Parameter<double> decision_height;			// appendix E 6.2.41
      } arinc429;
    } inputs;

    struct Outputs
    {
      struct _s_discretes
      {
	bool gpws_warning;		// appendix E 7.4.1, 3.15.2.5
	bool gpws_alert;		// appendix E 7.4.1, 3.15.2.6
	bool audio_on;			// appendix E 7.4.2, 3.15.2.10
	bool gpws_inop;			// appendix E 7.4.3, 3.15.2.3
	bool tad_inop;			// appendix E 7.4.3, 3.15.2.4
	bool flap_override;		// appendix E 7.4.5, 3.15.2.8
	bool glideslope_cancel;		// appendix E 7.4.6, 3.15.2.7
	bool steep_approach;		// appendix E 7.4.12, 3.15.2.9
      } discretes;

      struct _s_arinc429
      {
	int egpws_alert_discrete_1;	// appendix E 7.1.1.1
	int egpwc_logic_discretes;	// appendix E 7.1.1.2
	int mode6_callouts_discrete_1;	// appendix E 7.1.1.3
	int mode6_callouts_discrete_2;	// appendix E 7.1.1.4
	int egpws_alert_discrete_2;	// appendix E 7.1.1.5
	int egpwc_alert_discrete_3;	// appendix E 7.1.1.6
      } arinc429;
    };

    Outputs outputs;

    struct _s_data
    {
      Parameter<double> barometric_altitude_rate;
      Parameter<double> decision_height;
      Parameter<double> geometric_altitude;
      Parameter<double> glideslope_deviation_dots;
      Parameter<double> gps_altitude;
      Parameter<double> gps_latitude;
      Parameter<double> gps_longitude;
      Parameter<double> gps_vertical_figure_of_merit;
      Parameter<double> localizer_deviation_dots;
      Parameter<double> radio_altitude;
      Parameter<double> roll_angle;
      Parameter<double> terrain_clearance;
    } data;

    IOHandler (MK_VIII *device);

    void boot ();
    void post_boot ();
    void power_off ();

    void enter_ground ();
    void enter_takeoff ();

    void update_inputs ();
    void update_input_faults ();
    void update_alternate_discrete_input (bool *ptr);
    void update_internal_latches ();

    void update_egpws_alert_discrete_1 ();
    void update_egpwc_logic_discretes ();
    void update_mode6_callouts_discrete_1 ();
    void update_mode6_callouts_discrete_2 ();
    void update_egpws_alert_discrete_2 ();
    void update_egpwc_alert_discrete_3 ();
    void update_outputs ();
    void reposition ();

    void update_lamps ();
    void set_lamp (Lamp lamp);

    bool gpws_inhibit () const;
    bool real_flaps_down () const;
    bool flaps_down () const;
    bool flap_override () const;
    bool steep_approach () const;
    bool momentary_steep_approach_enabled () const;

    void bind (SGPropertyNode *node);

    MK_VIII *mk;

  private:

    ///////////////////////////////////////////////////////////////////////////
    // MK_VIII::IOHandler::TerrainClearanceFilter /////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    class TerrainClearanceFilter
    {
      typedef deque< Sample<double> > samples_type;
      samples_type		samples;
      double			value;
      double			last_update;

    public:
      inline TerrainClearanceFilter ()
	: value(0.0), last_update(-1.0) {}

      double update (double agl);
      void reset ();
    };

    ///////////////////////////////////////////////////////////////////////////
    // MK_VIII::IOHandler (continued) /////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    TerrainClearanceFilter terrain_clearance_filter;

    Lamp	_lamp;
    Timer	lamp_timer;

    Timer audio_inhibit_fault_timer;
    Timer landing_gear_fault_timer;
    Timer flaps_down_fault_timer;
    Timer momentary_flap_override_fault_timer;
    Timer self_test_fault_timer;
    Timer glideslope_cancel_fault_timer;
    Timer steep_approach_fault_timer;
    Timer gpws_inhibit_fault_timer;
    Timer ta_tcf_inhibit_fault_timer;

    bool last_landing_gear;
    bool last_real_flaps_down;

    typedef deque< Sample< Parameter<double> > > altitude_samples_type;
    altitude_samples_type altitude_samples;

    struct
    {
      bool glideslope_cancel;
    } power_saved;

    void update_terrain_clearance ();
    void reset_terrain_clearance ();

    void handle_input_fault (bool test, FaultHandler::Fault fault);
    void handle_input_fault (bool test,
			     Timer *timer,
			     double max_duration,
			     FaultHandler::Fault fault);

    void tie_input (SGPropertyNode *node,
		    const char *name,
		    bool *input,
		    bool *feed = NULL);
    void tie_input (SGPropertyNode *node,
		    const char *name,
		    Parameter<double> *input,
		    bool *feed = NULL);
    void tie_output (SGPropertyNode *node,
		     const char *name,
		     bool *output);
    void tie_output (SGPropertyNode *node,
		     const char *name,
		     int *output);

  public:

    bool get_discrete_input (bool *ptr) const;
    void set_discrete_input (bool *ptr, bool value);

    void present_status ();
    void present_status_section (const char *name);
    void present_status_item (const char *name, const char *value = NULL);
    void present_status_subitem (const char *name);

    bool get_present_status () const;
    void set_present_status (bool value);

    bool *get_lamp_output (Lamp lamp);
  };
  
  class VoicePlayer : public FGVoicePlayer
  {
  public:
      VoicePlayer (MK_VIII *device) :
          FGVoicePlayer(&device->properties_handler, "mk-viii")
      {}

      ~VoicePlayer() {}
      void init ();

      struct
      {
        Voice *application_data_base_failed;
        Voice *bank_angle;
        Voice *bank_angle_bank_angle;
        Voice *bank_angle_bank_angle_3;
        Voice *bank_angle_inop;
        Voice *bank_angle_pause_bank_angle;
        Voice *bank_angle_pause_bank_angle_3;
        Voice *callouts_inop;
        Voice *configuration_type_invalid;
        Voice *dont_sink;
        Voice *dont_sink_pause_dont_sink;
        Voice *five_hundred_above;
        Voice *glideslope;
        Voice *glideslope_inop;
        Voice *gpws_inop;
        Voice *hard_glideslope;
        Voice *minimums;
        Voice *minimums_minimums;
        Voice *pull_up;
        Voice *sink_rate;
        Voice *sink_rate_pause_sink_rate;
        Voice *soft_glideslope;
        Voice *terrain;
        Voice *terrain_pause_terrain;
        Voice *too_low_flaps;
        Voice *too_low_gear;
        Voice *too_low_terrain;
        Voice *altitude_callouts[n_altitude_callouts];
      } voices;
      
  };

private:
  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::SelfTestHandler /////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class SelfTestHandler
  {
    MK_VIII *mk;

    typedef enum
    {
      CANCEL_NONE,
      CANCEL_SHORT,
      CANCEL_LONG
    } Cancel;

    enum
    {
      ACTION_SLEEP		= 1 << 0,
      ACTION_VOICE		= 1 << 1,
      ACTION_DISCRETE_ON_OFF	= 1 << 2,
      ACTION_DONE		= 1 << 3
    };

    typedef struct
    {
      unsigned int	flags;
      double		sleep_duration;
      bool		*discrete;
    } Action;

    Cancel		cancel;
    Action		action;
    int			current;
    bool		button_pressed;
    double		button_press_timestamp;
    IOHandler::Outputs	saved_outputs;
    double		sleep_start;

    bool _was_here (int position);

    Action sleep (double duration);
    Action play (VoicePlayer::Voice *voice);
    Action discrete_on (bool *discrete, double duration);
    Action discrete_on_off (bool *discrete, double duration);
    Action discrete_on_off (bool *discrete, VoicePlayer::Voice *voice);
    Action done ();

    Action run ();

    void start ();
    void stop ();
    void shutdown ();

  public:
    typedef enum
    {
      STATE_NONE,
      STATE_START,
      STATE_RUNNING
    } State;

    State state;

    inline SelfTestHandler (MK_VIII *device)
      : mk(device), button_pressed(false), state(STATE_NONE) {}

    inline void power_off () { stop(); }
    inline void set_inop () { stop(); }
    void handle_button_event (bool value);
    bool update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::AlertHandler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class AlertHandler
  {
    MK_VIII *mk;

    unsigned int old_alerts;
    unsigned int voice_alerts;
    unsigned int repeated_alerts;
    VoicePlayer::Voice *altitude_callout_voice;

    void reset ();
    inline bool has_alerts (unsigned int test) const { return (alerts & test) != 0; }
    inline bool has_old_alerts (unsigned int test) const { return (old_alerts & test) != 0; }
    inline bool must_play_voice (unsigned int test) const { return ! has_old_alerts(test) || (repeated_alerts & test) != 0; }
    bool select_voice_alerts (unsigned int test);

  public:
    enum
    {
      ALERT_MODE1_PULL_UP				= 1 << 0,
      ALERT_MODE1_SINK_RATE				= 1 << 1,

      ALERT_MODE2A_PREFACE				= 1 << 2,
      ALERT_MODE2B_PREFACE				= 1 << 3,
      ALERT_MODE2A					= 1 << 4,
      ALERT_MODE2B					= 1 << 5,
      ALERT_MODE2B_LANDING_MODE				= 1 << 6,
      ALERT_MODE2A_ALTITUDE_GAIN			= 1 << 7,
      ALERT_MODE2A_ALTITUDE_GAIN_TERRAIN_CLOSING	= 1 << 8,

      ALERT_MODE3					= 1 << 9,

      ALERT_MODE4_TOO_LOW_FLAPS				= 1 << 10,
      ALERT_MODE4_TOO_LOW_GEAR				= 1 << 11,
      ALERT_MODE4AB_TOO_LOW_TERRAIN			= 1 << 12,
      ALERT_MODE4C_TOO_LOW_TERRAIN			= 1 << 13,

      ALERT_MODE5_SOFT					= 1 << 14,
      ALERT_MODE5_HARD					= 1 << 15,

      ALERT_MODE6_MINIMUMS				= 1 << 16,
      ALERT_MODE6_ALTITUDE_CALLOUT			= 1 << 17,
      ALERT_MODE6_LOW_BANK_ANGLE_1			= 1 << 18,
      ALERT_MODE6_HIGH_BANK_ANGLE_1			= 1 << 19,
      ALERT_MODE6_LOW_BANK_ANGLE_2			= 1 << 20,
      ALERT_MODE6_HIGH_BANK_ANGLE_2			= 1 << 21,
      ALERT_MODE6_LOW_BANK_ANGLE_3			= 1 << 22,
      ALERT_MODE6_HIGH_BANK_ANGLE_3			= 1 << 23,

      ALERT_TCF_TOO_LOW_TERRAIN				= 1 << 24
    };

    enum
    {
      ALERT_FLAG_REPEAT = 1 << 0
    };

    unsigned int alerts;

    inline AlertHandler (MK_VIII *device)
      : mk(device) {}

    void boot ();
    void reposition ();
    void update ();

    void set_alerts (unsigned int _alerts,
		     unsigned int flags = 0,
		     VoicePlayer::Voice *_altitude_callout_voice = NULL);
    void unset_alerts (unsigned int _alerts);

    inline void repeat_alert (unsigned int alert) { set_alerts(alert, ALERT_FLAG_REPEAT); }
    inline void set_altitude_callout_alert (VoicePlayer::Voice *voice) { set_alerts(ALERT_MODE6_ALTITUDE_CALLOUT, 0, voice); }
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::StateHandler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class StateHandler
  {
    MK_VIII *mk;

    Timer potentially_airborne_timer;

    void update_ground ();
    void enter_ground ();
    void leave_ground ();

    void update_takeoff ();
    void enter_takeoff ();
    void leave_takeoff ();

  public:
    bool ground;
    bool takeoff;

    inline StateHandler (MK_VIII *device)
      : mk(device), ground(true), takeoff(true) {}

    void post_reposition ();
    void update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Mode1Handler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Mode1Handler
  {
    MK_VIII *mk;

    Timer pull_up_timer;
    Timer sink_rate_timer;

    double sink_rate_tti;	// time-to-impact in minutes

    double get_pull_up_bias ();
    bool is_pull_up ();

    double get_sink_rate_bias ();
    bool is_sink_rate ();
    double get_sink_rate_tti ();

    void update_pull_up ();
    void update_sink_rate ();

  public:
    typedef struct
    {
      bool	flap_override_bias;
      int	min_agl;
      double	(*pull_up_min_agl1) (double vs);
      int	pull_up_max_agl1;
      double	(*pull_up_min_agl2) (double vs);
      int	pull_up_max_agl2;
    } EnvelopesConfiguration;

    struct
    {
      const EnvelopesConfiguration *envelopes;
    } conf;

    inline Mode1Handler (MK_VIII *device)
      : mk(device) {}

    void update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Mode2Handler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Mode2Handler
  {

    ///////////////////////////////////////////////////////////////////////////
    // MK_VIII::Mode2Handler::ClosureRateFilter ///////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    class ClosureRateFilter
    {
      /////////////////////////////////////////////////////////////////////////
      // MK_VIII::Mode2Handler::ClosureRateFilter::PassFilter /////////////////
      /////////////////////////////////////////////////////////////////////////

      class PassFilter
      {
	double a0;
	double a1;
	double b1;

	double last_input;
	double last_output;

      public:
	inline PassFilter (double _a0, double _a1, double _b1)
	  : a0(_a0), a1(_a1), b1(_b1) {}

	inline double filter (double input)
	{
	  last_output = a0 * input + a1 * last_input + b1 * last_output;
	  last_input = input;

	  return last_output;
	}

	inline void reset ()
	{
	  last_input = 0;
	  last_output = 0;
	}
      };

      /////////////////////////////////////////////////////////////////////////
      // MK_VIII::Mode2Handler::ClosureRateFilter (continued) /////////////////
      /////////////////////////////////////////////////////////////////////////

      MK_VIII *mk;

      Timer		timer;
      Parameter<double>	last_ra;	// last radio altitude
      Parameter<double> last_ba;	// last barometric altitude
      PassFilter	ra_filter;	// radio altitude rate filter
      PassFilter	ba_filter;	// barometric altitude rate filter

      double limit_radio_altitude_rate (double r);

    public:
      Parameter<double>	output;

      inline ClosureRateFilter (MK_VIII *device)
	: mk(device),
	  ra_filter(0.05, 0, 0.95),		// low-pass filter
	  ba_filter(0.93, -0.93, 0.86) {}	// high-pass-filter

      void init ();
      void update ();
    };

    ///////////////////////////////////////////////////////////////////////////
    // MK_VIII::Mode2Handler (continued) //////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    MK_VIII *mk;

    ClosureRateFilter closure_rate_filter;

    Timer takeoff_timer;
    Timer pull_up_timer;

    double	a_start_time;
    Timer	a_altitude_gain_timer;
    double	a_altitude_gain_alt;

    void check_pull_up (unsigned int preface_alert, unsigned int alert);

    bool b_conditions ();

    bool is_a ();
    bool is_b ();

    void update_a ();
    void update_b ();

  public:
    typedef struct
    {
      int airspeed1;
      int airspeed2;
    } Configuration;

    const Configuration *conf;

    inline Mode2Handler (MK_VIII *device)
      : mk(device), closure_rate_filter(device) {}

    void boot ();
    void power_off ();
    void leave_ground ();
    void enter_takeoff ();
    void update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Mode3Handler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Mode3Handler
  {
    MK_VIII *mk;

    bool	armed;
    bool	has_descent_alt;
    double	descent_alt;
    double	bias;

    double max_alt_loss (double _bias);
    double get_bias (double initial_bias, double alt_loss);
    bool is (double *alt_loss);

  public:
    typedef struct
    {
      int	min_agl;
      int	(*max_agl) (bool flap_override);
      double	(*max_alt_loss) (bool flap_override, double agl);
    } Configuration;

    const Configuration *conf;

    inline Mode3Handler (MK_VIII *device)
      : mk(device), armed(false), has_descent_alt(false) {}

    void enter_takeoff ();
    void update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Mode4Handler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Mode4Handler
  {
  public:
    typedef struct
    {
      int	airspeed1;
      int	airspeed2;
      int	min_agl1;
      double	(*min_agl2) (double airspeed);
      int	min_agl3;
    } EnvelopesConfiguration;

    typedef struct
    {
      const EnvelopesConfiguration *ac;
      const EnvelopesConfiguration *b;
    } ModesConfiguration;

    struct
    {
      VoicePlayer::Voice	*voice_too_low_gear;
      const ModesConfiguration	*modes;
    } conf;

    inline Mode4Handler (MK_VIII *device)
      : mk(device),ab_bias(0.0),ab_expanded_bias(0.0),c_bias(0.0) {}

    double get_upper_agl (const EnvelopesConfiguration *c);
    void update ();

  private:
    MK_VIII *mk;

    double ab_bias;
    double ab_expanded_bias;
    double c_bias;

    const EnvelopesConfiguration *get_ab_envelope ();
    double get_bias (double initial_bias, double min_agl);
    void handle_alert (unsigned int alert, double min_agl, double *bias);

    void update_ab ();
    void update_ab_expanded ();
    void update_c ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Mode5Handler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Mode5Handler
  {
    MK_VIII *mk;

    Timer hard_timer;
    Timer soft_timer;

    double soft_bias;

    bool is_hard ();
    bool is_soft (double bias);

    double get_soft_bias (double initial_bias);

    void update_hard (bool is);
    void update_soft (bool is);

  public:
    inline Mode5Handler (MK_VIII *device)
      : mk(device), soft_bias(0.0) {}

    void update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::Mode6Handler ////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Mode6Handler
  {
  public:
    // keep in sync with altitude_callout_definitions[]
    typedef enum
    {
      ALTITUDE_CALLOUT_1000,
      ALTITUDE_CALLOUT_500,
      ALTITUDE_CALLOUT_400,
      ALTITUDE_CALLOUT_300,
      ALTITUDE_CALLOUT_200,
      ALTITUDE_CALLOUT_100,
      ALTITUDE_CALLOUT_50,
      ALTITUDE_CALLOUT_40,
      ALTITUDE_CALLOUT_30,
      ALTITUDE_CALLOUT_20,
      ALTITUDE_CALLOUT_10
    } AltitudeCallout;

    typedef bool (*BankAnglePredicate) (Parameter<double> *agl,
					double abs_roll_deg,
					bool ap_engaged);

    struct
    {
      bool			minimums_enabled;
      bool			smart_500_enabled;
      VoicePlayer::Voice	*above_field_voice;

      bool			altitude_callouts_enabled[n_altitude_callouts];
      bool			bank_angle_enabled;
      BankAnglePredicate	is_bank_angle;
    } conf;

    static const int altitude_callout_definitions[];

    inline Mode6Handler (MK_VIII *device)
      : mk(device) {}

    void boot ();
    void power_off ();
    void enter_takeoff ();
    void leave_takeoff ();
    void set_volume (float volume);
    bool altitude_callouts_enabled ();
    void update ();

  private:
    MK_VIII *mk;

    bool		last_decision_height;
    Parameter<double>	last_radio_altitude;
    Parameter<double>	last_altitude_above_field;

    bool altitude_callouts_issued[n_altitude_callouts];
    bool minimums_issued;
    bool above_field_issued;

    Timer		runway_timer;
    Parameter<bool>	has_runway;

    struct
    {
      double elevation;		// elevation in feet
    } runway;

    void reset_minimums ();
    void reset_altitude_callouts ();
    bool is_playing_altitude_callout ();
    bool is_near_minimums (double callout);
    bool is_outside_band (double elevation, double callout);
    bool inhibit_smart_500 ();

    void update_minimums ();
    void update_altitude_callouts ();

    bool test_runway (const FGRunway *_runway);
    bool test_airport (const FGAirport *airport);
    void update_runway ();

    void get_altitude_above_field (Parameter<double> *parameter);
    void update_above_field_callout ();

    bool is_bank_angle (double abs_roll_angle, double bias);
    bool is_high_bank_angle ();
    unsigned int get_bank_angle_alerts ();
    void update_bank_angle ();
    
    class AirportFilter : public FGAirport::AirportFilter
    {
    public: 
      AirportFilter(Mode6Handler *s)
        : self(s) {}
        
      virtual bool passAirport(FGAirport *a) const;
      
      virtual FGPositioned::Type maxType() const {
        return FGPositioned::AIRPORT;
      }
      
    private:
      Mode6Handler* self;
    };
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII::TCFHandler //////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class TCFHandler
  {
    typedef struct
    {
      double latitude;		// latitude in degrees
      double longitude;		// longitude in degrees
    } Position;

    typedef struct
    {
      Position	position;	// position of threshold
      double	heading;	// runway heading
    } RunwayEdge;

    MK_VIII *mk;

    static const double k;

    Timer	runway_timer;
    bool	has_runway;

    struct
    {
      Position		center;		// center point
      double		elevation;	// elevation in feet
      double		half_length;	// runway half length, in nautical miles
      RunwayEdge	edges[2];	// runway threshold and end
      Position		bias_area[4];	// vertices of the bias area
    } runway;

    double bias;
    double *reference;
    double initial_value;

    double get_azimuth_difference (double from_lat,
				   double from_lon,
				   double to_lat,
				   double to_lon,
				   double to_heading);
    double get_azimuth_difference (const FGRunway *_runway);

    FGRunway* select_runway (const FGAirport *airport);
    void update_runway ();

    void get_bias_area_edges (Position *edge,
			      double reciprocal,
			      double half_width_m,
			      Position *bias_edge1,
			      Position *bias_edge2);

    bool is_inside_edge_triangle (RunwayEdge *edge);
    bool is_inside_bias_area ();

    bool is_tcf ();
    bool is_rfcf ();

    class AirportFilter : public FGAirport::AirportFilter
    {
    public: 
      AirportFilter(MK_VIII *device)
        : mk(device) {}
        
      virtual bool passAirport(FGAirport *a) const;
    private:
      MK_VIII* mk;
    };
  public:
    struct
    {
      bool enabled;
    } conf;

    inline TCFHandler (MK_VIII *device)
      : mk(device) {}

    void update ();
  };

  /////////////////////////////////////////////////////////////////////////////
  // MK_VIII (continued) //////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  string	name;
  int		num;

  PowerHandler		power_handler;
  SystemHandler		system_handler;
  ConfigurationModule	configuration_module;
  FaultHandler		fault_handler;
  IOHandler		io_handler;
  VoicePlayer		voice_player;
  SelfTestHandler	self_test_handler;
  AlertHandler		alert_handler;
  StateHandler		state_handler;
  Mode1Handler		mode1_handler;
  Mode2Handler		mode2_handler;
  Mode3Handler		mode3_handler;
  Mode4Handler		mode4_handler;
  Mode5Handler		mode5_handler;
  Mode6Handler		mode6_handler;
  TCFHandler		tcf_handler;

  struct
  {
    int runway_database;
  } conf;

public:
  MK_VIII (SGPropertyNode *node);

  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (double dt);
};

#ifdef _MSC_VER
#  pragma warning( pop )
#endif

#endif // __INSTRUMENTS_MK_VIII_HXX
