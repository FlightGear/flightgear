/*
 * RCS $Id$
 */


#ifndef _MOONPOS_HXX
#define _MOONPOS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <ctime>
#  ifdef macintosh
     SG_USING_STD(time_t);
#  endif
#else
#  include <time.h>
#endif

/* update the cur_time_params structure with the current moon position */
void fgUpdateMoonPos( void );

void fgMoonPosition(time_t ssue, double *lon, double *lat);


#endif /* _MOONPOS_HXX */
