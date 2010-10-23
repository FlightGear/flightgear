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

