/*
 * RCS $Id$
 */


#ifndef _SUNPOS_HXX
#define _SUNPOS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <ctime>
#else
#  include <time.h>
#endif

/* update the cur_time_params structure with the current sun position */
void fgUpdateSunPos( void );

/* update the cur_time_params structure with the current moon position */
void fgUpdateMoonPos( void );

void fgSunPosition(time_t ssue, double *lon, double *lat);

/* given a particular time expressed in side real time at prime
 * meridian (GST), compute position on the earth (lat, lon) such that
 * sun is directly overhead.  (lat, lon are reported in radians */
void fgSunPositionGST(double gst, double *lon, double *lat);


#endif /* _SUNPOS_HXX */
