#ifndef _ICE_H_
#define _ICE_H_

#include "uiuc_aircraft.h"

#ifdef __cplusplus
extern "C" {
#endif

extern double Simtime;

#ifdef __cplusplus
}
#endif

void uiuc_ice_eta();

double uiuc_ice_filter( double Ca_clean, double kCa );

#endif // _ICE_H_
