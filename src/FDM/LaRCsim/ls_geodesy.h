/* a quick ls_geodesy.h */


#ifndef _LS_GEODESY_H
#define _LS_GEODESY_H

#ifdef __cplusplus
extern "C" {
#endif

void ls_geoc_to_geod( SCALAR lat_geoc, SCALAR radius, 
		      SCALAR *lat_geod, SCALAR *alt, SCALAR *sea_level_r );

void ls_geod_to_geoc( SCALAR lat_geod, SCALAR alt, SCALAR *sl_radius, 
		      SCALAR *lat_geoc );

#ifdef __cplusplus
}
#endif

#endif /* _LS_GEODESY_H */
