#include "iaxclient_lib.h"

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#elif defined(OPENALSDK)
#include <al.h>
#include <alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

struct openal_priv_data
{
    int sample_rate;
    int num_buffers;
    int buffers_head;
    int buffers_tail;
    int buffers_free;
    ALuint* buffers;
    ALCcontext* out_ctx;
    ALuint source;
    ALCdevice* in_dev;
    double input_level;
    double output_level;
};

static struct iaxc_audio_device device = { 
    "default", 
    IAXC_AD_INPUT | IAXC_AD_OUTPUT | IAXC_AD_RING | IAXC_AD_INPUT_DEFAULT | IAXC_AD_OUTPUT_DEFAULT | IAXC_AD_RING_DEFAULT,
    0 
};

static int openal_error(const char* function, int err)
{
    fprintf(stderr, "OpenAl function %s failed with code %d\n", function, err);
    return -1;
}

int openal_input(struct iaxc_audio_driver *d, void *samples, int *nSamples)
{
    int err;
    struct openal_priv_data* priv = (struct openal_priv_data*)(d->priv);

    ALCint available;
    ALCsizei request;

    alcGetIntegerv(priv->in_dev, ALC_CAPTURE_SAMPLES, sizeof(available), &available);
    /* do not return less data than caller wanted, iaxclient does not like it */
    request = (available < *nSamples) ? 0 : *nSamples;
    if (request > 0)
    {
	err = alcGetError(priv->in_dev);
	alcCaptureSamples(priv->in_dev, samples, request);    
	err = alcGetError(priv->in_dev);
	if (err)
	{
	    openal_error("alcCaptureSamples", err);
	    *nSamples = 0;
	    return 1;
	}
	// software mute, but keep data flowing for sync purposes
	if (priv->input_level == 0)
	{
	    memset(samples, 0, 2 * request);
	}
    }
    *nSamples = request;
    
    return 0;
}

static void openal_unqueue(struct openal_priv_data* priv)
{
    int i;
    ALint err;
    ALint processed;

    alGetSourcei(priv->source, AL_BUFFERS_PROCESSED, &processed);

#ifdef OPENAL_DEBUG
    {
	ALint queued;
	ALint state;
	
	alGetSourcei(priv->source, AL_BUFFERS_QUEUED, &queued);
	alGetSourcei(priv->source, AL_SOURCE_STATE, &state);

	fprintf(stderr, "free: %d processed: %d queued: %d head: %d tail: %d state: %d\n", 
	    priv->buffers_free, processed, queued, priv->buffers_head, priv->buffers_tail, state);
    }
#endif
	
    alGetError();
    for(i = 0; i < processed; i++)
    {
	alSourceUnqueueBuffers(priv->source, 1, priv->buffers + priv->buffers_tail);
	err = alGetError();
	if (err)
	{
	    openal_error("alSourceUnqueueBuffers", err);
	    break;
	}
	if (++priv->buffers_tail >= priv->num_buffers)
	{
	    priv->buffers_tail = 0;
	}
	++priv->buffers_free;
    }
}

int openal_output(struct iaxc_audio_driver *d, void *samples, int nSamples)
{
    struct openal_priv_data* priv = (struct openal_priv_data*)(d->priv);

    openal_unqueue(priv);
    /* If we run out of buffers, wait for an arbitrary number to become free */
    if (priv->buffers_free == 0)
    {
	while(priv->buffers_free < 4)
	{
	    iaxc_millisleep(100);
    	    openal_unqueue(priv);
	}
    }
    
    if (priv->buffers_free > 0)
    {
	ALuint buffer = priv->buffers[priv->buffers_head++];
	if (priv->buffers_head >= priv->num_buffers)
	{
	    priv->buffers_head = 0;
	}
	
	alBufferData(buffer, AL_FORMAT_MONO16, samples, nSamples * 2, priv->sample_rate);
	alSourceQueueBuffers(priv->source, 1, &buffer);
	--priv->buffers_free;
	
	/* delay start of output until we have 2 buffers */
	if (priv->buffers_free == priv->num_buffers - 2)
	{
	    ALint state;
	
	    alGetSourcei(priv->source, AL_SOURCE_STATE, &state);
	    if (state != AL_PLAYING)
	    {
#ifdef OPENAL_DEBUG
		fprintf(stderr, "calling alSourcePlay\n");
#endif	    
		alSourcePlay(priv->source);
	    }
	}
    } else {
	fprintf(stderr, "openal_output buffer overflow\n");
	return 1;
    }
    
    return 0;
}

int openal_select_devices(struct iaxc_audio_driver *d, int input, int output, int ring)
{
    return (input != 0 || output !=0 || ring != 0) ? -1 : 0;
}

int openal_selected_devices(struct iaxc_audio_driver *d, int *input, int *output, int *ring)
{
    *input = 0;
    *output = 0;
    *ring = 0;
    
    return 0;
}

/* 
    Apparently iaxclient calls openal_start a gazillion times and doesn't call openal_stop.
    So let's just make them no-ops.
*/
int openal_start(struct iaxc_audio_driver *d)
{
    int iret = 0;
    struct openal_priv_data* priv = (struct openal_priv_data*)(d->priv);
    if (priv) /* just to stop compiler noise */
        iret = 0;
    return iret;
}

int openal_stop(struct iaxc_audio_driver *d)
{
    int iret = 0;
    struct openal_priv_data* priv = (struct openal_priv_data*)(d->priv);
    if (priv) /* just to stop compiler noise */
        iret = 0;
    return iret;
}

double openal_input_level_get(struct iaxc_audio_driver *d)
{
    struct openal_priv_data* priv = (struct openal_priv_data*)(d->priv);

    return priv->input_level;
}

double openal_output_level_get(struct iaxc_audio_driver *d)
{
    struct openal_priv_data* priv = (struct openal_priv_data*)(d->priv);

    return priv->output_level;
}

int openal_input_level_set(struct iaxc_audio_driver *d, double level)
{
    struct openal_priv_data* priv = (struct openal_priv_data*)(d->priv);
    priv->input_level = (level < 0.5) ? 0 : 1;

    return 0;
}

int openal_output_level_set(struct iaxc_audio_driver *d, double level)
{
    struct openal_priv_data* priv = (struct openal_priv_data*)(d->priv);
    priv->output_level = level;
    alSourcef(priv->source, AL_GAIN, level);

    return 0;
}

int openal_play_sound(struct iaxc_sound *s, int ring)
{
    return 0;
}

int openal_stop_sound(int id)
{
    return 0;
}

int openal_mic_boost_get(struct iaxc_audio_driver *d)
{
    return 0;
}

int openal_mic_boost_set(struct iaxc_audio_driver *d, int enable)
{
    return 0;
}

int openal_destroy(struct iaxc_audio_driver *d)
{
    return 0;
}

int openal_initialize(struct iaxc_audio_driver *d, int sample_rate)
{
    struct openal_priv_data* priv = malloc(sizeof(struct openal_priv_data));
    int err = alGetError();
    ALCdevice* out_dev = alcOpenDevice(0);
    if (out_dev == 0) return openal_error("alcOpenDevice", alGetError());
    d->priv = priv;
   
    priv->out_ctx = alcCreateContext(out_dev, 0);
    if (priv->out_ctx == 0) return openal_error("alcCreateContext", alGetError());
    
    alcMakeContextCurrent(priv->out_ctx);
    if ((err = alGetError())) return openal_error("alcMakeContextCurrent", err);

    priv->sample_rate = sample_rate;    
    priv->num_buffers = 20;
    priv->input_level = 1;
    priv->output_level = 1;
    priv->buffers_head = 0;
    priv->buffers_tail = 0;
    priv->buffers_free = priv->num_buffers;
    priv->buffers = (ALuint*)malloc(sizeof(ALuint) * priv->num_buffers);
    alGenBuffers(priv->num_buffers, priv->buffers);
    if ((err = alGetError())) return openal_error("alGenBuffers", err);
    
    alGenSources(1, &priv->source);
    if ((err = alGetError())) return openal_error("alGenSources", err);

    priv->in_dev = alcCaptureOpenDevice(0, 8000, AL_FORMAT_MONO16, 800);
    if (!priv->in_dev) return openal_error("alcCaptureOpenDevice", 0);

    alcCaptureStart(priv->in_dev);
    if ((err = alGetError())) return openal_error("alcCaptureStart", err);

    d->initialize = openal_initialize;
    d->destroy = openal_destroy;
    d->select_devices = openal_select_devices;
    d->selected_devices = openal_selected_devices;
    d->start = openal_start;
    d->stop = openal_stop;
    d->output = openal_output;
    d->input = openal_input;
    d->input_level_get = openal_input_level_get;
    d->input_level_set = openal_input_level_set;
    d->output_level_get = openal_output_level_get;
    d->output_level_set = openal_output_level_set;
    d->mic_boost_get = openal_mic_boost_get;
    d->mic_boost_set = openal_mic_boost_set;
    d->play_sound = openal_play_sound;
    d->stop_sound = openal_stop_sound;
    d->nDevices = 1;
    d->devices = &device;
    
    return 0;
}
