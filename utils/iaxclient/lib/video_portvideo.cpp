/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003 HorizonLive.com, (c) 2004, Horizon Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 *
 * Module: video_portvideo
 * Purpose: Video code to provide portvideo driver support for IAX library
 * Developed by: Steve Kann
 * Creation Date: April 7, 2005
 *
 *
 */

#include "iaxclient_lib.h"
#include "PortVideoSDL/common/cameraEngine.h"
#include "PortVideoSDL/common/cameraTool.h"

static cameraEngine *engine;
int running;


int pv_start (struct iaxc_video_driver *d ) {
    if(!running) {  
       if(!engine->startCamera()) {
	  fprintf(stderr, "pv: couldn't start camera\n");
	  return -1;
       }
       running = 1;
    }
   return 0;
}

int pv_stop (struct iaxc_video_driver *d ) {
    return 0;
}

void pv_shutdown() {
}

int pv_input(struct iaxc_video_driver *d, unsigned char **in) {
    unsigned char *data = NULL;
    data = engine->getFrame();
    if(!data) {
	if(!engine->stillRunning()) {
	    fprintf(stderr, "camera disconnected\n");
	    running = 0;
	}
	fprintf(stderr, "pv_input: no frame\n");
	return 0;
    }
    *in = data;
    return 0;
}

int pv_output(struct iaxc_video_driver *d, unsigned char *data) {
	return 0;
}

int pv_select_devices (struct iaxc_video_driver *d, int input, int output) {
    return 0;
}

int pv_selected_devices (struct iaxc_video_driver *d, int *input, int *output) {
    return 0;
}

int pv_destroy (struct iaxc_video_driver *d ) {
    //implementme
    return 0;
}



/* initialize video driver */
int pv_initialize (struct iaxc_video_driver *d, int w, int h, int framerate) {
    /* setup methods */
    d->initialize = pv_initialize;
    d->destroy = pv_destroy;
    d->select_devices = pv_select_devices;
    d->selected_devices = pv_selected_devices;
    d->start = pv_start;
    d->stop = pv_stop;
    d->output = pv_output;
    d->input = pv_input;

    engine = cameraTool::findCamera();
    if(!engine) {
	fprintf(stderr, "video_portvideo: can't find camera\n");
	return -1;
    }

    if(!engine->initCamera(w,h,true)) {
	fprintf(stderr, "can't initialize camera");
	return -1;
    }
	

    return 0;
}
