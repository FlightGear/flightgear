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
//////////////////////////////////////////////////////////////////////

#include <string>

#include "tiny_xdr.hxx"

/* XDR 8bit integers */
xdr_data_t
XDR_encode_int8 ( const int8_t & n_Val )
{
    return (SWAP32(static_cast<xdr_data_t> (n_Val)));
}

xdr_data_t
XDR_encode_uint8 ( const uint8_t & n_Val )
{
    return (SWAP32(static_cast<xdr_data_t> (n_Val)));
}

int8_t
XDR_decode_int8 ( const xdr_data_t & n_Val )
{
    return (static_cast<int8_t> (SWAP32(n_Val)));
}

uint8_t
XDR_decode_uint8 ( const xdr_data_t & n_Val )
{
    return (static_cast<uint8_t> (SWAP32(n_Val)));
}

/* XDR 16bit integers */
xdr_data_t
XDR_encode_int16 ( const int16_t & n_Val )
{
    return (SWAP32(static_cast<xdr_data_t> (n_Val)));
}

xdr_data_t
XDR_encode_uint16 ( const uint16_t & n_Val )
{
    return (SWAP32(static_cast<xdr_data_t> (n_Val)));
}

int16_t
XDR_decode_int16 ( const xdr_data_t & n_Val )
{
    return (static_cast<int16_t> (SWAP32(n_Val)));
}

uint16_t
XDR_decode_uint16 ( const xdr_data_t & n_Val )
{
    return (static_cast<uint16_t> (SWAP32(n_Val)));
}


/* XDR 32bit integers */
xdr_data_t
XDR_encode_int32 ( const int32_t & n_Val )
{
    return (SWAP32(static_cast<xdr_data_t> (n_Val)));
}

/*
 * Safely convert from an int into a short. Anything outside the bounds of a short will
 * simply be the max/min value of short (+/- 32767)
 */
static short XDR_convert_int_to_short(int v1)
{
    if (v1 < -32767)
        v1 = -32767;

    if (v1 > 32767)
        v1 = 32767;

    return (short)v1;
}

/*
 * Pack two 16bit shorts into a 32 bit int. By convention v1 is packed in the highword
 */
xdr_data_t XDR_encode_shortints32(const int v1, const int v2)
{
    return XDR_encode_uint32(((XDR_convert_int_to_short(v1) << 16) & 0xffff0000) | ((XDR_convert_int_to_short(v2)) & 0xffff));
}
/* Decode packed shorts into two ints. V1 in the highword ($V1..V2..)*/
void            XDR_decode_shortints32(const xdr_data_t & n_Val, int &v1, int &v2)
{
    int _v1 = XDR_decode_int32(n_Val);
    short s2 = (short)(_v1 & 0xffff);
    short s1 = (short)(_v1 >> 16);
    v1 = s1;
    v2 = s2;
}

xdr_data_t
XDR_encode_uint32 ( const uint32_t & n_Val )
{
    return (SWAP32(static_cast<xdr_data_t> (n_Val)));
}

int32_t
XDR_decode_int32 ( const xdr_data_t & n_Val )
{
    return (static_cast<int32_t> (SWAP32(n_Val)));
}

uint32_t
XDR_decode_uint32 ( const xdr_data_t & n_Val )
{
    return (static_cast<uint32_t> (SWAP32(n_Val)));
}


/* XDR 64bit integers */
xdr_data2_t
XDR_encode_int64 ( const int64_t & n_Val )
{
    return (SWAP64(static_cast<xdr_data2_t> (n_Val)));
}

xdr_data2_t
XDR_encode_uint64 ( const uint64_t & n_Val )
{
    return (SWAP64(static_cast<xdr_data2_t> (n_Val)));
}

int64_t
XDR_decode_int64 ( const xdr_data2_t & n_Val )
{
    return (static_cast<int64_t> (SWAP64(n_Val)));
}

uint64_t
XDR_decode_uint64 ( const xdr_data2_t & n_Val )
{
    return (static_cast<uint64_t> (SWAP64(n_Val)));
}


/* float */
xdr_data_t
XDR_encode_float ( const float & f_Val )
{
    union {
        xdr_data_t x;
        float f;
    } tmp;

    tmp.f = f_Val;
    return (XDR_encode_int32 (tmp.x));
}

float
XDR_decode_float ( const xdr_data_t & f_Val )
{
    union {
        xdr_data_t x;
        float f;
    } tmp;

    tmp.x = XDR_decode_int32 (f_Val);
    return tmp.f;
}

/* double */
xdr_data2_t
XDR_encode_double ( const double & d_Val )
{
    union {
        xdr_data2_t x;
        double d;
    } tmp;

    tmp.d = d_Val;
    return (XDR_encode_int64 (tmp.x));
}

double
XDR_decode_double ( const xdr_data2_t & d_Val )
{
    union {
        xdr_data2_t x;
        double d;
    } tmp;

    tmp.x = XDR_decode_int64 (d_Val);
    return tmp.d;
}

