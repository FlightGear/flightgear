/* a quick default_model_routines.h */


#ifndef _DEFAULT_MODEL_ROUTINES_H
#define _DEFAULT_MODEL_ROUTINES_H


#include "ls_types.h"


void inertias( SCALAR dt, int Initialize );
void subsystems( SCALAR dt, int Initialize );
void navion_aero( SCALAR dt, int Initialize );
void navion_engine( SCALAR dt, int Initialize );
void navion_gear( SCALAR dt, int Initialize );
void c172_init( void );
void c172_aero( SCALAR dt, int Initialize );
void c172_engine( SCALAR dt, int Initialize );
void c172_gear( SCALAR dt, int Initialize );
void cherokee_aero( SCALAR dt, int Initialize );
void cherokee_engine( SCALAR dt, int Initialize );
void cherokee_gear( SCALAR dt, int Initialize );
void basic_init( void );
void basic_aero( SCALAR dt, int Initialize );
void basic_engine( SCALAR dt, int Initialize );
void basic_gear( SCALAR dt, int Initialize );
void uiuc_init_2_wrapper( void );
void uiuc_network_recv_2_wrapper( void );
void uiuc_engine_2_wrapper( SCALAR dt, int Initialize );
void uiuc_wind_2_wrapper( SCALAR dt, int Initialize );
void uiuc_aero_2_wrapper( SCALAR dt, int Initialize );
void uiuc_gear_2_wrapper( SCALAR dt, int Initialize );
void uiuc_network_send_2_wrapper( void );
void uiuc_record_2_wrapper( SCALAR dt );
void uiuc_local_vel_init( void );


#endif /* _DEFAULT_MODEL_ROUTINES_H */
