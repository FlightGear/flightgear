/*
 * fgcom - VoIP-Client for the FlightGear-Radio-Infrastructure
 *
 * This program realizes the usage of the VoIP infractructure based
 * on flight data which is send from FlightGear with an external
 * protocol to this application.
 *
 * For more information read: http://squonk.abacab.org/dokuwiki/fgcom
 *
 * (c) H. Wirtz <dcoredump@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/timing/timestamp.hxx>

#include "fgcom.hxx"
#include "fgcom_init.hxx"
#include "utils.hxx"
#include "version.h"

/* Global variables */
int exitcode = 0;
int initialized = 0;
int connected = 0;
int reg_id;
const char* dialstring;
char airport[5];
int codec = DEFAULT_IAX_CODEC;
simgear::Socket sgSocket;

/* Variables declared as static */
static double selected_frequency = 0.0;
static char mode = 0;
static char rep_buf[MX_REPORT_BUF+2]; /* report output buffer - used on iax callback thread - note +2 to ensure null termination */
static char num_buf[1024];            /* number generation buffer - used on main thread */
static char states[256];	      /* buffer to hold ascii states */
static char delim = '\t';	      /* output field delimiter */
static int last_state = 0;	      /* previous state of the channel */

static const char *map[] = {
  "unknown", "active", "outgoing", "ringing", "complete", "selected",
  "busy", "transfer", NULL
};

static const char *radio_map[] = {"COM1", "COM2"};

char icao[5];
double special_frq[] = {
    999.999,
    911.000,
    910.000,
    123.450,
    122.750,
    123.500,
    121.500,
    732.340,
     -1.0 };
double *special_frequencies;

double previous_com_frequency = 0.0;
int previous_ptt = 0;
float previous_vol = 0.0;
int com_select = 0;
int max_com_instruments = 2;
struct airport *airportlist;
struct fgdata data;
char *prog; //program name

/* configuration values */
static bool debug = false;
static int port;
static const char *voipserver;
static const char *fgserver;
static const char *airport_option;
static double frequency = -1.0;
static const char *audio_in;
static const char *audio_out;
static double level_in = 0.7;
static double level_out = 0.7;
static bool mic_boost;
static char codec_option;
static const char *callsign;
static const char *username;
static const char *password;
static bool list_audio;
static char *positions_file;
static char *frequency_file;

static const OptionEntry fgcomOptionArray[] = {
  {"debug", 'd', false, OPTION_NONE, &debug, 0, "show debugging information",
   0},
  {"voipserver", 'S', true, OPTION_STRING, &voipserver, 0,
   "voip server to connect to", &DEFAULT_VOIP_SERVER},
  {"fgserver", 's', true, OPTION_STRING, &fgserver, 0, "fg to connect to ",
   &DEFAULT_FG_SERVER},
  {"port", 'p', true, OPTION_INT, &port, 0, "where we should listen to FG", &port},	// hm, good idea? don't think so...
  {"airport", 'a', true, OPTION_STRING, &airport_option, 0,
   "airport-id (ICAO) for ATC-mode", 0},
  {"frequency", 'f', true, OPTION_DOUBLE, &frequency, 0,
   "frequency for ATC-mode", 0},
  {"callsign", 'C', true, OPTION_STRING, &callsign, 0,
   "callsign to use", &DEFAULT_USER},
  {"user", 'U', true, OPTION_STRING, &username, 0,
   "username for VoIP account", &DEFAULT_USER},
  {"password", 'P', true, OPTION_STRING, &password, 0,
   "password for VoIP account", &DEFAULT_PASSWORD},
  {"mic", 'i', true, OPTION_DOUBLE, &level_in, 0,
   "mic input level (0.0 - 1.0)", 0},
  {"speaker", 'o', true, OPTION_DOUBLE, &level_out, 0,
   "speaker output level (0.0 - 1.0)", 0},
  {"mic-boost", 'b', false, OPTION_BOOL, &mic_boost, 0, "enable mic boost",
   0},
  {"list-audio", 'l', false, OPTION_BOOL, &list_audio, 0,
   "list audio devices", 0},
  {"set-audio-in", 'r', true, OPTION_STRING, &audio_in, 0,
   "use <devicename> as audio input", 0},
  {"set-audio-out", 'k', true, OPTION_STRING, &audio_out, 0,
   "use <devicename> as audio output", 0},
  {"codec", 'c', true, OPTION_CHAR, &codec_option, 0,
   "use codec <codec> as transfer codec", &codec_option},
  {"positions", 'T', true, OPTION_STRING, &positions_file, 0,
   "location positions file", &DEFAULT_POSITIONS_FILE},
  {"special", 'Q', true, OPTION_STRING, &frequency_file, 0,
   "location spl. frequency file (opt)", &SPECIAL_FREQUENCIES_FILE},
  {NULL}
};

void
process_packet (char *buf)
{

  /* cut off ending \n */
  buf[strlen (buf) - 1] = '\0';

  /* parse the data into a struct */
  parse_fgdata (&data, buf);

  /* get the selected frequency */
  if (com_select == 0 && data.COM1_SRV == 1)
    selected_frequency = data.COM1_FRQ;
  else if (com_select == 1 && data.COM2_SRV == 1)
    selected_frequency = data.COM2_FRQ;

  /* Check for com frequency changes */
  if (previous_com_frequency != selected_frequency)
    {
      SG_LOG( SG_GENERAL, SG_ALERT, "Selected frequency: " << selected_frequency );

      /* remark the new frequency */
      previous_com_frequency = selected_frequency;

      if (connected == 1)
	{
	  /* hangup call, if connected */
	  iaxc_dump_call ();
	  iaxc_millisleep (5 * DEFAULT_MILLISLEEP);
	  connected = 0;
	}

      strcpy (icao,
	      icaobypos (airportlist, selected_frequency, data.LAT,
			 data.LON, DEFAULT_RANGE));
      icao2number (icao, selected_frequency, num_buf);

      SG_LOG( SG_GENERAL, SG_DEBUG, "Dialing " << icao << " " << selected_frequency << " MHz: " << num_buf );
      do_iaxc_call (username, password, voipserver, num_buf);

      connected = 1;
    }

  /* Check for pressed PTT key */
  if (previous_ptt != data.PTT)
    {
      if (data.PTT == 2)
	{
	  /* select the next com equipment */
	  com_select = (com_select + 1) % 2;
          SG_LOG( SG_GENERAL, SG_ALERT, "Radio selected is " << radio_map[com_select] );
	}
      else if (connected == 1)
	{
	  ptt (data.PTT);
	}
      previous_ptt = data.PTT;
    }

  /* Check for output volume change */
  if (previous_vol != data.OUTPUT_VOL)
    {
      SG_LOG( SG_GENERAL, SG_ALERT, "Set speaker volume to " << data.OUTPUT_VOL );

      iaxc_output_level_set( data.OUTPUT_VOL );
      previous_vol = data.OUTPUT_VOL;
    }

  /* Check for callsign change */
  if (strcmp(callsign, data.CALLSIGN) != 0)
    {
      iaxc_dump_call ();
      callsign = data.CALLSIGN;
      SG_LOG( SG_GENERAL, SG_ALERT, "FGCom will restart now with callsign " << callsign );
      std::string app = "FGCOM-";
      app += FGCOM_VERSION;
      iaxc_set_callerid ( callsign, app.c_str() );
    }
}

static char *base_name( char *name )
{
    char *bn = name;
    size_t len = strlen(name);
    size_t i;
    int c;
    for ( i = 0; i < len; i++ ) {
        c = name[i];
        if (( c == '/' ) || ( c == '\\'))
            bn = &name[i+1];
    }
    return bn;
}

// if default FAILS, for OSX and WIN try EXE path
int fix_input_files()
{
    int iret = 0;
    char *def_freq = (char *) SPECIAL_FREQUENCIES_FILE;
    char *def_pos  = (char *) DEFAULT_POSITIONS_FILE;
    char exepath[MX_PATH_SIZE+2];
    exepath[0] = 0;
    if (strcmp( frequency_file,def_freq) == 0) {
        /* ok is default value - do some fixes */
        if (is_file_or_directory( frequency_file ) != 1) {
            exepath[0] = 0;
            iret |= get_data_path_per_os( exepath, MX_PATH_SIZE );
            strcat(exepath,def_freq);
            frequency_file = strdup(exepath);
        }
    }
    if (strcmp( positions_file,def_pos) == 0) {
        // if default FAILS, for OSX and WIN try EXE path
        if (is_file_or_directory( positions_file ) != 1) {
            exepath[0] = 0;
            iret |= get_data_path_per_os( exepath, MX_PATH_SIZE );
            strcat(exepath,def_pos);
            positions_file = strdup(exepath);
        }
    }
    return iret;
}

int
main (int argc, char *argv[])
{
  int numbytes;
  static char pkt_buf[MAXBUFLEN+2];

  sglog().setLogLevels( SG_ALL, SG_ALERT );

  prog = strdup( base_name(argv[0]) );
  
  /* program header */
  SG_LOG( SG_GENERAL, SG_ALERT, prog << " - a communication radio based on VoIP with IAX/Asterisk" );
  SG_LOG( SG_GENERAL, SG_ALERT, "Original (c) 2007-2011 by H. Wirtz <wirtz@dfn.de>" );
  SG_LOG( SG_GENERAL, SG_ALERT, "OSX and Windows ports 2012-2013 by Yves Sablonier and Geoff R. McLane, resp." );
  SG_LOG( SG_GENERAL, SG_ALERT, "Version " << FGCOM_VERSION << " compiled " << __DATE__ << ", at " << __TIME__ );
  SG_LOG( SG_GENERAL, SG_ALERT, "Using iaxclient library Version " << iaxc_version (pkt_buf) );
  SG_LOG( SG_GENERAL, SG_ALERT, "" );

  /* init values */
  voipserver = DEFAULT_VOIP_SERVER;
  fgserver = DEFAULT_FG_SERVER;
  port = DEFAULT_FG_PORT;
  username = DEFAULT_USER;
  password = DEFAULT_PASSWORD;
  codec_option = DEFAULT_CODEC;
  mode = 0; // 0 = ATC mode, 1 = FG mode
  positions_file = (char *) DEFAULT_POSITIONS_FILE;
  frequency_file = (char *) SPECIAL_FREQUENCIES_FILE;

#ifndef _WIN32
  /* catch signals */
  signal (SIGINT, quit);
  signal (SIGQUIT, quit);
  signal (SIGTERM, quit);
#endif

  /* setup iax */
#ifdef _MSC_VER
  /* MSVC only - In certain circumstances the addresses placed in iaxc_sendto and iaxc_recvfrom 
     can be an offset to a jump table, making a compare of the current address to the address of 
     the actual imported function fail. So here ensure they are the same. */
  iaxc_set_networking( (iaxc_sendto_t)sendto, (iaxc_recvfrom_t)recvfrom );
#endif // _MSC_VER

  if (iaxc_initialize (DEFAULT_MAX_CALLS))
    fatal_error ("cannot initialize iaxclient!\nHINT: Have you checked the mic and speakers?");

  initialized = 1;

  // option parser
  fgcomInitOptions (fgcomOptionArray, argc, argv);

  if( debug )
    sglog().setLogLevels( SG_ALL, SG_DEBUG );

  // codec
  if (codec_option)
    {
      switch (codec_option)
	{
	case 'u':
	  codec = IAXC_FORMAT_ULAW;
	  break;
	case 'a':
	  codec = IAXC_FORMAT_ALAW;
	  break;
	case 'g':
	  codec = IAXC_FORMAT_GSM;
	  break;
	case '7':
	  codec = IAXC_FORMAT_G726;
	  break;
	case 's':
	  codec = IAXC_FORMAT_SPEEX;
	  break;
	}
    }

  // airport
  if (airport_option)
    {
      strtoupper (airport_option, airport, sizeof (airport));
    }

  // input level
  if (level_in > 1.0)
    {
      level_in = 1.0;
    }
  if (level_in < 0.0)
    {
      level_in = 0.0;
    }

  // output level
  if (level_out > 1.0)
    {
      level_out = 1.0;
    }
  if (level_out < 0.0)
    {
      level_out = 0.0;
    }

  // microphone boost
  if (mic_boost)
    {
      iaxc_mic_boost_set (1);
    }

  if (list_audio)
    {
      SG_LOG( SG_GENERAL, SG_ALERT, "Input audio devices:" );
      SG_LOG( SG_GENERAL, SG_ALERT, report_devices(IAXC_AD_INPUT) );

      SG_LOG( SG_GENERAL, SG_ALERT, "Output audio devices:" );
      SG_LOG( SG_GENERAL, SG_ALERT, report_devices(IAXC_AD_OUTPUT) );

      iaxc_shutdown ();
      exit (1);
    }


  if (audio_in)
    {
      set_device (audio_in, 0);
    }

  if (audio_out)
    {
      set_device (audio_out, 1);
    }

  /* checking consistency of arguments */
  if (frequency > 0.0 && frequency < 1000.0)
    {
      if (strlen (airport) == 0 || strlen (airport) > 4)
	{
	  strcpy (airport, "ZZZZ");
	}
      /* airport and frequency are given => ATC mode */
      mode = 0;
    }
  else
    {
      /* no airport => FG mode */
      mode = 1;
    }

	/* Read special frequencies file (if exists).
	 * If no file $(INSTALL_DIR)/special_frequencies.txt exists, then default frequencies
	 * are used and are hard coded.
	 */

    if (fix_input_files()) { /* adjust default input per OS */
        fatal_error ("cannot adjust default input files per OS!\nHINT: Maybe recompile with larger buffer.");
    }

	if((special_frequencies = read_special_frequencies(frequency_file)) == 0) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to load file [" << frequency_file << "] !" );
        SG_LOG( SG_GENERAL, SG_ALERT, "Using internal defaults" );
        special_frequencies = special_frq;
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Loaded file [" << frequency_file << "]." );
    }

	/* read airport frequencies and positions */
	airportlist = read_airports (positions_file);   /* never returns if fail! */

  /* preconfigure iax */
  SG_LOG( SG_GENERAL, SG_ALERT, "Initializing IAX client as " << username << ":" << "xxxxxxxxxxx@" << voipserver );

  std::string app = "FGCOM-";
  app += FGCOM_VERSION;

  if( !callsign )
    callsign = DEFAULT_USER;

  iaxc_set_callerid ( callsign, app.c_str() );
  iaxc_set_formats (IAXC_FORMAT_SPEEX, IAXC_FORMAT_SPEEX);
  iaxc_set_speex_settings(1, 5, 0, 1, 0, 3);
  iaxc_set_event_callback (iaxc_callback);

  iaxc_start_processing_thread ();

  if (username && password && voipserver)
    {
      reg_id =
	iaxc_register (const_cast < char *>(username),
		       const_cast < char *>(password),
		       const_cast < char *>(voipserver));
      SG_LOG( SG_GENERAL, SG_DEBUG, "Registered as '" << username << "' at '" << voipserver );
    }
  else
    {
      SG_LOG( SG_GENERAL, SG_ALERT, "Failed iaxc_register !" );
      SG_LOG( SG_GENERAL, SG_ALERT, "HINT: Check username, passwordd and address of server" );
      exitcode = 130;
      quit (0);
    }

  iaxc_millisleep (DEFAULT_MILLISLEEP);

  /* main loop */
  if (mode == 1)
    {
      SG_LOG( SG_GENERAL, SG_DEBUG, "Entering main loop in mode FGFS" );
      /* only in FG mode */
      simgear::Socket::initSockets();
      sgSocket.open (false);
      sgSocket.bind (fgserver, port);

      /* mute mic, speaker on */
      iaxc_input_level_set (0);
      iaxc_output_level_set (level_out);

      SGTimeStamp sg_clock;
      sg_clock.stamp();

      double sg_next_update = sg_clock.toSecs() + DEFAULT_ALARM_TIMER;
      /* get data from flightgear */
      while (1)
	{
          sg_clock.stamp();
	  double sg_wait = sg_next_update - sg_clock.toSecs();
	  if (sg_wait > 0.001)
	    {
             simgear::Socket *readSockets[2] = { &sgSocket, 0 };
             if (sgSocket.select (readSockets, readSockets + 1,
				   (int) (sg_wait * 1000)) == 1)
		{
                  simgear::IPAddress their_addr;
                  numbytes = sgSocket.recvfrom(pkt_buf, MAXBUFLEN - 1, 0, &their_addr);
                  if (numbytes == -1)
		    {
		      perror ("recvfrom");
		      exit (1);
		    }
		  pkt_buf[numbytes] = '\0';
                  SG_LOG( SG_GENERAL, SG_DEBUG, "Got packet from " << their_addr.getHost () << ":" << their_addr.getPort () );
                  SG_LOG( SG_GENERAL, SG_DEBUG, "Packet is " << numbytes << " bytes long" );
                  SG_LOG( SG_GENERAL, SG_DEBUG, "Packet contains \"" << pkt_buf << "\"" );
		  process_packet (pkt_buf);
		}
	    }
	  else
	    {
	      alarm_handler (0);
              sg_clock.stamp();
	      sg_next_update = sg_clock.toSecs() + DEFAULT_ALARM_TIMER;
	    }
	}
    }
  else
    {
      SG_LOG( SG_GENERAL, SG_DEBUG, "Entering main loop in mode ATC" );
      /* mic on, speaker on */
      iaxc_input_level_set (1.0);
      iaxc_output_level_set (1.0);

      icao2atisnumber (airport, frequency, num_buf);
      SG_LOG( SG_GENERAL, SG_DEBUG, "Dialing " << airport << " " << frequency << " MHz: " << num_buf );
      do_iaxc_call (username, password, voipserver, num_buf);

      while (1)
	{
	  /* sleep endless */
          SGTimeStamp::sleepForMSec(3600000);
	}
    }

  /* should never be reached */
  exitcode = 999;
  quit (0);
}

void
quit (int signal)
{
  SG_LOG( SG_GENERAL, SG_ALERT, "Stopping service" );

  if (initialized)
    iaxc_shutdown ();
  if (reg_id)
    iaxc_unregister (reg_id);

  exit (exitcode);
}

void
alarm_handler (int signal)
{
  /* Check every DEFAULT_ALARM_TIMER seconds if position related things should happen */
  if (check_special_frq (selected_frequency))
    {
      strcpy (icao, "ZZZZ");
    }
  else
    {
      strcpy (icao,
	      icaobypos (airportlist, selected_frequency, data.LAT, data.LON,
			 DEFAULT_RANGE));
    }

  /* Check if we are out of range */
  if (strlen (icao) == 0 && connected == 1)
    {
      /* Yes, we are out of range so hangup */
      iaxc_dump_call ();
      iaxc_millisleep (5 * DEFAULT_MILLISLEEP);
      connected = 0;
    }

  /* Check if we are now in range */
  else if (strlen (icao) != 0 && connected == 0)
    {
      icao2number (icao, selected_frequency, num_buf);
      SG_LOG( SG_GENERAL, SG_DEBUG, "Dialing " << icao << " " << selected_frequency << " MHz: " << num_buf );
      do_iaxc_call (username, password, voipserver, num_buf);
      connected = 1;
    }
}

void
strtoupper (const char *str, char *buf, size_t len)
{
  unsigned int i;
  for (i = 0; str[i] && i < len - 1; i++)
    {
      buf[i] = toupper (str[i]);
    }

  buf[i++] = '\0';
}

void
fatal_error (const char *err)
{
  SG_LOG( SG_GENERAL, SG_ALERT, "FATAL ERROR: " << err );
  if (initialized)
    iaxc_shutdown ();
  exit (1);
}

int
iaxc_callback (iaxc_event e)
{
  switch (e.type)
    {
    case IAXC_EVENT_LEVELS:
      event_level (e.ev.levels.input, e.ev.levels.output);
      break;
    case IAXC_EVENT_TEXT:
      event_text (e.ev.text.type, e.ev.text.message);
      break;
    case IAXC_EVENT_STATE:
      event_state (e.ev.call.state, e.ev.call.remote, e.ev.call.remote_name,
		   e.ev.call.local, e.ev.call.local_context);
      break;
    case IAXC_EVENT_NETSTAT:
      event_netstats (e.ev.netstats);
    case IAXC_EVENT_REGISTRATION:
      event_register (e.ev.reg.id, e.ev.reg.reply, e.ev.reg.msgcount);
      break;
    default:
      event_unknown (e.type);
      break;
    }
  return 1;
}

void
event_state (int state, char *remote, char *remote_name,
	     char *local, char *local_context)
{
  last_state = state;
  /* This is needed for auto-reconnect */
  if (state == 0)
    {
      connected = 0;
      /* FIXME: we should wake up the main thread somehow */
      /* in fg mode the next incoming packet will do that anyway */
    }

  snprintf (rep_buf, MX_REPORT_BUF,
	    "S%c0x%x%c%s%c%.50s%c%.50s%c%.50s%c%.50s", delim, state,
	    delim, map_state (state), delim, remote, delim, remote_name,
	    delim, local, delim, local_context);
  rep_buf[MX_REPORT_BUF] = 0; /* ensure null termination */
  report (rep_buf);
}

void
event_text (int type, char *message)
{
  snprintf (rep_buf, MX_REPORT_BUF, "T%c%d%c%.200s", delim, type, delim, message);
  SG_LOG( SG_GENERAL, SG_ALERT, message );
  rep_buf[MX_REPORT_BUF] = 0; /* ensure null termination */
  report (rep_buf);
}

void
event_register (int id, int reply, int count)
{
  const char *reason;
  switch (reply)
    {
    case IAXC_REGISTRATION_REPLY_ACK:
      reason = "accepted";
      break;
    case IAXC_REGISTRATION_REPLY_REJ:
      reason = "denied";
      if (strcmp (username, "guest") != 0)
	{
          SG_LOG( SG_GENERAL, SG_ALERT, "Registering denied" );
	}
      break;
    case IAXC_REGISTRATION_REPLY_TIMEOUT:
      reason = "timeout";
      break;
    default:
      reason = "unknown";
    }
  snprintf (rep_buf, MX_REPORT_BUF, "R%c%d%c%s%c%d", delim, id, delim,
	    reason, delim, count);
  rep_buf[MX_REPORT_BUF] = 0; /* ensure null termination */
  report (rep_buf);
}

void
event_netstats (struct iaxc_ev_netstats stat)
{
  struct iaxc_netstat local = stat.local;
  struct iaxc_netstat remote = stat.remote;
  snprintf (rep_buf, MX_REPORT_BUF,
	    "N%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d",
	    delim, stat.callNo, delim, stat.rtt,
	    delim, local.jitter, delim, local.losspct, delim,
	    local.losscnt, delim, local.packets, delim, local.delay,
	    delim, local.dropped, delim, local.ooo, delim,
	    remote.jitter, delim, remote.losspct, delim, remote.losscnt,
	    delim, remote.packets, delim, remote.delay, delim,
	    remote.dropped, delim, remote.ooo);
  rep_buf[MX_REPORT_BUF] = 0; /* ensure null termination */
  report (rep_buf);
}

void
event_level (double in, double out)
{
  snprintf (rep_buf, MX_REPORT_BUF, "L%c%.1f%c%.1f", delim, in, delim, out);
  rep_buf[MX_REPORT_BUF] = 0; /* ensure null termination */
  report (rep_buf);
}

const char *
map_state (int state)
{
  int i, j;
  int next = 0;
  *states = '\0';
  if (state == 0)
    {
      return "free";
    }
  for (i = 0, j = 1; map[i] != NULL; i++, j <<= 1)
    {
      if (state & j)
	{
	  if (next)
	    strcat (states, ",");
	  strcat (states, map[i]);
	  next = 1;
	}
    }
  return states;
}

void
event_unknown (int type)
{
  snprintf (rep_buf, MX_REPORT_BUF, "U%c%d", delim, type);
  rep_buf[MX_REPORT_BUF] = 0; /* ensure null termination */
  report (rep_buf);
}

void
report (char *text)
{
  if (debug)
    {
      SG_LOG( SG_GENERAL, SG_ALERT, text );
      fflush (stdout);
    }
}

void
ptt (int mode)
{
  if (mode == 1)
    {
      /* mic is muted so unmute and mute speaker */
      iaxc_input_level_set (level_in);
      /*if (!check_special_frq (selected_frequency))
	{*/
	  iaxc_output_level_set (0.0);
          SG_LOG( SG_GENERAL, SG_ALERT, "[SPEAK] unmute mic, mute speaker" );
	/*}
      else
	{
          SG_LOG( SG_GENERAL, SG_ALERT, "[SPEAK] unmute mic" );
	}*/
    }
  else
    {
      /* mic is unmuted so mute and unmute speaker */
      iaxc_input_level_set (0.0);
      /*if (!check_special_frq (selected_frequency))
	{*/
	  iaxc_output_level_set (level_out);
          SG_LOG( SG_GENERAL, SG_ALERT, "[LISTEN] mute mic, unmute speaker" );
	/*}
      else
	{
          SG_LOG( SG_GENERAL, SG_ALERT, "[LISTEN] mute mic" );
	}*/
    }
}

int
split (char *string, char *fields[], int nfields, const char *sep)
{
  register char *p = string;
  register char c;		/* latest character */
  register char sepc = sep[0];
  register char sepc2;
  register int fn;
  register char **fp = fields;
  register const char *sepp;
  register int trimtrail;
  /* white space */
  if (sepc == '\0')
    {
      while ((c = *p++) == ' ' || c == '\t')
	continue;
      p--;
      trimtrail = 1;
      sep = " \t";		/* note, code below knows this is 2 long */
      sepc = ' ';
    }
  else
    trimtrail = 0;
  sepc2 = sep[1];		/* now we can safely pick this up */
  /* catch empties */
  if (*p == '\0')
    return (0);
  /* single separator */
  if (sepc2 == '\0')
    {
      fn = nfields;
      for (;;)
	{
	  *fp++ = p;
	  fn--;
	  if (fn == 0)
	    break;
	  while ((c = *p++) != sepc)
	    if (c == '\0')
	      return (nfields - fn);
	  *(p - 1) = '\0';
	}
      /* we have overflowed the fields vector -- just count them */
      fn = nfields;
      for (;;)
	{
	  while ((c = *p++) != sepc)
	    if (c == '\0')
	      return (fn);
	  fn++;
	}
      /* not reached */
    }

  /* two separators */
  if (sep[2] == '\0')
    {
      fn = nfields;
      for (;;)
	{
	  *fp++ = p;
	  fn--;
	  while ((c = *p++) != sepc && c != sepc2)
	    if (c == '\0')
	      {
		if (trimtrail && **(fp - 1) == '\0')
		  fn++;
		return (nfields - fn);
	      }
	  if (fn == 0)
	    break;
	  *(p - 1) = '\0';
	  while ((c = *p++) == sepc || c == sepc2)
	    continue;
	  p--;
	}
      /* we have overflowed the fields vector -- just count them */
      fn = nfields;
      while (c != '\0')
	{
	  while ((c = *p++) == sepc || c == sepc2)
	    continue;
	  p--;
	  fn++;
	  while ((c = *p++) != '\0' && c != sepc && c != sepc2)
	    continue;
	}
      /* might have to trim trailing white space */
      if (trimtrail)
	{
	  p--;
	  while ((c = *--p) == sepc || c == sepc2)
	    continue;
	  p++;
	  if (*p != '\0')
	    {
	      if (fn == nfields + 1)
		*p = '\0';
	      fn--;
	    }
	}
      return (fn);
    }

  /* n separators */
  fn = 0;
  for (;;)
    {
      if (fn < nfields)
	*fp++ = p;
      fn++;
      for (;;)
	{
	  c = *p++;
	  if (c == '\0')
	    return (fn);
	  sepp = sep;
	  while ((sepc = *sepp++) != '\0' && sepc != c)
	    continue;
	  if (sepc != '\0')	/* it was a separator */
	    break;
	}
      if (fn < nfields)
	*(p - 1) = '\0';
      for (;;)
	{
	  c = *p++;
	  sepp = sep;
	  while ((sepc = *sepp++) != '\0' && sepc != c)
	    continue;
	  if (sepc == '\0')	/* it wasn't a separator */
	    break;
	}
      p--;
    }

  /* not reached */
}

/**
 *
 * \fn double *read_special_frequencies(const char *file)
 *
 * \brief Reads the file "special_frequencies.txt" if it exists.
 * If no file exists, then no special frequencies are useable.
 *
 * \param file	pointer on the filename.
 *
 * \return Returns the pointer on an array containing doubles if file
 * has been successfully opened and read, otherwise returns NULL.
 *
 */
double *read_special_frequencies(const char *file)
{
	double *l_pfrq = NULL;
	double l_value;
	int l_count;
	int l_allocated = 0;
	int l_new_size;

	if((l_pfrq = (double *)malloc(ALLOC_CHUNK_SIZE * sizeof(double))) != NULL)
	{
		l_allocated += ALLOC_CHUNK_SIZE;

		if(FGC_SUCCESS(parser_init(file)))
		{
			l_count = 0;

			while(FGC_SUCCESS(parser_get_next_value(&l_value)))
			{
				if(l_count >= l_allocated)
				{
					l_new_size = ALLOC_CHUNK_SIZE * (l_count / ALLOC_CHUNK_SIZE + 1);
					l_pfrq = (double *)realloc(l_pfrq, l_new_size * sizeof(double));
					l_allocated += ALLOC_CHUNK_SIZE;
				}

				l_pfrq[l_count] = l_value;

				l_count++;
			}

			/* Last value of the array must be -1.0 which is the terminator. */
			if(l_count == l_allocated)
				l_pfrq = (double *)realloc(l_pfrq, (l_count + 1) * sizeof(double));
			l_pfrq[l_count] = -1.0;
		} else {
            // failed to open file
            parser_exit();
            free(l_pfrq);
            return 0;
        }
	}

	parser_exit();

	return(l_pfrq);
}


struct airport *
read_airports (const char *file)
{
  FILE *fp;
  int ret;
  struct airport airport_tmp;
  struct airport *first = NULL;
  struct airport *my_airport = NULL;
  struct airport *previous_airport = NULL;
  size_t counter = 0;

  SG_LOG( SG_GENERAL, SG_ALERT, "Reading airports [" << file << "]" );

  if ((fp = fopen (file, "rt")) == NULL)
    {
      SG_LOG( SG_GENERAL, SG_ALERT, "ERROR: open failed!" );
      perror ("fopen");
      exitcode = 120;
      quit (0);
    }

  airport_tmp.next = NULL;
  while ((ret = fscanf (fp, " %4[^,],%f,%lf,%lf,%128[^,],%128[^\r\n]",
			airport_tmp.icao, &airport_tmp.frequency,
			&airport_tmp.lat, &airport_tmp.lon,
			airport_tmp.type, airport_tmp.text)) == 6)
    {
        counter++;
      if ((my_airport =
	   (struct airport *) malloc (sizeof (struct airport))) == NULL)
	{
          SG_LOG( SG_GENERAL, SG_ALERT, "Error allocating memory for airport data" );
	  exitcode = 900;
	  quit (0);
	}

      if (first == NULL)
	first = my_airport;
      memcpy (my_airport, &airport_tmp, sizeof (airport_tmp));
      if (previous_airport != NULL)
	{
	  previous_airport->next = my_airport;
	}
      previous_airport = my_airport;
    }

  fclose (fp);
  if (ret != EOF)
    {
      SG_LOG( SG_GENERAL, SG_ALERT, "ERROR during reading airports!" );
      exitcode = 900;
      quit (0);
    }

  SG_LOG( SG_GENERAL, SG_ALERT, "loaded " << counter << " entries" );
  return (first);
}

char *
report_devices (int in)
{
  struct iaxc_audio_device *devs;  //audio devices
  int ndevs;			   //audio dedvice count
  int input, output, ring;	   //audio device id's
  int current, i;
  int flag = in ? IAXC_AD_INPUT : IAXC_AD_OUTPUT;
  iaxc_audio_devices_get (&devs, &ndevs, &input, &output, &ring);
  current = in ? input : output;
  snprintf (rep_buf, MX_REPORT_BUF, "%s\n", devs[current].name);
  for (i = 0; i < ndevs; i++)
    {
      if (devs[i].capabilities & flag && i != current)
	{
	  snprintf (rep_buf + strlen (rep_buf), MX_REPORT_BUF - strlen (rep_buf), "%s\n",
		    devs[i].name);
	}
        rep_buf[MX_REPORT_BUF] = 0; /* ensure null termination */
        if (strlen(rep_buf) >= MX_REPORT_BUF)
            break;
    }
  rep_buf[MX_REPORT_BUF] = 0; /* ensure null termination */
  return rep_buf;
}

int
set_device (const char *name, int out)
{
  struct iaxc_audio_device *devs;	/* audio devices */
  int ndevs;			/* audio dedvice count */
  int input, output, ring;	/* audio device id's */
  int i;
  iaxc_audio_devices_get (&devs, &ndevs, &input, &output, &ring);
  for (i = 0; i < ndevs; i++)
    {
      if (devs[i].capabilities & (out ? IAXC_AD_OUTPUT : IAXC_AD_INPUT) &&
	  strcmp (name, devs[i].name) == 0)
	{
	  if (out)
	    {
	      output = devs[i].devID;
	    }
	  else
	    {
	      input = devs[i].devID;
	    }
	  fprintf (stderr, "device %s = %s (%d)\n", out ? "out" : "in", name,
		   devs[i].devID);
	  iaxc_audio_devices_set (input, output, ring);
	  return 1;
	}
    }
  return 0;
}

void
parse_fgdata (struct fgdata *data, char *buf)
{
  char *data_pair = NULL;
  char *fields[2];
  fields[0] = '\0';
  fields[1] = '\0';
  SG_LOG( SG_GENERAL, SG_DEBUG, "Parsing data: [" << buf << "]" );
  /* Parse data from FG */
  data_pair = strtok (buf, ",");
  while (data_pair != NULL)
    {
      split (data_pair, fields, 2, "=");
      if (strcmp (fields[0], "COM1_FRQ") == 0)
	{
	  data->COM1_FRQ = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "COM1_FRQ=" << data->COM1_FRQ );
	}
      else if (strcmp (fields[0], "COM2_FRQ") == 0)
	{
	  data->COM2_FRQ = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "COM2_FRQ=" << data->COM2_FRQ );
	}
      else if (strcmp (fields[0], "NAV1_FRQ") == 0)
	{
	  data->NAV1_FRQ = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "NAV1_FRQ=" << data->NAV1_FRQ );
	}
      else if (strcmp (fields[0], "NAV2_FRQ") == 0)
	{
	  data->NAV2_FRQ = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "NAV2_FRQ=" << data->NAV2_FRQ );
	}
      else if (strcmp (fields[0], "PTT") == 0)
	{
	  data->PTT = atoi (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "PTT=" << data->PTT );
	}
      else if (strcmp (fields[0], "TRANSPONDER") == 0)
	{
	  data->TRANSPONDER = atoi (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "TRANSPONDER=" << data->TRANSPONDER );
	}
      else if (strcmp (fields[0], "IAS") == 0)
	{
	  data->IAS = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "IAS=" << data->IAS );
	}
      else if (strcmp (fields[0], "GS") == 0)
	{
	  data->GS = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "GS=" << data->GS );
	}
      else if (strcmp (fields[0], "LON") == 0)
	{
	  data->LON = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "LON=" << data->LON );
	}
      else if (strcmp (fields[0], "LAT") == 0)
	{
	  data->LAT = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "LAT=" << data->LAT );
	}
      else if (strcmp (fields[0], "ALT") == 0)
	{
	  data->ALT = atoi (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "ALT=" << data->ALT );
	}
      else if (strcmp (fields[0], "HEAD") == 0)
	{
	  data->HEAD = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "HEAD=" << data->HEAD );
	}
      else if (strcmp (fields[0], "COM1_SRV") == 0)
	{
	  data->COM1_SRV = atoi (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "COM1_SRV" << data->COM1_SRV );
	}
      else if (strcmp (fields[0], "COM2_SRV") == 0)
	{
	  data->COM2_SRV = atoi (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "COM2_SRV=" << data->COM2_SRV );
	}
      else if (strcmp (fields[0], "NAV1_SRV") == 0)
	{
	  data->NAV1_SRV = atoi (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "NAV1_SRV=" << data->NAV1_SRV );
	}
      else if (strcmp (fields[0], "NAV2_SRV") == 0)
	{
	  data->NAV2_SRV = atoi (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "NAV2_SRV=" << data->NAV2_SRV );
	}
      else if (strcmp (fields[0], "OUTPUT_VOL") == 0)
	{
	  data->OUTPUT_VOL = atof (fields[1]);
          SG_LOG( SG_GENERAL, SG_DEBUG, "OUTPUT_VOL=" << data->OUTPUT_VOL );
	}
      else if (strcmp (fields[0], "CALLSIGN") == 0)
	{
	  data->CALLSIGN = fields[1];
          SG_LOG( SG_GENERAL, SG_DEBUG, "CALLSIGN=" << data->CALLSIGN );
	}
      else
	{
          SG_LOG( SG_GENERAL, SG_DEBUG, "Unknown field " << fields[0] << "=" << fields[1] );
	}

      data_pair = strtok (NULL, ",");
    }
  SG_LOG( SG_GENERAL, SG_DEBUG, "" );
}

/**
 *
 * \fn int check_special_frq (double frq)
 *
 * \brief Check to see if specified frequency is a special frequency.
 *
 * \param frq	frequency to check against special frequencies
 *
 * \return Returns 1 if successful, otherwise returns 0.
 *
 */
int check_special_frq (double frq)
{
	int i = 0;
    frq = ceilf(frq*1000.0)/1000.0; // 20130602: By Clement de l'Hamaide, to 'Make 123.450Mhz usable'
	while (special_frequencies[i] >= 0.0)
	{
		if (frq == special_frequencies[i])
		{
                        SG_LOG( SG_GENERAL, SG_ALERT, "Special frequency: " << frq );
			return (1);
		}
		i++;
	}
	return (0);
}

void
do_iaxc_call (const char *username, const char *password,
	      const char *voipserver, char *number)
{
  char dest[256];
  size_t len = strlen(number);

  if( strcmp(voipserver, "delta384.server4you.de") == 0 ) {
    if( number[len-1] == '5' ) {
      number[len-1] = '0';
    }
  }

  if( strcmp(number, "9990909090910000") == 0)
    number = (char *)"0190909090910000";

  snprintf (dest, sizeof (dest), "%s:%s@%s/%s", username, password,
	    voipserver, number);
  iaxc_call (dest);
  iaxc_millisleep (DEFAULT_MILLISLEEP);
}

/* eof - fgcom.cpp */
