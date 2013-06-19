/*
 * testcall: make a single test call with IAXCLIENT
 *
 * IAX Support for talking to Asterisk and other Gnophone clients
 *
 * Copyright (C) 1999, Linux Support Services, Inc.
 *
 * Mark Spencer <markster@linux-support.net>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 */

/* #define	PRINTCHUCK /\* enable this to indicate chucked incomming packets *\/ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "iaxclient.h"

#define LEVEL_INCREMENT	0.10

/* static int answered_call; */
static char *output_filename = NULL;
static char *username = NULL;
static char *password = NULL;
static char *host = NULL;
static char *snd_filename = NULL;
static int  reg = 0;
int do_levels = 0;
int intercom = 0;
int initialized = 0;
int reg_id = 0;

/* routine called at exit to shutdown audio I/O and close nicely.
NOTE: If all this isnt done, the system doesnt not handle this
cleanly and has to be rebooted. What a pile of doo doo!! */
void killem(void)
{
	if (initialized) iaxc_shutdown();
	if (reg_id)
	{
		iaxc_unregister(reg_id);
	}
	return;
}

void signal_handler(int signum)
{
	if ( signum == SIGTERM || signum == SIGINT )
	{
		killem();
		exit(0);
	}
}

void fatal_error(char *err)
{
	killem();
	fprintf(stderr, "FATAL ERROR: %s\n", err);
	exit(1);
}

void mysleep(void)
{
	iaxc_millisleep(10);
}

/*
 * Load a sound file to be played when pressing p
 * the file should be signed 16 bit, 8000 Hz, big endian, raw format
 * returns NULL if error
 */
struct iaxc_sound * load_sound(const char *filename)
{
	struct iaxc_sound	*snd = NULL;
	FILE			*f = NULL;
	long			i, len = 0;

	if ( (f = fopen(filename, "r")) == NULL ) return NULL;

	// Find file length
	fseek(f, 0, SEEK_END);
	len = ftell(f) / 2; // len is the number of samples, not bytes
	fseek(f, 0, SEEK_SET);
	
	snd = (struct iaxc_sound *) calloc(1, sizeof(struct iaxc_sound));
	snd->data = (short *) calloc(len, 2);
	snd->len = fread((void *)snd->data, 2, len, f);
	
	// For some reason, on Cygwin, fread does not return the correct number of elements read
	// We need to set the record straight here
	snd->len = len;
	
	for ( i=0 ; i<len ; i++ ) snd->data[i] = ntohs(snd->data[i]);

	fclose(f);

	return snd;
}

int state_event_callback(struct iaxc_ev_call_state call)
{
    if((call.state & IAXC_CALL_STATE_RINGING))
    {
		printf("Receiving Incoming Call Request...\n");
		if ( intercom )
		{
			printf("Intercom mode, answer automatically\n");
			return iaxc_select_call(call.callNo);
		}
    }
    return 0;
}

int levels_callback(float input, float output)
{
    if(do_levels) fprintf(stderr, "IN: %f OUT: %f\n", input, output);
    return 0;
}

int netstat_callback(struct iaxc_ev_netstats n) {
    static int i;
    if(i++%25 == 0)
	fprintf(stderr, "RTT\t"
	    "Rjit\tRlos%\tRlosC\tRpkts\tRdel\tRdrop\tRooo\t"
	    "Ljit\tLlos%\tLlosC\tLpkts\tLdel\tLdrop\tLooo\n"
	    );

    fprintf(stderr, "%d\t"
	  "%d\t%d\t%d\t%d\t%d\t%d\t%d\t"
	  "%d\t%d\t%d\t%d\t%d\t%d\t%d\n",

	    n.rtt,

	    n.remote.jitter,
	    n.remote.losspct,
	    n.remote.losscnt,
	    n.remote.packets,
	    n.remote.delay,
	    n.remote.dropped,
	    n.remote.ooo,

	    n.local.jitter,
	    n.local.losspct,
	    n.local.losscnt,
	    n.local.packets,
	    n.local.delay,
	    n.local.dropped,
	    n.local.ooo
    );

    return 0;
}

int iaxc_callback(iaxc_event e)
{
    switch(e.type) {
        case IAXC_EVENT_LEVELS:
            return levels_callback(e.ev.levels.input, e.ev.levels.output);
        case IAXC_EVENT_NETSTAT:
            return netstat_callback(e.ev.netstats);
        case IAXC_EVENT_TEXT:
            return 0; // don't handle
        case IAXC_EVENT_STATE:
            return state_event_callback(e.ev.call);
        default:
            return 0;  // not handled
    }
}

void list_devices()
{
    struct iaxc_audio_device *devs;
    int nDevs, input, output, ring;
    int i;

    iaxc_audio_devices_get(&devs,&nDevs, &input, &output, &ring);
    for(i=0;i<nDevs;i++) {
	fprintf(stderr, "DEVICE ID=%d NAME=%s CAPS=%lx\n", devs[i].devID, devs[i].name, devs[i].capabilities);
    }
}

void usage()
{
    fprintf(stderr, "Usage: testcall [-?] [-v] [-i] [-t SILENCE_THRESHOLD] [-s SOUND_FILENAME] [-f OUTPUT_FILENAME] [-u USERNAME -p PASSWORD -h HOST]\n");
    exit(1);
}

int main(int argc, char **argv)
{
	FILE			*f;
	char			c;
	int			i;
	char			*dest = NULL;
	double			silence_threshold = -99;
	double			level;
	struct iaxc_sound	*snd = NULL;


	f = stdout;

	for(i=1;i<argc;i++)
	{
	   if(argv[i][0] == '-')
	   {
	      switch(tolower(argv[i][1]))
	      {
		case '?':
		  usage();
		  break;
		case 'v':
		  do_levels = 1;
		  break;
		case 'i':
		  intercom = 1;
		  break;
		case 't':
		  if(i+1 >= argc) usage();
		  silence_threshold = atof(argv[++i]);
		  break;
		case 'f':
		  if(i+1 >= argc) usage();
		  output_filename = argv[++i];
		  break;
		case 'u':
		  if(i+1 >= argc) usage();
		  username = argv[++i];
		  break;
		case 'p':
		  if(i+1 >= argc) usage();
		  password = argv[++i];
		  break;
		case 'h':
		  if(i+1 >= argc) usage();
		  host = argv[++i];
		  break;
		case 's':
		  if (i+1 >= argc) usage();
		  snd_filename = argv[++i];
		  break;
		default:
		  usage();
	      }
	    } else {
	      dest=argv[i];
	    }
	}


	printf("settings: \n");
	printf("\tsilence threshold: %f\n", silence_threshold);
	printf("\tlevel output: %s\n", do_levels ? "on" : "off");
	printf("\tsound filename: %s\n", snd_filename);

	/* activate the exit handler */
	atexit(killem);

	/* install signal handler to catch CRTL-Cs */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	if(output_filename) {
	  FILE *outfile;
	  if(iaxc_initialize(AUDIO_INTERNAL_FILE,1))
	    fatal_error("cannot initialize iaxclient!");
	  initialized = 1;
	  outfile = fopen(output_filename,"w");
	  iaxc_set_files(NULL, outfile);
	} else {
	  if(iaxc_initialize(AUDIO_INTERNAL_PA,1))
	    fatal_error("cannot initialize iaxclient!");
	  initialized = 1;
	}
	
	if ( snd_filename )
	{
		snd = load_sound(snd_filename);
	}

//	iaxc_set_formats(IAXC_FORMAT_SPEEX,IAXC_FORMAT_ULAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX);
	iaxc_set_formats(IAXC_FORMAT_SPEEX,IAXC_FORMAT_SPEEX);
//	iaxc_set_formats(IAXC_FORMAT_GSM,IAXC_FORMAT_GSM);
//	iaxc_set_formats(IAXC_FORMAT_ULAW,IAXC_FORMAT_ULAW);
	//iaxc_set_formats(IAXC_FORMAT_ULAW,IAXC_FORMAT_ILBC|IAXC_FORMAT_ULAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX);
	iaxc_set_silence_threshold(silence_threshold);

	list_devices();

	//if(do_levels)
	  iaxc_set_event_callback(iaxc_callback); 


	fprintf(f, "\n\
	    TestCall accept some keyboard input while it's running.\n\
	    You must hit 'enter' for your keypresses to be recognized,\n\
	    although you can type more than one key on a line\n\
\n\
	    q: hangup and exit.\n\
	    a: answer incoming call\n\
	    t: terminate call\n\
	    0-9 * or #: dial those DTMF digits.\n\
	    g: increase input level\n\
	    b: decrease input level\n\
	    h: increase output level\n\
	    n: decrease output level\n\
	    p: play sound file\n\
	    Enter: display current audio levels\n");

	if(dest)
	{
		fprintf(f, "Calling %s\n", dest);
		iaxc_call(dest);
	} 

	iaxc_start_processing_thread();

	if (username && password && host)
		   reg_id = iaxc_register(username, password, host);

	printf("ready for keyboard input\n");
	
	if(output_filename) {
	    for(;;)
	      iaxc_millisleep(10*1000);
	}
	while((c = getc(stdin))) 
	{
	    switch (tolower(c)) 
	    {
	    case 'a':
			printf("Answering call 0\n");
			iaxc_select_call(0);
	      	break;	
	    case 'g':
	    	level = iaxc_input_level_get();
	    	level += LEVEL_INCREMENT;
	    	if ( level > 1.00 ) level = 1.00;
	    	printf("Increasing input level to %f\n", level);
	    	iaxc_input_level_set(level);
	    	break;
	    case 'b':
	    	level = iaxc_input_level_get();
	    	level -= LEVEL_INCREMENT;
	    	if ( level < 0 ) level = 0.00;
	    	printf("Decreasing input level to %f\n", level);
	    	iaxc_input_level_set(level);
	    	break;
	    case 'h': 
	    	level = iaxc_output_level_get();
	    	level += LEVEL_INCREMENT;
	    	if ( level > 1.00 ) level = 1.00;
	    	printf("Increasing output level to %f\n", level);
	    	iaxc_output_level_set(level);
	    	break;
	    case 'n':
	    	level = iaxc_output_level_get();
	    	level -= LEVEL_INCREMENT;
	    	if ( level < 0 ) level = 0.00;
	    	printf("Decreasing output level to %f\n", level);
	    	iaxc_output_level_set(level);
	    	break;
	    case 'p':
	    	if ( snd )
	    	{
			printf("Playing sound %s\n", snd_filename);
			iaxc_play_sound(snd, 0);
		} else
		{
			printf("No sound file loaded\n");
		}
		break;
	    case 'q':
			printf("Hanging up and exiting\n");
			iaxc_dump_call();
			iaxc_millisleep(1000);
			iaxc_stop_processing_thread();
			exit(0);
	      	break;
		case 't':
			 printf("Terminating call 0\n");
			 iaxc_dump_call();
			 break;
	    case '1': case '2': case '3': case '4': case '5':
	    case '6': case '7': case '8': case '9': case '0':
	    case '#': case '*':
			printf ("sending %c\n", c);
			iaxc_send_dtmf(c);
	      	break;
	    case '\r':
	    	break;
	    case '\n':
	    	printf("Input level = %f -- Output level = %f\n", iaxc_input_level_get(), iaxc_output_level_get());
	    	break;
	    default:
	    	printf("Unknown command '%c'\n", c);
	    }
	}

	return 0;
}
