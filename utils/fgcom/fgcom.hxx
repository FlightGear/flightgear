/*
 * fgcom - VoIP-Client for the FlightGear-Radio-Infrastructure
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

#ifndef __FGCOM_H__
#define __FGCOM_H__

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <iaxclient.h>
#include <math.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning ( disable : 4244 ) // from double to float
#pragma warning ( disable : 4996 ) // depreciaed, really
#include <io.h> // for open. read, ...
#else
#include <signal.h>
#endif


#define DEFAULT_USER            "guest"
#define DEFAULT_PASSWORD        "guest"
#define DEFAULT_FG_SERVER       "localhost"
#define DEFAULT_FG_PORT         16661
#define DEFAULT_CODE            1
#define ATIS_CODE               99
#define DEFAULT_VOIP_SERVER     "fgcom.flightgear.org"
#define DEFAULT_CODEC           'u'
#define DEFAULT_IAX_CODEC       IAXC_FORMAT_ULAW
#define DEFAULT_IAX_AUDIO       AUDIO_INTERNAL
#define DEFAULT_MAX_CALLS       2
#define DEFAULT_MILLISLEEP      100
#define DEFAULT_RANGE           100.0
#define DEFAULT_LOWER_FRQ_LIMIT 108.0
#define DEFAULT_UPPER_FRQ_LIMIT 140.0
#define MAXBUFLEN               1024
#define DEFAULT_ALARM_TIMER     5
#define ALLOC_CHUNK_SIZE        5 //Size of a memory chunk to allocate
#define MX_REPORT_BUF           1024
#define MX_PATH_SIZE            2000


/* avoid name clash with winerror.h */
#define FGC_SUCCESS(__x__)		(__x__ == 0)
#define FGC_FAILED(__x__)		(__x__ < 0)

#ifndef SPECIAL_FREQUENCIES_FILE
#define SPECIAL_FREQUENCIES_FILE "fgcom-data\\special_frequencies.txt"
#endif
#ifndef DEFAULT_POSITIONS_FILE
#define DEFAULT_POSITIONS_FILE "fgcom-data\\positions.txt"
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#define inline __inline
#ifdef WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#endif

#ifndef FGCOM_VERSION
#ifndef FLIGHTGEAR_VERSION
#define FGCOM_VERSION FLIGHTGEAR_VERSION
#else
#define FGCOM_VERSION "2.99"
#endif
#endif

struct airport
{
  char icao[5];
  float frequency;
  double lat;
  double lon;
  char type[33];
  char text[129];
  struct airport *next;
};

struct pos
{
  double lon;
  double lat;
};

struct fgdata
{
  float COM1_FRQ;
  float COM2_FRQ;
  float NAV1_FRQ;
  float NAV2_FRQ;
  int COM1_SRV;
  int COM2_SRV;
  int NAV1_SRV;
  int NAV2_SRV;
  int PTT;
  int TRANSPONDER;
  float IAS;
  float GS;
  double LON;
  double LAT;
  int ALT;
  float HEAD;
  float OUTPUT_VOL;
  char* CALLSIGN;
};

/* function declaratons */
void quit (int signal);
void alarm_handler (int signal);
void strtoupper (const char *str, char *buf, size_t len);
void usage (char *prog);
int create_socket (int port);
void fatal_error (const char *err);
int iaxc_callback (iaxc_event e);
void event_state (int state, char *remote, char *remote_name, char *local,
		  char *local_context);
void event_text (int type, char *message);
void event_register (int id, int reply, int count);
void report (char *text);
const char *map_state (int state);
void event_unknown (int type);
void event_netstats (struct iaxc_ev_netstats stat);
void event_level (double in, double out);
void icao2number (char *icao, float frequency, char *buf);
void icao2atisnumber (char *icao, float frequency, char *buf);
void ptt (int mode);
double distance (double lat1, double lon1, double lat2, double lon2);
int split (char *string, char *fields[], int nfields, const char *sep);
char *readln (FILE * fp, char *buf, int len);
double *read_special_frequencies(const char *file);
struct airport *read_airports (const char *file);
const char *icaobypos (struct airport *airports, double frequency,
		       double plane_lat, double plane_lon, double range);
void vor (char *icao, double frequency, int mode);
char *report_devices (int in);
int set_device (const char *name, int out);
struct pos posbyicao (struct airport *airports, char *icao);
void parse_fgdata (struct fgdata *data, char *buf);
int check_special_frq (double frq);
void do_iaxc_call (const char *username, const char *password,
		   const char *voipserver, char *number);
#endif
