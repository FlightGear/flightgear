// LaRCsim.cxx -- interface to the LaRCsim flight model
//
// Written by Curtis Olson, started October 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$





#include <FDM/LaRCsim/ls_cockpit.h>
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_interface.h>
#include <FDM/LaRCsim/ls_constants.h>
#include <FDM/LaRCsim/atmos_62.h>
/* #include <FDM/LaRCsim/ls_trim_fs.h> */
#include <FDM/LaRCsim/c172_aero.h>
#include <FDM/LaRCsim/ic.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>




void do_trims(int kmax,FILE *out,InitialConditions IC)
{
	int k=0,i,j;
	double speed,elevator,cmcl,maxspeed;
	out=fopen("trims.out","w");
	speed=55;
	
	for(j=0;j<=30;j+=10)
	{
		IC.flap_handle=j;
		for(i=1;i<=5;i++)
		{
			switch(i)
			{
				case 1: IC.weight=1500;IC.cg=0.155;break;
				case 2: IC.weight=1500;IC.cg=0.364;break;
				case 3: IC.weight=1950;IC.cg=0.155;break;
				case 4: IC.weight=2550;IC.cg=0.257;break;
				case 5: IC.weight=2550;IC.cg=0.364;break;
			}

			speed=40;
			if(j > 0) { maxspeed = 90; }
			else { maxspeed = 170; }
			while(speed <= maxspeed)
			{
			   IC.vc=speed;
			   Long_control=0;Theta=0;Throttle_pct=0.0;

			   k=trim_long(kmax,IC);
			   if(Long_control <= 0)
				  elevator=Long_control*28;
			   else
				 elevator=Long_control*23;	
			   if(fabs(CL) > 1E-3)
			   {
	   				cmcl=cm / CL;
			   }	
			   if(k < kmax)
			   {
	   				fprintf(out,"%g,%g,%g,%g,%g,%d",V_calibrated_kts,Alpha*RAD_TO_DEG,Long_control,Throttle_pct,Flap_Position,k);
	   				fprintf(out,",%g,%g,%g,%g,%g\n",CL,cm,cmcl,Weight,Cg);
	/* 	   			printf("%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",V_calibrated_kts,Alpha*RAD_TO_DEG,elevator,CL,cm,Cmo,Cma,Cmde,Mass*32.174,Dx_cg);
	 */		   }	
    		   else
			   {
		   		 printf("kmax exceeded at: %g knots, %g lbs, %g %%MAC, Flaps: %g\n",V_calibrated_kts,Weight,Cg,Flap_Position);
				 printf("wdot: %g, udot: %g, qdot: %g\n",W_dot_body,U_dot_body,Q_dot_body);
            	 printf("Alpha: %g, Throttle_pct: %g, Long_control: %g\n\n",Alpha*RAD_TO_DEG,Throttle_pct,Long_control);
			   }
			   speed+=10;	  
			}
    	}
	}	
	fclose(out);
}	

void find_trim_stall(int kmax,FILE *out,InitialConditions IC)
{
	int k=0,i,j;
	int failf;
	char axis[10];
	double speed,elevator,cmcl,speed_inc,lastgood;
	out=fopen("trim_stall.summary","w");
	speed=90;
	speed_inc=10;
	//failf=malloc(sizeof(int));
	
	for(j=0;j<=30;j+=10)
	{
		IC.flap_handle=j;
		for(i=1;i<=6;i++)
		{
			switch(i)
			{
				case 1: IC.weight=1500;IC.cg=0.155;break;
				case 2: IC.weight=1500;IC.cg=0.364;break;
				case 3: IC.weight=2400;IC.cg=0.155;break;
				case 4: IC.weight=2400;IC.cg=0.364;break;
				case 5: IC.weight=2550;IC.cg=0.257;break;
				case 6: IC.weight=2550;IC.cg=0.364;break;
			}

			speed=90;
			speed_inc=10;
			while(speed_inc >= 0.5)
			{
			   IC.vc=speed;
			   Long_control=0;Theta=0;Throttle_pct=0.0;
			   failf=trim_longfr(kmax,IC);
			   if(Long_control <= 0)
				  elevator=Long_control*28;
			   else
				 elevator=Long_control*23;	
			   if(fabs(CL) > 1E-3)
			   {
	   				cmcl=cm / CL;
			   }	
			   if(failf == 0)
			   {
	   				lastgood=speed;
					axis[0]='\0';
					//fprintf(out,"%g,%g,%g,%g,%g,%d",V_calibrated_kts,Alpha*RAD_TO_DEG,Long_control,Throttle_pct,Flap_Position,k);
	   				//fprintf(out,",%g,%g,%g,%g,%g\n",CL,cm,cmcl,Weight,Cg);
	/* 	   			printf("%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",V_calibrated_kts,Alpha*RAD_TO_DEG,elevator,CL,cm,Cmo,Cma,Cmde,Mass*32.174,Dx_cg);
	 */		   }	
    		   else
			   {
		   		 printf("trim failed at: %g knots, %g lbs, %g %%MAC, Flaps: %g\n",V_calibrated_kts,Weight,Cg,Flap_Position);
				 printf("wdot: %g, udot: %g, qdot: %g\n",W_dot_body,U_dot_body,Q_dot_body);
            	 printf("Alpha: %g, Throttle_pct: %g, Long_control: %g\n\n",Alpha*RAD_TO_DEG,Throttle_pct,Long_control);
				 printf("Speed increment: %g\n",speed_inc);
				 speed+=speed_inc;
				 speed_inc/=2;
			   }
			   speed-=speed_inc;
			   
			   	  
			}
			printf("failf %d\n",failf); 
			if(failf == 1)
			   strcpy(axis,"lift");
			else if(failf == 2)
			   strcpy(axis,"thrust");
			else if(failf == 3)
			   strcpy(axis,"pitch");	  	  
			fprintf(out,"Last good speed: %g, Flaps: %g, Weight: %g, CG: %g, failed axis: %s\n",lastgood,Flap_handle,Weight,Cg,axis);

			
    	}
	}
	fclose(out);
	//free(failf);
}	


// Initialize the LaRCsim flight model, dt is the time increment for
// each subsequent iteration through the EOM
int fgLaRCsimInit(double dt) {
    ls_toplevel_init(dt);

    return(1);
}



// Run an iteration of the EOM (equations of motion)
int main(int argc, char *argv[]) {
    
	
	double save_alt = 0.0;
    int multiloop=1,k=0,i,j;
	double time=0,elev_trim,elev_trim_save,elevator,speed,cmcl;
	FILE *out;
	double hgain,hdiffgain,herr,herrprev,herr_diff,htarget;
	double lastVt,vtdots,vtdott;
	InitialConditions IC;
    SCALAR *control[7];
	SCALAR *state[7];
	float old_state,effectiveness,tol,delta_state,lctrim;
	float newcm,lastcm,cmalpha;
	
    if(argc < 6)
	{
	    printf("Need args: $c172 speed alt alpha elev throttle\n");
		exit(1);
	}	
	initIC(&IC);
	
	IC.latitude=47.5299892; //BFI
	IC.longitude=122.3019561;
	Runway_altitude =   18.0;
	IC.altitude=strtod(argv[2],NULL); 
	IC.vc=strtod(argv[1],NULL);
	IC.alpha=0;
	IC.beta=0;
	IC.gamma=strtod(argv[3],NULL);
	IC.use_gamma_tmg=1;
	IC.phi=0;
	IC.psi=10;
	IC.weight=2300;
	IC.cg=0.25;
	IC.flap_handle=0;
	IC.long_control=strtod(argv[4],NULL);
	IC.rudder_pedal=0;
    
	printf("IC.vc: %g\n",IC.vc);
	ls_ForceAltitude(IC.altitude);  
    fgLaRCsimInit(0.01);
	printf("\nLong_control: %g\n\n",Long_control);
	
	
	IC.cg=0.155;
    IC.alpha=-5;
	setIC(IC);ls_loop(0.0,-1);
	newcm=CLwbh*(IC.cg - 0.557);
	lastcm=newcm;
	out=fopen("cmcl.out","w");
	while(IC.alpha < 22)
	{
		IC.alpha+=1;
		setIC(IC);ls_loop(0.0,-1);
		newcm=CLwbh*(IC.cg - 0.557);
		cmalpha=newcm-lastcm;
		printf("alpha: %4.0f, CL: %5.2f, Cm: %5.2f, Cma: %7.4f\n",Alpha*RAD_TO_DEG,CLwbh,newcm,cmalpha);
		fprintf(out,"%g %g\n",newcm,CLwbh);
		lastcm=newcm;
	}	
	fclose(out);
	/* find_trim_stall(200,out,IC);
	
    IC.vc=120;
	IC.altitude=8000;
	IC.weight=2300;
	IC.cg=0.25;
	IC.flap_handle=0;

	
    setIC(IC);
	printIC(IC);
	k=trim_long(100,IC);

	printf("Flap_handle: %g, Flap_Position: %g\n",Flap_handle,Flap_Position);
	printf("k: %d, %g knots, %g lbs, %g %%MAC\n",k,V_calibrated_kts,Weight,Cg);
	printf("wdot: %g, udot: %g, qdot: %g\n",W_dot_body,U_dot_body,Q_dot_body);
    printf("Alpha: %g, Throttle_pct: %g, Long_control: %g\n\n",Alpha,Throttle_pct,Long_control);

	printf("Cme: %g, elevator: %g, Cmde: %g\n",elevator*Cmde,elevator,Cmde);
 
    IC.cg=0.155;
    setIC(IC);
	k=trim_long(100,IC);

	printf("Flap_handle: %g, Flap_Position: %g\n",Flap_handle,Flap_Position);
	printf("k: %d, %g knots, %g lbs, %g %%MAC\n",k,V_calibrated_kts,Weight,Cg);
	printf("wdot: %g, udot: %g, qdot: %g\n",W_dot_body,U_dot_body,Q_dot_body);
    printf("Alpha: %g, Throttle_pct: %g, Long_control: %g\n\n",Alpha,Throttle_pct,Long_control);

	printf("Cme: %g, elevator: %g, Cmde: %g\n",elevator*Cmde,elevator,Cmde);
	
	IC.cg=0.364;
	setIC(IC);
	k=trim_long(100,IC);

	printf("Flap_handle: %g, Flap_Position: %g\n",Flap_handle,Flap_Position);
	printf("k: %d, %g knots, %g lbs, %g %%MAC\n",k,V_calibrated_kts,Weight,Cg);
	printf("wdot: %g, udot: %g, qdot: %g\n",W_dot_body,U_dot_body,Q_dot_body);
    printf("Alpha: %g, Throttle_pct: %g, Long_control: %g\n\n",Alpha,Throttle_pct,Long_control);

	printf("Cme: %g, elevator: %g, Cmde: %g\n",elevator*Cmde,elevator,Cmde);
	 */

	
	
				
	
	/* do_trims(400,out,IC); */
	
	/* ls_loop(0.0,-1);
	
	control[1]=&IC.long_control;
	control[2]=&IC.throttle;
	control[3]=&IC.alpha;
	control[4]=&IC.beta;
	control[5]=&IC.phi;
	control[6]=&IC.lat_control;
	
	state[1]=&Q_dot_body;state[2]=&U_dot_body;state[3]=&W_dot_body;
	state[4]=&R_dot_body;state[5]=&V_dot_body;state[6]=&P_dot_body;
	
	
	for(i=1;i<=6;i++)
	{
		old_state=*state[i];
	    tol=1E-4;
		for(j=1;j<=6;j++)
		{
			*control[j]+=0.1;
			setIC(IC);
			ls_loop(0.0,-1);
			delta_state=*state[i]-old_state;
			effectiveness=(delta_state)/ 0.1;
			if(delta_state < tol)
				effectiveness = 0;
			printf("%8.4f,",delta_state);
			*control[j]-=0.1;
			
		}
		printf("\n");
		setIC(IC);
		ls_loop(0.0,-1);
	}		 */
	
	    return 1;
}

/*
void do_stick_pull(int kmax, SCALAR tmax,FILE *out,InitialConditions IC)
{
	
	SCALAR htarget,hgain,hdiffgain,herr,herr_diff,herrprev;
	SCALAR theta_trim,elev_trim,time;
	int k;
	k=trim_long(kmax,IC);
	printf("Trim:\n\tAlpha: %10.6f, elev: %10.6f, Throttle: %10.6f\n\twdot: %10.6f, qdot: %10.6f, udot: %10.6f\n",Alpha*RAD_TO_DEG,Long_control,Throttle_pct,W_dot_body,U_dot_body,Q_dot_body);

	
	htarget=0;
	
	hgain=1;
	hdiffgain=1;
	elev_trim=Long_control;
	out=fopen("stick_pull.out","w");
	herr=Q_body-htarget;
		
	//fly steady-level for 2 seconds, well, zero pitch rate anyway
	while(time < 2.0)
	{
		 herrprev=herr;
		 ls_update(1);
		 herr=Q_body-htarget;
		 herr_diff=herr-herrprev;
		 Long_control=elev_trim+(hgain*herr + hdiffgain*herr_diff);
		 time+=0.01;  
 		 //printf("Time: %7.4f, Alt: %7.4f, Alpha: %7.4f, pelev: %7.4f, qdot: %7.4f, udot: %7.4f, Phi: %7.4f, Psi: %7.4f\n",time,Altitude,Alpha*RAD_TO_DEG,Long_control*100,Q_body*RAD_TO_DEG,U_dot_body,Phi,Psi);
         //printf("Mcg: %7.4f, Mrp: %7.4f, Maero: %7.4f, Meng: %7.4f, Mgear: %7.4f, Dx_cg: %7.4f, Dz_cg: %7.4f\n\n",M_m_cg,M_m_rp,M_m_aero,M_m_engine,M_m_gear,Dx_cg,Dz_cg);
		 fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,",time,V_true_kts,Theta*RAD_TO_DEG,Alpha*RAD_TO_DEG,Q_body*RAD_TO_DEG,Alpha_dot*RAD_TO_DEG,Q_dot_body*RAD_TO_DEG,Throttle_pct,elevator*RAD_TO_DEG);
		 fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f\n",CL,CLwbh,cm,cd,Altitude);
	}

	//begin untrimmed climb at theta_trim + 2 degrees
	hgain=4;
	hdiffgain=2;
	theta_trim=Theta;
	htarget=theta_trim;
	herr=Theta-htarget;
	while(time < tmax)
	{
		//ramp in the target theta
		if(htarget < (theta_trim + 2*DEG_TO_RAD))
		{
			htarget+= 0.01*DEG_TO_RAD;
		}	
		herrprev=herr;
		 ls_update(1);
		 herr=Theta-htarget;
		 herr_diff=herr-herrprev;
		 Long_control=elev_trim+(hgain*herr + hdiffgain*herr_diff);
		 time+=0.01;  
		 //printf("Time: %7.4f, Alt: %7.4f, Alpha: %7.4f, pelev: %7.4f, qdot: %7.4f, udot: %7.4f, Phi: %7.4f, Psi: %7.4f\n",time,Altitude,Alpha*RAD_TO_DEG,Long_control*100,Q_body*RAD_TO_DEG,U_dot_body,Phi,Psi);
         //printf("Mcg: %7.4f, Mrp: %7.4f, Maero: %7.4f, Meng: %7.4f, Mgear: %7.4f, Dx_cg: %7.4f, Dz_cg: %7.4f\n\n",M_m_cg,M_m_rp,M_m_aero,M_m_engine,M_m_gear,Dx_cg,Dz_cg);
		 fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,",time,V_true_kts,Theta*RAD_TO_DEG,Alpha*RAD_TO_DEG,Q_body*RAD_TO_DEG,Alpha_dot*RAD_TO_DEG,Q_dot_body*RAD_TO_DEG,Throttle_pct,elevator*RAD_TO_DEG);
		 fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f\n",CL,CLwbh,cm,cd,Altitude);
	}
	printf("%g,%g\n",theta_trim*RAD_TO_DEG,htarget*RAD_TO_DEG);	 
	fclose(out);
}	

void do_takeoff(FILE *out)
{
	SCALAR htarget,hgain,hdiffgain,elev_trim,elev_trim_save,herr;
	SCALAR time,herrprev,herr_diff;
	
	htarget=0;
	
	hgain=1;
	hdiffgain=1;
	elev_trim=Long_control;
	elev_trim_save=elev_trim;
	
	
	out=fopen("takeoff.out","w");
	herr=Q_body-htarget;
	 	 
		// attempt to maintain zero pitch rate during the roll
		while((V_calibrated_kts < 61) && (time < 30.0))
		{
			// herrprev=herr
			ls_update(1);
			// herr=Q_body-htarget;
			// herr_diff=herr-herrprev;
			// Long_control=elev_trim+(hgain*herr + hdiffgain*herr_diff); 
			time+=0.01;  
			printf("Time: %7.4f, Vc: %7.4f, Alpha: %7.4f, pelev: %7.4f, qdot: %7.4f, udot: %7.4f, U: %7.4f, W: %7.4f\n",time,V_calibrated_kts,Alpha*RAD_TO_DEG,Long_control*100,Q_body*RAD_TO_DEG,U_dot_body,U_body,W_body);
//        	printf("Mcg: %7.4f, Mrp: %7.4f, Maero: %7.4f, Meng: %7.4f, Mgear: %7.4f, Dx_cg: %7.4f, Dz_cg: %7.4f\n\n",M_m_cg,M_m_rp,M_m_aero,M_m_engine,M_m_gear,Dx_cg,Dz_cg);
//			fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,",time,V_calibrated_kts,Theta*RAD_TO_DEG,Alpha*RAD_TO_DEG,Q_body*RAD_TO_DEG,Alpha_dot*RAD_TO_DEG,Q_dot_body*RAD_TO_DEG,Throttle_pct,elevator*RAD_TO_DEG);
			fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f\n",CL,CLwbh,cm,cd,Altitude);
			
		}
		//At Vr, ramp in 10% nose up elevator in 0.5 seconds
		elev_trim_save=0;
		printf("At Vr, rotate...\n");
		while((Q_body < 3.0*RAD_TO_DEG) && (time < 30.0))
		{
			Long_control-=0.01;
			ls_update(1);
			printf("Time: %7.4f, Vc: %7.4f, Alpha: %7.4f, pelev: %7.4f, q: %7.4f, cm: %7.4f, U: %7.4f, W: %7.4f\n",time,V_calibrated_kts,Alpha*RAD_TO_DEG,Long_control*100,Q_body*RAD_TO_DEG,cm,U_body,W_body);

			fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,",time,V_calibrated_kts,Theta*RAD_TO_DEG,Alpha*RAD_TO_DEG,Q_body*RAD_TO_DEG,Alpha_dot*RAD_TO_DEG,Q_dot_body*RAD_TO_DEG,Throttle_pct,elevator*RAD_TO_DEG);
			fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f\n",CL,CLwbh,cm,cd,Altitude);
			time +=0.01;

		}
		//Maintain 15 degrees theta for the climbout
		htarget=15*DEG_TO_RAD;
		herr=Theta-htarget;
		hgain=10;
		hdiffgain=1;
		elev_trim=Long_control;
		while(time < 30.0)
		{
			herrprev=herr;
			ls_update(1);
			herr=Theta-htarget;
			herr_diff=herr-herrprev;
			Long_control=elev_trim+(hgain*herr + hdiffgain*herr_diff);
			time+=0.01;  
			printf("Time: %7.4f, Alt: %7.4f, Speed: %7.4f, Theta: %7.4f\n",time,Altitude,V_calibrated_kts,Theta*RAD_TO_DEG);
			fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,",time,V_calibrated_kts,Theta*RAD_TO_DEG,Alpha*RAD_TO_DEG,Q_body*RAD_TO_DEG,Alpha_dot*RAD_TO_DEG,Q_dot_body*RAD_TO_DEG,Throttle_pct,elevator*RAD_TO_DEG);
			fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f\n",CL,CLwbh,cm,cd,Altitude);
		}	
		fclose(out);	
		printf("Speed: %7.4f, Alt: %7.4f, Alpha: %7.4f, pelev: %7.4f, q: %7.4f, udot: %7.4f\n",V_true_kts,Altitude,Alpha*RAD_TO_DEG,Long_control,Q_body*RAD_TO_DEG,U_dot_body);
		printf("F_down_total: %7.4f, F_Z_aero: %7.4f, F_X: %7.4f, M_m_cg: %7.4f\n\n",F_down+Mass*Gravity,F_Z_aero,F_X,M_m_cg);

   
    
    
}
*/



