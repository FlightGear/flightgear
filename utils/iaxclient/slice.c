/*
 * iaxclient: a portable telephony toolkit
 *
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Mihai Balea <mihai at hates dot ms>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 *
 * A codec independent frame slicer/assembler library
 */
 
#include "slice.h"

struct slicer_context * create_slicer_context(unsigned short source_id, unsigned int slice_size)
{
	struct slicer_context *sc;
	
	sc = calloc(1, sizeof(struct slicer_context));
	sc->source_id = source_id;
	sc->slice_size = slice_size;
	return sc;
}

int free_slicer_context(struct slicer_context *sc)
{
	if ( sc == NULL )
		return -1;
	
	free(sc);
	return 0;
}

int slice(const char *data, 
          unsigned int size,
          struct slice_set_t *slice_set,
	  struct slicer_context *sc
         )
{
	int i, ssize;
	
	if ( data == NULL || slice_set == NULL || sc == NULL)
		return -1;
	
	// Figure out how many slices we need
	slice_set->num_slices = (size - 1) / sc->slice_size + 1;
	
	for ( i = 0; i < slice_set->num_slices; i++ )
	{
		slice_set->data[i][0] = 0;
		slice_set->data[i][1] = (unsigned char)(sc->source_id >> 8);
		slice_set->data[i][2] = (unsigned char)(sc->source_id & 0xff);
		slice_set->data[i][3] = sc->frame_index;
		slice_set->data[i][4] = (unsigned char)i;
		slice_set->data[i][5] = (unsigned char)slice_set->num_slices;
		ssize = (i == slice_set->num_slices - 1) ? 
			size % sc->slice_size : sc->slice_size;
		memcpy(&slice_set->data[i][6], data, ssize);
		slice_set->size[i] = ssize + 6;
		data += ssize;
	}
	sc->frame_index++;
	
	return 0;
}

struct deslicer_context * create_deslicer_context(unsigned int slice_size)
{
	struct deslicer_context *dsc;
	
	dsc = calloc(1, sizeof(struct deslicer_context));
	dsc->slice_size = slice_size;
	return dsc;
}

int free_deslicer_context(struct deslicer_context *dsc)
{
	if ( dsc == NULL )
		return -1;
	free(dsc);
	return 0;
}

static void reset_deslicer_context(struct deslicer_context *dsc)
{
	if ( dsc == NULL )
		return;
	
	memset(dsc->buffer, 0, sizeof(dsc->buffer));
	dsc->frame_size = 0;
	dsc->slice_count = 0;
	dsc->frame_complete = 0;
}

char *
deslice(const char *in, int inlen, int *outlen, struct deslicer_context *dsc)
{
	unsigned char		frame_index, slice_index, num_slices, version;
	unsigned short		source_id;

	// Sanity checks
	if ( dsc == NULL || in == NULL || inlen <= 0 || outlen == NULL )
		return NULL;

	// If previous call returned a complete frame, clean up the context
	if ( dsc->frame_complete )
	{
		reset_deslicer_context(dsc);
	}

	version = *in++;
	source_id = (unsigned short)(*in++) << 8;
	source_id |= *in++;
	frame_index = *in++ & 0x0f;
	slice_index = *in++;
	num_slices = *in++;
	inlen -= 6;

	if ( version & 0x80 )
	{
		fprintf(stderr, "deslice: unknown slice protocol\n");
		return NULL;
	}

	if ( source_id == dsc->source_id )
	{
		/* We use only the least significant bits to calculate delta
		* this helps with conferencing and video muting/unmuting
		*/
		unsigned char frame_delta = (frame_index - dsc->frame_index) & 0x0f;

		if ( frame_delta > 8 )
		{
			/* Old slice coming in late, ignore. */
			return NULL;
		} else if ( frame_delta > 0 )
		{
			/* Slice belongs to a new frame */
			dsc->frame_index = frame_index;

			if ( dsc->slice_count > 0 )
			{
				/* Current frame is incomplete, drop it */
				reset_deslicer_context(dsc);
			}
		}
	} else
	{
		/* Video stream was switched, the existing frame/slice
		* indexes are meaningless.
		*/
		reset_deslicer_context(dsc);
		dsc->source_id = source_id;
		dsc->frame_index = frame_index;
	}

	// Process current slice
	if ( dsc->slice_size * slice_index + inlen > MAX_ENCODED_FRAME_SIZE )
	{
		// Frame would be too large, ignore slice
		return NULL;
	}

	memcpy(dsc->buffer + dsc->slice_size * slice_index, in, inlen);
	dsc->slice_count++;

	/* We only know the size of the frame when we get the final slice */
	if ( slice_index == num_slices - 1 )
	{
		dsc->frame_size = dsc->slice_size * slice_index + inlen;
	}

	if ( dsc->slice_count < num_slices )
	{
		// we're still waiting for some slices
		return NULL;
	} else
	{
		// Frame complete, set the flag and return the buffer
		dsc->frame_complete = 1;
		*outlen = dsc->frame_size;
		return dsc->buffer;
	}
}
