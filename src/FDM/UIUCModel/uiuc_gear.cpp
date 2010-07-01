/**********************************************************************
								       
 FILENAME:     uiuc_gear.cpp

----------------------------------------------------------------------

 DESCRIPTION:  determine the gear forces and moments
               
----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on c172_gear by Tony Peden and others

----------------------------------------------------------------------

 HISTORY:      03/09/2001   initial release

----------------------------------------------------------------------

 AUTHOR(S):    David Megginson <david@megginson.com

----------------------------------------------------------------------

 VARIABLES:    [TODO]

----------------------------------------------------------------------

 INPUTS:       [TODO]

----------------------------------------------------------------------

 OUTPUTS:      [TODO]

----------------------------------------------------------------------

 CALLED BY:   uiuc_wrapper.cpp 

----------------------------------------------------------------------

 CALLS TO:     none

----------------------------------------------------------------------

 COPYRIGHT:    (c) 2001 by David Megginson

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>
#include <Main/fg_props.hxx>

#include "uiuc_gear.h"

#define HEIGHT_AGL_WHEEL d_wheel_rwy_local_v[2]


static void sub3( DATA v1[],  DATA v2[], DATA result[] )
{
  result[0] = v1[0] - v2[0];
  result[1] = v1[1] - v2[1];
  result[2] = v1[2] - v2[2];
}

static void add3( DATA v1[],  DATA v2[], DATA result[] )
{
  result[0] = v1[0] + v2[0];
  result[1] = v1[1] + v2[1];
  result[2] = v1[2] + v2[2];
}

static void cross3( DATA v1[],  DATA v2[], DATA result[] )
{
  result[0] = v1[1]*v2[2] - v1[2]*v2[1];
  result[1] = v1[2]*v2[0] - v1[0]*v2[2];
  result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

static void multtrans3x3by3( DATA m[][3], DATA v[], DATA result[] )
{
  result[0] = m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2];
  result[1] = m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2];
  result[2] = m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2];
}

static void mult3x3by3( DATA m[][3], DATA v[], DATA result[] )
{
  result[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2];
  result[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2];
  result[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2];
}

static void clear3( DATA v[] )
{
  v[0] = 0.; v[1] = 0.; v[2] = 0.;
}

void uiuc_gear()
{
  
  /*
   * Aircraft specific initializations and data goes here
   */
  
  static DATA percent_brake[MAX_GEAR] =	    /* percent applied braking */
  { 0.,  0.,  0., 0.,
    0.,  0.,  0., 0.,
    0.,  0.,  0., 0.,
    0.,  0.,  0., 0. };			    /* 0 = none, 1 = full */
  static DATA caster_angle_rad[MAX_GEAR] =	    /* steerable tires - in */
  { 0., 0., 0., 0.,
    0., 0., 0., 0.,
    0., 0., 0., 0.,
    0., 0., 0., 0. };                         /* radians, +CW */	
  /*
   * End of aircraft specific code
   */
  
  /*
   * Constants & coefficients for tyres on tarmac - ref [1]
   */
  
  /* skid function looks like:
   * 
   *           mu  ^
   *               |
   *       max_mu  |       +		
   *               |      /|		
   *   sliding_mu  |     / +------	
   *               |    /		
   *               |   /		
   *               +--+------------------------> 
   *               |  |    |      sideward V
   *               0 bkout skid
   *	               V     V
   */
  
  
  static int it_rolls[MAX_GEAR] =
  { 1, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0 };	
  static DATA sliding_mu[MAX_GEAR] =
  { 0.5, 0.5, 0.5, 0.3,
    0.3, 0.3, 0.3, 0.3,
    0.3, 0.3, 0.3, 0.3,
    0.3, 0.3, 0.3, 0.3 };	
  static DATA max_brake_mu[MAX_GEAR] =
  { 0.0, 0.6, 0.6, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0 };	
  static DATA max_mu	     = 0.8;	
  static DATA bkout_v	     = 0.1;
  static DATA skid_v       = 1.0;
  /*
   * Local data variables
   */
  
  DATA d_wheel_cg_body_v[3];		/* wheel offset from cg,  X-Y-Z	*/
  DATA d_wheel_cg_local_v[3];		/* wheel offset from cg,  N-E-D	*/
  DATA d_wheel_rwy_local_v[3];	/* wheel offset from rwy, N-E-U */
  DATA v_wheel_cg_local_v[3];    /*wheel velocity rel to cg N-E-D*/
  // DATA v_wheel_body_v[3];		/* wheel velocity,	  X-Y-Z	*/
  DATA v_wheel_local_v[3];		/* wheel velocity,	  N-E-D	*/
  DATA f_wheel_local_v[3];		/* wheel reaction force,  N-E-D	*/
  // DATA altitude_local_v[3];       /*altitude vector in local (N-E-D) i.e. (0,0,h)*/
  // DATA altitude_body_v[3];        /*altitude vector in body (X,Y,Z)*/
  DATA temp3a[3];
  // DATA temp3b[3];
  DATA tempF[3];
  DATA tempM[3];	
  DATA reaction_normal_force;		/* wheel normal (to rwy) force	*/
  DATA cos_wheel_hdg_angle, sin_wheel_hdg_angle;	/* temp storage */
  DATA v_wheel_forward, v_wheel_sideward,  abs_v_wheel_sideward;
  DATA forward_mu, sideward_mu;	/* friction coefficients	*/
  DATA beta_mu;			/* breakout friction slope	*/
  DATA forward_wheel_force, sideward_wheel_force;
  
  int i;				/* per wheel loop counter */
  
  /*
   * Execution starts here
   */
  
  beta_mu = max_mu/(skid_v-bkout_v);
  clear3( F_gear_v );		/* Initialize sum of forces...	*/
  clear3( M_gear_v );		/* ...and moments		*/
  
  /*
   * Put aircraft specific executable code here
   */
  
  percent_brake[1] = Brake_pct[0];
  percent_brake[2] = Brake_pct[1];
  
  caster_angle_rad[0] =
    (0.01 + 0.04 * (1 - V_calibrated_kts / 130)) * Rudder_pedal;
  
  
  for (i=0;i<MAX_GEAR;i++)	    /* Loop for each wheel */
    {
      // Execute only if the gear has been defined
      if (!gear_model[i]) 
	{
	  // do nothing
	  continue;
	}       
      else
	{
	  
	  /*========================================*/
	  /* Calculate wheel position w.r.t. runway */
	  /*========================================*/
	  
	  /* printf("\thgcg: %g, theta: %g,phi: %g\n",D_cg_above_rwy,Theta*RAD_TO_DEG,Phi*RAD_TO_DEG); */
	  
	  /* First calculate wheel location w.r.t. cg in body (X-Y-Z) axes... */
	  
	  sub3( D_gear_v[i], D_cg_rp_body_v, d_wheel_cg_body_v );
	  
	  /* then converting to local (North-East-Down) axes... */
	  
	  multtrans3x3by3( T_local_to_body_m,  d_wheel_cg_body_v, d_wheel_cg_local_v );
	  
	  
	  /* Runway axes correction - third element is Altitude, not (-)Z... */
	  
	  d_wheel_cg_local_v[2] = -d_wheel_cg_local_v[2]; /* since altitude = -Z */
	  
	  /* Add wheel offset to cg location in local axes */
	  
	  add3( d_wheel_cg_local_v, D_cg_rwy_local_v, d_wheel_rwy_local_v );
	  
	  /* remove Runway axes correction so right hand rule applies */
	  
	  d_wheel_cg_local_v[2] = -d_wheel_cg_local_v[2]; /* now Z positive down */
	  
	  /*============================*/
	  /* Calculate wheel velocities */
	  /*============================*/
	  
	  /* contribution due to angular rates */
	  
	  cross3( Omega_body_v, d_wheel_cg_body_v, temp3a );
	  
	  /* transform into local axes */
	  
	  multtrans3x3by3( T_local_to_body_m, temp3a,v_wheel_cg_local_v );
	  
	  /* plus contribution due to cg velocities */
	  
	  add3( v_wheel_cg_local_v, V_local_rel_ground_v, v_wheel_local_v );
	  
	  clear3(f_wheel_local_v);
	  reaction_normal_force=0;
#if 0
	  static const SGPropertyNode * gear_wow
	    = fgGetNode("/gear/gear[0]/wow", false);
	  static const SGPropertyNode * gear_wow1
	    = fgGetNode("/gear/gear[1]/wow", false);
	  static const SGPropertyNode * gear_wow2
	    = fgGetNode("/gear/gear[2]/wow", false);
#endif
	  fgSetBool("/gear/gear[0]/wow", false);
	  fgSetBool("/gear/gear[1]/wow", false);
	  fgSetBool("/gear/gear[2]/wow", false);
	  if( HEIGHT_AGL_WHEEL < 0. ) 
	    /*the wheel is underground -- which implies ground contact 
	      so calculate reaction forces */ 
	    {
	      //set the property - weight on wheels
	      //  	  if (i==0) 
	      //  	    {
	      //  	      fgSetBool("/gear/gear[0]/wow", true);
	      //  	    }
	      //	  if (i==1) 
	      //	    {
	      //	      fgSetBool("/gear/gear[1]/wow", true);
	      //	    }
	      //  	  if (i==2) 
	      //  	    {
	      //  	      fgSetBool("/gear/gear[2]/wow", true);
	      //  	    }
	      
	      /*===========================================*/
	      /* Calculate forces & moments for this wheel */
	      /*===========================================*/
	      
	      /* Add any anticipation, or frame lead/prediction, here... */
	      
		    		/* no lead used at present */
	      
	      /* Calculate sideward and forward velocities of the wheel 
		 in the runway plane					*/
	      
	      cos_wheel_hdg_angle = cos(caster_angle_rad[i] + Psi);
	      sin_wheel_hdg_angle = sin(caster_angle_rad[i] + Psi);
	      
	      v_wheel_forward  = v_wheel_local_v[0]*cos_wheel_hdg_angle
		+ v_wheel_local_v[1]*sin_wheel_hdg_angle;
	      v_wheel_sideward = v_wheel_local_v[1]*cos_wheel_hdg_angle
		- v_wheel_local_v[0]*sin_wheel_hdg_angle;
	      
	      
	      /* Calculate normal load force (simple spring constant) */
	      
	      reaction_normal_force = 0.;
	      
	      reaction_normal_force = kgear[i]*d_wheel_rwy_local_v[2]
		- v_wheel_local_v[2]*cgear[i];
	      /* printf("\treaction_normal_force: %g\n",reaction_normal_force); */
	      
	      if (reaction_normal_force > 0.) reaction_normal_force = 0.;
	      /* to prevent damping component from swamping spring component */
	      
	      
	      /* Calculate friction coefficients */
	      
	      if(it_rolls[i])
		{
		  forward_mu = (max_brake_mu[i] - muGear[i])*percent_brake[i] + muGear[i];
		  abs_v_wheel_sideward = sqrt(v_wheel_sideward*v_wheel_sideward);
		  sideward_mu = sliding_mu[i];
		  if (abs_v_wheel_sideward < skid_v) 
		    sideward_mu = (abs_v_wheel_sideward - bkout_v)*beta_mu;
		  if (abs_v_wheel_sideward < bkout_v) sideward_mu = 0.;
		}
	      else
		{
		  forward_mu=sliding_mu[i];
		  sideward_mu=sliding_mu[i];
		}	   
	      
	      /* Calculate foreward and sideward reaction forces */
	      
	      forward_wheel_force  =   forward_mu*reaction_normal_force;
	      sideward_wheel_force =  sideward_mu*reaction_normal_force;
	      if(v_wheel_forward < 0.) forward_wheel_force = -forward_wheel_force;
	      if(v_wheel_sideward < 0.) sideward_wheel_force = -sideward_wheel_force;
	      /* printf("\tFfwdgear: %g Fsidegear: %g\n",forward_wheel_force,sideward_wheel_force);
	       */
	      /* Rotate into local (N-E-D) axes */
	      
	      f_wheel_local_v[0] = forward_wheel_force*cos_wheel_hdg_angle
		- sideward_wheel_force*sin_wheel_hdg_angle;
	      f_wheel_local_v[1] = forward_wheel_force*sin_wheel_hdg_angle
		+ sideward_wheel_force*cos_wheel_hdg_angle;
	      f_wheel_local_v[2] = reaction_normal_force;	  
	      
	      /* Convert reaction force from local (N-E-D) axes to body (X-Y-Z) */
	      mult3x3by3( T_local_to_body_m, f_wheel_local_v, tempF );
	      
	      /* Calculate moments from force and offsets in body axes */
	      
	      cross3( d_wheel_cg_body_v, tempF, tempM );
	      
	      /* Sum forces and moments across all wheels */
	      if (tempF[0] != 0.0 || tempF[1] != 0.0 || tempF[2] != 0.0) {
		fgSetBool("/gear/gear[1]/wow", true);
	      }
	      
	      add3( tempF, F_gear_v, F_gear_v );
	      add3( tempM, M_gear_v, M_gear_v );   
	      
	    }	  
	}
      
      
      
      /* printf("\tN: %g,dZrwy: %g dZdotrwy: %g\n",reaction_normal_force,HEIGHT_AGL_WHEEL,v_wheel_cg_local_v[2]); */
      
      /*printf("\tFxgear: %g Fygear: %g, Fzgear: %g\n",F_X_gear,F_Y_gear,F_Z_gear);
	printf("\tMgear: %g, Lgear: %g, Ngear: %g\n\n",M_m_gear,M_l_gear,M_n_gear); */
      
      
    }
}

// end uiuc_engine.cpp
