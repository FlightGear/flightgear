/* a quick default_model_routines.h */


#ifndef _DEFAULT_MODEL_ROUTINES_H
#define _DEFAULT_MODEL_ROUTINES_H


#include "ls_types.h"


void inertias( SCALAR dt, int Initialize );
void subsystems( SCALAR dt, int Initialize );
void aero( SCALAR dt, int Initialize );
void engine( SCALAR dt, int Initialize );
void gear( SCALAR dt, int Initialize );


#endif /* _DEFAULT_MODEL_ROUTINES_H */
