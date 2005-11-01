//////////////////////////////////////////////////////////////////////
//
//	Tiny XDR implementation for flightgear
//	written by Oliver Schroeder
//	released to the public domain
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

#include <simgear/misc/stdint.hxx>

#define SWAP32(arg) sgIsLittleEndian() ? sg_bswap_32(arg) : arg
#define SWAP64(arg) sgIsLittleEndian() ? sg_bswap_64(arg) : arg

#define XDR_BYTES_PER_UNIT  4

typedef uint32_t    xdr_data_t;      /* 4 Bytes */
typedef uint64_t    xdr_data2_t;     /* 8 Bytes */

/* XDR 8bit integers */
xdr_data_t      XDR_encode_int8     ( const int8_t &  n_Val );
xdr_data_t      XDR_encode_uint8    ( const uint8_t & n_Val );
int8_t          XDR_decode_int8     ( const xdr_data_t & n_Val );
uint8_t         XDR_decode_uint8    ( const xdr_data_t & n_Val );

/* XDR 16bit integers */
xdr_data_t      XDR_encode_int16    ( const int16_t & n_Val );
xdr_data_t      XDR_encode_uint16   ( const uint16_t & n_Val );
int16_t         XDR_decode_int16    ( const xdr_data_t & n_Val );
uint16_t        XDR_decode_uint16   ( const xdr_data_t & n_Val );

/* XDR 32bit integers */
xdr_data_t      XDR_encode_int32    ( const int32_t & n_Val );
xdr_data_t      XDR_encode_uint32   ( const uint32_t & n_Val );
int32_t         XDR_decode_int32    ( const xdr_data_t & n_Val );
uint32_t        XDR_decode_uint32   ( const xdr_data_t & n_Val );

/* XDR 64bit integers */
xdr_data2_t     XDR_encode_int64    ( const int64_t & n_Val );
xdr_data2_t     XDR_encode_uint64   ( const uint64_t & n_Val );
int64_t         XDR_decode_int64    ( const xdr_data2_t & n_Val );
uint64_t        XDR_decode_uint64   ( const xdr_data2_t & n_Val );

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
xdr_data_t      XDR_encode_float    ( const float & f_Val );
float           XDR_decode_float    ( const xdr_data_t & f_Val );

/* double */
xdr_data2_t     XDR_encode_double   ( const double & d_Val );
double          XDR_decode_double   ( const xdr_data2_t & d_Val );

#endif
