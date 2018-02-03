#ifndef _YASIM_COMMON_HPP
#define _YASIM_COMMON_HPP

#include <string>
#include <vector>
#include <algorithm>
/*
 common file for YASim wide constants and static helper functions
 */
namespace yasim {
    static const float YASIM_PI = 3.14159265358979323846f;
    static const float PI2 = YASIM_PI*2;
    static const float RAD2DEG = 180/YASIM_PI;
    static const float DEG2RAD = YASIM_PI/180;
    static const float RPM2RAD = YASIM_PI/30;

    static const float KTS2MPS = 1852.0f/3600.0f;
    static const float MPS2KTS = 3600.0f/1852.0f;
    static const float KMH2MPS = 1/3.6f;

    static const float FT2M = 0.3048f;
    static const float M2FT = 1/FT2M;

    static const float LBS2N = 4.44822f;
    static const float N2LB = 1/LBS2N;
    static const float KG2N = 9.81f;
    static const float LBS2KG = 0.45359237f;
    static const float KG2LBS = 1/LBS2KG;
    static const float CM2GALS = 264.172037284f;
    static const float HP2W = 745.700f;
    static const float INHG2PA = 3386.389f;
    static const float K2DEGF = 1.8f;
    static const float K2DEGFOFFSET = -459.4f;
    static const float CIN2CM = 1.6387064e-5f;
    
    static const float NM2FTLB = (1/(LBS2N*FT2M));
    static const float SLUG2KG = 14.59390f;

    static const float INCIDENCE_MIN = -20*DEG2RAD;
    static const float INCIDENCE_MAX = 20*DEG2RAD;
}; //namespace yasim

#endif // ifndef _YASIM_COMMON_HPP
