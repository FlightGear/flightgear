//
// CMDLINE Argument External Definitions. A hack to make command line
// argument values visible to affected program locations.
//
//  When implementing this feature my design intent was that program
//  options should be set according to the following rules of
//  option hierarchy.
//
//   1. Command line options have top priority.
//   2. Environmental options over ride default options.
//   3. All options must have a meaningful state. On a given platform,
//      some option setting is most likely to be desired by that community.
//
// CHotchkiss 10 Feb 98
//
// $Id$


#ifndef _CMDARGS_H
#define _CMDARGS_H

// buffers here are all MAXPATH in length. IS THIS DEFINE UNIVERSAL?

extern char  acArgbuf[];
extern int   debugArgValue;
extern int   priorityArgValue;
extern char  rootArgbuf[];
extern int   viewArg;
extern char  logArgbuf[];

// These are used by initialization and RE initialization routines
// (none right now) to set up for running (or from warm reset.)

extern const char *DefaultRootDir;
extern const char *DefaultAircraft;
extern const char *DefaultDebuglog;
extern const int   DefaultViewMode;

#endif
// end of cmdargs.h


