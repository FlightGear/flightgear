/***************************************************************************

    TITLE:		ls_aux
		
----------------------------------------------------------------------------

    FUNCTION:	Atmospheric and auxilary relationships for LaRCSim EOM

----------------------------------------------------------------------------

    MODULE STATUS:	developmental

----------------------------------------------------------------------------

    GENEALOGY:	Created 9208026	as part of C-castle simulation project.

----------------------------------------------------------------------------

    DESIGNED BY:	B. Jackson
    
    CODED BY:		B. Jackson
    
    MAINTAINED BY:	B. Jackson

----------------------------------------------------------------------------

    MODIFICATION HISTORY:
    
    DATE    PURPOSE	
    
    931006  Moved calculations of auxiliary accelerations from here
	    to ls_accel.c and corrected minus sign in front of A_Y_Pilot
	    contribution from Q_body*P_body*D_X_pilot term.	    EBJ
    931014  Changed calculation of Alpha from atan to atan2 so sign is correct.
								    EBJ
    931220  Added calculations for static and total temperatures & pressures,
	    as well as dynamic and impact pressures and calibrated airspeed.
								    EBJ
    940111  Changed #included header files from old "ls_eom.h" to newer
	    "ls_types.h", "ls_constants.h" and "ls_generic.h".	    EBJ

    950207  Changed use of "abs" to "fabs" in calculation of signU. EBJ
    
    950228  Fixed bug in calculation of beta_dot.		    EBJ

    CURRENT RCS HEADER INFO:

$Header$
$Log$
Revision 1.1  1997/05/29 00:09:54  curt
Initial Flight Gear revision.

 * Revision 1.12  1995/02/28  17:57:16  bjax
 * Corrected calculation of beta_dot. EBJ
 *
 * Revision 1.11  1995/02/07  21:09:47  bjax
 * Corrected calculation of "signU"; was using divide by
 * abs(), which returns an integer; now using fabs(), which
 * returns a double.  EBJ
 *
 * Revision 1.10  1994/05/10  20:09:42  bjax
 * Fixed a major problem with dx_pilot_from_cg, etc. not being calculated locally.
 *
 * Revision 1.9  1994/01/11  18:44:33  bjax
 * Changed header files to use ls_types, ls_constants, and ls_generic.
 *
 * Revision 1.8  1993/12/21  14:36:33  bjax
 * Added calcs of pressures, temps and calibrated airspeeds.
 *
 * Revision 1.7  1993/10/14  11:25:38  bjax
 * Changed calculation of Alpha to use 'atan2' instead of 'atan' so alphas
 * larger than +/- 90 degrees are calculated correctly.			EBJ
 *
 * Revision 1.6  1993/10/07  18:45:56  bjax
 * A little cleanup; no significant changes. EBJ
 *
 * Revision 1.5  1993/10/07  18:42:22  bjax
 * Moved calculations of auxiliary accelerations here from ls_aux, and
 * corrected sign on Q_body*P_body*d_x_pilot term of A_Y_pilot calc.  EBJ
 *
 * Revision 1.4  1993/07/16  18:28:58  bjax
 * Changed call from atmos_62 to ls_atmos. EBJ
 *
 * Revision 1.3  1993/06/02  15:02:42  bjax
 * Changed call to geodesy calcs from ls_geodesy to ls_geoc_to_geod.
 *
 * Revision 1.1  92/12/30  13:17:39  bjax
 * Initial revision
 * 


----------------------------------------------------------------------------

    REFERENCES:	[ 1] Shapiro, Ascher H.: "The Dynamics and Thermodynamics
			of Compressible Fluid Flow", Volume I, The Ronald 
			Press, 1953.

----------------------------------------------------------------------------

		CALLED BY:

----------------------------------------------------------------------------

		CALLS TO:

----------------------------------------------------------------------------

		INPUTS:

----------------------------------------------------------------------------

		OUTPUTS:

--------------------------------------------------------------------------*/
#include "ls_types.h"
#include "ls_constants.h"
#include "ls_generic.h"
#include <math.h>

void	ls_aux()
{

	SCALAR	dx_pilot_from_cg, dy_pilot_from_cg, dz_pilot_from_cg;
	SCALAR	inv_Mass;
	SCALAR	v_XZ_plane_2, signU, v_tangential;
	SCALAR	inv_radius_ratio;
	SCALAR	cos_rwy_hdg, sin_rwy_hdg;
	SCALAR	mach2, temp_ratio, pres_ratio;
	
    /* update geodetic position */

	ls_geoc_to_geod( Lat_geocentric, Radius_to_vehicle, 
				&Latitude, &Altitude, &Sea_level_radius );
	Longitude = Lon_geocentric - Earth_position_angle;

    /* Calculate body axis velocities */

	/* Form relative velocity vector */

	V_north_rel_ground = V_north;
	V_east_rel_ground  = V_east 
	  - OMEGA_EARTH*Sea_level_radius*cos( Lat_geocentric );
	V_down_rel_ground  = V_down;
	
	V_north_rel_airmass = V_north_rel_ground - V_north_airmass;
	V_east_rel_airmass  = V_east_rel_ground  - V_east_airmass;
	V_down_rel_airmass  = V_down_rel_ground  - V_down_airmass;
	
	U_body = T_local_to_body_11*V_north_rel_airmass 
	  + T_local_to_body_12*V_east_rel_airmass
	    + T_local_to_body_13*V_down_rel_airmass + U_gust;
	V_body = T_local_to_body_21*V_north_rel_airmass 
	  + T_local_to_body_22*V_east_rel_airmass
	    + T_local_to_body_23*V_down_rel_airmass + V_gust;
	W_body = T_local_to_body_31*V_north_rel_airmass 
	  + T_local_to_body_32*V_east_rel_airmass
	    + T_local_to_body_33*V_down_rel_airmass + W_gust;
				
	V_rel_wind = sqrt(U_body*U_body + V_body*V_body + W_body*W_body);


    /* Calculate alpha and beta rates	*/

	v_XZ_plane_2 = (U_body*U_body + W_body*W_body);
	
	if (U_body == 0)
		signU = 1;
	else
		signU = U_body/fabs(U_body);
		
	if( (v_XZ_plane_2 == 0) || (V_rel_wind == 0) )
	{
		Alpha_dot = 0;
		Beta_dot = 0;
	}
	else
	{
		Alpha_dot = (U_body*W_dot_body - W_body*U_dot_body)/
		  v_XZ_plane_2;
		Beta_dot = (signU*v_XZ_plane_2*V_dot_body 
		  - V_body*(U_body*U_dot_body + W_body*W_dot_body))
		    /(V_rel_wind*V_rel_wind*sqrt(v_XZ_plane_2));
	}

    /* Calculate flight path and other flight condition values */

	if (U_body == 0) 
		Alpha = 0;
	else
		Alpha = atan2( W_body, U_body );
		
	Cos_alpha = cos(Alpha);
	Sin_alpha = sin(Alpha);
	
	if (V_rel_wind == 0)
		Beta = 0;
	else
		Beta = asin( V_body/ V_rel_wind );
		
	Cos_beta = cos(Beta);
	Sin_beta = sin(Beta);
	
	V_true_kts = V_rel_wind * V_TO_KNOTS;
	
	V_ground_speed = sqrt(V_north_rel_ground*V_north_rel_ground
			      + V_east_rel_ground*V_east_rel_ground );

	V_rel_ground = sqrt(V_ground_speed*V_ground_speed
			    + V_down_rel_ground*V_down_rel_ground );
	
	v_tangential = sqrt(V_north*V_north + V_east*V_east);
	
	V_inertial = sqrt(v_tangential*v_tangential + V_down*V_down);
	
	if( (V_ground_speed == 0) && (V_down == 0) )
	  Gamma_vert_rad = 0;
	else
	  Gamma_vert_rad = atan2( -V_down, V_ground_speed );
		
	if( (V_north_rel_ground == 0) && (V_east_rel_ground == 0) )
	  Gamma_horiz_rad = 0;
	else
	  Gamma_horiz_rad = atan2( V_east_rel_ground, V_north_rel_ground );
	
	if (Gamma_horiz_rad < 0) 
	  Gamma_horiz_rad = Gamma_horiz_rad + 2*PI;
	
    /* Calculate local gravity	*/
	
	ls_gravity( Radius_to_vehicle, Lat_geocentric, &Gravity );
	
    /* call function for (smoothed) density ratio, sonic velocity, and
	   ambient pressure */

	ls_atmos(Altitude, &Sigma, &V_sound, 
		 &Static_temperature, &Static_pressure);
	
	Density = Sigma*SEA_LEVEL_DENSITY;
	
	Mach_number = V_rel_wind / V_sound;
	
	V_equiv	= V_rel_wind*sqrt(Sigma);
	
	V_equiv_kts = V_equiv*V_TO_KNOTS;

    /* calculate temperature and pressure ratios (from [1]) */

	mach2 = Mach_number*Mach_number;
	temp_ratio = 1.0 + 0.2*mach2; 
	pres_ratio = pow( temp_ratio, 3.5 );

	Total_temperature = temp_ratio*Static_temperature;
	Total_pressure    = pres_ratio*Static_pressure;

    /* calculate impact and dynamic pressures */
	
	Impact_pressure = Total_pressure - Static_pressure; 

	Dynamic_pressure = 0.5*Density*V_rel_wind*V_rel_wind;

    /* calculate calibrated airspeed indication */

	V_calibrated = sqrt( 2.0*Dynamic_pressure / SEA_LEVEL_DENSITY );
	V_calibrated_kts = V_calibrated*V_TO_KNOTS;
	
	Centrifugal_relief = 1 - v_tangential/(Radius_to_vehicle*Gravity);
	
/* Determine location in runway coordinates */

	Radius_to_rwy = Sea_level_radius + Runway_altitude;
	cos_rwy_hdg = cos(Runway_heading*DEG_TO_RAD);
	sin_rwy_hdg = sin(Runway_heading*DEG_TO_RAD);
	
	D_cg_north_of_rwy = Radius_to_rwy*(Latitude - Runway_latitude);
	D_cg_east_of_rwy = Radius_to_rwy*cos(Runway_latitude)
		*(Longitude - Runway_longitude);
	D_cg_above_rwy	= Radius_to_vehicle - Radius_to_rwy;
	
	X_cg_rwy = D_cg_north_of_rwy*cos_rwy_hdg 
	  + D_cg_east_of_rwy*sin_rwy_hdg;
	Y_cg_rwy =-D_cg_north_of_rwy*sin_rwy_hdg 
	  + D_cg_east_of_rwy*cos_rwy_hdg;
	H_cg_rwy = D_cg_above_rwy;
	
	dx_pilot_from_cg = Dx_pilot - Dx_cg;
	dy_pilot_from_cg = Dy_pilot - Dy_cg;
	dz_pilot_from_cg = Dz_pilot - Dz_cg;

	D_pilot_north_of_rwy = D_cg_north_of_rwy 
	  + T_local_to_body_11*dx_pilot_from_cg 
	    + T_local_to_body_21*dy_pilot_from_cg
	      + T_local_to_body_31*dz_pilot_from_cg;
	D_pilot_east_of_rwy  = D_cg_east_of_rwy 
	  + T_local_to_body_12*dx_pilot_from_cg 
	    + T_local_to_body_22*dy_pilot_from_cg
	      + T_local_to_body_32*dz_pilot_from_cg;
	D_pilot_above_rwy    = D_cg_above_rwy 
	  - T_local_to_body_13*dx_pilot_from_cg 
	    - T_local_to_body_23*dy_pilot_from_cg
	      - T_local_to_body_33*dz_pilot_from_cg;
							
	X_pilot_rwy =  D_pilot_north_of_rwy*cos_rwy_hdg
	  + D_pilot_east_of_rwy*sin_rwy_hdg;
	Y_pilot_rwy = -D_pilot_north_of_rwy*sin_rwy_hdg
	  + D_pilot_east_of_rwy*cos_rwy_hdg;
	H_pilot_rwy =  D_pilot_above_rwy;
								
/* Calculate Euler rates */
	
	Sin_phi	= sin(Phi);
	Cos_phi = cos(Phi);
	Sin_theta = sin(Theta);
	Cos_theta = cos(Theta);
	Sin_psi = sin(Psi);
	Cos_psi = cos(Psi);
	
	if( Cos_theta == 0 )
	  Psi_dot = 0;
	else
	  Psi_dot = (Q_total*Sin_phi + R_total*Cos_phi)/Cos_theta;
	
	Theta_dot = Q_total*Cos_phi - R_total*Sin_phi;
	Phi_dot = P_total + Psi_dot*Sin_theta;
	
/* end of ls_aux */

}
/*************************************************************************/
