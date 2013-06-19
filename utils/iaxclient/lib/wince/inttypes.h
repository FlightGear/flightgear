/* A basic inttypes.h, for platforms which lack their own. Take care that it
   suits the integer lengths of the tools with which it is used. */

#ifndef _INTTYPES_H
#define _INTTYPES_H

/*
 *    ISO C99: 7.18 Integer types <stdint.h/inttypes.h>
 */
#ifndef __int8_t_defined
#define __int8_t_defined
typedef signed char                                 int8_t;
typedef short int                                   int16_t;
typedef long int                                    int32_t;
#if defined(__GNUC__)  
__extension__ typedef long long int                 int64_t;
#endif

typedef unsigned char                               uint8_t;
typedef unsigned short int                          uint16_t;
typedef unsigned long int                           uint32_t;
#if defined(__GNUC__)  
__extension__ typedef unsigned long long int        uint64_t;
#endif
#endif

/* Small types.  */

/* Signed.  */
typedef signed char                                 int_least8_t;
typedef short int                                   int_least16_t;
typedef long int                                    int_least32_t;
#if defined(__GNUC__)  
__extension__ typedef long long int                 int_least64_t;
#endif

/* Unsigned.  */
typedef unsigned char                               uint_least8_t;
typedef unsigned short int                          uint_least16_t;
typedef unsigned long int                           uint_least32_t;
#if defined(__GNUC__)  
__extension__ typedef unsigned long long int        uint_least64_t;
#endif


/* Fast types.  */

/* Signed.  */
typedef signed char                                 int_fast8_t;
typedef short int                                   int_fast16_t;
typedef long int                                    int_fast32_t;
#if defined(__GNUC__)  
__extension__ typedef long long int                 int_fast64_t;
#endif

/* Unsigned.  */
typedef unsigned char                               uint_fast8_t;
typedef unsigned int                                uint_fast16_t;
typedef unsigned long int                           uint_fast32_t;
#if defined(__GNUC__)  
__extension__ typedef unsigned long long int        uint_fast64_t;
#endif


/* Types for `void *' pointers.  */
#ifndef __intptr_t_defined
#define __intptr_t_defined
typedef int                                         intptr_t;
typedef unsigned int                                uintptr_t;
#endif


/* Largest integral types.  */
#if defined(__GNUC__)  
__extension__ typedef long long int                 intmax_t;
__extension__ typedef unsigned long long int        uintmax_t;
#endif


/* The ISO C99 standard specifies that in C++ implementations these
   macros should only be defined if explicitly requested.  */
#if !defined __cplusplus || defined __STDC_LIMIT_MACROS

#define __INT64_C(c)        c ## LL
#define __UINT64_C(c)       c ## ULL

/* Limits of integral types.  */

/* Minimum of signed integral types.  */
#define INT8_MIN            (-128)
#define INT16_MIN           (-32767-1)
#define INT32_MIN           (-2147483647-1)
#define INT64_MIN           (-__INT64_C(9223372036854775807)-1)
/* Maximum of signed integral types.  */
#define INT8_MAX            (127)
#define INT16_MAX           (32767)
#define INT32_MAX           (2147483647)
#define INT64_MAX           (__INT64_C(9223372036854775807))

/* Maximum of unsigned integral types.  */
#define UINT8_MAX           (255)
#define UINT16_MAX          (65535)
#define UINT32_MAX          (4294967295U)
#define UINT64_MAX          (__UINT64_C(18446744073709551615))


/* Minimum of signed integral types having a minimum size.  */
#define INT_LEAST8_MIN      (-128)
#define INT_LEAST16_MIN     (-32767-1)
#define INT_LEAST32_MIN     (-2147483647-1)
#define INT_LEAST64_MIN     (-__INT64_C(9223372036854775807)-1)
/* Maximum of signed integral types having a minimum size.  */
#define INT_LEAST8_MAX      (127)
#define INT_LEAST16_MAX     (32767)
#define INT_LEAST32_MAX     (2147483647)
#define INT_LEAST64_MAX     (__INT64_C(9223372036854775807))

/* Maximum of unsigned integral types having a minimum size.  */
#define UINT_LEAST8_MAX     (255)
#define UINT_LEAST16_MAX    (65535)
#define UINT_LEAST32_MAX    (4294967295U)
#define UINT_LEAST64_MAX    (__UINT64_C(18446744073709551615))


/* Minimum of fast signed integral types having a minimum size.  */
#define INT_FAST8_MIN       (-128)
#define INT_FAST16_MIN      (-32768)
#define INT_FAST32_MIN      (-2147483647-1)
#define INT_FAST64_MIN      (-__INT64_C(9223372036854775807)-1)
/* Maximum of fast signed integral types having a minimum size.  */
#define INT_FAST8_MAX       (127)
#define INT_FAST16_MAX      (32767)
#define INT_FAST32_MAX      (2147483647)
#define INT_FAST64_MAX      (__INT64_C(9223372036854775807))

/* Maximum of fast unsigned integral types having a minimum size.  */
#define UINT_FAST8_MAX      (255U)
#define UINT_FAST16_MAX     (65535U)
#define UINT_FAST32_MAX     (4294967295UL)
#define UINT_FAST64_MAX     (__UINT64_C(18446744073709551615))


/* Values to test for integral types holding `void *' pointer.  */
#define INTPTR_MIN          (-32768)
#define INTPTR_MAX          (32767)
#define UINTPTR_MAX         (65535U)


/* Minimum for largest signed integral type.  */
#define INTMAX_MIN          (-__INT64_C(9223372036854775807)-1)
/* Maximum for largest signed integral type.  */
#define INTMAX_MAX          (__INT64_C(9223372036854775807))

/* Maximum for largest unsigned integral type.  */
#define UINTMAX_MAX         (__UINT64_C(18446744073709551615))


/* Limits of other integer types.  */

/* Limits of `ptrdiff_t' type.  */
#define PTRDIFF_MIN         (-32768)
#define PTRDIFF_MAX         (32767)

/* Limits of `sig_atomic_t'.  */
#define SIG_ATOMIC_MIN      (-32768)
#define SIG_ATOMIC_MAX      (32767)

/* Limit of `size_t' type.  */
#ifndef SIZE_MAX
#define SIZE_MAX            (65535U)
#endif

/* Limits of `wchar_t'.  */
#ifndef WCHAR_MIN
/* These constants might also be defined in <wchar.h>.  */
#define WCHAR_MIN           __WCHAR_MIN
#define WCHAR_MAX           __WCHAR_MAX
#endif

/* Limits of `wint_t'.  */
#define WINT_MIN            (0U)
#define WINT_MAX            (65535U)

#endif    /* C++ && limit macros */


/* The ISO C99 standard specifies that in C++ implementations these
   should only be defined if explicitly requested.  */
#if !defined __cplusplus || defined __STDC_CONSTANT_MACROS

/* Signed.  */
#define INT8_C(c)           c
#define INT16_C(c)          c
#define INT32_C(c)          c
#define INT64_C(c)          c ## LL

/* Unsigned.  */
#define UINT8_C(c)          c ## U
#define UINT16_C(c)         c ## U
#define UINT32_C(c)         c ## U
#define UINT64_C(c)         c ## ULL

/* Maximal type.  */
#define INTMAX_C(c)         c ## LL
#define UINTMAX_C(c)        c ## ULL

#endif    /* C++ && constant macros */
#endif
