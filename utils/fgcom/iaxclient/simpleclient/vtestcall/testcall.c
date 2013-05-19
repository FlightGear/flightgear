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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "iaxclient.h"

#include "SDL.h"

static int answered_call;
static char *output_filename = NULL;
static int video;

int format, framerate, bitrate, width, height;

int display_video(struct iaxc_ev_video v);

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
	//exit(1);
}

void mysleep(void)
{
	iaxc_millisleep(10);
}

int levels_callback(float input, float output) {
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
	case IAXC_EVENT_VIDEO:
	    return display_video(e.ev.video);
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

static unsigned char *videobuf;
static SDL_Surface *window;
static SDL_Surface *videosurf;
static SDL_Overlay *videoolay;

int init_sdl() {
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE) < 0) {
	    fprintf(stderr, "could not init SDL: %d\n", SDL_GetError());
	    return -1;
	}
	// XXX check these flags, do right thing with format, watch changes, etc.
	window = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if(!window) {
	    fprintf(stderr, "could not open window\n");
	    return -1;
	}

	videoolay = SDL_CreateYUVOverlay(width, height, SDL_IYUV_OVERLAY, window);
	if(!videoolay) {
	    fprintf(stderr, "could not create videoolay\n");
	    return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	FILE *f;
	char c;
	int i;
	char *dest = NULL;
	double silence_threshold = -99;


	f = stdout;


	for(i=1;i<argc;i++)
	{
	   if(argv[i][0] == '-') 
	   {
	      switch(argv[i][1])
	      {
		case 'V': /* v is taken; p: picturephone.  Ok, that's lame.. */
		  video = 1;
		  break;
		case 'F': /* set video formats */
		  {
		      format = 1<<atoi(argv[++i]);
		      framerate = atoi(argv[++i]);
		      bitrate = atoi(argv[++i]);
		      width = atoi(argv[++i]);
		      height = atoi(argv[++i]);
		      iaxc_video_format_set(format, format, framerate, bitrate, width, height);
		  }
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

	if(init_sdl()) {
	    fprintf(stderr, "could not init SDL\n");
	    return -1;
	}

	printf("settings: \n");
	printf("\tsilence threshold: %f\n", silence_threshold);

	/* activate the exit handler */
	atexit(killem);
	
	if(output_filename) {
	  FILE *outfile;
	  if(iaxc_initialize(AUDIO_INTERNAL_FILE,1))
	    fatal_error("cannot initialize iaxclient!");
	  outfile = fopen(output_filename,"w");
	  file_set_files(NULL, outfile);
	} else {
	  if(iaxc_initialize(AUDIO_INTERNAL_PA,1))
	    fatal_error("cannot initialize iaxclient!");
	}


	iaxc_video_mode_set(IAXC_VIDEO_MODE_PREVIEW_ENCODED);

	iaxc_set_formats(IAXC_FORMAT_SPEEX,IAXC_FORMAT_ULAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX);
	//iaxc_set_formats(IAXC_FORMAT_ULAW,IAXC_FORMAT_ULAW);
	//iaxc_set_formats(IAXC_FORMAT_ULAW,IAXC_FORMAT_ULAW);
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
	    q: drop the call and hangup.\n\
	    0-9 * or #: dial those DTMF digits.\n");
	if(dest) {
	    fprintf(f, "Calling %s\n", dest);
	    
	    iaxc_call(dest);
	}


	//iaxc_start_processing_thread();
	printf("ready for keyboard input\n");

	for(;;) {
	  SDL_Event event;
	  iaxc_millisleep(5);
	  iaxc_process_calls();
	  while(SDL_PollEvent(&event)) {
	      if(event.type == SDL_QUIT) {
		  break;
	      } else if(event.type == SDL_KEYDOWN) {
		  switch(event.key.keysym.sym) {
		  case SDLK_a:
		    printf("Answering call 0\n");
		    iaxc_select_call(0);
		  break;		
		  case SDLK_q:
		    printf("Hanging up and exiting\n");
		    iaxc_dump_call();
		    iaxc_millisleep(1000);
		    iaxc_stop_processing_thread();
		    exit(0);
		  case SDLK_r: /* raw */
		    iaxc_video_mode_set(IAXC_VIDEO_MODE_PREVIEW_RAW);
		  break;		
		  case SDLK_e: /* encoded */
		    iaxc_video_mode_set(IAXC_VIDEO_MODE_PREVIEW_ENCODED);
		  break;		
		  case SDLK_t: /* transmit-only */
		    iaxc_video_mode_set(IAXC_VIDEO_MODE_ACTIVE);
		  break;		
		  case SDLK_w: /* off */
		    iaxc_video_mode_set(IAXC_VIDEO_MODE_NONE);
		  break;		
#if 0
		  case '1': case '2': case '3': case '4': case '5':
		  case '6': case '7': case '8': case '9': case '0':
		  case '#': case '*':
		    printf ("sending %c\n", c);
		    iaxc_send_dtmf(c);
		  break;
#endif
		  }
	      }
	  }
	}
	return 0;
}

int display_video(struct iaxc_ev_video v) {
    Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER != SDL_BIG_ENDIAN
    rmask = 0xff0000;
    gmask = 0x00ff00;
    bmask = 0x0000ff;
#else
    rmask = 0x0000ff;
    gmask = 0x00ff00;
    bmask = 0xff0000;
#endif
    amask=0;

//    fprintf(stderr, "testcall: got video event, %dx%d %x\n", v.width, v.height, v.data);

    SDL_LockYUVOverlay(videoolay);
      videoolay->pixels[0] = v.data;  
      videoolay->pixels[1] = v.data + (v.width * v.height);  
      videoolay->pixels[2] = v.data + (v.width * v.height * 5 / 4);;  


    SDL_UnlockYUVOverlay(videoolay);
    //SDL_BlitSurface(videosurf, NULL, window, NULL);
    //SDL_Flip(window);
    {
      SDL_Rect rect = {1,1,v.width,v.height};
      SDL_DisplayYUVOverlay(videoolay, &rect);
    }

    return 0;
}

