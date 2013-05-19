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

/* #define	PRINTCHUCK /* enable this to indicate chucked incomming packets */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "iaxclient.h"

#define NETSTATS
#define JITTERMAKER

#ifdef JITTERMAKER
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>


int netfd = -1;
int preferredportno = 0;

#define JM_BUCKETS 500

static int jm_buckets_full= 0;

static int jm_dropped = 0;
static int jm_delayed = 0;

static int jm_droppct = 0;
static int jm_buckpct = 0;
static double jm_jitter = 0;

static int jm_starttime = 0;


struct jm_frame {
    char buf[2048];
    size_t len;
    struct sockaddr from;
    int occupied;
    unsigned deltime;
};

struct jm_frame jm_buckets[JM_BUCKETS];


long jm_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int jm_init() {
        int portno = preferredportno;
        struct sockaddr_in sin;
        unsigned int sinlen;
        int flags;

        if (netfd > -1) {
                /* Sokay, just don't do anything */
                fprintf(stderr,  "Already initialized.");
                return 0;
        }
        netfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (netfd < 0) {
                fprintf(stderr,  "Unable to allocate UDP socket\n");
                fprintf(stderr,  "Unable to allocate UDP socket\n");
                return -1;
        }

        if (preferredportno == 0)
                preferredportno = 4569;

        if (preferredportno > 0) {
                sin.sin_family = AF_INET;
                sin.sin_addr.s_addr = 0;
                sin.sin_port = htons((short)preferredportno);
                if (bind(netfd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
                        fprintf(stderr,  "Unable to bind to preferred port.  Using random one instead.");
                }
        }
        sinlen = sizeof(sin);
        if (getsockname(netfd, (struct sockaddr *) &sin, &sinlen) < 0) {
                close(netfd);
                netfd = -1;
                fprintf(stderr,  "Unable to figure out what I'm bound to.");
                fprintf(stderr,  "Unable to determine bound port number.");
        }
#ifdef  WIN32
        flags = 1;
        if (ioctlsocket(netfd,FIONBIO,(unsigned long *) &flags)) {
                _close(netfd);
                netfd = -1;
                fprintf(stderr,  "Unable to set non-blocking mode.");
                fprintf(stderr,  "Unable to set non-blocking mode.");
        }

#else
        if ((flags = fcntl(netfd, F_GETFL)) < 0) {
                close(netfd);
                netfd = -1;
                fprintf(stderr,  "Unable to retrieve socket flags.");
                fprintf(stderr,  "Unable to retrieve socket flags.");
        }
        if (fcntl(netfd, F_SETFL, flags | O_NONBLOCK) < 0) {
                close(netfd);
                netfd = -1;
                fprintf(stderr,  "Unable to set non-blocking mode.");
                fprintf(stderr,  "Unable to set non-blocking mode.");
        }
#endif
}

int jm_sendto(int fd, const void *buf, size_t sz, int i, const struct sockaddr * addr, socklen_t asz) {
	(void)fd;

	// X% outgoing loss
	//if((1.0+rand())/RAND_MAX  < 0.10) return 0;

	return sendto(netfd, buf, sz, i, addr, asz);	
}




int jm_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen) {
	int ret;
	static int report;

	(void)s;

	ret = recvfrom(netfd,buf,len,flags,from,fromlen);

	/* always return errors/empty */
	if(ret <= 0) return ret;

	if((report++ % 200) == 0) 
	    fprintf(stderr, "JM: %d dropped, %d delayed\n", jm_dropped, jm_delayed);

	/* randomly throw some away */
	if( (100.0*rand()/(RAND_MAX+1.0)) < jm_droppct) {
	  //fprintf(stderr, "JM: dropping\n");
	  jm_dropped++;
	  errno = EAGAIN;
	  return -1;
	}

	/* JM_BUCKET_PCT of the time, put in a bucket, and try to remove from a bucket */
	if( 1 && (100.0*rand()/(RAND_MAX+1.0)) < jm_buckpct ) {
	    int i,j;
	    unsigned earlytm;

	    /* try to put in a bucket */
	    for(i=0;i<JM_BUCKETS;i++) {
		//fprintf(stderr, "JM: bucketing\n");
		if(!jm_buckets[i].occupied) {
		    memcpy(jm_buckets[i].buf, buf, ret); 
		    jm_buckets[i].len = ret;
		    jm_buckets[i].from = *from;
		    jm_buckets[i].occupied = 1;
		    jm_buckets[i].deltime = jm_time() + (int)(jm_jitter*rand()/(RAND_MAX+1.0)) ;
		    //fprintf(stderr, "time = %ld, setting deltime to %ld\n", jm_time(), jm_buckets[i].deltime);
		    jm_buckets_full++;
		    break;
		}
	    }
	    if(i == JM_BUCKETS) jm_dropped++;

	    j = -1;
	    earlytm = -1;

	    for(i = 0; i < JM_BUCKETS; i++) {
	      if(jm_buckets[i].occupied && jm_buckets[i].deltime < earlytm) {	
		j = i;
		earlytm = jm_buckets[i].deltime;
	      }
	    }

	    i=j;
	    if(earlytm < jm_time() && i < JM_BUCKETS) {
		  ret = jm_buckets[i].len;
		  memcpy(buf,jm_buckets[i].buf, ret); 
		  *from = jm_buckets[i].from;
		  jm_buckets[i].occupied = 0;
		  jm_buckets_full --;
		  jm_delayed++;
		  //fprintf(stderr, "delivering deltime of %ld\n", jm_buckets[i].deltime);
		  return ret;
	    }




#if 0
	    /* randomly try to find a full bucket.  Otherwise go on */
	    for(j=0;j<JM_BUCKETS/2;j++) {
		i = (int) ((double)JM_BUCKETS*rand()/(RAND_MAX+1.0));
		if(jm_buckets[i].occupied) {
		    ret = jm_buckets[i].len;
		    memcpy(buf,jm_buckets[i].buf, ret); 
		    *from = jm_buckets[i].from;
		    jm_buckets[i].occupied = 0;
		    jm_buckets_full --;
		    jm_delayed++;
		    return ret;
		}
	    }
#endif

	    /* if we got here, we didn't find something in our buckets */
	    jm_dropped++;
	    errno = EAGAIN;
	    return -1;
	}


	return ret;
}



#endif


static int answered_call;
static char *output_filename = NULL;
int do_levels = 0;

/* routine called at exit to shutdown audio I/O and close nicely.
NOTE: If all this isnt done, the system doesnt not handle this
cleanly and has to be rebooted. What a pile of doo doo!! */
void killem(void)
{
	iaxc_shutdown();
	return;
}

void fatal_error(char *err) {
	killem();
	fprintf(stderr, "FATAL ERROR: %s\n", err);
	exit(1);
}

void mysleep(void)
{
	iaxc_millisleep(10);
}

int levels_callback(float input, float output) {
    static int i;
#ifdef notdefNETSTATS    
    if(i%25 == 0)
	fprintf(stderr, "RTT\tRJIT\tRLOSP\tRLOSC\tRPKTS\tRDEL\tLJIT\tLLOSP\tLLOSC\tLPKTS\tLDEL\n");
    if(i++ % 5 == 0) {
	unsigned int info[10];
	iaxc_get_netstats(iaxc_selected_call(),info,10);
	fprintf(stderr, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n",
	    info[0], info[1], info[2]>>24, info[2] & 0xffffff, info[3], info[4],
	    info[5], info[6]>>24, info[6] & 0xffffff, info[7], info[8]);
    }

#endif
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
            return 0;
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
	fprintf(stderr, "DEVICE ID=%d NAME=%s CAPS=%x\n", devs[i].devID, devs[i].name, devs[i].capabilities);
    }
}

void usage()
{ 
    fprintf(stderr, "Usage is XXX\n");
    exit(1);
}

int main(int argc, char **argv)
{
	FILE *f;
	char c;
	int i;
	char *dest = "guest@10.23.1.31/9999";
	double silence_threshold = -99;


	f = stdout;

	for(i=1;i<argc;i++)
	{
	   if(argv[i][0] == '-') 
	   {
	      switch(tolower(argv[i][1]))
	      {
		case 'v':
		  do_levels = 1;
		  break;
		case 's':
		  if(i+1 >= argc) usage();
		  silence_threshold = atof(argv[++i]);
		  break;
		case 'f':
		  if(i+1 >= argc) usage();
		  output_filename = argv[++i];
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

	/* activate the exit handler */
	atexit(killem);

#ifdef JITTERMAKER	
	jm_init();	
	iaxc_set_networking(jm_sendto, jm_recvfrom);
#endif
	
	if(output_filename) {
	  FILE *outfile;
	  if(iaxc_initialize(AUDIO_INTERNAL_FILE,1))
	    fatal_error("cannot initialize iaxclient!");
	  outfile = fopen(output_filename,"w");
	  iaxc_set_files(NULL, outfile);
	} else {
	  if(iaxc_initialize(AUDIO_INTERNAL_PA,1))
	    fatal_error("cannot initialize iaxclient!");
	}

	//iaxc_set_formats(IAXC_FORMAT_SPEEX,IAXC_FORMAT_ULAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX);
	iaxc_set_formats(IAXC_FORMAT_SPEEX,IAXC_FORMAT_ILBC|IAXC_FORMAT_ALAW|IAXC_FORMAT_ULAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX);
	//iaxc_set_formats(IAXC_FORMAT_ILBC,IAXC_FORMAT_ILBC);
	//iaxc_set_formats(IAXC_FORMAT_SPEEX,IAXC_FORMAT_SPEEX);

	iaxc_set_silence_threshold(silence_threshold);

	list_devices();

	iaxc_set_event_callback(iaxc_callback); 


	fprintf(f, "\n\
	    TestCall accept some keyboard input while it's running.\n\
	    You must hit 'enter' for your keypresses to be recognized,\n\
	    although you can type more than one key on a line\n\
\n\
	    q: drop the call and hangup.\n\
	    0-9 * or #: dial those DTMF digits.\n");
	fprintf(f, "Calling %s\n", dest);
	
	iaxc_call(dest);

	iaxc_start_processing_thread();
	printf("ready for keyboard input\n");
	
	if(output_filename) {
	    for(;;)
	      sleep(10);
	}
	while(c = getc(stdin)) {
	    switch (tolower(c)) {
	      case 'q':
		printf("Hanging up and exiting\n");
		iaxc_dump_call();
		iaxc_millisleep(1000);
		iaxc_stop_processing_thread();
		exit(0);
	      break;		

	      case '1': case '2': case '3': case '4': case '5':
		jm_droppct = 10 * (c - '1');
		fprintf(stderr, "setting jm_droppct to %d\n", jm_droppct);
	      break;
	      case '6': case '7': case '8': case '9': case '0':
		jm_buckpct = 15 * (c - '6');
		fprintf(stderr, "setting jm_buckpct to %d\n", jm_buckpct);
	      break;
	      case '#': case '*':
		printf ("sending %c\n", c);
		iaxc_send_dtmf(c);
	      break;
	    }
	}

	return 0;
}
