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
 * (c) C. de l Hamaide <clemaez@hotmail.fr> - 2013
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
#ifdef _MSC_VER
#include "winpoop.h"
#endif
#include <sys/types.h>
#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <simgear/io/raw_socket.hxx>
#include <plib/netSocket.h> /* FIXME: need to replace ulClock	 */
#include "fgcom.h"
#include "fgcom_init.h"
#include "utils.h"
#include "version.h"

/**
 *
 * \define ALLOCK_CHUNK_SIZE
 * \brief Size of a memory chunk to allocate.
 *
 */
#define ALLOC_CHUNK_SIZE	5
#define FGCOM_VERSION		2.11



/* global vars */
int exitcode = 0;

int initialized = 0;
int connected = 0;
int reg_id;

/*
static int port;
const char* voipserver;
const char* fgserver;
*/

/**
 * Global variables.
 */
const char* dialstring;
char airport[5];
int codec = DEFAULT_IAX_CODEC;

/**
 * Variables declared as static.
 */
static double selected_frequency = 0.0;
static char mode = 0;
static char tmp[1024];		/* report output buffer */
static int last_state = 0;	/* previous state of the channel */
static char states[256];	/* buffer to hold ascii states */
static char delim = '\t';	/* output field delimiter */
static const char *map[] = {
  "unknown", "active", "outgoing", "ringing", "complete", "selected",
  "busy", "transfer", NULL
};

#ifdef DEBUG /* only used in DEBUG, to show mode */
static const char *mode_map[] = {
  (const char *)"ATC", (const char *)"Flightgear"
};
#endif

static const char *radio_map[] = {
  "COM1", "NAV1", "COM2", "NAV2"
};

/*const */ double earthradius = 6370.0;
/* radius of  earth */
/*const */ double uf = 0.01745329251994329509;
/* conversion factor pi/180 degree->rad */
struct airport *airportlist;
struct fgdata data;
char icao[5];
double special_frq[] =
  { 999.999, 910.0, 123.45, 122.75, 123.5, 121.5, 732.34, -1.0 };
double *special_frequencies;

simgear::Socket sgSocket;

double previous_com_frequency = 0.0;
int previous_ptt = 0;
int com_select = 0;
int max_com_instruments = 2;

/* program name */
char *prog;

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
  else if (com_select == 1 && data.NAV1_SRV == 1)
    selected_frequency = data.NAV1_FRQ;
  else if (com_select == 2 && data.COM2_SRV == 1)
    selected_frequency = data.COM2_FRQ;
  else if (com_select == 3 && data.NAV2_SRV == 1)
    selected_frequency = data.NAV2_FRQ;

  /* Check for com frequency changes */
  if (previous_com_frequency != selected_frequency)
    {
      printf ("Selected frequency: %3.3f\n", selected_frequency);

      /* remark the new frequency */
      previous_com_frequency = selected_frequency;

      if (connected == 1)
	{
	  /* hangup call, if connected */
printf("#### DUMP0 ####");
	  iaxc_dump_call ();
	  iaxc_millisleep (5 * DEFAULT_MILLISLEEP);
	  connected = 0;
	}

      strcpy (icao,
	      icaobypos (airportlist, selected_frequency, data.LAT,
			 data.LON, DEFAULT_RANGE));
      icao2number (icao, selected_frequency, tmp);
#ifdef DEBUG
      printf ("DEBUG: dialing %s %3.3f MHz: %s\n", icao, selected_frequency, tmp);
#endif
      do_iaxc_call (username, password, voipserver, tmp);
      /* iaxc_select_call (0); */

      connected = 1;
    }
  /* Check for pressed PTT key */
  if (previous_ptt != data.PTT)
    {
      if (data.PTT == 2)
	{
	  /* select the next com equipment */
	  com_select = (com_select + 1) % 4;
	  printf ("Radio-Select: %s\n", radio_map[com_select]);
	}
      else if (connected == 1)
	{
	  ptt (data.PTT);
	}
      previous_ptt = data.PTT;
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

/* adjust default input per OS 
   but no adjustment if it is a user input
#ifdef MACOSX
#endif // MACOSX   
*/

#define MX_PATH_SIZE 2000

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
  static char buf[MAXBUFLEN];
  //int c;
  //int ret = 0;

	prog = strdup( base_name(argv[0]) );
  
	/* program header */
	std::cout << prog << " - a communication radio based on VoIP with IAX/Asterisk" << std::endl;
	std::cout << "Original (c) 2007-2011 by H. Wirtz <wirtz@dfn.de>" << std::endl;
    std::cout << "OSX and Windows ports 2012-2013 by Yves Sablonier and Geoff R. McLane, resp." << std::endl;
	std::cout << "Version " << FGCOM_VERSION << " compiled " << __DATE__ << ", at " << __TIME__ << std::endl;
	std::cout << "Using iaxclient library Version " << iaxc_version (tmp) << std::endl;
	std::cout << std::endl;

  /* init values */
  voipserver = DEFAULT_VOIP_SERVER;
  fgserver = DEFAULT_FG_SERVER;
  port = DEFAULT_FG_PORT;
  username = DEFAULT_USER;
  password = DEFAULT_PASSWORD;
  codec_option = DEFAULT_CODEC;
  mode = 0;			/* 0 = ATC mode, 1 = FG mode */
  positions_file = (char *) DEFAULT_POSITIONS_FILE;
  frequency_file = (char *) SPECIAL_FREQUENCIES_FILE;

#ifndef _WIN32
  /* catch signals */
  signal (SIGINT, quit);
  signal (SIGQUIT, quit);
  signal (SIGTERM, quit);
#endif

  /* setup iax */
#ifdef HAVE_IAX12
  if (iaxc_initialize (DEFAULT_MAX_CALLS))
#else
  if (iaxc_initialize (DEFAULT_IAX_AUDIO, DEFAULT_MAX_CALLS))
#endif
    fatal_error ("cannot initialize iaxclient!\nHINT: Have you checked the mic and speakers?");

  initialized = 1;

  // option parser
  fgcomInitOptions (fgcomOptionArray, argc, argv);

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
      std::cout << "Input audio devices:" << std::endl;
      std::cout << report_devices (IAXC_AD_INPUT) << std::endl;

      std::cout << "Output audio devices:" << std::endl;
      std::cout << report_devices (IAXC_AD_OUTPUT) << std::endl;

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

//#ifdef DEBUG
  /* Print any remaining command line arguments (not options). */
  //if (optind < argc) {
  //      printf ("non-option ARGV-elements: ");
  //      while (optind < argc)
  //              printf ("%s ", argv[optind++]);
  //      putchar ('\n');
  //}
//#endif

  /* checking consistency of arguments */
  if (frequency > 0.0 && frequency < 1000.0)
    {
      if (strlen (airport) == 0 || strlen (airport) > 4)
	{
	  strcpy (airport, "ZZZZ");
	}
      /* airport and frequency are given => ATC mode */
      std::cout << "# FGCOM is used in ATC mode" << std::endl;
      mode = 0;
    }
  else
    {
      /* no airport => FG mode */
      std::cout << "# FGCOM is used in FG mode" << std::endl;
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
        std::cout << "Failed to load file [" << frequency_file << "]!\nUsing internal defaults." << std::endl;
        special_frequencies = special_frq;
    } else {
        std::cout << "Loaded file [" << frequency_file << "]." << std::endl;
    }

	/* read airport frequencies and positions */
	airportlist = read_airports (positions_file);   /* never returns if fail! */

  /* preconfigure iax */
  std::cout << "Initializing IAX client as " << username << ":" <<
    "xxxxxxxxxxx@" << voipserver << std::endl;

  iaxc_set_callerid (const_cast < char *>(username),
		     const_cast < char *>("0125252525122750"));

  iaxc_set_formats (IAXC_FORMAT_GSM, IAXC_FORMAT_GSM);
  iaxc_set_event_callback (iaxc_callback);

  iaxc_start_processing_thread ();

  if (username && password && voipserver)
    {
      reg_id =
	iaxc_register (const_cast < char *>(username),
		       const_cast < char *>(password),
		       const_cast < char *>(voipserver));
#ifdef DEBUG
      std::cout << "DEBUG: Registered as '" << username << "' at '" << voipserver <<
	"'." << std::endl;
#endif
    }
  else
    {
      std::cout << "Failed iaxc_register!\nHINT: Check user name, pwd and ip of server." << std::endl;
      exitcode = 130;
      quit (0);
    }

  iaxc_millisleep (DEFAULT_MILLISLEEP);

  /* main loop */
#ifdef DEBUG
  std::cout << "Entering main loop in mode " << mode_map[mode] << "." <<
    std::endl;
#endif

  if (mode == 1)
    {
      /* only in FG mode */
      simgear::Socket::initSockets();
      sgSocket.open (false);
      sgSocket.bind (fgserver, port);

      /* mute mic, speaker on */
      iaxc_input_level_set (0);
      iaxc_output_level_set (level_out);

      ulClock clock;
      clock.update ();
      double next_update = clock.getAbsTime () + DEFAULT_ALARM_TIMER;
      /* get data from flightgear */
      while (1)
	{
	  clock.update ();
	  double wait = next_update - clock.getAbsTime ();
	  if (wait > 0.001)
	    {
	      simgear::Socket *readSockets[2] = { &sgSocket, 0 };
	      if (sgSocket.select (readSockets, readSockets + 1,
				   (int) (wait * 1000)) == 1)
		{
                  simgear::IPAddress their_addr;
                  numbytes = sgSocket.recvfrom(buf, MAXBUFLEN - 1, 0, &their_addr);
		  if (numbytes == -1)
		    {
		      perror ("recvfrom");
		      exit (1);
		    }
		  buf[numbytes] = '\0';
#ifdef DEBUG
		  std::
		    cout << "DEBUG: got packet from " << their_addr.getHost () << ":"
		    << their_addr.getPort () << std::endl;
		  std::cout << "packet is " << numbytes << " bytes long" <<
		    std::endl;
		  std::
		    cout << "packet contains \"" << buf << "\"" << std::endl;
#endif
		  process_packet (buf);
		}
	    }
	  else
	    {
	      alarm_handler (0);
	      clock.update ();
	      next_update = clock.getAbsTime () + DEFAULT_ALARM_TIMER;
	    }
	}
    }
  else
    {
      /* only in ATC mode */
      //struct pos p;

      /* mic on, speaker on */
      iaxc_input_level_set (1.0);
      iaxc_output_level_set (0.0);

      /* get geo positions of the airport */
      //p = posbyicao (airportlist, airport);

      icao2atisnumber (airport, frequency, tmp);
#ifdef DEBUG
      printf ("DEBUG: dialing %s %3.3f MHz: %s\n", airport, frequency, tmp);
#endif
      do_iaxc_call (username, password, voipserver, tmp);
      /* iaxc_select_call (0); */

      while (1)
	{
	  /* sleep endless */
	  ulSleep (3600);
	}
    }

  /* should never be reached */
  exitcode = 999;
  quit (0);
}

void
quit (int signal)
{
  std::cout << "Stopping service." << std::endl;

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
printf("#### DUMP1 ####");
      iaxc_dump_call ();
      iaxc_millisleep (5 * DEFAULT_MILLISLEEP);
      connected = 0;
    }

  /* Check if we are now in range */
  else if (strlen (icao) != 0 && connected == 0)
    {
      icao2number (icao, selected_frequency, tmp);
#ifdef DEBUG
      printf ("DEBUG: dialing %s %3.3f MHz: %s\n", icao, selected_frequency, tmp);
#endif
      do_iaxc_call (username, password, voipserver, tmp);

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
  std::cerr << "FATAL ERROR: " << err << std::endl;
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

  snprintf (tmp, sizeof (tmp),
	    "S%c0x%x%c%s%c%.50s%c%.50s%c%.50s%c%.50s", delim, state,
	    delim, map_state (state), delim, remote, delim, remote_name,
	    delim, local, delim, local_context);
  report (tmp);
}

void
event_text (int type, char *message)
{
  snprintf (tmp, sizeof (tmp), "T%c%d%c%.200s", delim, type, delim, message);
  std::cout << message << std::endl;
  report (tmp);
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
	  std::cout << "Registering denied" << std::endl;
	  /* exitcode = 110;
	     quit (SIGTERM); */
	}
      break;
    case IAXC_REGISTRATION_REPLY_TIMEOUT:
      reason = "timeout";
      break;
    default:
      reason = "unknown";
    }
  snprintf (tmp, sizeof (tmp), "R%c%d%c%s%c%d", delim, id, delim,
	    reason, delim, count);
  report (tmp);
}

void
event_netstats (struct iaxc_ev_netstats stat)
{
  struct iaxc_netstat local = stat.local;
  struct iaxc_netstat remote = stat.remote;
  snprintf (tmp, sizeof (tmp),
	    "N%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d%c%d",
	    delim, stat.callNo, delim, stat.rtt,
	    delim, local.jitter, delim, local.losspct, delim,
	    local.losscnt, delim, local.packets, delim, local.delay,
	    delim, local.dropped, delim, local.ooo, delim,
	    remote.jitter, delim, remote.losspct, delim, remote.losscnt,
	    delim, remote.packets, delim, remote.delay, delim,
	    remote.dropped, delim, remote.ooo);
  report (tmp);
}

void
event_level (double in, double out)
{
  snprintf (tmp, sizeof (tmp), "L%c%.1f%c%.1f", delim, in, delim, out);
  report (tmp);
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
  snprintf (tmp, sizeof (tmp), "U%c%d", delim, type);
  report (tmp);
}

void
report (char *text)
{
  if (debug)
    {
      std::cout << text << std::endl;
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
      if (!check_special_frq (selected_frequency))
	{
	  iaxc_output_level_set (0.0);
	  std::cout << "[SPEAK] unmute mic, mute speaker" << std::endl;
	}
      else
	{
	  std::cout << "[SPEAK] unmute mic" << std::endl;
	}
    }
  else
    {
      /* mic is unmuted so mute and unmute speaker */
      iaxc_input_level_set (0.0);
      if (!check_special_frq (selected_frequency))
	{
	  iaxc_output_level_set (level_out);
	  std::cout << "[LISTEN] mute mic, unmute speaker" << std::endl;
	}
      else
	{
	  std::cout << "[LISTEN] mute mic" << std::endl;
	}
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

  printf ("Reading airports [%s]... ",file);

  if ((fp = fopen (file, "rt")) == NULL)
    {
      printf ("ERROR: open failed!\n");
      perror ("fopen");
      exitcode = 120;
      quit (0);
    }

  airport_tmp.next = NULL;
  while ((ret = fscanf (fp, " %4[^,],%f,%lf,%lf,%32[^,],%128[^\r\n]",
			airport_tmp.icao, &airport_tmp.frequency,
			&airport_tmp.lat, &airport_tmp.lon,
			airport_tmp.type, airport_tmp.text)) == 6)
    {
        counter++;
      if ((my_airport =
	   (struct airport *) malloc (sizeof (struct airport))) == NULL)
	{
	  printf ("Error allocating memory for airport data\n");
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
      printf ("ERROR during reading airports!\n");
      exitcode = 900;
      quit (0);
    }

  printf ("loaded %lu entries.\n",counter);
  return (first);
}

char *
report_devices (int in)
{
  struct iaxc_audio_device *devs;	/* audio devices */
  int ndevs;			/* audio dedvice count */
  int input, output, ring;	/* audio device id's */
  int current, i;
  int flag = in ? IAXC_AD_INPUT : IAXC_AD_OUTPUT;
  iaxc_audio_devices_get (&devs, &ndevs, &input, &output, &ring);
  current = in ? input : output;
  snprintf (tmp, sizeof (tmp), "%s\n", devs[current].name);
  for (i = 0; i < ndevs; i++)
    {
      if (devs[i].capabilities & flag && i != current)
	{
	  snprintf (tmp + strlen (tmp), sizeof (tmp) - strlen (tmp), "%s\n",
		    devs[i].name);
	}
    }
  return tmp;
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
#ifdef DEBUG
  std::cout << "DEBUG: Parsing data: [" << buf << "]" << std::endl;
#endif
  /* Parse data from FG */
  data_pair = strtok (buf, ",");
  while (data_pair != NULL)
    {
      split (data_pair, fields, 2, "=");
      if (strcmp (fields[0], "COM1_FRQ") == 0)
	{
	  data->COM1_FRQ = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: COM1_FRQ=%3.3f\n", data->COM1_FRQ);
#endif
	}
      else if (strcmp (fields[0], "COM2_FRQ") == 0)
	{
	  data->COM2_FRQ = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: COM2_FRQ=%3.3f\n", data->COM2_FRQ);
#endif
	}
      else if (strcmp (fields[0], "NAV1_FRQ") == 0)
	{
	  data->NAV1_FRQ = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: NAV1_FRQ=%3.3f\n", data->NAV1_FRQ);
#endif
	}
      else if (strcmp (fields[0], "NAV2_FRQ") == 0)
	{
	  data->NAV2_FRQ = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: NAV2_FRQ=%3.3f\n", data->NAV2_FRQ);
#endif
	}
      else if (strcmp (fields[0], "PTT") == 0)
	{
	  data->PTT = atoi (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: PTT=%d\n", data->PTT);
#endif
	}
      else if (strcmp (fields[0], "TRANSPONDER") == 0)
	{
	  data->TRANSPONDER = atoi (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: TRANSPONDER=%d\n", data->TRANSPONDER);
#endif
	}
      else if (strcmp (fields[0], "IAS") == 0)
	{
	  data->IAS = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: IAS=%f\n", data->IAS);
#endif
	}
      else if (strcmp (fields[0], "GS") == 0)
	{
	  data->GS = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: GS=%f\n", data->GS);
#endif
	}
      else if (strcmp (fields[0], "LON") == 0)
	{
	  data->LON = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: LON=%f\n", data->LON);
#endif
	}
      else if (strcmp (fields[0], "LAT") == 0)
	{
	  data->LAT = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: LAT=%f\n", data->LAT);
#endif
	}
      else if (strcmp (fields[0], "ALT") == 0)
	{
	  data->ALT = atoi (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: ALT=%d\n", data->ALT);
#endif
	}
      else if (strcmp (fields[0], "HEAD") == 0)
	{
	  data->HEAD = atof (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: HEAD=%f\n", data->HEAD);
#endif
	}
      else if (strcmp (fields[0], "COM1_SRV") == 0)
	{
	  data->COM1_SRV = atoi (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: COM1_SRV=%d\n", data->COM1_SRV);
#endif
	}
      else if (strcmp (fields[0], "COM2_SRV") == 0)
	{
	  data->COM2_SRV = atoi (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: COM2_SRV=%d\n", data->COM2_SRV);
#endif
	}
      else if (strcmp (fields[0], "NAV1_SRV") == 0)
	{
	  data->NAV1_SRV = atoi (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: NAV1_SRV=%d\n", data->NAV1_SRV);
#endif
	}
      else if (strcmp (fields[0], "NAV2_SRV") == 0)
	{
	  data->NAV2_SRV = atoi (fields[1]);
#ifdef DEBUG
	  printf ("DEBUG: NAV2_SRV=%d\n", data->NAV2_SRV);
#endif
	}
#ifdef DEBUG
      else
	{
	  printf ("DEBUG: Unknown val: %s (%s)\n", fields[0], fields[1]);
	}
#endif

      data_pair = strtok (NULL, ",");
    }
#ifdef DEBUG
  printf ("\n");
#endif
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
        frq = ceilf(frq*1000.0)/1000.0;
	while (special_frequencies[i] >= 0.0)
	{
		if (frq == special_frequencies[i])
		{
			printf("Special freq : %3.3f\n", frq);
			return (1);
		}
		i++;
	}
	return (0);
}

void
do_iaxc_call (const char *username, const char *password,
	      const char *voipserver, const char *number)
{
  char dest[256];

  snprintf (dest, sizeof (dest), "%s:%s@%s/%s", username, password,
	    voipserver, number);
  iaxc_call (dest);
  iaxc_millisleep (DEFAULT_MILLISLEEP);
}

/* eof - fgcom.cpp */
