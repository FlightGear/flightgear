/****************************************************************************
*
*               Copyright (C) 1991-1997 SciTech Software, Inc.
*                            All rights reserved.
*
*  ======================================================================
*       This library is free software; you can use it and/or
*       modify it under the terms of the SciTech MGL Software License.
*
*       This library is distributed in the hope that it will be useful,
*       but WITHOUT ANY WARRANTY; without even the implied warranty of
*       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*       SciTech MGL Software License for more details.
*  ======================================================================
*
* Filename:     $Workfile:   getopt.h  $
* Version:      $Revision$
*
* Language:     ANSI C
* Environment:  any
*
* Description:  Header file for command line parsing module. This module
*               contains code to parse the command line, extracting options
*               and parameters in standard System V style.
*
* $Date$ $Author$
*
* $Id$
* (Log is kept at end of this file)
*
****************************************************************************/


#ifndef __GETOPT_H
#define __GETOPT_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


//#ifndef __DEBUG_H
//#include "debug.h"
//#endif

/*---------------------------- Typedef's etc -----------------------------*/

#define ALLDONE     -1
#define PARAMETER   -2
#define INVALID     -3
#define HELP        -4

#define MAXARG      80

/* Option type sepecifiers */

#define OPT_INTEGER     'd'
#define OPT_HEX         'h'
#define OPT_OCTAL       'o'
#define OPT_UNSIGNED    'u'
#define OPT_LINTEGER    'D'
#define OPT_LHEX        'H'
#define OPT_LOCTAL      'O'
#define OPT_LUNSIGNED   'U'
#define OPT_FLOAT       'f'
#define OPT_DOUBLE      'F'
#define OPT_LDOUBLE     'L'
#define OPT_STRING      's'
#define OPT_SWITCH      '!'

// I need to generate a typedefs file for this.
//
#ifndef uchar
typedef unsigned char uchar;
#endif
#ifndef uint
typedef unsigned int uint;
#endif

#ifndef ulong
typedef unsigned long ulong;
#endif

#ifndef bool
#ifndef BOOL
typedef int BOOL;
#endif
typedef BOOL bool;
#endif

#ifndef true
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define true TRUE
#define false FALSE
#endif

typedef struct {
    uchar   opt;                /* The letter to describe the option    */
    uchar   type;               /* Type descriptor for the option       */
    void    *arg;               /* Place to store the argument          */
    char    *desc;              /* Description for this option          */
    } Option;

#define NUM_OPT(a)  sizeof(a) / sizeof(Option)


/*--------------------------- Global variables ---------------------------*/

extern  int     nextargv;
extern  char    *nextchar;

/*------------------------- Function Prototypes --------------------------*/

// extern int getopt(int argc,char **argv,char *format,char **argument);

extern int getargs(int argc, char *argv[],int num_opt, Option ** optarr,
		   int (*do_param)(char *param,int num));

extern void print_desc(int num_opt, Option **optarr);  // Not original code

#ifdef __cplusplus
}
#endif

#endif


/* $Log$
/* Revision 1.2  1998/04/21 17:02:41  curt
/* Prepairing for C++ integration.
/*
 * Revision 1.1  1998/02/13 00:23:39  curt
 * Initial revision.
 *
 */
