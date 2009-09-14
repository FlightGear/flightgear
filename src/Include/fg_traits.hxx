#ifndef _FG_TRAITS_HXX
#define _FG_TRAITS_HXX

#include "Include/compiler.h"

#ifndef FG_HAVE_TRAITS

// Dummy up some char traits for now.
template<class charT> struct char_traits{};

FG_TEMPLATE_NULL
struct char_traits<char>
{
    typedef char      char_type;
    typedef int       int_type;
    typedef streampos pos_type;
    typedef streamoff off_type;

    static int_type eof() { return EOF; }
};
#endif // FG_HAVE_TRAITS

#endif // _FG_TRAITS_HXX
