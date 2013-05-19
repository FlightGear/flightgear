/* Copyright (C) Jean-Marc Valin

   File: speex_echo.c
   Echo cancelling based on the MDF algorithm described in:

   J. S. Soo, K. K. Pang Multidelay block frequency adaptive filter, 
   IEEE Trans. Acoust. Speech Signal Process., Vol. ASSP-38, No. 2, 
   February 1990.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "misc.h"
#include "speex/speex_echo.h"
#include "smallft.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BETA .65

#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))

/** Compute inner product of two real vectors */
static inline float inner_prod(float *x, float *y, int N)
{
   int i;
   float ret=0;
   for (i=0;i<N;i++)
      ret += x[i]*y[i];
   return ret;
}

/** Compute power spectrum of a half-complex (packed) vector */
static inline void power_spectrum(float *X, float *ps, int N)
{
   int i, j;
   ps[0]=X[0]*X[0];
   for (i=1,j=1;i<N-1;i+=2,j++)
   {
      ps[j] =  X[i]*X[i] + X[i+1]*X[i+1];
   }
   ps[j]=X[i]*X[i];
}

/** Compute cross-power spectrum of a half-complex (packed) vectors and add to acc */
static inline void spectral_mul_accum(float *X, float *Y, float *acc, int N)
{
   int i;
   acc[0] += X[0]*Y[0];
   for (i=1;i<N-1;i+=2)
   {
      acc[i] += (X[i]*Y[i] - X[i+1]*Y[i+1]);
      acc[i+1] += (X[i+1]*Y[i] + X[i]*Y[i+1]);
   }
   acc[i] += X[i]*Y[i];
}

/** Compute cross-power spectrum of a half-complex (packed) vector with conjugate */
static inline void spectral_mul_conj(float *X, float *Y, float *prod, int N)
{
   int i;
   prod[0] = X[0]*Y[0];
   for (i=1;i<N-1;i+=2)
   {
      prod[i] = (X[i]*Y[i] + X[i+1]*Y[i+1]);
      prod[i+1] = (-X[i+1]*Y[i] + X[i]*Y[i+1]);
   }
   prod[i] = X[i]*Y[i];
}


/** Compute weighted cross-power spectrum of a half-complex (packed) vector with conjugate */
static inline void weighted_spectral_mul_conj(float *w, float *X, float *Y, float *prod, int N)
{
   int i, j;
   prod[0] = w[0]*X[0]*Y[0];
   for (i=1,j=1;i<N-1;i+=2,j++)
   {
      prod[i] = w[j]*(X[i]*Y[i] + X[i+1]*Y[i+1]);
      prod[i+1] = w[j]*(-X[i+1]*Y[i] + X[i]*Y[i+1]);
   }
   prod[i] = w[j]*X[i]*Y[i];
}


/** Creates a new echo canceller state */
SpeexEchoState *speex_echo_state_init(int frame_size, int filter_length)
{
   int i,j,N,M;
   SpeexEchoState *st = (SpeexEchoState *)speex_alloc(sizeof(SpeexEchoState));

   st->frame_size = frame_size;
   st->window_size = 2*frame_size;
   N = st->window_size;
   M = st->M = (filter_length+st->frame_size-1)/frame_size;
   st->cancel_count=0;
   st->adapt_rate = .01f;
   st->sum_adapt = 0;
   st->Sey = 0;
   st->Syy = 0;
   st->See = 0;
         
   st->fft_lookup = (struct drft_lookup*)speex_alloc(sizeof(struct drft_lookup));
   spx_drft_init(st->fft_lookup, N);
   
   st->x = (float*)speex_alloc(N*sizeof(float));
   st->d = (float*)speex_alloc(N*sizeof(float));
   st->y = (float*)speex_alloc(N*sizeof(float));
   st->y2 = (float*)speex_alloc(N*sizeof(float));
   st->Yps = (float*)speex_alloc(N*sizeof(float));
   st->last_y = (float*)speex_alloc(N*sizeof(float));
   st->Yf = (float*)speex_alloc((st->frame_size+1)*sizeof(float));
   st->Rf = (float*)speex_alloc((st->frame_size+1)*sizeof(float));
   st->Xf = (float*)speex_alloc((st->frame_size+1)*sizeof(float));
   st->fratio = (float*)speex_alloc((st->frame_size+1)*sizeof(float));
   st->regul = (float*)speex_alloc(N*sizeof(float));

   st->X = (float*)speex_alloc(M*N*sizeof(float));
   st->D = (float*)speex_alloc(N*sizeof(float));
   st->Y = (float*)speex_alloc(N*sizeof(float));
   st->Y2 = (float*)speex_alloc(N*sizeof(float));
   st->E = (float*)speex_alloc(N*sizeof(float));
   st->W = (float*)speex_alloc(M*N*sizeof(float));
   st->PHI = (float*)speex_alloc(M*N*sizeof(float));
   st->power = (float*)speex_alloc((frame_size+1)*sizeof(float));
   st->power_1 = (float*)speex_alloc((frame_size+1)*sizeof(float));
   st->grad = (float*)speex_alloc(N*M*sizeof(float));
   
   for (i=0;i<N*M;i++)
   {
      st->W[i] = st->PHI[i] = 0;
   }
   
   st->regul[0] = (.01+(10.)/((4.)*(4.)))/M;
   for (i=1,j=1;i<N-1;i+=2,j++)
   {
      st->regul[i] = .01+((10.)/((j+4.)*(j+4.)))/M;
      st->regul[i+1] = .01+((10.)/((j+4.)*(j+4.)))/M;
   }
   st->regul[i] = .01+((10.)/((j+4.)*(j+4.)))/M;
         
   st->adapted = 0;
   return st;
}

/** Resets echo canceller state */
void speex_echo_state_reset(SpeexEchoState *st)
{
   int i, M, N;
   st->cancel_count=0;
   st->adapt_rate = .01f;
   N = st->window_size;
   M = st->M;
   for (i=0;i<N*M;i++)
   {
      st->W[i] = 0;
      st->X[i] = 0;
   }
   for (i=0;i<=st->frame_size;i++)
      st->power[i] = 0;
   
   st->adapted = 0;
   st->adapt_rate = .01f;
   st->sum_adapt = 0;
   st->Sey = 0;
   st->Syy = 0;
   st->See = 0;

}

/** Destroys an echo canceller state */
void speex_echo_state_destroy(SpeexEchoState *st)
{
   spx_drft_clear(st->fft_lookup);
   speex_free(st->fft_lookup);
   speex_free(st->x);
   speex_free(st->d);
   speex_free(st->y);
   speex_free(st->last_y);
   speex_free(st->Yps);
   speex_free(st->Yf);
   speex_free(st->Rf);
   speex_free(st->Xf);
   speex_free(st->fratio);
   speex_free(st->regul);

   speex_free(st->X);
   speex_free(st->D);
   speex_free(st->Y);
   speex_free(st->E);
   speex_free(st->W);
   speex_free(st->PHI);
   speex_free(st->power);
   speex_free(st->power_1);
   speex_free(st->grad);

   speex_free(st);
}

      
/** Performs echo cancellation on a frame */
void speex_echo_cancel(SpeexEchoState *st, short *ref, short *echo, short *out, float *Yout)
{
   int i,j,m;
   int N,M;
   float scale;
   float ESR;
   float SER;
   //float Sry=0
   float Srr=0,Syy=0,Sey=0,See=0,Sxx=0;
   float leak_estimate;
   
   leak_estimate = .1+(.9/(1+2*st->sum_adapt));
         
   N = st->window_size;
   M = st->M;
   scale = 1.0f/N;
   st->cancel_count++;

   /* Copy input data to buffer */
   for (i=0;i<st->frame_size;i++)
   {
      st->x[i] = st->x[i+st->frame_size];
      st->x[i+st->frame_size] = echo[i];

      st->d[i] = st->d[i+st->frame_size];
      st->d[i+st->frame_size] = ref[i];
   }

   /* Shift memory: this could be optimized eventually*/
   for (i=0;i<N*(M-1);i++)
      st->X[i]=st->X[i+N];

   /* Copy new echo frame */
   for (i=0;i<N;i++)
      st->X[(M-1)*N+i]=st->x[i];

   /* Convert x (echo input) to frequency domain */
   spx_drft_forward(st->fft_lookup, &st->X[(M-1)*N]);

   /* Compute filter response Y */
   for (i=0;i<N;i++)
      st->Y[i] = 0;
   for (j=0;j<M;j++)
      spectral_mul_accum(&st->X[j*N], &st->W[j*N], st->Y, N);
   
   /* Convert Y (filter response) to time domain */
   for (i=0;i<N;i++)
      st->y[i] = st->Y[i];
   spx_drft_backward(st->fft_lookup, st->y);
   for (i=0;i<N;i++)
      st->y[i] *= scale;

   /* Transform d (reference signal) to frequency domain */
   for (i=0;i<N;i++)
      st->D[i]=st->d[i];
   spx_drft_forward(st->fft_lookup, st->D);

   /* Compute error signal (signal with echo removed) */ 
   for (i=0;i<st->frame_size;i++)
   {
      float tmp_out;
      tmp_out = (float)ref[i] - st->y[i+st->frame_size];
      
      st->E[i] = 0;
      st->E[i+st->frame_size] = tmp_out;
      
      /* Saturation */
      if (tmp_out>32767)
         tmp_out = 32767;
      else if (tmp_out<-32768)
         tmp_out = -32768;
      out[i] = tmp_out;
   }
   
   /* This bit of code provides faster adaptation by doing a projection of the previous gradient on the 
      "MMSE surface" */
   if (1)
   {
      float Sge, Sgg, Syy;
      float gain;
      Syy = inner_prod(st->y+st->frame_size, st->y+st->frame_size, st->frame_size);
      for (i=0;i<N;i++)
         st->Y2[i] = 0;
      for (j=0;j<M;j++)
         spectral_mul_accum(&st->X[j*N], &st->PHI[j*N], st->Y2, N);
      for (i=0;i<N;i++)
         st->y2[i] = st->Y2[i];
      spx_drft_backward(st->fft_lookup, st->y2);
      for (i=0;i<N;i++)
         st->y2[i] *= scale;
      Sge = inner_prod(st->y2+st->frame_size, st->E+st->frame_size, st->frame_size);
      Sgg = inner_prod(st->y2+st->frame_size, st->y2+st->frame_size, st->frame_size);
      /* Compute projection gain */
      gain = Sge/(N+.03*Syy+Sgg);
      if (gain>2)
         gain = 2;
      if (gain < -2)
         gain = -2;
      
      /* Apply gain to weights, echo estimates, output */
      for (i=0;i<N;i++)
         st->Y[i] += gain*st->Y2[i];
      for (i=0;i<st->frame_size;i++)
      {
         st->y[i+st->frame_size] += gain*st->y2[i+st->frame_size];
         st->E[i+st->frame_size] -= gain*st->y2[i+st->frame_size];
      }
      for (i=0;i<M*N;i++)
         st->W[i] += gain*st->PHI[i];
   }

   /* Compute power spectrum of output (D-Y) and filter response (Y) */
   for (i=0;i<N;i++)
      st->D[i] -= st->Y[i];
   power_spectrum(st->D, st->Rf, N);
   power_spectrum(st->Y, st->Yf, N);
   
   /* Compute frequency-domain adaptation mask */
   for (j=0;j<=st->frame_size;j++)
   {
      float r;
      r = leak_estimate*st->Yf[j] / (1+st->Rf[j]);
      if (r>1)
         r = 1;
      st->fratio[j] = r;
   }

   /* Compute a bunch of correlations */
   //Sry = inner_prod(st->y+st->frame_size, st->d+st->frame_size, st->frame_size);
   Sey = inner_prod(st->y+st->frame_size, st->E+st->frame_size, st->frame_size);
   See = inner_prod(st->E+st->frame_size, st->E+st->frame_size, st->frame_size);
   Syy = inner_prod(st->y+st->frame_size, st->y+st->frame_size, st->frame_size);
   Srr = inner_prod(st->d+st->frame_size, st->d+st->frame_size, st->frame_size);
   Sxx = inner_prod(st->x+st->frame_size, st->x+st->frame_size, st->frame_size);

   /* Compute smoothed cross-correlation and energy */   
   st->Sey = .98*st->Sey + .02*Sey;
   st->Syy = .98*st->Syy + .02*Syy;
   st->See = .98*st->See + .02*See;
   
   /* Check if filter is completely mis-adapted (if so, reset filter) */
   if (st->Sey/(1+st->Syy + .01*st->See) < -1)
   {
      /*fprintf (stderr, "reset at %d\n", st->cancel_count);*/
      speex_echo_state_reset(st);
      return;
   }

   SER = Srr / (1+Sxx);
   ESR = leak_estimate*Syy / (1+See);
   if (ESR>1)
      ESR = 1;
#if 1
   /* If over-cancellation (creating echo with 180 phase) damp filter */
   if (st->Sey/(1+st->Syy) < -.1 && (ESR > .3))
   {
      for (i=0;i<M*N;i++)
         st->W[i] *= .95;
      st->Sey *= .5;
      /*fprintf (stderr, "corrected down\n");*/
   }
#endif
#if 1
   /* If under-cancellation (leaving echo with 0 phase) scale filter up */
   if (st->Sey/(1+st->Syy) > .1 && (ESR > .1 || SER < 10))
   {
      for (i=0;i<M*N;i++)
         st->W[i] *= 1.05;
      st->Sey *= .5;
      /*fprintf (stderr, "corrected up %d\n", st->cancel_count);*/
   }
#endif
   
   /* We consider that the filter is adapted if the following is true*/
   if (ESR>.6 && st->sum_adapt > 1)
   {
      /*if (!st->adapted)
         fprintf(stderr, "Adapted at %d %f\n", st->cancel_count, st->sum_adapt);*/
      st->adapted = 1;
   }
   
   /* Update frequency-dependent energy ratio with the total energy ratio */
   for (i=0;i<=st->frame_size;i++)
   {
      st->fratio[i]  = (.2*ESR+.8*min(.005+ESR,st->fratio[i]));
   }   

   if (st->adapted)
   {
      st->adapt_rate = .95f/(2+M);
   } else {
      /* Temporary adaption rate if filter is not adapted correctly */
      if (SER<.1)
         st->adapt_rate =.8/(2+M);
      else if (SER<1)
         st->adapt_rate =.4/(2+M);
      else if (SER<10)
         st->adapt_rate =.2/(2+M);
      else if (SER<30)
         st->adapt_rate =.08/(2+M);
      else
         st->adapt_rate = 0;
   }
   
   /* How much have we adapted so far? */
   st->sum_adapt += st->adapt_rate;

   /* Compute echo power in each frequency bin */
   {
      float ss = 1.0f/st->cancel_count;
      if (ss < .3/M)
         ss=.3/M;
      power_spectrum(&st->X[(M-1)*N], st->Xf, N);
      /* Smooth echo energy estimate over time */
      for (j=0;j<=st->frame_size;j++)
         st->power[j] = (1-ss)*st->power[j] + ss*st->Xf[j];
      
      
      /* Combine adaptation rate to the the inverse energy estimate */
      if (st->adapted)
      {
         /* If filter is adapted, include the frequency-dependent ratio too */
         for (i=0;i<=st->frame_size;i++)
            st->power_1[i] = st->adapt_rate*st->fratio[i] /(1.f+st->power[i]);
      } else {
         for (i=0;i<=st->frame_size;i++)
            st->power_1[i] = st->adapt_rate/(1.f+st->power[i]);
      }
   }

   
   /* Convert error to frequency domain */
   spx_drft_forward(st->fft_lookup, st->E);

   /* Do some regularization (prevents problems when system is ill-conditoned) */
   for (m=0;m<M;m++)
      for (i=0;i<N;i++)
         st->W[m*N+i] *= 1-st->regul[i]*ESR;
   
   /* Compute weight gradient */
   for (j=0;j<M;j++)
   {
      weighted_spectral_mul_conj(st->power_1, &st->X[j*N], st->E, st->PHI+N*j, N);
   }

   /* Gradient descent */
   for (i=0;i<M*N;i++)
      st->W[i] += st->PHI[i];
   
   /* AUMDF weight constraint */
   for (j=0;j<M;j++)
   {
      /* Remove the "if" to make this an MDF filter */
      if (st->cancel_count%M == j)
      {
         spx_drft_backward(st->fft_lookup, &st->W[j*N]);
         for (i=0;i<N;i++)
            st->W[j*N+i]*=scale;
         for (i=st->frame_size;i<N;i++)
         {
            st->W[j*N+i]=0;
         }
         spx_drft_forward(st->fft_lookup, &st->W[j*N]);
      }
   }

   /* Compute spectrum of estimated echo for use in an echo post-filter (if necessary)*/
   if (Yout)
   {
      if (st->adapted)
      {
         /* If the filter is adapted, take the filtered echo */
         for (i=0;i<st->frame_size;i++)
            st->last_y[i] = st->last_y[st->frame_size+i];
         for (i=0;i<st->frame_size;i++)
            st->last_y[st->frame_size+i] = st->y[st->frame_size+i];
      } else {
         /* If filter isn't adapted yet, all we can do is take the echo signal directly */
         for (i=0;i<N;i++)
            st->last_y[i] = st->x[i];
      }
      
      /* Apply hanning window (should pre-compute it)*/
      for (i=0;i<N;i++)
         st->Yps[i] = (.5-.5*cos(2*M_PI*i/N))*st->last_y[i];
      
      /* Compute power spectrum of the echo */
      spx_drft_forward(st->fft_lookup, st->Yps);
      power_spectrum(st->Yps, st->Yps, N);
      
      /* Estimate residual echo */
      for (i=0;i<=st->frame_size;i++)
         Yout[i] = 2*leak_estimate*st->Yps[i];
   }

}

