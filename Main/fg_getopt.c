/****************************************************************************
* Modified from the original version found in SciTech's library. Note that
* this in fact may have been copied/derived from a much earlier work as I
* remember seeing a version of it 10 years ago for use in CP/M and it wasn't
* new then. I peeled a routine out of the end of this as it was an alternative
* for Windows code. CH
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
* Filename:     $Workfile:   getopt.c  $
* Version:      $Revision$
*
* Language:     ANSI C
* Environment:  any
*
* Description:  This module contains code to parse the command line,
*               extracting options and parameters in standard System V
*               style.
*
* $Date$ $Author$
*
* $Id$
* (Log is kept at end of this file)
*
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "fg_getopt.h"

/*------------------------- Global variables ------------------------------*/

int     nextargv    =   1;          /* Index into argv array            */
char    *nextchar   =   NULL;       /* Pointer to next character        */

/*-------------------------- Implementation -------------------------------*/

#ifdef  __WIN32__
#define IS_SWITCH_CHAR(c)       (c == '-') || (c == '/')
#define IS_NOT_SWITCH_CHAR(c)   (c != '-') && (c != '/')
#else
#define IS_SWITCH_CHAR(c)       (c == '-')
#define IS_NOT_SWITCH_CHAR(c)   (c != '-')
#endif

int getopt(int argc,char **argv,char *format,char **argument)
/****************************************************************************
*
* Function:     getopt
* Parameters:   argc        -   Value passed to program through argc
*                               variable in the function main.
*               argv        -   Pointer to the argv array that is passed to
*                               the program in function main.
*               format      -   A string representing the expected format
*                               of the command line options that need to be
*                               parsed.
*               argument    -   Pointer to optional argument on command
*                               line.
*
* Returns:      Character code representing the next option parsed from the
*               command line by getopt. Returns ALLDONE (-1) when there are
*               no more parameters to be parsed on the command line,
*               PARAMETER (-2) when the argument being parsed is a
*               parameter and not an option switch and lastly INVALID (-3)
*               if an error occured while parsing the command line.
*
* Description:  Function to parse the command line option switches in
*               UNIX System V style. When getopt is called, it returns the
*               character code of the next valid option that is parsed from
*               the command line as specified by the Format string. The
*               format string should be in the following form:
*
*                       "abcd:e:f:"
*
*               where a,b and c represent single switch style options and
*               the character code returned by getopt is the only value
*               returned. Also d, e and f represent options that expect
*               arguments immediately after them on the command line. The
*               argument that follows the option on the command line is
*               returned via a reference in the pointer argument. Thus
*               a valid command line for this format string might be:
*
*                   myprogram -adlines /b /f format infile outfile
*
*               where a and b will be returned as single character options
*               with no argument, while d is returned with the argument
*               lines and f is returned with the argument format. Note that
*               either UNIX style or MS-DOS command switches may be used
*               interchangeably under MSDOS, but under UNIX only the UNIX
*               style switches are supported.
*
*               When getopt returns with PARAMETER (we attempted to parse
*               a paramter, not an option), the global variable NextArgv
*               will hold an index in the argv array to the argument on the
*               command line AFTER the options, ie in the above example the
*               string 'infile'. If the parameter is successfully used,
*               NextArgv should be incremented and getopt can be called
*               again to parse any more options. Thus you can also have
*               options interspersed throught the command line. eg:
*
*                   myprogram -adlines infile /b outfile /f format
*
*               can be made to be a valid form of the above command line.
*
****************************************************************************/
{
    char    ch;
    char    *formatchar;

    if (argc > nextargv) {
        if (nextchar == NULL) {
            nextchar = argv[nextargv];      /* Index next argument      */
            if(nextchar == NULL) {
                nextargv++;
                return ALLDONE;             /* No more options          */
                }
            if(IS_NOT_SWITCH_CHAR(*nextchar)) {
                nextchar = NULL;
                return PARAMETER;           /* We have a parameter      */
                }
            nextchar++;                 /* Move past switch operator    */
            if(IS_SWITCH_CHAR(*nextchar)) {
                nextchar = NULL;
                return INVALID;             /* Ignore rest of line      */
                }
            }

        if ((ch = *(nextchar++)) == 0) {
            nextchar = NULL;
            return INVALID;                 /* No options on line       */
            }

        if (ch == ':' ||  (formatchar = strchr(format, ch)) == NULL)
            return INVALID;

        if (*(++formatchar) == ':') {   /* Expect an argument after option */
            nextargv++;
            if (*nextchar == 0) {
                if (argc <= nextargv)
                    return INVALID;
                nextchar = argv[nextargv++];
                }
            *argument = nextchar;
            nextchar = NULL;
            }
        else {                      /* We have a switch style option    */
            if (*nextchar == 0) {
                nextargv++;
                nextchar = NULL;
            }
            *argument = NULL;
            }
        return ch;                  /* return the option specifier      */
        }
    nextchar = NULL;
    nextargv++;
    return ALLDONE;                 /* no arguments on command line     */
}

static int parse_option(Option *optarr, char *argument)
/****************************************************************************
*
* Function:     parse_option
* Parameters:   optarr      - Description for the option we are parsing
*               argument    - String to parse
* Returns:      INVALID on error, ALLDONE on success.
*
* Description:  Parses the argument string depending on the type of argument
*               that is expected, filling in the argument for that option.
*               Note that to parse a string, we simply return a pointer
*               to argument.
*
****************************************************************************/
{
    int     num_read = 0;

    switch ((int)(optarr->type)) {
        case OPT_INTEGER:
            num_read = sscanf(argument,"%d",(int*)optarr->arg);
            break;
        case OPT_HEX:
            num_read = sscanf(argument,"%x",(int*)optarr->arg);
            break;
        case OPT_OCTAL:
            num_read = sscanf(argument,"%o",(int*)optarr->arg);
            break;
        case OPT_UNSIGNED:
            num_read = sscanf(argument,"%u",(uint*)optarr->arg);
            break;
        case OPT_LINTEGER:
            num_read = sscanf(argument,"%ld",(long*)optarr->arg);
            break;
        case OPT_LHEX:
            num_read = sscanf(argument,"%lx",(long*)optarr->arg);
            break;
        case OPT_LOCTAL:
            num_read = sscanf(argument,"%lo",(long*)optarr->arg);
            break;
        case OPT_LUNSIGNED:
            num_read = sscanf(argument,"%lu",(ulong*)optarr->arg);
            break;
        case OPT_FLOAT:
            num_read = sscanf(argument,"%f",(float*)optarr->arg);
            break;
        case OPT_DOUBLE:
            num_read = sscanf(argument,"%lf",(double*)optarr->arg);
            break;
        case OPT_LDOUBLE:
            num_read = sscanf(argument,"%Lf",(long double*)optarr->arg);
            break;
        case OPT_STRING:
            num_read = 1;           /* This always works    */
            *((char**)optarr->arg) = argument;
            break;
        default:
            return INVALID;
        }

    if (num_read == 0)
        return INVALID;
    else
        return ALLDONE;
}

int getargs( int argc,char
             *argv[],
             int num_opt,
             Option **optarr,
             int (*do_param)(char *param,int num))

/****************************************************************************
*
* Function:     getargs
* Parameters:   argc        - Number of arguments on command line
*               argv        - Array of command line arguments
*               num_opt     - Number of options in option array
*               optarr      - Array to specify how to parse the command line
*               do_param    - Routine to handle a command line parameter
* Returns:      ALLDONE, INVALID or HELP
*
* Description:  Function to parse the command line according to a table of
*               options. This routine calls getopt above to parse each
*               individual option and attempts to parse each option into
*               a variable of the specified type. The routine can parse
*               integers and long integers in either decimal, octal,
*               hexadecimal notation, unsigned integers and unsigned longs,
*               strings and option switches. Option switches are simply
*               boolean variables that get turned on if the switch was
*               parsed.
*
*               Parameters are extracted from the command line by calling
*               a user supplied routine do_param() to handle each parameter
*               as it is encountered. The routine do_param() should accept
*               a pointer to the parameter on the command line and an
*               integer representing how many parameters have been
*               encountered (ie: 1 if this is the first parameter, 10 if
*               it is the 10th etc), and return ALLDONE upon successfully
*               parsing it or INVALID if the parameter was invalid.
*
*               We return either ALLDONE if all the options were
*               successfully parsed, INVALID if an invalid option was
*               encountered or HELP if any of -h, -H or -? were present
*               on the command line.
*
****************************************************************************/
{
    int     i,opt;
    char    *argument;
    int     param_num = 1;
    char    cmdstr[MAXARG*2 + 4];

    /* Build the command string from the array of options   */

    strcpy(cmdstr,"hH?");
    for (i = 0,opt = 3; i < num_opt; i++,opt++) {
        cmdstr[opt] = optarr[i]->opt;
        if (optarr[i]->type != OPT_SWITCH) {
            cmdstr[++opt] = ':';
            }
        }
    cmdstr[opt] = '\0';

    while (true) {
        opt = getopt(argc,argv,cmdstr,&argument);
        switch (opt) {
            case 'H':
            case 'h':
            case '?':
                return HELP;
            case ALLDONE:
                return ALLDONE;
            case INVALID:
                return INVALID;
            case PARAMETER:
                if (do_param == NULL)
                    return INVALID;
                if (do_param(argv[nextargv],param_num) == INVALID)
                    return INVALID;
                nextargv++;
                param_num++;
                break;
            default:

                /* Search for the option in the option array. We are
                 * guaranteed to find it.
                 */

                for (i = 0; i < num_opt; i++) {
                    if (optarr[i]->opt == opt)
                        break;
                    }
                if (optarr[i]->type == OPT_SWITCH)
                    *((bool*)optarr[i]->arg) = true;
                else {
                    if (parse_option( optarr[i],argument) == INVALID)
                        return INVALID;
                    }
                break;
            }
        }
}

void print_desc(int num_opt,Option **optarr)
/****************************************************************************
*
* Function:     print_desc
* Parameters:   num_opt - Number of options in the table
*               optarr  - Table of option descriptions
*
* Description:  Prints the description of each option in a standard format
*               to the standard output device. The description for each
*               option is obtained from the table of options.
*
****************************************************************************/
{
    int     i;

    for (i = 0; i < num_opt; i++) {
        if (optarr[i]->type == OPT_SWITCH)
            printf("  -%c       %s\n",optarr[i]->opt,optarr[i]->desc);
        else
            printf("  -%c<arg>  %s\n",optarr[i]->opt,optarr[i]->desc);
        }
}

// End of getopt.c


/* $Log$
/* Revision 1.1  1998/02/13 00:23:40  curt
/* Initial revision.
/*
 */                   
