#include "sph_errtypes.h"

#ifdef THINK_C
/* We hide this from gnu's compiler, which doesn't understand it. */
void SPH__error (int errtype, ...);
#endif


#define ERR_ERROR(A,B,C)   \
   if (1) {char cstr[256]; sprintf C; SPH__error(ERR_MAT3_PACKAGE, cstr); } else


#define ERR_S  cstr,"%s\n"
#define ERR_SI cstr,"%s: %d\n"
#define ERR_SS cstr,"%s: %s\n"

#define ERR_SEVERE 0
#define ERR_FATAL  0

#define ERR_ALLOC1 0

typedef int ERRid;

#define ERRregister_package(S)    100


