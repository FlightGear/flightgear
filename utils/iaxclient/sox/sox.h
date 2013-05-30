/*
 * FILE: resample.h
 *   BY: Julius Smith (at CCRMA, Stanford U)
 * C BY: translated from SAIL to C by Christopher Lee Fraley
 *          (cf0v@andrew.cmu.edu)
 * DATE: 7-JUN-88
 * VERS: 2.0  (17-JUN-88, 3:00pm)
 */

/*
 * October 29, 1999
 * Various changes, bugfixes(?), increased precision, by Stan Brooks.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/* Conversion constants */
#define Lc        7
#define Nc       (1<<Lc)
#define La        16
#define Na       (1<<La)
#define Lp       (Lc+La)
#define Np       (1<<Lp)
#define Amask    (Na-1)
#define Pmask    (Np-1)

#define MAXNWING  (80<<Lc)
/* Description of constants:
 *
 * Nc - is the number of look-up values available for the lowpass filter
 *    between the beginning of its impulse response and the "cutoff time"
 *    of the filter.  The cutoff time is defined as the reciprocal of the
 *    lowpass-filter cut off frequence in Hz.  For example, if the
 *    lowpass filter were a sinc function, Nc would be the index of the
 *    impulse-response lookup-table corresponding to the first zero-
 *    crossing of the sinc function.  (The inverse first zero-crossing
 *    time of a sinc function equals its nominal cutoff frequency in Hz.)
 *    Nc must be a power of 2 due to the details of the current
 *    implementation. The default value of 128 is sufficiently high that
 *    using linear interpolation to fill in between the table entries
 *    gives approximately 16-bit precision, and quadratic interpolation
 *    gives about 23-bit (float) precision in filter coefficients.
 *
 * Lc - is log base 2 of Nc.
 *
 * La - is the number of bits devoted to linear interpolation of the
 *    filter coefficients.
 *
 * Lp - is La + Lc, the number of bits to the right of the binary point
 *    in the integer "time" variable. To the left of the point, it indexes
 *    the input array (X), and to the right, it is interpreted as a number
 *    between 0 and 1 sample of the input X.  The default value of 23 is
 *    about right.  There is a constraint that the filter window must be
 *    "addressable" in a int32_t, more precisely, if Nmult is the number
 *    of sinc zero-crossings in the right wing of the filter window, then
 *    (Nwing<<Lp) must be expressible in 31 bits.
 *
 */

/* this Float MUST match that in filter.c */
#define Float double/*float*/
#define ISCALE 0x10000

/* largest factor for which exact-coefficients upsampling will be used
 * */
#define NQMAX 511

#define BUFFSIZE 8192 /*16384*/  /* Total I/O buffer size */

typedef short st_sample_t;
typedef unsigned long st_size_t;
typedef int st_ssize_t;

#define ST_SAMPLE_MAX 0x7fff
#define ST_SAMPLE_MIN (-ST_SAMPLE_MAX - 1)

#define ST_SUCCESS 0
#define ST_EOF 1

#define ST_SAMPLE_RATE 8000
#define ST_CHANNELS 1


/* Private data for Lerp via LCM file */
typedef struct resamplestuff {
   double Factor;     /* Factor = Fout/Fin sample rates */
   double rolloff;    /* roll-off frequency */
   double beta;       /* passband/stopband tuning magic */
   int quadr;         /* non-zero to use qprodUD quadratic interpolation */
   long Nmult;        
   long Nwing;
   long Nq;
   Float *Imp;        /* impulse [Nwing+1] Filter coefficients */
   
   double Time;       /* Current time/pos in input sample */
   long dhb;
   
   long a,b;          /* gcd-reduced input,output rates   */
   long t;            /* Current time/pos for exact-coeff's method */
   
   long Xh;           /* number of past/future samples needed by filter */
   long Xoff;         /* Xh plus some room for creep  */
   long Xread;        /* X[Xread] is start-position to enter new samples */
   long Xp;           /* X[Xp] is position to start filter application */
   long Xsize,Ysize;  /* size (Floats) of X[],Y[]         */
   Float *X, *Y;      /* I/O buffers */
} *resample_t;


typedef struct compand {
  int expectedChannels; /* Also flags that channels aren't to be treated
                           individually when = 1 and input not mono */
  int transferPoints;   /* Number of points specified on the transfer
                           function */
  double *attackRate;   /* An array of attack rates */
  double *decayRate;    /*    ... and of decay rates */
  double *transferIns;  /*    ... and points on the transfer function */
  double *transferOuts;
  double *volume;       /* Current "volume" of each channel */
  double outgain;       /* Post processor gain */
  double delay;         /* Delay to apply before companding */
  st_sample_t *delay_buf;   /* Old samples, used for delay processing */
  st_ssize_t delay_buf_size;/* Size of delay_buf in samples */
  st_ssize_t delay_buf_ptr; /* Index into delay_buf */
  st_ssize_t delay_buf_cnt; /* No. of active entries in delay_buf */
  short int delay_buf_full; /* Shows buffer situation (important for st_compand_drain) */
} *compand_t;


int st_resample_start(resample_t *rH, int inrate, int outrate);
int st_resample_flow(resample_t *rH, st_sample_t *ibuf, st_sample_t *obuf,
                     st_size_t *isamp, st_size_t *osamp);
int st_resample_drain(resample_t *rH, st_sample_t *obuf, st_size_t *osamp);
int st_resample_stop(resample_t *rH);


int st_compand_getopts(compand_t l, int n, char **argv); 
int st_compand_start(compand_t *lH, char **opts, int nopts);
int st_compand_flow(compand_t l, st_sample_t *ibuf, st_sample_t *obuf, 
                     st_size_t *isamp, st_size_t *osamp);
int st_compand_drain(compand_t l, st_sample_t *obuf, st_size_t *osamp);
int st_compand_stop(compand_t l);

void st_report(const char *fmt, ...);
void st_fail(const char *fmt, ...);
void st_warn(const char *fmt, ...);
long st_gcd(long a, long b);
