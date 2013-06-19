/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2006 Panfilov Dmitry
 *
 * Contributors:
 * Panfilov Dmitry
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 *
 */

#include "iaxclient_lib.h"
#include <alsa/asoundlib.h>

static snd_pcm_t *stream_out;
static snd_pcm_t *stream_in;

#define FRAMES_PER_BUFFER 80 /* 80 frames == 10ms */


static int alsa_play_sound(struct iaxc_sound *inSound, int ring) {
  return 0;
}

int alsa_stop_sound(int soundID) {
  return 0;
}


int alsa_start (struct iaxc_audio_driver *d ) {
    return 0;
}

int alsa_stop (struct iaxc_audio_driver *d ) {
    return 0;
}

void alsa_shutdown_audio()
{
    return;
}


int alsa_input(struct iaxc_audio_driver *d, void *samples, int *nSamples) {

    /* we don't return partial buffers */
    long r;
    long byteread=*nSamples;
    static int h;
    *nSamples=0;
    snd_pcm_start(stream_in);
    if(h==1) { h=0; return 0;}
    do{
	r = snd_pcm_readi(stream_in, samples, byteread);
        if (r == -EAGAIN){
            continue;
	}
	if (r == - EPIPE) {
	    snd_pcm_prepare(stream_in);
	    continue;
	}
	 samples += (r * 2);
	 byteread -= r;
	 *nSamples += r;
    }while(r >=0 && byteread >0);
    h=1;
    return 0;
}

int alsa_output(struct iaxc_audio_driver *d, void *samples, int nSamples) {

        long r;
	snd_pcm_start(stream_out);
        while (nSamples > 0) {
                r = snd_pcm_writei(stream_out, samples, nSamples);
                if (r == -EAGAIN){
                    continue;
		}
		if (r == - EPIPE) {
		    snd_pcm_prepare(stream_out);
		    continue;
		}
                if (r < 0) {
		    fprintf(stderr, "r=%d\n",r);
		}
                samples += r * 2;
                nSamples -= r;
        }
        return 0;
}

int alsa_select_devices (struct iaxc_audio_driver *d, int input, int output, int ring) {
    return 0;
}

int alsa_selected_devices (struct iaxc_audio_driver *d, int *input, int *output, int *ring) {
    *input = 0;
    *output = 0;
    *ring = 0;
    return 0;
}

int alsa_destroy (struct iaxc_audio_driver *d )
{
	/* TODO: something should happen here */
    return 0;
}

double alsa_input_level_get(struct iaxc_audio_driver *d){
    return -1;
}

double alsa_output_level_get(struct iaxc_audio_driver *d){
    return -1;
}

int alsa_input_level_set(struct iaxc_audio_driver *d, double level){
    return -1;
}

int alsa_output_level_set(struct iaxc_audio_driver *d, double level){
    return -1;
}


/* initialize audio driver */
int alsa_initialize (struct iaxc_audio_driver *d ,int sample_rate) {
    int i;
    int err;
    short buf[128];
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;

    if ((err = snd_pcm_open (&stream_out, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf (stderr, "cannot open audio device default (%s)\n",
        snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_any (stream_out, hw_params)) < 0) {
	fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_set_access (stream_out, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
	fprintf (stderr, "cannot set access type (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_set_format (stream_out, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
	fprintf (stderr, "cannot set sample format (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_set_rate (stream_out, hw_params, sample_rate, 0)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_set_channels (stream_out, hw_params, 1)) < 0) {
	fprintf (stderr, "cannot set channel count (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params (stream_out, hw_params)) < 0) {
	fprintf (stderr, "cannot set parameters (%s)\n",
	snd_strerror (err));
	exit (1);
    }

    snd_pcm_sw_params_malloc(&sw_params);

    err = snd_pcm_sw_params_current(stream_out, sw_params);
    if (err < 0) {
	printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return err;
    }
    err = snd_pcm_sw_params_set_start_threshold(stream_out, sw_params, 80);
    if (err < 0) {
        fprintf(stderr, "Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
        return err;
    }
    err = snd_pcm_sw_params(stream_out, sw_params);
    if (err < 0) {
        fprintf(stderr, "Unable to set sw params for playback: %s\n", snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_open (&stream_in, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf (stderr, "cannot open audio device default (%s)\n",
        snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_any (stream_in, hw_params)) < 0) {
	fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_set_access (stream_in, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
	fprintf (stderr, "cannot set access type (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_set_format (stream_in, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
	fprintf (stderr, "cannot set sample format (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_set_rate (stream_in, hw_params, sample_rate, 0)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params_set_channels (stream_in, hw_params, 1)) < 0) {
	fprintf (stderr, "cannot set channel count (%s)\n",
	snd_strerror (err));
	exit (1);
    }
    if ((err = snd_pcm_hw_params (stream_in, hw_params)) < 0) {
	fprintf (stderr, "cannot set parameters (%s)\n",
	snd_strerror (err));
	exit (1);
    }

    err = snd_pcm_sw_params_current(stream_in, sw_params);
    if (err < 0) {
	printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return err;
    }
    err = snd_pcm_sw_params_set_start_threshold(stream_in, sw_params, 80);
    if (err < 0) {
        fprintf(stderr, "Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
        return err;
    }
    err = snd_pcm_sw_params(stream_in, sw_params);
    if (err < 0) {
        fprintf(stderr, "Unable to set sw params for playback: %s\n", snd_strerror(err));
        return err;
    }


    if ((err = snd_pcm_prepare (stream_in)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
	snd_strerror (err));
	 exit (1);
    }

    if ((err = snd_pcm_prepare (stream_out)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
	snd_strerror (err));
	 exit (1);
    }

    d->initialize = alsa_initialize;
    d->destroy = alsa_destroy;
    d->select_devices = alsa_select_devices;
    d->selected_devices = alsa_selected_devices;
    d->start = alsa_start;
    d->stop = alsa_stop;
    d->output = alsa_output;
    d->input = alsa_input;
    d->input_level_get = alsa_input_level_get;
    d->input_level_set = alsa_input_level_set;
    d->output_level_get = alsa_output_level_get;
    d->output_level_set = alsa_output_level_set;
    d->play_sound = alsa_play_sound;
    d->stop_sound = alsa_stop_sound;

    return 0;
}
