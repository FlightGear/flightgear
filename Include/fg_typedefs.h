/*
//  Alterations: Copyright C. Hotchkiss 1996
//
// $Log$
// Revision 1.1  1999/04/05 21:32:40  curt
// Initial revision
//
// Revision 1.2  1998/05/13 18:23:46  curt
// fg_typedefs.h: updated version by Charlie Hotchkiss
// general.h: moved fg_root info to fgOPTIONS structure.
//
// Revision 1.1  1998/05/11 18:26:12  curt
// Initial revision.
//
//   Rev 1.4   11 Nov 1997 15:34:28   CHOTCHKISS
// Expanded definitions.
//
//   Rev 1.3   20 Jan 1997  9:21:26   CHOTCHKISS
// Minor additions.
//
//   Rev 1.2   12 Nov 1996 15:06:52   CHOTCHKISS
// Dropped PC Write print format control lines.
//
//  Rev 1.1   20 Nov 1995 15:59:02   CHOTCHKISS
// Additions and improvements. Memcheck compatibilities.
//
//  Rev 1.0   06 Apr 1995 14:00:32   CHOTCHKISS
// Initial revision.

*/
/*
//    TYPEDEFS.H - General purpose definition file
//    Copyright (C) 1992 Paradigm Systems.  All rights reserved.
//
//    Function
//    ========
//    This file contains the general purpose definitions common to the
//    all Paradigm applications.  By defining synonyms for the physical
//    data types to be manipulated, portability between memory models
//    and machines is maximized.
//
//    Note that this file follows the system include files and before
//    any application include files.
*/

#if !defined(_TYPEDEFS)
#define _TYPEDEFS

//
//    Define the types to be used to manipulate 8-, 16-, and 32-bit
//    data.
//
typedef unsigned int   BIT ;     // Use for defining Borland bit fields
typedef char           CHAR ;    // 8-bit signed data
typedef const char     COCHAR;
typedef unsigned char  UCHAR ;   // 8-bit unsigned data
typedef unsigned char  BYTE;
typedef int            INT ;     // 16-bit signed data
typedef unsigned int   UINT ;    // 16-bit unsigned data
typedef const int      COINT;    // 16=bit constant int
typedef const UINT     COUINT;
typedef long           LONG ;    // 32-bit signed data
typedef unsigned long  ULONG ;   // 32-bit unsigned data

typedef unsigned short UWORD;   // Unsigned 16 bit quantity (WIN=SHORT)
#if !defined(WIN32)
typedef signed   short WORD;    // Signed   16 bit quantity
#endif
typedef BYTE           UBYTE;    // Used in some 3rd party code
#ifndef WIN32
typedef int            BOOLEAN;  //
#endif

typedef float          FLOAT ;   // 32-bit floating point data
typedef double         DOUBLE ;  // 64-bit floating point data
typedef long double    LDOUBLE ; // 80-bit floating point data

#ifndef __cplusplus
typedef int bool;
typedef int BOOL;
typedef int Bool;
#else
#ifndef WIN32
#define BOOL int
#endif
#endif

#define Bool int

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef true          // C++ defines bool, true and false.
#define true TRUE
#define false FALSE
#endif

#ifndef EOF
#define EOF (-1)
#endif

typedef void(*VFNPTR)   ( void );
typedef void(*VFNINTPTR)( int  );
typedef int (*FNPTR)    ( void );
typedef int (*FNINTPTR) ( int  );
typedef int (*FNUIPTR)  ( UINT );
typedef double( *DBLFNPTR)( void );

#endif

  /* !defined(_TYPEDEFS) */
