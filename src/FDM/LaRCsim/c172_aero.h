/*c172_aero.h*/

/*global declarations of aero model parameters*/

  static SCALAR CLadot;
  static SCALAR CLq;
  static SCALAR CLde;
  static SCALAR CLo;
  
  
  static SCALAR Cdo;
  static SCALAR Cda;  /*Not used*/
  static SCALAR Cdde;
  
  static SCALAR Cma;
  static SCALAR Cmadot;
  static SCALAR Cmq;
  static SCALAR Cmo; 
  static SCALAR Cmde;
  
  static SCALAR Clbeta;
  static SCALAR Clp;
  static SCALAR Clr;
  static SCALAR Clda;
  static SCALAR Cldr;
  
  static SCALAR Cnbeta;
  static SCALAR Cnp;
  static SCALAR Cnr;
  static SCALAR Cnda;
  static SCALAR Cndr;
  
  static SCALAR Cybeta;
  static SCALAR Cyp;
  static SCALAR Cyr;
  static SCALAR Cyda;
  static SCALAR Cydr;
  
  /*nondimensionalization quantities*/
  /*units here are ft and lbs */
  static SCALAR cbar; /*mean aero chord ft*/
  static SCALAR b; /*wing span ft */
  static SCALAR Sw; /*wing planform surface area ft^2*/
  static SCALAR rPiARe; /*reciprocal of Pi*AR*e*/
  
  
  
  SCALAR CLwbh,CL,cm,cd,cn,cy,croll,cbar_2V,b_2V,qS,qScbar,qSb,ps,rs;
  
  SCALAR F_X_wind,F_Y_wind,F_Z_wind;
  
  SCALAR long_trim;

  
  SCALAR elevator, aileron, rudder;
