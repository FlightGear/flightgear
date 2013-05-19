/* Copyright (C) 2002 Jean-Marc Valin 
   File: cb_search.c

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cb_search.h"
#include "filters.h"
#include "stack_alloc.h"
#include "vq.h"
#include "misc.h"

#ifdef _USE_SSE
#include "cb_search_sse.h"
#elif defined(ARM4_ASM) || defined(ARM5E_ASM)
#include "cb_search_arm4.h"
#else

static void compute_weighted_codebook(const signed char *shape_cb, const spx_word16_t *r, spx_word16_t *resp, spx_word16_t *resp2, spx_word32_t *E, int shape_cb_size, int subvect_size, char *stack)
{
   int i, j, k;
   VARDECL(spx_word16_t *shape);
   ALLOC(shape, subvect_size, spx_word16_t);
   for (i=0;i<shape_cb_size;i++)
   {
      spx_word16_t *res;
      
      res = resp+i*subvect_size;
      for (k=0;k<subvect_size;k++)
         shape[k] = (spx_word16_t)shape_cb[i*subvect_size+k];
      E[i]=0;

      /* Compute codeword response using convolution with impulse response */
      for(j=0;j<subvect_size;j++)
      {
         spx_word32_t resj=0;
         spx_word16_t res16;
         for (k=0;k<=j;k++)
            resj = MAC16_16(resj,shape[k],r[j-k]);
#ifdef FIXED_POINT
         res16 = EXTRACT16(SHR32(resj, 11));
#else
         res16 = 0.03125f*resj;
#endif
         /* Compute codeword energy */
         E[i]=MAC16_16(E[i],res16,res16);
         res[j] = res16;
         /*printf ("%d\n", (int)res[j]);*/
      }
   }

}

#endif



static void split_cb_search_shape_sign_N1(
spx_sig_t target[],			/* target vector */
spx_coef_t ak[],			/* LPCs for this subframe */
spx_coef_t awk1[],			/* Weighted LPCs for this subframe */
spx_coef_t awk2[],			/* Weighted LPCs for this subframe */
const void *par,                      /* Codebook/search parameters*/
int   p,                        /* number of LPC coeffs */
int   nsf,                      /* number of samples in subframe */
spx_sig_t *exc,
spx_word16_t *r,
SpeexBits *bits,
char *stack,
int   complexity,
int   update_target
)
{
   int i,j,m,n,q;
   VARDECL(spx_word16_t *resp);
#ifdef _USE_SSE
   VARDECL(__m128 *resp2);
   VARDECL(__m128 *E);
#else
   spx_word16_t *resp2;
   VARDECL(spx_word32_t *E);
#endif
   VARDECL(spx_word16_t *t);
   VARDECL(spx_sig_t *e);
   const signed char *shape_cb;
   int shape_cb_size, subvect_size, nb_subvect;
   const split_cb_params *params;
   int N=2;
   int best_index;
   spx_word32_t best_dist;
   int have_sign;
   N=complexity;
   if (N>10)
      N=10;
   if (N<1)
      N=1;
   
   params = (const split_cb_params *) par;
   subvect_size = params->subvect_size;
   nb_subvect = params->nb_subvect;
   shape_cb_size = 1<<params->shape_bits;
   shape_cb = params->shape_cb;
   have_sign = params->have_sign;
   ALLOC(resp, shape_cb_size*subvect_size, spx_word16_t);
#ifdef _USE_SSE
   ALLOC(resp2, (shape_cb_size*subvect_size)>>2, __m128);
   ALLOC(E, shape_cb_size>>2, __m128);
#else
   resp2 = resp;
   ALLOC(E, shape_cb_size, spx_word32_t);
#endif
   ALLOC(t, nsf, spx_word16_t);
   ALLOC(e, nsf, spx_sig_t);
   
   /* FIXME: make that adaptive? */
   for (i=0;i<nsf;i++)
      t[i]=EXTRACT16(PSHR32(target[i],6));

   compute_weighted_codebook(shape_cb, r, resp, resp2, E, shape_cb_size, subvect_size, stack);

   for (i=0;i<nb_subvect;i++)
   {
      spx_word16_t *x=t+subvect_size*i;
      /*Find new n-best based on previous n-best j*/
      if (have_sign)
         vq_nbest_sign(x, resp2, subvect_size, shape_cb_size, E, 1, &best_index, &best_dist, stack);
      else
         vq_nbest(x, resp2, subvect_size, shape_cb_size, E, 1, &best_index, &best_dist, stack);
      
      speex_bits_pack(bits,best_index,params->shape_bits+have_sign);
      
      {
         int rind;
         spx_word16_t *res;
         spx_word16_t sign=1;
         rind = best_index;
         if (rind>=shape_cb_size)
         {
            sign=-1;
            rind-=shape_cb_size;
         }
         res = resp+rind*subvect_size;
         if (sign>0)
            for (m=0;m<subvect_size;m++)
               t[subvect_size*i+m] = SUB16(t[subvect_size*i+m], res[m]);
         else
            for (m=0;m<subvect_size;m++)
               t[subvect_size*i+m] = ADD16(t[subvect_size*i+m], res[m]);

#ifdef FIXED_POINT
         if (sign)
         {
            for (j=0;j<subvect_size;j++)
               e[subvect_size*i+j]=SHL32(EXTEND32(shape_cb[rind*subvect_size+j]),SIG_SHIFT-5);
         } else {
            for (j=0;j<subvect_size;j++)
               e[subvect_size*i+j]=NEG32(SHL32(EXTEND32(shape_cb[rind*subvect_size+j]),SIG_SHIFT-5));
         }
#else
         for (j=0;j<subvect_size;j++)
            e[subvect_size*i+j]=sign*0.03125*shape_cb[rind*subvect_size+j];
#endif
      
      }
            
      for (m=0;m<subvect_size;m++)
      {
         spx_word16_t g;
         int rind;
         spx_word16_t sign=1;
         rind = best_index;
         if (rind>=shape_cb_size)
         {
            sign=-1;
            rind-=shape_cb_size;
         }
         
         q=subvect_size-m;
#ifdef FIXED_POINT
         g=sign*shape_cb[rind*subvect_size+m];
         for (n=subvect_size*(i+1);n<nsf;n++,q++)
            t[n] = SUB32(t[n],MULT16_16_Q11_32(g,r[q]));
#else
         g=sign*0.03125*shape_cb[rind*subvect_size+m];
         for (n=subvect_size*(i+1);n<nsf;n++,q++)
            t[n] = SUB32(t[n],g*r[q]);
#endif
      }
   }

   /* Update excitation */
   /* FIXME: We could update the excitation directly above */
   for (j=0;j<nsf;j++)
      exc[j]=ADD32(exc[j],e[j]);
   
   /* Update target: only update target if necessary */
   if (update_target)
   {
      VARDECL(spx_sig_t *r2);
      ALLOC(r2, nsf, spx_sig_t);
      syn_percep_zero(e, ak, awk1, awk2, r2, nsf,p, stack);
      for (j=0;j<nsf;j++)
         target[j]=SUB32(target[j],r2[j]);
   }
}



void split_cb_search_shape_sign(
spx_sig_t target[],			/* target vector */
spx_coef_t ak[],			/* LPCs for this subframe */
spx_coef_t awk1[],			/* Weighted LPCs for this subframe */
spx_coef_t awk2[],			/* Weighted LPCs for this subframe */
const void *par,                      /* Codebook/search parameters*/
int   p,                        /* number of LPC coeffs */
int   nsf,                      /* number of samples in subframe */
spx_sig_t *exc,
spx_word16_t *r,
SpeexBits *bits,
char *stack,
int   complexity,
int   update_target
)
{
   int i,j,k,m,n,q;
   VARDECL(spx_word16_t *resp);
#ifdef _USE_SSE
   VARDECL(__m128 *resp2);
   VARDECL(__m128 *E);
#else
   spx_word16_t *resp2;
   VARDECL(spx_word32_t *E);
#endif
   VARDECL(spx_word16_t *t);
   VARDECL(spx_sig_t *e);
   VARDECL(spx_sig_t *r2);
   VARDECL(spx_word16_t *tmp);
   VARDECL(spx_word32_t *ndist);
   VARDECL(spx_word32_t *odist);
   VARDECL(int *itmp);
   VARDECL(spx_word16_t **ot2);
   VARDECL(spx_word16_t **nt2);
   spx_word16_t **ot, **nt;
   VARDECL(int **nind);
   VARDECL(int **oind);
   VARDECL(int *ind);
   const signed char *shape_cb;
   int shape_cb_size, subvect_size, nb_subvect;
   const split_cb_params *params;
   int N=2;
   VARDECL(int *best_index);
   VARDECL(spx_word32_t *best_dist);
   int have_sign;
   N=complexity;
   if (N>10)
      N=10;
   if (N<1)
      N=1;
   
   if (N==1)
   {
      split_cb_search_shape_sign_N1(target,ak,awk1,awk2,par,p,nsf,exc,r,bits,stack,complexity,update_target);
      return;
   }
   ALLOC(ot2, N, spx_word16_t*);
   ALLOC(nt2, N, spx_word16_t*);
   ALLOC(oind, N, int*);
   ALLOC(nind, N, int*);

   params = (const split_cb_params *) par;
   subvect_size = params->subvect_size;
   nb_subvect = params->nb_subvect;
   shape_cb_size = 1<<params->shape_bits;
   shape_cb = params->shape_cb;
   have_sign = params->have_sign;
   ALLOC(resp, shape_cb_size*subvect_size, spx_word16_t);
#ifdef _USE_SSE
   ALLOC(resp2, (shape_cb_size*subvect_size)>>2, __m128);
   ALLOC(E, shape_cb_size>>2, __m128);
#else
   resp2 = resp;
   ALLOC(E, shape_cb_size, spx_word32_t);
#endif
   ALLOC(t, nsf, spx_word16_t);
   ALLOC(e, nsf, spx_sig_t);
   ALLOC(r2, nsf, spx_sig_t);
   ALLOC(ind, nb_subvect, int);

   ALLOC(tmp, 2*N*nsf, spx_word16_t);
   for (i=0;i<N;i++)
   {
      ot2[i]=tmp+2*i*nsf;
      nt2[i]=tmp+(2*i+1)*nsf;
   }
   ot=ot2;
   nt=nt2;
   ALLOC(best_index, N, int);
   ALLOC(best_dist, N, spx_word32_t);
   ALLOC(ndist, N, spx_word32_t);
   ALLOC(odist, N, spx_word32_t);
   
   ALLOC(itmp, 2*N*nb_subvect, int);
   for (i=0;i<N;i++)
   {
      nind[i]=itmp+2*i*nb_subvect;
      oind[i]=itmp+(2*i+1)*nb_subvect;
      for (j=0;j<nb_subvect;j++)
         nind[i][j]=oind[i][j]=-1;
   }
   
   /* FIXME: make that adaptive? */
   for (i=0;i<nsf;i++)
      t[i]=EXTRACT16(PSHR32(target[i],6));

   for (j=0;j<N;j++)
      for (i=0;i<nsf;i++)
         ot[j][i]=t[i];

   /*for (i=0;i<nsf;i++)
     printf ("%d\n", (int)t[i]);*/

   /* Pre-compute codewords response and energy */
   compute_weighted_codebook(shape_cb, r, resp, resp2, E, shape_cb_size, subvect_size, stack);

   for (j=0;j<N;j++)
      odist[j]=0;
   /*For all subvectors*/
   for (i=0;i<nb_subvect;i++)
   {
      /*"erase" nbest list*/
      for (j=0;j<N;j++)
         ndist[j]=-2;

      /*For all n-bests of previous subvector*/
      for (j=0;j<N;j++)
      {
         spx_word16_t *x=ot[j]+subvect_size*i;
         /*Find new n-best based on previous n-best j*/
         if (have_sign)
            vq_nbest_sign(x, resp2, subvect_size, shape_cb_size, E, N, best_index, best_dist, stack);
         else
            vq_nbest(x, resp2, subvect_size, shape_cb_size, E, N, best_index, best_dist, stack);

         /*For all new n-bests*/
         for (k=0;k<N;k++)
         {
            spx_word16_t *ct;
            spx_word32_t err=0;
            ct = ot[j];
            /*update target*/

            /*previous target*/
            for (m=i*subvect_size;m<(i+1)*subvect_size;m++)
               t[m]=ct[m];

            /* New code: update only enough of the target to calculate error*/
            {
               int rind;
               spx_word16_t *res;
               spx_word16_t sign=1;
               rind = best_index[k];
               if (rind>=shape_cb_size)
               {
                  sign=-1;
                  rind-=shape_cb_size;
               }
               res = resp+rind*subvect_size;
               if (sign>0)
                  for (m=0;m<subvect_size;m++)
                     t[subvect_size*i+m] = SUB16(t[subvect_size*i+m], res[m]);
               else
                  for (m=0;m<subvect_size;m++)
                     t[subvect_size*i+m] = ADD16(t[subvect_size*i+m], res[m]);
            }
            
            /*compute error (distance)*/
            err=odist[j];
            for (m=i*subvect_size;m<(i+1)*subvect_size;m++)
               err = MAC16_16(err, t[m],t[m]);
            /*update n-best list*/
            if (err<ndist[N-1] || ndist[N-1]<-1)
            {

               /*previous target (we don't care what happened before*/
               for (m=(i+1)*subvect_size;m<nsf;m++)
                  t[m]=ct[m];
               /* New code: update the rest of the target only if it's worth it */
               for (m=0;m<subvect_size;m++)
               {
                  spx_word16_t g;
                  int rind;
                  spx_word16_t sign=1;
                  rind = best_index[k];
                  if (rind>=shape_cb_size)
                  {
                     sign=-1;
                     rind-=shape_cb_size;
                  }

                  q=subvect_size-m;
#ifdef FIXED_POINT
                  g=sign*shape_cb[rind*subvect_size+m];
                  for (n=subvect_size*(i+1);n<nsf;n++,q++)
                     t[n] = SUB32(t[n],MULT16_16_Q11_32(g,r[q]));
#else
                  g=sign*0.03125*shape_cb[rind*subvect_size+m];
                  for (n=subvect_size*(i+1);n<nsf;n++,q++)
                     t[n] = SUB32(t[n],g*r[q]);
#endif
               }


               for (m=0;m<N;m++)
               {
                  if (err < ndist[m] || ndist[m]<-1)
                  {
                     for (n=N-1;n>m;n--)
                     {
                        for (q=(i+1)*subvect_size;q<nsf;q++)
                           nt[n][q]=nt[n-1][q];
                        for (q=0;q<nb_subvect;q++)
                           nind[n][q]=nind[n-1][q];
                        ndist[n]=ndist[n-1];
                     }
                     for (q=(i+1)*subvect_size;q<nsf;q++)
                        nt[m][q]=t[q];
                     for (q=0;q<nb_subvect;q++)
                        nind[m][q]=oind[j][q];
                     nind[m][i]=best_index[k];
                     ndist[m]=err;
                     break;
                  }
               }
            }
         }
         if (i==0)
           break;
      }

      /*update old-new data*/
      /* just swap pointers instead of a long copy */
      {
         spx_word16_t **tmp2;
         tmp2=ot;
         ot=nt;
         nt=tmp2;
      }
      for (j=0;j<N;j++)
         for (m=0;m<nb_subvect;m++)
            oind[j][m]=nind[j][m];
      for (j=0;j<N;j++)
         odist[j]=ndist[j];
   }

   /*save indices*/
   for (i=0;i<nb_subvect;i++)
   {
      ind[i]=nind[0][i];
      speex_bits_pack(bits,ind[i],params->shape_bits+have_sign);
   }
   
   /* Put everything back together */
   for (i=0;i<nb_subvect;i++)
   {
      int rind;
      spx_word16_t sign=1;
      rind = ind[i];
      if (rind>=shape_cb_size)
      {
         sign=-1;
         rind-=shape_cb_size;
      }
#ifdef FIXED_POINT
      if (sign==1)
      {
         for (j=0;j<subvect_size;j++)
            e[subvect_size*i+j]=SHL32(EXTEND32(shape_cb[rind*subvect_size+j]),SIG_SHIFT-5);
      } else {
         for (j=0;j<subvect_size;j++)
            e[subvect_size*i+j]=NEG32(SHL32(EXTEND32(shape_cb[rind*subvect_size+j]),SIG_SHIFT-5));
      }
#else
      for (j=0;j<subvect_size;j++)
         e[subvect_size*i+j]=sign*0.03125*shape_cb[rind*subvect_size+j];
#endif
   }   
   /* Update excitation */
   for (j=0;j<nsf;j++)
      exc[j]=ADD32(exc[j],e[j]);
   
   /* Update target: only update target if necessary */
   if (update_target)
   {
      syn_percep_zero(e, ak, awk1, awk2, r2, nsf,p, stack);
      for (j=0;j<nsf;j++)
         target[j]=SUB32(target[j],r2[j]);
   }
}


void split_cb_shape_sign_unquant(
spx_sig_t *exc,
const void *par,                      /* non-overlapping codebook */
int   nsf,                      /* number of samples in subframe */
SpeexBits *bits,
char *stack
)
{
   int i,j;
   VARDECL(int *ind);
   VARDECL(int *signs);
   const signed char *shape_cb;
   //int shape_cb_size;
   int subvect_size, nb_subvect;
   const split_cb_params *params;
   int have_sign;

   params = (const split_cb_params *) par;
   subvect_size = params->subvect_size;
   nb_subvect = params->nb_subvect;
   //shape_cb_size = 1<<params->shape_bits;
   shape_cb = params->shape_cb;
   have_sign = params->have_sign;

   ALLOC(ind, nb_subvect, int);
   ALLOC(signs, nb_subvect, int);

   /* Decode codewords and gains */
   for (i=0;i<nb_subvect;i++)
   {
      if (have_sign)
         signs[i] = speex_bits_unpack_unsigned(bits, 1);
      else
         signs[i] = 0;
      ind[i] = speex_bits_unpack_unsigned(bits, params->shape_bits);
   }
   /* Compute decoded excitation */
   for (i=0;i<nb_subvect;i++)
   {
      spx_word16_t s=1;
      if (signs[i])
         s=-1;
#ifdef FIXED_POINT
      if (s==1)
      {
         for (j=0;j<subvect_size;j++)
            exc[subvect_size*i+j]=SHL32(EXTEND32(shape_cb[ind[i]*subvect_size+j]),SIG_SHIFT-5);
      } else {
         for (j=0;j<subvect_size;j++)
            exc[subvect_size*i+j]=NEG32(SHL32(EXTEND32(shape_cb[ind[i]*subvect_size+j]),SIG_SHIFT-5));
      }
#else
      for (j=0;j<subvect_size;j++)
         exc[subvect_size*i+j]+=s*0.03125*shape_cb[ind[i]*subvect_size+j];      
#endif
   }
}

void noise_codebook_quant(
spx_sig_t target[],			/* target vector */
spx_coef_t ak[],			/* LPCs for this subframe */
spx_coef_t awk1[],			/* Weighted LPCs for this subframe */
spx_coef_t awk2[],			/* Weighted LPCs for this subframe */
const void *par,                      /* Codebook/search parameters*/
int   p,                        /* number of LPC coeffs */
int   nsf,                      /* number of samples in subframe */
spx_sig_t *exc,
spx_word16_t *r,
SpeexBits *bits,
char *stack,
int   complexity,
int   update_target
)
{
   int i;
   VARDECL(spx_sig_t *tmp);
   ALLOC(tmp, nsf, spx_sig_t);
   residue_percep_zero(target, ak, awk1, awk2, tmp, nsf, p, stack);

   for (i=0;i<nsf;i++)
      exc[i]+=tmp[i];
   for (i=0;i<nsf;i++)
      target[i]=0;

}


void noise_codebook_unquant(
spx_sig_t *exc,
const void *par,                      /* non-overlapping codebook */
int   nsf,                      /* number of samples in subframe */
SpeexBits *bits,
char *stack
)
{
   speex_rand_vec(1, exc, nsf);
}
