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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>


//simple "one-at-a-time" longitudinal trimming routine
typedef struct
{
	double latitude,longitude,altitude;
	double vc,alpha,beta,gamma;
	double theta,phi,psi;
	double weight,cg;
	int use_gamma_tmg;
}InitialConditions;

// Units for setIC
// vc       knots (calibrated airspeed, close to indicated)
// altitude ft
// all angles in degrees
// weight lbs
// cg %MAC
// if use_gamma_tmg =1 then theta will be computed
// from theta=alpha+gamma and the value given will
// be ignored. Otherwise gamma is computed from
// gamma=theta-alpha
void setIC(InitialConditions IC)
{
	SCALAR vtfps,u,v,w,vt_east;
	SCALAR vnu,vnv,vnw,vteu,vtev,vtew,vdu,vdv,vdw;
	SCALAR alphar,betar,thetar,phir,psir,gammar;
	SCALAR sigma,ps,Ts,a;
	
	Mass=IC.weight*INVG;
	Dx_cg=(IC.cg-0.25)*4.9;
	
	Latitude=IC.latitude*DEG_TO_RAD;
	Longitude=IC.longitude*DEG_TO_RAD;
	Altitude=IC.altitude;
	ls_geod_to_geoc( Latitude, Altitude, &Sea_level_radius, &Lat_geocentric);
	
	ls_atmos(IC.altitude,&sigma,&a,&Ts,&ps);
	vtfps=sqrt(1/sigma*IC.vc*IC.vc)*1.68781;
	alphar=IC.alpha*DEG_TO_RAD;
	betar=IC.beta*DEG_TO_RAD;
	gammar=IC.gamma*DEG_TO_RAD;
	
	
	phir=IC.phi*DEG_TO_RAD;
	psir=IC.psi*DEG_TO_RAD;
	
	if(IC.use_gamma_tmg == 1)
	{
	   thetar=alphar+gammar;
	}
	else
	{
	   thetar=IC.theta*DEG_TO_RAD;
	   gammar=thetar-alphar;
	}   	   
    
	u=vtfps*cos(alphar)*cos(betar);
	v=vtfps*sin(betar);
	w=vtfps*sin(alphar)*cos(betar);
	
	vnu=u*cos(thetar)*cos(psir);
	vnv=v*(-sin(psir)*cos(phir)+sin(phir)*sin(thetar)*cos(psir));
	vnw=w*(sin(phir)*sin(psir)+cos(phir)*sin(thetar)*cos(psir));
	
	V_north=vnu+vnv+vnw;
	
	vteu=u*cos(thetar)*sin(psir);
	vtev=v*(cos(phir)*cos(psir)+sin(phir)*sin(thetar)*sin(psir));
	vtew=w*(-sin(phir)*cos(psir)+cos(phir)*sin(thetar)*sin(psir));
	
	vt_east=vteu+vtev+vtew;
	V_east=vt_east+ OMEGA_EARTH*Sea_level_radius*cos(Lat_geocentric);
 
    vdu=u*-sin(thetar);
	vdv=v*cos(thetar)*sin(phir);
	vdw=w*cos(thetar)*cos(phir);
	
	V_down=vdu+vdv+vdw;
	
	Theta=thetar;
	Phi=phir;
	Psi=psir;

}
	

int trim_long(int kmax, InitialConditions IC)
{
	double elevator,alpha;
	double tol=1E-3;
	double a_tol=tol/10;
	double alpha_step=0.001;
	int k=0,i,j=0,jmax=10,sum=0;
	ls_loop(0.0,-1);
	do{
		//printf("k: %d\n",k);
		while((fabs(W_dot_body) > tol) && (j < jmax))
		{
            
			IC.alpha+=W_dot_body*0.05;
			if((IC.alpha < -5) || (IC.alpha > 21))
			   j=jmax;
			setIC(IC);
            ls_loop(0.0,-1);
/* 			printf("IC.alpha: %g, Alpha: %g, wdot: %g\n",IC.alpha,Alpha*RAD_TO_DEG,W_dot_body);
 */			j++;
		}
		sum+=j;
/* 		printf("\tTheta: %7.4f, Alpha: %7.4f, wdot: %10.6f, j: %d\n",Theta*RAD_TO_DEG,Alpha*RAD_TO_DEG,W_dot_body,j);
 */		j=0;
		while((fabs(U_dot_body) > tol) && (j < jmax))
		{

			Throttle_pct-=U_dot_body*0.005;
			if((Throttle_pct < 0) || (Throttle_pct > 1))
				Throttle_pct=0.2;
			setIC(IC);
            ls_loop(0.0,-1);
			j++;
		}
		sum+=j;
/* 		printf("\tThrottle_pct: %7.4f, udot: %10.6f, j: %d\n",Throttle_pct,U_dot_body,j);
 */        j=0;
		while((fabs(Q_dot_body) > a_tol) && (j < jmax))
		{

            Long_control+=Q_dot_body*0.001;
			if((Long_control < -1) || (Long_control > 1))
				j=jmax;
            setIC(IC);
			ls_loop(0.0,-1);
			j++;
		}
		sum+=j;
		if(Long_control >= 0)
			elevator=Long_control*23;
		else
			elevator=Long_control*28;	
/* 		printf("\televator: %7.4f, qdot: %10.6f, j: %d\n",elevator,Q_dot_body,j);
 */        k++;j=0;
    }while(((fabs(W_dot_body) > tol) || (fabs(U_dot_body) > tol) || (fabs(Q_dot_body) > tol)) && (k < kmax));
	/* printf("Total Iterations: %d\n",sum); */
	return k;			 		
}

int trim_ground(int kmax, InitialConditions IC)
{
	double elevator,alpha,qdot_prev,alt_prev,step;
	double tol=1E-3;
	double a_tol=tol/10;
	double alpha_step=0.001;
	int k=0,i,j=0,jmax=40,sum=0,m=0;
	Throttle_pct=0;
	Brake_pct=1;
	Theta=5*DEG_TO_RAD;
	IC.altitude=Runway_altitude;
	printf("udot: %g\n",U_dot_body);
	setIC(IC);
	printf("Altitude: %g, Runway_altitude: %g\n",Altitude,Runway_altitude);
	qdot_prev=1.0E6;
	
	ls_loop(0.0,-1);
	
	do{
		//printf("k: %d\n",k);
		step=1;
	    printf("IC.altitude: %g, Altitude: %g, Runway_altitude: %g,wdot: %g,F_Z_gear: %g, M_m_gear: %g,F_Z: %g\n",IC.altitude,Altitude,Runway_altitude,W_dot_body,F_Z_gear,M_m_gear,F_Z);

		m=0;
		while((fabs(W_dot_body) > tol) && (m < 10))
		{
			
			j=0;
			
			do{
				alt_prev=IC.altitude;
				IC.altitude+=step;
				setIC(IC);
            	ls_loop(0.0,-1);
				printf("IC.altitude: %g, Altitude: %g, Runway_altitude: %g,wdot: %g,F_Z: %g\n",IC.altitude,Altitude,Runway_altitude,W_dot_body,F_Z);
				j++;
			}while((W_dot_body < 0) && (j < jmax));
			IC.altitude-=step;
			step/=10;
			printf("step: %g\n",step);
			m++;
			
		}	
		sum+=j;
        printf("IC.altitude: %g, Altitude: %g, Runway_altitude: %g,wdot: %g,F_Z_gear: %g, M_m_gear: %g,F_Z: %g\n",IC.altitude,Altitude,Runway_altitude,W_dot_body,F_Z_gear,M_m_gear,F_Z);

        j=0;
		
		while((Q_dot_body <= qdot_prev) && (j < jmax))
		{

            
			qdot_prev=Q_dot_body;
			IC.theta+=Q_dot_body;
            setIC(IC);
			ls_loop(0.0,-1);
			j++;
			
			printf("\tTheta: %7.4f, qdot: %10.6f, qdot_prev: %10.6f, j: %d\n",Theta*RAD_TO_DEG,Q_dot_body,qdot_prev,j);
		}
		IC.theta-=qdot_prev;
		sum+=j;
		
		printf("\tTheta: %7.4f, qdot: %10.6f, W_dot_body: %g\n",Theta,Q_dot_body,W_dot_body);
        j=0;
		if(W_dot_body > tol)
		{
			step=1;
			while((W_dot_body > 0) && (j <jmax))
			{
				IC.altitude-=step;
				setIC(IC);
				ls_loop(0.0,-1);
				j++;
			}
		}		
		k++;j=0;
    }while(((fabs(W_dot_body) > tol) || (fabs(Q_dot_body) > tol)) && (k < kmax));
	printf("Total Iterations: %d\n",sum);
	return k;			 		
}
void do_trims(int kmax,FILE *out,InitialConditions IC)
{
	int k=0,i;
	double speed,elevator,cmcl;
	out=fopen("trims.out","w");
	speed=55;
	
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
		
		speed=50;
		while(speed <= 150)
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
	   			fprintf(out,"%g,%g,%g,%g,%g,%d",V_calibrated_kts,Alpha*RAD_TO_DEG,Long_control,Throttle_pct,Gamma_vert_rad,k);
	   			fprintf(out,",%g,%g,%g,%g,%g\n",CL,cm,cmcl,Weight,Cg);
/* 	   			printf("%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",V_calibrated_kts,Alpha*RAD_TO_DEG,elevator,CL,cm,Cmo,Cma,Cmde,Mass*32.174,Dx_cg);
 */		   }	
    	   else
		   {
	   		 printf("kmax exceeded at: %g knots, %g lbs, %g %%MAC\n",V_calibrated_kts,Weight,Cg);
			 printf("wdot: %g, udot: %g, qdot: %g\n\n",W_dot_body,U_dot_body,Q_dot_body);

		   }
		   speed+=10;	  
		}
    }
	fclose(out);
}	

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
/* 		 printf("Time: %7.4f, Alt: %7.4f, Alpha: %7.4f, pelev: %7.4f, qdot: %7.4f, udot: %7.4f, Phi: %7.4f, Psi: %7.4f\n",time,Altitude,Alpha*RAD_TO_DEG,Long_control*100,Q_body*RAD_TO_DEG,U_dot_body,Phi,Psi);
         printf("Mcg: %7.4f, Mrp: %7.4f, Maero: %7.4f, Meng: %7.4f, Mgear: %7.4f, Dx_cg: %7.4f, Dz_cg: %7.4f\n\n",M_m_cg,M_m_rp,M_m_aero,M_m_engine,M_m_gear,Dx_cg,Dz_cg);
 */		 fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,",time,V_true_kts,Theta*RAD_TO_DEG,Alpha*RAD_TO_DEG,Q_body*RAD_TO_DEG,Alpha_dot*RAD_TO_DEG,Q_dot_body*RAD_TO_DEG,Throttle_pct,elevator*RAD_TO_DEG);
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
/* 		 printf("Time: %7.4f, Alt: %7.4f, Alpha: %7.4f, pelev: %7.4f, qdot: %7.4f, udot: %7.4f, Phi: %7.4f, Psi: %7.4f\n",time,Altitude,Alpha*RAD_TO_DEG,Long_control*100,Q_body*RAD_TO_DEG,U_dot_body,Phi,Psi);
         printf("Mcg: %7.4f, Mrp: %7.4f, Maero: %7.4f, Meng: %7.4f, Mgear: %7.4f, Dx_cg: %7.4f, Dz_cg: %7.4f\n\n",M_m_cg,M_m_rp,M_m_aero,M_m_engine,M_m_gear,Dx_cg,Dz_cg);
 */		 fprintf(out,"%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,%20.8f,",time,V_true_kts,Theta*RAD_TO_DEG,Alpha*RAD_TO_DEG,Q_body*RAD_TO_DEG,Alpha_dot*RAD_TO_DEG,Q_dot_body*RAD_TO_DEG,Throttle_pct,elevator*RAD_TO_DEG);
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
	 	 
		//attempt to maintain zero pitch rate during the roll
		while((V_calibrated_kts < 61) && (time < 30.0))
		{
			/* herrprev=herr;*/
			ls_update(1);
			/*herr=Q_body-htarget;
			herr_diff=herr-herrprev;
			Long_control=elev_trim+(hgain*herr + hdiffgain*herr_diff); */
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

// Initialize the LaRCsim flight model, dt is the time increment for
// each subsequent iteration through the EOM
int fgLaRCsimInit(double dt) {
    ls_toplevel_init(dt);

    return(1);
}



// Run an iteration of the EOM (equations of motion)
int main(int argc, char *argv[]) {
    
	
	double save_alt = 0.0;
    int multiloop=1,k=0,i;
	double time=0,elev_trim,elev_trim_save,elevator,speed,cmcl;
	FILE *out;
	double hgain,hdiffgain,herr,herrprev,herr_diff,htarget;
	InitialConditions IC;
    
    if(argc < 6)
	{
	    printf("Need args: $c172 speed alt alpha elev throttle\n");
		exit(1);
	}	
	
	IC.latitude=47.5299892; //BFI
	IC.longitude=122.3019561;
	Runway_altitude =   18.0;
	IC.altitude=strtod(argv[2],NULL); 
	IC.vc=strtod(argv[1],NULL);
	IC.alpha=10;
	IC.beta=0;
	IC.theta=strtod(argv[3],NULL);
	IC.use_gamma_tmg=0;
	IC.phi=0;
	IC.psi=0;
	IC.weight=1500;
	IC.cg=0.155;
	Long_control=strtod(argv[4],NULL);
    setIC(IC);
	printf("Out setIC\n");
	ls_ForceAltitude(IC.altitude);  
    fgLaRCsimInit(0.01);
	
	while(IC.alpha < 30.0)
	{
		setIC(IC);
		ls_loop(0.0,-1);
		printf("CL: %g ,Alpha: %g\n",CL,IC.alpha);
		IC.alpha+=1.0;
	}
	
	/*trim_ground(10,IC);*/
	/* printf("%g,%g\n",Theta,Gamma_vert_rad); 
	printf("trim_long():\n");
	k=trim_long(200,IC);
	Throttle_pct=Throttle_pct-0.2;
	printf("%g,%g\n",Theta,Gamma_vert_rad); 
	out=fopen("dive.out","w");
	time=0;
	while(time < 30.0)
	{
			ls_update(1);
			
			cmcl=cm/CL;
			fprintf(out,"%g,%g,%g,%g,%g,%d",V_calibrated_kts,Alpha*RAD_TO_DEG,Long_control,Throttle_pct,Gamma_vert_rad,k);
	   		fprintf(out,",%g,%g,%g\n",CL,cm,cmcl);
			time+=0.01;
    }
	fclose(out);
	printf("V_rel_wind: %8.2f, Alpha: %8.2f, Beta: %8.2f\n",V_rel_wind,Alpha*RAD_TO_DEG,Beta*RAD_TO_DEG);
	printf("Theta: %8.2f, Gamma: %8.2f, Alpha_tmg: %8.2f\n",Theta*RAD_TO_DEG,Gamma_vert_rad*RAD_TO_DEG,Theta*RAD_TO_DEG-Gamma_vert_rad*RAD_TO_DEG);
	printf("V_north: %8.2f, V_east_rel_ground: %8.2f, V_east: %8.2f, V_down: %8.2f\n",V_north,V_east_rel_ground,V_east,V_down);
	printf("Long_control:  %8.2f, Throttle_pct: %8.2f\n",Long_control,Throttle_pct);
	printf("k: %d, udot: %8.4f, wdot: %8.4f, qdot: %8.5f\n",k,U_dot_body,W_dot_body,Q_dot_body);
    
	printf("\nls_update():\n");
	ls_update(1);
	printf("V_rel_wind: %8.2f, Alpha: %8.2f, Beta: %8.2f\n",V_rel_wind,Alpha*RAD_TO_DEG,Beta*RAD_TO_DEG);
	printf("Theta: %8.2f, Gamma: %8.2f, Alpha_tmg: %8.2f\n",Theta*RAD_TO_DEG,Gamma_vert_rad*RAD_TO_DEG,Theta*RAD_TO_DEG-Gamma_vert_rad*RAD_TO_DEG);
 */
	
    /* Inform LaRCsim of the local terrain altitude */
    
    
    
    return 1;
}


/*// Convert from the FGInterface struct to the LaRCsim generic_ struct
int FGInterface_2_LaRCsim (FGInterface& f) {

    Mass =      f.get_Mass();
    I_xx =      f.get_I_xx();
    I_yy =      f.get_I_yy();
    I_zz =      f.get_I_zz();
    I_xz =      f.get_I_xz();
    // Dx_pilot =  f.get_Dx_pilot();
    // Dy_pilot =  f.get_Dy_pilot();
    // Dz_pilot =  f.get_Dz_pilot();
    Dx_cg =     f.get_Dx_cg();
    Dy_cg =     f.get_Dy_cg();
    Dz_cg =     f.get_Dz_cg();
    // F_X =       f.get_F_X();
    // F_Y =       f.get_F_Y();
    // F_Z =       f.get_F_Z();
    // F_north =   f.get_F_north();
    // F_east =    f.get_F_east();
    // F_down =    f.get_F_down();
    // F_X_aero =  f.get_F_X_aero();
    // F_Y_aero =  f.get_F_Y_aero();
    // F_Z_aero =  f.get_F_Z_aero();
    // F_X_engine =        f.get_F_X_engine();
    // F_Y_engine =        f.get_F_Y_engine();
    // F_Z_engine =        f.get_F_Z_engine();
    // F_X_gear =  f.get_F_X_gear();
    // F_Y_gear =  f.get_F_Y_gear();
    // F_Z_gear =  f.get_F_Z_gear();
    // M_l_rp =    f.get_M_l_rp();
    // M_m_rp =    f.get_M_m_rp();
    // M_n_rp =    f.get_M_n_rp();
    // M_l_cg =    f.get_M_l_cg();
    // M_m_cg =    f.get_M_m_cg();
    // M_n_cg =    f.get_M_n_cg();
    // M_l_aero =  f.get_M_l_aero();
    // M_m_aero =  f.get_M_m_aero();
    // M_n_aero =  f.get_M_n_aero();
    // M_l_engine =        f.get_M_l_engine();
    // M_m_engine =        f.get_M_m_engine();
    // M_n_engine =        f.get_M_n_engine();
    // M_l_gear =  f.get_M_l_gear();
    // M_m_gear =  f.get_M_m_gear();
    // M_n_gear =  f.get_M_n_gear();
    // V_dot_north =       f.get_V_dot_north();
    // V_dot_east =        f.get_V_dot_east();
    // V_dot_down =        f.get_V_dot_down();
    // U_dot_body =        f.get_U_dot_body();
    // V_dot_body =        f.get_V_dot_body();
    // W_dot_body =        f.get_W_dot_body();
    // A_X_cg =    f.get_A_X_cg();
    // A_Y_cg =    f.get_A_Y_cg();
    // A_Z_cg =    f.get_A_Z_cg();
    // A_X_pilot = f.get_A_X_pilot();
    // A_Y_pilot = f.get_A_Y_pilot();
    // A_Z_pilot = f.get_A_Z_pilot();
    // N_X_cg =    f.get_N_X_cg();
    // N_Y_cg =    f.get_N_Y_cg();
    // N_Z_cg =    f.get_N_Z_cg();
    // N_X_pilot = f.get_N_X_pilot();
    // N_Y_pilot = f.get_N_Y_pilot();
    // N_Z_pilot = f.get_N_Z_pilot();
    // P_dot_body =        f.get_P_dot_body();
    // Q_dot_body =        f.get_Q_dot_body();
    // R_dot_body =        f.get_R_dot_body();
    V_north =   f.get_V_north();
    V_east =    f.get_V_east();
    V_down =    f.get_V_down();
    // V_north_rel_ground =        f.get_V_north_rel_ground();
    // V_east_rel_ground = f.get_V_east_rel_ground();
    // V_down_rel_ground = f.get_V_down_rel_ground();
    // V_north_airmass =   f.get_V_north_airmass();
    // V_east_airmass =    f.get_V_east_airmass();
    // V_down_airmass =    f.get_V_down_airmass();
    // V_north_rel_airmass =       f.get_V_north_rel_airmass();
    // V_east_rel_airmass =        f.get_V_east_rel_airmass();
    // V_down_rel_airmass =        f.get_V_down_rel_airmass();
    // U_gust =    f.get_U_gust();
    // V_gust =    f.get_V_gust();
    // W_gust =    f.get_W_gust();
    // U_body =    f.get_U_body();
    // V_body =    f.get_V_body();
    // W_body =    f.get_W_body();
    // V_rel_wind =        f.get_V_rel_wind();
    // V_true_kts =        f.get_V_true_kts();
    // V_rel_ground =      f.get_V_rel_ground();
    // V_inertial =        f.get_V_inertial();
    // V_ground_speed =    f.get_V_ground_speed();
    // V_equiv =   f.get_V_equiv();
    // V_equiv_kts =       f.get_V_equiv_kts();
    // V_calibrated =      f.get_V_calibrated();
    // V_calibrated_kts =  f.get_V_calibrated_kts();
    P_body =    f.get_P_body();
    Q_body =    f.get_Q_body();
    R_body =    f.get_R_body();
    // P_local =   f.get_P_local();
    // Q_local =   f.get_Q_local();
    // R_local =   f.get_R_local();
    // P_total =   f.get_P_total();
    // Q_total =   f.get_Q_total();
    // R_total =   f.get_R_total();
    // Phi_dot =   f.get_Phi_dot();
    // Theta_dot = f.get_Theta_dot();
    // Psi_dot =   f.get_Psi_dot();
    // Latitude_dot =      f.get_Latitude_dot();
    // Longitude_dot =     f.get_Longitude_dot();
    // Radius_dot =        f.get_Radius_dot();
    Lat_geocentric =    f.get_Lat_geocentric();
    Lon_geocentric =    f.get_Lon_geocentric();
    Radius_to_vehicle = f.get_Radius_to_vehicle();
    Latitude =  f.get_Latitude();
    Longitude = f.get_Longitude();
    Altitude =  f.get_Altitude();
    Phi =       f.get_Phi();
    Theta =     f.get_Theta();
    Psi =       f.get_Psi();
    // T_local_to_body_11 =        f.get_T_local_to_body_11();
    // T_local_to_body_12 =        f.get_T_local_to_body_12();
    // T_local_to_body_13 =        f.get_T_local_to_body_13();
    // T_local_to_body_21 =        f.get_T_local_to_body_21();
    // T_local_to_body_22 =        f.get_T_local_to_body_22();
    // T_local_to_body_23 =        f.get_T_local_to_body_23();
    // T_local_to_body_31 =        f.get_T_local_to_body_31();
    // T_local_to_body_32 =        f.get_T_local_to_body_32();
    // T_local_to_body_33 =        f.get_T_local_to_body_33();
    // Gravity =   f.get_Gravity();
    // Centrifugal_relief =        f.get_Centrifugal_relief();
    // Alpha =     f.get_Alpha();
    // Beta =      f.get_Beta();
    // Alpha_dot = f.get_Alpha_dot();
    // Beta_dot =  f.get_Beta_dot();
    // Cos_alpha = f.get_Cos_alpha();
    // Sin_alpha = f.get_Sin_alpha();
    // Cos_beta =  f.get_Cos_beta();
    // Sin_beta =  f.get_Sin_beta();
    // Cos_phi =   f.get_Cos_phi();
    // Sin_phi =   f.get_Sin_phi();
    // Cos_theta = f.get_Cos_theta();
    // Sin_theta = f.get_Sin_theta();
    // Cos_psi =   f.get_Cos_psi();
    // Sin_psi =   f.get_Sin_psi();
    // Gamma_vert_rad =    f.get_Gamma_vert_rad();
    // Gamma_horiz_rad =   f.get_Gamma_horiz_rad();
    // Sigma =     f.get_Sigma();
    // Density =   f.get_Density();
    // V_sound =   f.get_V_sound();
    // Mach_number =       f.get_Mach_number();
    // Static_pressure =   f.get_Static_pressure();
    // Total_pressure =    f.get_Total_pressure();
    // Impact_pressure =   f.get_Impact_pressure();
    // Dynamic_pressure =  f.get_Dynamic_pressure();
    // Static_temperature =        f.get_Static_temperature();
    // Total_temperature = f.get_Total_temperature();
    Sea_level_radius =  f.get_Sea_level_radius();
    Earth_position_angle =      f.get_Earth_position_angle();
    Runway_altitude =   f.get_Runway_altitude();
    // Runway_latitude =   f.get_Runway_latitude();
    // Runway_longitude =  f.get_Runway_longitude();
    // Runway_heading =    f.get_Runway_heading();
    // Radius_to_rwy =     f.get_Radius_to_rwy();
    // D_cg_north_of_rwy = f.get_D_cg_north_of_rwy();
    // D_cg_east_of_rwy =  f.get_D_cg_east_of_rwy();
    // D_cg_above_rwy =    f.get_D_cg_above_rwy();
    // X_cg_rwy =  f.get_X_cg_rwy();
    // Y_cg_rwy =  f.get_Y_cg_rwy();
    // H_cg_rwy =  f.get_H_cg_rwy();
    // D_pilot_north_of_rwy =      f.get_D_pilot_north_of_rwy();
    // D_pilot_east_of_rwy =       f.get_D_pilot_east_of_rwy();
    // D_pilot_above_rwy = f.get_D_pilot_above_rwy();
    // X_pilot_rwy =       f.get_X_pilot_rwy();
    // Y_pilot_rwy =       f.get_Y_pilot_rwy();
    // H_pilot_rwy =       f.get_H_pilot_rwy();

    return( 0 );
}
*/



