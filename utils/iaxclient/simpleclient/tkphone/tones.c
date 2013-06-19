/*
 * Utilities for generating dual sine wave tones for telephony apps.
 * using 2nd order recursion (SAU 10/92).
 * Modified 5/97, 8/04 SAU (c) 1997, 2004 Sun Microsystems.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission. This software
 * is provided "AS IS," without a warranty of any kind.  ALL EXPRESS OR
 * IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED. SUN MIDROSYSTEMS, INC.
 * ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED
 * BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS
 * SOFTWARE OR ITS DERIVATIVES. IN NO EVENT WILL SUN OR ITS LICENSORS BE
 * LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT,
 * SPECIAL, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER CAUSED
 * AND REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF THE USE OF
 * OR INABILITY TO USE THIS SOFTWARE, EVEN IF SUN HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES. You acknowledge that this software is not
 * designed, licensed or intended for use in the design, construction,
 * operation or maintenance of any nuclear facility.
 */

#ifdef TONES_MAIN
#  include <stdio.h>
#endif
#include <strings.h>
#include <stdlib.h>
#include <math.h>
#include "tones.h"

#define BETWEEN(a,x,b)     ((x)<(a)?a:((x)>(b)?b:x)) 

/* --
 * Generate dtmf tones
 *
 *  tone frequency matrix
 * 679   1    2    3    A
 * 770   4    5    6    B
 * 852   7    8    9    C
 * 941   *    0    #    D
 *     1209 1336 1447 1633
 */

static double dtmf_freq[] = {
     697.0,  770.0,  852.0,  941.0,
    1209.0, 1336.0, 1447.0, 1633.0
};

static char *dtmf_tones = "123A456B789C*0#D";	/* valid touch tones */

/*
 * Generate a DTMF tone, signed short, 8000hz 
 *  tone:	The tone to generate (from tones)
 *  samples:	The duration in samples (at 8 ksamples/sec)
 *  vol:	The output volume (in %)
 *  data:	Where to place the result
 */

void
tone_dtmf(char tone, int samples, double vol, short *data) {
    char *s;			/* index into tones */
    tone_gen t;			/* tone generator */

    vol = BETWEEN(0.0,vol,100.0);
    if (!(s=strchr(dtmf_tones,tone))) {
	memset(data, 0, samples*sizeof(*data));
    } else {
        tone_create(dtmf_freq[(s-dtmf_tones)/4], dtmf_freq[4+(s-dtmf_tones)%4],
		vol, FREQ, &t);
        tone_dual(&t, samples, data);
    }
}

/*
 * Create a tone generator.
 * f1, f2:	The 2 tone frequencies
 * volume:	The volume (0-100)
 * freq:	The sampling frequency
 * t:		The tone_gen struct (or NULL)
 */

tone_gen *
tone_create(double f1, double f2, double volume, double freq, tone_gen *t) {
    if (t == ((tone_gen *) 0)) {
        t = (tone_gen *) malloc(sizeof (struct tone_gen_struct));
        t-> malloced = 1;
    } else {
        t-> malloced = 0;
    }
    f1 = 2.0 * M_PI * f1 / freq;
    f2 = 2.0 * M_PI * f2 / freq;
     
    t->x1 = t->y1 = 0.0;
    t->x0 = sin(f1) * volume * SCALE;
    t->y0 = sin(f2) * volume * SCALE;
    t->xc = 2.0 * cos(f1);
    t->yc = 2.0 * cos(f2);
    t->malloced = 1;

    return t;
}

/*
 * Free a tone generator
 */

void
tone_free(tone_gen *t) {
    if (t->malloced) {
        free(t);
    }
}
    
/*
 * Generate a dual-tone sound via 2nd order recursion
 * t:		The tone generator
 * samples:	# of samples (must be even)
 * data:	where to put the result.
 * returns	The tone generator, set up for the next buffer
 */


tone_gen *
tone_dual(tone_gen *t, int samples, short *data) {
    samples /= 2;
    while(samples-- > 0) {
	*data++ = ((int)(t->x1+t->y1));
	*data++ = ((int)(t->x0+t->y0));
        t->x1 = t->xc * t->x0 - t->x1;
        t->x0 = t->xc * t->x1 - t->x0;
        t->y1 = t->yc * t->y0 - t->y1;
        t->y0 = t->yc * t->y1 - t->y0;
    }
    return t;
}

#ifdef TONES_MAIN

int
main(int argc, char **argv) {
    int i;
    short *samples;
    if (argc < 2) {
	fprintf(stderr, "usage: %s <tones>\n", *argv);
	exit(0);
    }
 
    short data[2000];  /* tone buffer: 1600 sample, 400 "blank" */
    char *tones = argv[1];
    char c;

    while (c = *tones++) {
        tone_dtmf(c, 1600, 75, data);
	tone_dtmf('x',400, 75, data+1600);
	fwrite(data, 2000, 2, stdout);
    }
}

#endif
