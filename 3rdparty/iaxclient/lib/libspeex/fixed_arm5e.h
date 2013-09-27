/* Copyright (C) 2003 Jean-Marc Valin */
/**
   @file fixed_generic.h
   @brief ARM-tuned fixed-point operations
*/
/*
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

#ifndef FIXED_ARM5E_H
#define FIXED_ARM5E_H

#define NEG16(x) (-(x))
#define NEG32(x) (-(x))
#define EXTRACT16(x) ((spx_word16_t)x)
#define EXTEND32(x) ((spx_word32_t)x)
#define SHR16(a,shift) ((a) >> (shift))
#define SHL16(a,shift) ((a) << (shift))
#define SHR32(a,shift) ((a) >> (shift))
#define SHL32(a,shift) ((a) << (shift))
#define PSHR16(a,shift) (SHR16((a)+(1<<((shift)-1)),shift))
#define PSHR32(a,shift) (SHR32((a)+(1<<((shift)-1)),shift))
#define SATURATE16(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))
#define SATURATE32(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))

#define SHR(a,shift) ((a) >> (shift))
#define SHL(a,shift) ((a) << (shift))
#define SATURATE(x,a) ((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x))
#define PSHR(a,shift) (SHR((a)+(1<<((shift)-1)),shift))


#define ADD16(a,b) ((short)((short)(a)+(short)(b)))
#define SUB16(a,b) ((a)-(b))
#define ADD32(a,b) ((a)+(b))
#define SUB32(a,b) ((a)-(b))
#define ADD64(a,b) ((a)+(b))


/* result fits in 16 bits */
#define MULT16_16_16(a,b)     (((short)(a))*((short)(b)))

static inline spx_word32_t MULT16_16(spx_word16_t x, spx_word16_t y) {
  int res;
  asm ("smulbb  %0,%1,%2;\n"
              : "=&r"(res)
              : "%r"(x),"r"(y));
  return(res);
}

static inline spx_word32_t MAC16_16(spx_word32_t a, spx_word16_t x, spx_word32_t y) {
  int res;
  asm ("smlabb  %0,%1,%2,%3;\n"
              : "=&r"(res)
               : "%r"(x),"r"(y),"r"(a));
  return(res);
}

#define MULT16_32_Q12(a,b) ADD32(MULT16_16((a),SHR((b),12)), SHR(MULT16_16((a),((b)&0x00000fff)),12))
#define MULT16_32_Q13(a,b) ADD32(MULT16_16((a),SHR((b),13)), SHR(MULT16_16((a),((b)&0x00001fff)),13))
#define MULT16_32_Q14(a,b) ADD32(MULT16_16((a),SHR((b),14)), SHR(MULT16_16((a),((b)&0x00003fff)),14))

static inline spx_word32_t MULT16_32_Q15(spx_word16_t x, spx_word32_t y) {
  int res;
  asm ("smulwb  %0,%1,%2;\n"
              : "=&r"(res)
               : "%r"(y<<1),"r"(x));
  return(res);
}
static inline spx_word32_t MAC16_32_Q15(spx_word32_t a, spx_word16_t x, spx_word32_t y) {
  int res;
  asm ("smlawb  %0,%1,%2,%3;\n"
              : "=&r"(res)
               : "%r"(y<<1),"r"(x),"r"(a));
  return(res);
}
static inline spx_word32_t MULT16_32_Q11(spx_word16_t x, spx_word32_t y) {
  int res;
  asm ("smulwb  %0,%1,%2;\n"
              : "=&r"(res)
               : "%r"(y<<5),"r"(x));
  return(res);
}
static inline spx_word32_t MAC16_32_Q11(spx_word32_t a, spx_word16_t x, spx_word32_t y) {
  int res;
  asm ("smlawb  %0,%1,%2,%3;\n"
              : "=&r"(res)
               : "%r"(y<<5),"r"(x),"r"(a));
  return(res);
}

#define MAC16_16_Q11(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),11)))
#define MAC16_16_Q13(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),13)))

#define MULT16_16_Q11_32(a,b) (SHR(MULT16_16((a),(b)),11))
#define MULT16_16_Q13(a,b) (SHR(MULT16_16((a),(b)),13))
#define MULT16_16_Q14(a,b) (SHR(MULT16_16((a),(b)),14))
#define MULT16_16_Q15(a,b) (SHR(MULT16_16((a),(b)),15))

#define MULT16_16_P13(a,b) (SHR(ADD32(4096,MULT16_16((a),(b))),13))
#define MULT16_16_P14(a,b) (SHR(ADD32(8192,MULT16_16((a),(b))),14))
#define MULT16_16_P15(a,b) (SHR(ADD32(16384,MULT16_16((a),(b))),15))

#define MUL_16_32_R15(a,bh,bl) ADD32(MULT16_16((a),(bh)), SHR(MULT16_16((a),(bl)),15))


/*
#define DIV32_16(a,b) ((short)(((signed int)(a))/((short)(b))))
*/
static inline short DIV3216(int a, int b)
{
   int res=0;
   int dead1, dead2, dead3, dead4, dead5;
   __asm__ __volatile__ (
         "\teor %5, %0, %1\n"
         "\tmovs %4, %0\n"
         "\trsbmi %0, %0, #0 \n"
         "\tmovs %4, %1\n"
         "\trsbmi %1, %1, #0 \n"
         "\tmov %4, #1\n"

         "\tsubs %3, %0, %1, asl #14 \n"
         "\torrpl %2, %2, %4, asl #14 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #13 \n"
         "\torrpl %2, %2, %4, asl #13 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #12 \n"
         "\torrpl %2, %2, %4, asl #12 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #11 \n"
         "\torrpl %2, %2, %4, asl #11 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #10 \n"
         "\torrpl %2, %2, %4, asl #10 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #9 \n"
         "\torrpl %2, %2, %4, asl #9 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #8 \n"
         "\torrpl %2, %2, %4, asl #8 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #7 \n"
         "\torrpl %2, %2, %4, asl #7 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #6 \n"
         "\torrpl %2, %2, %4, asl #6 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #5 \n"
         "\torrpl %2, %2, %4, asl #5 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #4 \n"
         "\torrpl %2, %2, %4, asl #4 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #3 \n"
         "\torrpl %2, %2, %4, asl #3 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #2 \n"
         "\torrpl %2, %2, %4, asl #2 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1, asl #1 \n"
         "\torrpl %2, %2, %4, asl #1 \n"
         "\tmovpl %0, %3 \n"

         "\tsubs %3, %0, %1 \n"
         "\torrpl %2, %2, %4 \n"
         "\tmovpl %0, %3 \n"
         
         "\tmovs %5, %5, lsr #31 \n"
         "\trsbne %2, %2, #0 \n"
   : "=r" (dead1), "=r" (dead2), "=r" (res),
   "=r" (dead3), "=r" (dead4), "=r" (dead5)
   : "0" (a), "1" (b), "2" (res)
   : "memory", "cc"
                        );
   return res;
}


#define DIV32(a,b) (((signed int)(a))/((signed int)(b)))



#endif
