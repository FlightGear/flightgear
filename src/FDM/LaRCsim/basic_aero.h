/*basic_aero.h*/

#ifndef __BASIC_AERO_H
#define __BASIC_AERO_H



#include <FDM/LaRCsim/ls_types.h>

/*global declarations of aero model parameters*/

   extern SCALAR CLadot;
   extern SCALAR CLq;
   extern SCALAR CLde;
   extern SCALAR CLob;

   extern SCALAR CLa;
   extern SCALAR CLo;
  
  
   extern SCALAR Cdob;
   extern SCALAR Cda;  /*Not used*/
   extern SCALAR Cdde;
  
   extern SCALAR Cma;
   extern SCALAR Cmadot;
   extern SCALAR Cmq;
   extern SCALAR Cmob; 
   extern SCALAR Cmde;
  
   extern SCALAR Clbeta;
   extern SCALAR Clp;
   extern SCALAR Clr;
   extern SCALAR Clda;
   extern SCALAR Cldr;
  
   extern SCALAR Cnbeta;
   extern SCALAR Cnp;
   extern SCALAR Cnr;
   extern SCALAR Cnda;
   extern SCALAR Cndr;
  
   extern SCALAR Cybeta;
   extern SCALAR Cyp;
   extern SCALAR Cyr;
   extern SCALAR Cyda;
   extern SCALAR Cydr;
  
  /*nondimensionalization quantities*/
  /*units here are ft and lbs */
   extern SCALAR cbar; /*mean aero chord ft*/
   extern SCALAR b; /*wing span ft */
   extern SCALAR Sw; /*wing planform surface area ft^2*/
   extern SCALAR rPiARe; /*reciprocal of Pi*AR*e*/
   extern SCALAR lbare;  /*elevator moment arm  MAC*/
   
   extern SCALAR Weight; /*lbs*/
   extern SCALAR MaxTakeoffWeight,EmptyWeight;
   extern SCALAR Cg;     /*%MAC*/
   extern SCALAR Zcg;    /*%MAC*/
  
  
  extern SCALAR CLwbh,CL,cm,cd,cn,cy,croll,cbar_2V,b_2V,qS,qScbar,qSb;
  extern SCALAR CLo,Cdo,Cmo;
  
  extern SCALAR F_X_wind,F_Y_wind,F_Z_wind;
  
  extern SCALAR long_trim;

  
  extern SCALAR elevator, aileron, rudder;

  
  extern SCALAR Flap_Position;
 
  extern int Flaps_In_Transit;
  


#endif

