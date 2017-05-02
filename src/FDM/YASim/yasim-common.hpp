#ifndef _YASIM_COMMON_HPP
#define _YASIM_COMMON_HPP

namespace yasim {
  static const float YASIM_PI = 3.14159265358979323846;
  static const float PI2 = YASIM_PI*2;
  static const float RAD2DEG = 180/YASIM_PI;
  static const float DEG2RAD = YASIM_PI/180;
  static const float RPM2RAD = YASIM_PI/30;

  static const float KTS2MPS = 1852.0/3600.0;
  static const float MPS2KTS = 3600.0/1852.0;
  static const float KMH2MPS = 1/3.6;

  static const float FT2M = 0.3048;
  static const float M2FT = 1/FT2M;

  static const float LBS2N = 4.44822;
  static const float N2LB = 1/LBS2N;
  static const float LBS2KG = 0.45359237;
  static const float KG2LBS = 1/LBS2KG;
  static const float CM2GALS = 264.172037284;
  static const float HP2W = 745.700;
  static const float INHG2PA = 3386.389;
  static const float K2DEGF = 1.8;
  static const float K2DEGFOFFSET = -459.4;
  static const float CIN2CM = 1.6387064e-5;
  
  static const float NM2FTLB = (1/(LBS2N*FT2M));
  static const float SLUG2KG = 14.59390;
    
};

#endif // ifndef _YASIM_COMMON_HPP
