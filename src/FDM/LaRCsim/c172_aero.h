/*c172_aero.h*/

/*global declarations of aero model parameters*/

   SCALAR CLadot;
   SCALAR CLq;
   SCALAR CLde;
   SCALAR CLob;
  
  
   SCALAR Cdob;
   SCALAR Cda;  /*Not used*/
   SCALAR Cdde;
  
   SCALAR Cma;
   SCALAR Cmadot;
   SCALAR Cmq;
   SCALAR Cmob; 
   SCALAR Cmde;
  
   SCALAR Clbeta;
   SCALAR Clp;
   SCALAR Clr;
   SCALAR Clda;
   SCALAR Cldr;
  
   SCALAR Cnbeta;
   SCALAR Cnp;
   SCALAR Cnr;
   SCALAR Cnda;
   SCALAR Cndr;
  
   SCALAR Cybeta;
   SCALAR Cyp;
   SCALAR Cyr;
   SCALAR Cyda;
   SCALAR Cydr;
  
  /*nondimensionalization quantities*/
  /*units here are ft and lbs */
   SCALAR cbar; /*mean aero chord ft*/
   SCALAR b; /*wing span ft */
   SCALAR Sw; /*wing planform surface area ft^2*/
   SCALAR rPiARe; /*reciprocal of Pi*AR*e*/
   SCALAR lbare;  /*elevator moment arm  MAC*/
   
   SCALAR Weight; /*lbs*/
   SCALAR MaxTakeoffWeight,EmptyWeight;
   SCALAR Cg;     /*%MAC*/
   SCALAR Zcg;    /*%MAC*/
  
  
  SCALAR CLwbh,CL,cm,cd,cn,cy,croll,cbar_2V,b_2V,qS,qScbar,qSb;
  SCALAR CLo,Cdo,Cmo;
  
  SCALAR F_X_wind,F_Y_wind,F_Z_wind;
  
  SCALAR long_trim;

  
  SCALAR elevator, aileron, rudder;
  
  SCALAR Flap_Position;
  /* float Flap_Handle; */
  int Flaps_In_Transit;
  
