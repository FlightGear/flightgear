/*
//  Alterations: Copyright C. Hotchkiss 1996
//
// $Log$
// Revision 1.2  2010/01/23 22:26:52  fredb
// MINGW patch from Benoît Laniel
//
// Revision 1.1.1.1  2002-09-10 01:14:04  curt
// Initial revision of FlightGear-0.9.0
//
// Revision 1.2  2001/05/16 21:27:59  curt
// Added David Megginson's patch for reconfigurable keyboard bindings.
//
// Revision 1.1.1.1  1999/06/17 18:07:30  curt
// Start of 0.7.x branch
//
// Revision 1.2  1999/04/22 18:45:42  curt
// Borland tweaks.
//
// Revision 1.1.1.1  1999/04/05 21:32:40  curt
// Start of 0.6.x branch.
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

#ifndef _FG_TYPEDEFS
#define _FG_TYPEDEFS

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
#ifndef _WIN32
typedef signed   short WORD;    // Signed   16 bit quantity
#endif
typedef BYTE           UBYTE;    // Used in some 3rd party code
#ifndef _WIN32
typedef int            BOOLEAN;  //
#endif

typedef float          FLOAT ;   // 32-bit floating point data
typedef double         DOUBLE ;  // 64-bit floating point data
typedef long double    LDOUBLE ; // 80-bit floating point data

typedef void(*VFNPTR)   ( void );
typedef void(*VFNINTPTR)( int  );
typedef int (*FNPTR)    ( void );
typedef int (*FNINTPTR) ( int  );
typedef int (*FNUIPTR)  ( UINT );
typedef double( *DBLFNPTR)( void );
typedef float( *FLTFNPTR)( void );

#endif // _FG_TYPEDEFS
