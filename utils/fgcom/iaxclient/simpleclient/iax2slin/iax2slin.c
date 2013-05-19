/*
 * iax2slin: Originate an IAX2 call, and send the audio from this call to stdout
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "iaxclient.h"

static int answered_call;
static char *output_filename = NULL;

void killem(void)
{
	iaxc_shutdown();
	return;
}


int iaxc_callback(iaxc_event e)
{
    switch(e.type) {
        default:
            return 0;  // not handled
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
	FILE *outfile;
	char c;
	int i;
	char *dest = NULL;
	double silence_threshold = -99;


	f = stderr;

	for(i=1;i<argc;i++)
	{
	   if(argv[i][0] == '-') 
	   {
	      switch(tolower(argv[i][1]))
	      {
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


	/* activate the exit handler */
	atexit(killem);
	
	if(iaxc_initialize(AUDIO_INTERNAL_FILE,1)) {
	    fprintf(stderr, "cannot initialize iaxclient!");
	    exit(1);
	}
	if(output_filename) {
	  outfile = fopen(output_filename,"wb");
	  iaxc_set_files(NULL, outfile);
	} else {
	  iaxc_set_files(NULL, stdout);
	}

//	iaxc_set_formats(IAXC_FORMAT_SPEEX,IAXC_FORMAT_ULAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX);
	//iaxc_set_formats(IAXC_FORMAT_SPEEX,IAXC_FORMAT_SPEEX);
//	iaxc_set_formats(IAXC_FORMAT_GSM,IAXC_FORMAT_GSM);
	//iaxc_set_formats(IAXC_FORMAT_ULAW,IAXC_FORMAT_ULAW);
	iaxc_set_formats(IAXC_FORMAT_ULAW,IAXC_FORMAT_ULAW|IAXC_FORMAT_ALAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX);
	iaxc_set_silence_threshold(0);  // mute input


	//if(do_levels)
	  iaxc_set_event_callback(iaxc_callback); 


	if(dest) {
	    fprintf(stderr, "Calling %s\n", dest);
	    iaxc_call(dest);
	}

	iaxc_start_processing_thread();

	for(;;)
	    sleep(10);

	return 0;
}
