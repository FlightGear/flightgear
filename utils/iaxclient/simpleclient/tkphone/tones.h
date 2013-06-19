/* tones.h */

#define SCALE 160.0	   /* scale factor for full range sound  in short */
#define FREQ  8000.0	   /* sampling rate */

/* define a tone generator; all state needed to generate 2 sine waves */

typedef struct tone_gen_struct {
    double x0, x1;	/* 2 sample values of sine 1 */
    double y0, y1;	/* 2 sample values for sine 2 */
    double xc, yc;	/* recursion coefficients */
    int malloced;	/* true if malloc'd space */;
} tone_gen;

void tone_dtmf(char tone, int samples, double vol, short *data);
tone_gen *tone_create(double f1, double f2, double volume, double freq,
	tone_gen *t);
void tone_free(tone_gen *t);
tone_gen *tone_dual(tone_gen *t, int samples, short *data);
