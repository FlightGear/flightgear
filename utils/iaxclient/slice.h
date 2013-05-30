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

/*
 * This API can be used with codecs that do not provide internal support for
 * splitting encoded frames in arbitrary-sized slices.  This is useful for
 * things like transmitting encoded frame over size constained packet 
 * protocols such as UDP. 
 *
 * The slicer adds 6 bytes at the beginning of each slice.  The 
 * format of this header is :
 *   - version: right now, first bit should be 0, the rest are undefined
 *
 *   - source id: 2 bytes random number used to identify stream changes in
 *     conference applications this number is transmitted in big endian
 *     format over the wire
 *
 *   - frame index number - used to detect a new frame when some of the
 *     slices of the current frame are missing (only the least significant
 *     4 bits are used)
 *
 *   - index of slice in the frame, starting at 0
 *
 *   - total number of slices in the frame
 *
 */
#ifndef __SLICE_H__
#define __SLICE_H__

#include "iaxclient_lib.h"

#define MAX_ENCODED_FRAME_SIZE 48 * 1024

struct slicer_context
{
	unsigned char  frame_index;
	unsigned short source_id;
	unsigned int   slice_size;
};

struct deslicer_context
{
	unsigned char  frame_index;
	unsigned char  slice_count;
	int            frame_size;
	unsigned short source_id;
	unsigned int   slice_size;
	int            frame_complete;
	char           buffer[MAX_ENCODED_FRAME_SIZE];
};

/*
 * Allocates and initializes a slicer context with the given souirce_id
 */
struct slicer_context * create_slicer_context(unsigned short source_id, unsigned int slice_size);

/*
 * Deallocates a slicer_context
 */
int free_slicer_context(struct slicer_context *sc);

/*
 * Fragments a frame into one or several slices
 * data      - frame data
 * size      - size of frame data
 * slice-set - pointer to a preallocated structure that will hold slices and slice information
 * sc        - holds stream information such as source id and frame index
 * Returns 0 if completed successfully or a negative value if failure.
 */
int slice(const char *data, 
          unsigned int size,
          struct slice_set_t *slice_set,
          struct slicer_context *sc
         );

/*
 * Allocates and initializes a deslicer context with the given souirce_id
 */
struct deslicer_context * create_deslicer_context(unsigned int slice_size);

/*
 * Deallocates a slicer_context
 */
int free_deslicer_context(struct deslicer_context *dsc);

/*
 * Assembles one frame out of multiple slices
 * in     - slice data
 * inlen  - length of slice
 * outlen - length of assembled frame
 * dsc    - holds stream information 
 * Returns NULL if there is an error or the current frame is incomplete
 * Returns a pointer to a buffer containing the completed frame and updates
 * outlen with the frame size if successful
 */
char *
deslice(const char *in, int inlen, int *outlen, struct deslicer_context *dsc);

#endif
