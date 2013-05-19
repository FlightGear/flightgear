/*
 * iaxclient_lib: An Inter-Asterisk eXchange communication library
 *
 * Module: audio_file
 * Purpose: Audio code to read/write to files
 * based on audio_portaudio, originally Developed by: Shawn Lawrence, Terrace Communications Inc.
 * Developed by: Steve Kann
 * Creation Date: October 30, 2003
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 *
 * IAX library Copyright (c) 2001 Linux Support Services
 * IAXlib is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 *
 * This library uses the PortAudio Portable Audio Library
 * For more information see: http://www.portaudio.com
 * PortAudio Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 */

#include "iaxclient_lib.h"

typedef short SAMPLE;

static FILE *inFile=NULL, *outFile=NULL;

#define FRAMES_PER_BUFFER 80 /* 80 frames == 10ms */


static int file_play_sound(struct iaxc_sound *inSound, int ring) {
  return 0;
}

static int file_stop_sound(int soundID) {
  return 0; 
}


static int file_start (struct iaxc_audio_driver *d ) {
    return 0;
}

static int file_stop (struct iaxc_audio_driver *d ) {
    return 0;
}

/* not used
static void file_shutdown_audio() {
    return;
}
*/

static int file_input(struct iaxc_audio_driver *d, void *samples, int *nSamples) {
	*nSamples = 0;
	return 0;
}

static int file_output(struct iaxc_audio_driver *d, void *samples, int nSamples) {
	
	if(outFile) {
	    fwrite(samples, sizeof(SAMPLE), nSamples, outFile);
	}
	return 0;
}

static int file_select_devices (struct iaxc_audio_driver *d, int input, int output, int ring) {
    return 0;
}

static int file_selected_devices (struct iaxc_audio_driver *d, int *input, int *output, int *ring) {
    *input = 0;
    *output = 0;
    *ring = 0;
    return 0;
}

static int file_destroy (struct iaxc_audio_driver *d ) 
{
	/* TODO: something should happen here */
    return 0;
}

static double file_input_level_get(struct iaxc_audio_driver *d){
    return -1;
}

static double file_output_level_get(struct iaxc_audio_driver *d){
    return -1;
}

static int file_input_level_set(struct iaxc_audio_driver *d, double level){
    return -1;
}

static int file_output_level_set(struct iaxc_audio_driver *d, double level){
    return -1;
}

EXPORT int iaxc_set_files(FILE *input, FILE *output) {
    inFile = input;
    outFile = output;
    return 0;
}


/* initialize audio driver */
int file_initialize (struct iaxc_audio_driver *d , int sample_rate) {

    if(sample_rate != 8000 ) return -1;

    /* setup methods */
    d->initialize = file_initialize;
    d->destroy = file_destroy;
    d->select_devices = file_select_devices;
    d->selected_devices = file_selected_devices;
    d->start = file_start;
    d->stop = file_stop;
    d->output = file_output;
    d->input = file_input;
    d->input_level_get = file_input_level_get;
    d->input_level_set = file_input_level_set;
    d->output_level_get = file_output_level_get;
    d->output_level_set = file_output_level_set;
    d->play_sound = file_play_sound;
    d->stop_sound = file_stop_sound;

    return 0;
}
