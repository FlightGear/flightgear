/* Sox-compatibility functions:  stuff to make sox dsp routines compile outside 	of sox itself */
#include "sox.h"
#include <stdio.h>

#if defined(__STDC__) || defined(_MSC_VER)
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#endif

static int verbose = 0;
static char *myname="iaxclient-sox";

void st_report(const char *fmt, ...)
{
        va_list args;

        if (! verbose)
                return;

        fprintf(stderr, "%s: ", myname);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
}

void st_warn(const char *fmt, ...)
{
        va_list args;

        fprintf(stderr, "%s: ", myname);
        va_start(args, fmt);

        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
}

void st_fail(const char *fmt, ...)
{
        va_list args;
        extern void cleanup();

        fprintf(stderr, "%s: ", myname);

        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(2);
}

long st_gcd(long a, long b)
{
        if (b == 0)
                return a;
        else
                return st_gcd(b, a % b);
}


