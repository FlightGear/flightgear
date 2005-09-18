//////////////////////////////////////////////////////////////////////
//
//	Tiny XDR implementation for flightgear
//	written by Oliver Schroeder
//	released to the puiblic domain
//
//	This implementation is not complete, but implements
//	everything we need.
//
//	For further reading on XDR read RFC 1832.
//
//  NEW
//
//////////////////////////////////////////////////////////////////////

#ifndef TINY_XDR_HEADER
#define TINY_XDR_HEADER

#if defined HAVE_CONFIG_H
#   include <config.h>
#endif

#include <simgear/compiler.h>
#if defined HAVE_STDINT_H
#   include <stdint.h>
#endif

#include <plib/ul.h>

//////////////////////////////////////////////////////////////////////
//
//  There are many sick systems out there:
//
//  check for sizeof(float) and sizeof(double)
//  if sizeof(float) != 4 this code must be patched
//  if sizeof(double) != 8 this code must be patched
//
//  Those are comments I fetched out of glibc source:
//  - s390 is big-endian
//  - Sparc is big-endian, but v9 supports endian conversion
//    on loads/stores and GCC supports such a mode.  Be prepared.  
//  - The MIPS architecture has selectable endianness.
//  - x86_64 is little-endian.
//  - CRIS is little-endian.
//  - m68k is big-endian.
//  - Alpha is little-endian.
//  - PowerPC can be little or big endian.
//  - SH is bi-endian but with a big-endian FPU.
//  - hppa1.1 big-endian. 
//  - ARM is (usually) little-endian but with a big-endian FPU. 
//
//////////////////////////////////////////////////////////////////////
inline uint32_t bswap_32(unsigned int b) {
    unsigned x = b;
    ulEndianSwap(&x);
    return x;
}

inline uint64_t bswap_64(unsigned long long b) {
    uint64_t x = b;
    x = ((x >> 32) & 0x00000000FFFFFFFFLL) | ((x << 32) & 0xFFFFFFFF00000000LL);
    x = ((x >> 16) & 0x0000FFFF0000FFFFLL) | ((x << 16) & 0xFFFF0000FFFF0000LL);
    x = ((x >>  8) & 0x00FF00FF00FF00FFLL) | ((x <<  8) & 0xFF00FF00FF00FF00LL);
    return x;
}

#if BYTE_ORDER == BIG_ENDIAN
#   define SWAP32(arg) arg
#   define SWAP64(arg) arg
#   define LOW  0
#   define HIGH 1
#else
#   define SWAP32(arg) bswap_32(arg)
#   define SWAP64(arg) bswap_64(arg)
#   define LOW  1
#   define HIGH 0
#endif

#define XDR_BYTES_PER_UNIT  4

typedef uint32_t    xdr_data_t;      /* 4 Bytes */
typedef uint64_t    xdr_data2_t;     /* 8 Bytes */

/* XDR 8bit integers */
xdr_data_t      XDR_encode_int8     ( int8_t  n_Val );
xdr_data_t      XDR_encode_uint8    ( uint8_t n_Val );
int8_t          XDR_decode_int8     ( xdr_data_t n_Val );
uint8_t         XDR_decode_uint8    ( xdr_data_t n_Val );

/* XDR 16bit integers */
xdr_data_t      XDR_encode_int16    ( int16_t  n_Val );
xdr_data_t      XDR_encode_uint16   ( uint16_t n_Val );
int16_t         XDR_decode_int16    ( xdr_data_t n_Val );
uint16_t        XDR_decode_uint16   ( xdr_data_t n_Val );

/* XDR 32bit integers */
xdr_data_t      XDR_encode_int32    ( int32_t  n_Val );
xdr_data_t      XDR_encode_uint32   ( const uint32_t n_Val );
int32_t         XDR_decode_int32    ( xdr_data_t n_Val );
uint32_t        XDR_decode_uint32   ( const xdr_data_t n_Val );

/* XDR 64bit integers */
xdr_data2_t     XDR_encode_int64    ( int64_t n_Val );
xdr_data2_t     XDR_encode_uint64   ( uint64_t n_Val );
int64_t         XDR_decode_int64    ( xdr_data2_t n_Val );
uint64_t        XDR_decode_uint64   ( xdr_data2_t n_Val );

//////////////////////////////////////////////////
//
//  FIXME: #1 these funtions must be fixed for
//         none IEEE-encoding architecturs
//         (eg. vax, big suns etc)
//  FIXME: #2 some compilers return 'double'
//         regardless of return-type 'float'
//         this must be fixed, too
//  FIXME: #3 some machines may need to use a
//         different endianess for floats!
//
//////////////////////////////////////////////////
/* float */
xdr_data_t      XDR_encode_float    ( float f_Val );
float           XDR_decode_float    ( xdr_data_t f_Val );

/* double */
xdr_data2_t     XDR_encode_double   ( double d_Val );
double          XDR_decode_double   ( xdr_data2_t d_Val );

#endif
