/* a quick atmos_62.h */


/* UNITS

  t_amb - degrees Rankine
  p_amb - Pounds per square foot

*/


#ifndef _ATMOS_62_H
#define _ATMOS_62_H

#ifdef __cplusplus
extern "C" {
#endif


void ls_atmos( SCALAR altitude, SCALAR * sigma, SCALAR * v_sound, 
		SCALAR * t_amb, SCALAR * p_amb );

#ifdef __cplusplus
}
#endif

#endif /* _ATMOS_62_H */
