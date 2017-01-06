// AISim.cxx -- interface to the AI Sim
//
// Written by Erik Hofman, started November 2016
//
// Copyright (C) 2016  Erik Hofman <erik@ehofman.com>
//
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "AISim.hpp"

#include <cmath>
#include <limits>
#include <stdio.h>

#ifdef ENABLE_SP_FDM
# include <simgear/constants.h>
# include <simgear/math/simd.hxx>
# include <simgear/math/simd4x4.hxx>
# include <simgear/math/sg_geodesy.hxx>

# include <Aircraft/controls.hxx>
# include <Main/fg_props.hxx>
# include <Main/globals.hxx>
# include <FDM/flight.hxx>
#else
# include "simd.hxx"
# include "simd4x4.hxx"
#endif


FGAISim::FGAISim(double dt) :
    br(0),
    location_geod(0.0),
    NED_distance(0.0f),
    NED_body(0.0f),
    UVW_body(0.0f),
    UVW_dot(0.0f),
    PQR_body(0.0f),
    PQR_dot(0.0f),
    AOA_body(0.0f),
    AOA_dot(0.0f),
    euler_body(0.0f),
    euler_dot(0.0f),
    wind_ned(0.0f),
    UVW_aero(0.0f),
    IQR_dot_fact(0.0f),
    agl(0.0f),
    velocity(0.0f),
    mach(0.0f),
    b_2U(0.0f),
    cbar_2U(0.0f),
    WoW(true),
    no_engines(0),
    no_gears(0),
    cg(0.0f),
    I(0.0f),
    S(0.0f),
    cbar(0.0f),
    b(0.0f),
    m(0.0f),
    gravity_ned(0.0f, 0.0f, AISIM_G),
    rho(0.0f),
    qbar(0.0f),
    sigma(0.0f)
{

    for (int i=0; i<2; ++i) {
        xCqadot[i] = 0.0f;
        xCpr[i] = 0.0f;
    }
    for (int i=0; i<4; ++i) {
        xCDYL[i] = 0.0f;
        xClmn[i] = 0.0f;
    }

    for (int i=0; i<AISIM_MAX; ++i) {
        FT[i] = FTM[i] = MT[i] = 0.0f;

        gear_pos[i] = 0.0f;
        Cg_spring[i] = Cg_damp[i] = 0.0f;
    }


#ifdef ENABLE_SP_FDM
    SGPropertyNode_ptr aero = fgGetNode("sim/aero", true);
    load(aero->getStringValue());
#else
    load("");
#endif

    set_rudder_norm(0.0f);
    set_elevator_norm(0.0f);
    set_aileron_norm(0.0f);
    set_throttle_norm(0.0f);
    set_flaps_norm(0.0f);
    set_brake_norm(0.0f);

    set_velocity_fps(0.0f);
    set_alpha_rad(0.0f);
    set_beta_rad(0.0f);

    /* useful constants */
    xCpr[SIDE][0] = CYp;
    xCpr[SIDE][1] = CYr;
    xCpr[ROLL][0] = Clp;
    xCpr[ROLL][1] = Clr;
    xCpr[YAW][0]  = Cnp;
    xCpr[YAW][1]  = Cnr;
    xCqadot[LIFT][0] = CLq;
    xCqadot[LIFT][1] = CLadot;
    xCqadot[PITCH][0] = Cmq;
    xCqadot[PITCH][1] = Cmadot;
    xCDYL[MIN][LIFT] = CLmin;
    xCDYL[MIN][DRAG] = CDmin;

    // calculate the initial c.g. position above ground level to position
    // the aircraft on it's wheels.
    for (int i=0; i<no_gears; ++i) {
        if (gear_pos[i][Z] < agl) agl = gear_pos[i][Z];
    }
    // Genter of Gravity
    if (no_gears > (AISIM_MAX-1)) no_gears = AISIM_MAX-1;
    gear_pos[no_gears] = 0.0f;
    Cg_spring[no_gears] = 20000.0f;
    Cg_damp[no_gears] = 2000.0f;
    no_gears++;

    IQR_dot_fact = simd4_t<float,3>(I[ZZ]-I[YY], I[XX]-I[ZZ], I[YY]-I[XX]);
}

FGAISim::~FGAISim()
{
}

// Initialize the AISim flight model, dt is the time increment for
// each subsequent iteration through the EOM
void
FGAISim::init() {
#ifdef ENABLE_SP_FDM
    // do init common to all the FDM's           
    common_init();

    // now do init specific to the AISim
    SG_LOG( SG_FLIGHT, SG_INFO, "Starting initializing AISim" );

    // right now agl holds the lowest contact point of the aircraft
    set_Altitude( get_Altitude()-agl );
    set_location_geod( get_Latitude(), get_Longitude(), get_Altitude() );
    set_altitude_agl_ft( get_Altitude_AGL() );
    set_altitude_agl_ft( 0.0f );

    set_euler_angles_rad( get_Phi(), get_Theta(), get_Psi() );

    set_velocity_fps( fgGetFloat("sim/presets/uBody-fps"),
                      fgGetFloat("sim/presets/vBody-fps"),
                      fgGetFloat("sim/presets/wBody-fps"));
#endif
}

void
FGAISim::update(double ddt)
{
#ifdef ENABLE_SP_FDM
    if (is_suspended() || ddt == 0)
        return;
#endif

    // initialize all of AISim vars
    float dt = float(ddt);

#ifdef ENABLE_SP_FDM
    copy_to_AISim();
#endif

    /* Earth-to-Body-Axis Transformation Matrix */
    /* body = pitch, roll, yaw */
    simd4_t<float,3> vector = euler_body;
    float angle = simd4::normalize(vector);
    simd4x4_t<float,4> EB_mtx = simd4x4::rotation_matrix<float>(angle, vector);
    simd4x4_t<float,4> BE_mtx = simd4x4::transpose(EB_mtx);

    /* Air-Relative velocity vector */
    UVW_aero = UVW_body + EB_mtx*wind_ned;
    update_velocity( simd4::magnitude(UVW_aero) );
printf("velocity: %f, UVW_aero: %3.2f, %3.2f, %3.2f, mach: %3.2f\n", velocity, UVW_aero[0], UVW_aero[1], UVW_aero[2], mach);

    simd4_t<float,2> AOA_body_prev = AOA_body;
    set_alpha_rad(UVW_aero[W]==0 ? 0.0f : std::atan2(UVW_aero[W], UVW_aero[U]));
    set_beta_rad(velocity==0 ? 0.0f : std::asin(UVW_aero[V]/velocity) );
    AOA_dot = (AOA_body - AOA_body_prev)*(1/dt);
printf("agl: %5.4f, alpha: %5.4f, beta: %5.4f, adot: %5.4f, bdot: %5.4f\n", agl, AOA_body[ALPHA], AOA_body[BETA], AOA_dot[ALPHA], AOA_dot[BETA]);

    /* Force and Moment Coefficients */
    /* Sum all Drag, Side, Lift, Roll, Pitch, Yaw and Thrust coefficients */
    float p = PQR_body[P];
    float q = PQR_body[Q];
    float r = PQR_body[R];
    float adot = AOA_dot[ALPHA];

    /*
     * CDYL[LIFT]  = (CLq*q + CLadot*adot)*cbar_2U;
     * Clmn[PITCH] = (Cmq*q + Cmadot*adot)*cbar_2U;
     *
     * CDYL[SIDE]  = (CYp*p       + CYr*r)*b_2U;
     * Clmn[ROLL]  = (Clp*p       + Clr*r)*b_2U;
     * Clmn[YAW]   = (Cnp*p       + Cnr*r)*b_2U;
     */
    simd4_t<float,3> Ccbar2U = (xCqadot[0]*q + xCqadot[1]*adot)*cbar_2U;
    simd4_t<float,3> Cb2U    = (xCpr[0]*p    + xCpr[1]*r      )*b_2U;

    simd4_t<float,3> CDYL(0.0f, Cb2U[SIDE], Ccbar2U[LIFT]);
    simd4_t<float,3> Clmn(Cb2U[ROLL], Ccbar2U[PITCH], Cb2U[YAW]);
    for (int i=0; i<4; ++i) {
        CDYL += xCDYL[i];
        Clmn += xClmn[i];
    }
    float CL = CDYL[LIFT];
    CDYL += simd4_t<float,3>(CDi*CL*CL, 0.0f, 0.0f);
printf("CDYL:  %7.2f, %7.2f, %7.2f\n", CDYL[DRAG], CDYL[SIDE], CDYL[LIFT]);

    /* State Accelerations (convert coefficients to forces and moments) */
    simd4_t<float,3> FDYL = CDYL*C2F;
    simd4_t<float,3> Mlmn = Clmn*C2M;
printf("FDYL:  %7.2f, %7.2f, %7.2f\n", FDYL[DRAG], FDYL[SIDE], FDYL[LIFT]);

    /* convert from wind axes to body axes */
    vector = AOA_body;
    angle = simd4::normalize(vector);
    simd4x4_t<float,4> WB_mtx = simd4x4::rotation_matrix<float>(angle, vector);
    simd4_t<float,3>FXYZ = WB_mtx*FDYL;

    /* Thrust */
    for (int i=0; i<no_engines; ++i) {
        FXYZ += FT[i]*th + FTM[i]*mach;
//      Mlmn += MT*th;
printf("FT: %10.2f, %7.2f, MT: %7.2f\n", FT[i][X]*th, FTM[i][X]*mach, MT[i][X]*th);
    }

    /* gear forces */
#if 1
    WoW = false;
    if (agl < 100.0f) {
        int WoW_main = 0;
        simd4_t<float,3> cg_agl_neu(0.0f, 0.0f, agl);
        for (int i=0; i<no_gears; ++i) {
            simd4_t<float,3> gear_ned = BE_mtx*gear_pos[i];
            simd4_t<float,3> lg_ground_neu = gear_ned + cg_agl_neu;
            if (lg_ground_neu[Z] < 0.0f) {         // weight on wheel
                simd4_t<float,3> lg_vrot = simd4::cross(PQR_body, gear_pos[i]);
                simd4_t<float,3> lg_cg_vned = BE_mtx*lg_vrot;
                simd4_t<float,3> lg_vned = NED_body + lg_cg_vned;
                float Fn;

                Fn = Cg_spring[i]*lg_ground_neu[Z] - Cg_damp[i]*lg_vned[Z];
                if (Fn > 0.0f) Fn = 0.0f;

                simd4_t<float,3> Fn_lg(0.0f, 0.0f, Fn);
                simd4_t<float,3> mu_body(-0.02f-0.7f*br, 0.8f, 0.0f);

                simd4_t<float,3> FLGear = EB_mtx*Fn_lg + UVW_aero*mu_body;
                simd4_t<float,3> MLGear = simd4::cross(gear_pos[i], FLGear);
printf("FLGear[%i]: %10.2f %10.2f %10.2f\n",i,FLGear[0], FLGear[1], FLGear[2]);
printf("MLGear[%i]: %10.2f %10.2f %10.2f\n",i,MLGear[0], MLGear[1], MLGear[2]);

                FXYZ += FLGear;
//              Mlmn += MLGear;
                if (i<3) WoW_main++;
            }
            WoW = (WoW_main == 3);
        }
    }
#endif
printf("FXYZ/m:%7.2f, %7.2f, %7.2f\n", FXYZ[X]/m, FXYZ[Y]/m, FXYZ[Z]/m);
printf("Mlmn:  %7.2f, %7.2f, %7.2f\n", Mlmn[ROLL], Mlmn[PITCH], Mlmn[YAW]);

    /* Dynamic Equations */

    /* body-axis rotational accelerations: rolling, pitching, yawing
     *  PQR_dot[P] = (Mlmn[ROLL]  - q*r*(I[ZZ] - I[YY]))/I[XX];
     *  PQR_dot[Q] = (Mlmn[PITCH] - p*r*(I[XX] - I[ZZ]))/I[YY];
     *  PQR_dot[R] = ( Mlmn[YAW]  - p*q*(I[YY] - I[XX]))/I[ZZ];
     *                                  \-------------/
     *                                    IQR_dot_fact
     */
    simd4_t<float,3> qp(PQR_body[Q], PQR_body[P], PQR_body[P]);
    simd4_t<float,3> rq(PQR_body[R], PQR_body[R], PQR_body[Q]);
    PQR_dot = (Mlmn - qp*rq*IQR_dot_fact)/I;
    PQR_body = PQR_dot*dt;
printf("PQR:   %7.2f, %7.2f, %7.2f\n", PQR_body[P], PQR_body[Q], PQR_body[R]);

    /* body-axis translational accelerations: forward, sideward, upward
        gravity(x,y,z) = EB_mtx*(0,0,AISIM_G)
        Udot = FX/m + gravity(x) + Vbody*Rbody - Wbody*Qbody;
        Vdot = FY/m + gravity(y) + Wbody*Pbody - Ubody*Rbody;
        Wdot = FZ/m + gravity(z) + Ubody*Qbody - Vbody*Pbody;
     */
    simd4_t<float,3> gravity_body = EB_mtx*gravity_ned;
    UVW_dot = FXYZ*(1.0f/m) + gravity_body + simd4::cross(UVW_aero, PQR_body);
    UVW_body += UVW_dot*dt;


    if (WoW && UVW_body[Z] > 0) UVW_body[Z] = 0.0f;
printf("UVWdot:%7.2f, %7.2f, %7.2f\n", UVW_dot[U], UVW_dot[V], UVW_dot[W]);
printf("UVW:   %7.2f, %7.2f, %7.2f\n", UVW_body[U], UVW_body[V], UVW_body[W]);

    /* position of center of mass wrt earth: north, east, down */
    NED_body = BE_mtx*UVW_body;
    NED_distance = NED_body*dt;
printf("vNED:  %7.2f, %7.2f, %7.2f\n", NED_body[0], NED_body[1], NED_body[2]);
printf("dNED:  %7.2f, %7.2f, %7.2f\n", NED_distance[0], NED_distance[1], NED_distance[2]);

    double dist = simd4::magnitude(simd4_t<float,2>(NED_distance));
#ifdef ENABLE_SP_FDM
    double lat2 = 0.0, lon2 = 0.0, az2 = 0.0;
    geo_direct_wgs_84( 0.0, location_geod[LATITUDE] * SGD_RADIANS_TO_DEGREES,
                            location_geod[LONGITUDE] * SGD_RADIANS_TO_DEGREES,
                            euler_body[PSI] * SGD_RADIANS_TO_DEGREES,
                            dist * SG_FEET_TO_METER, &lat2, &lon2, &az2 );
    set_location_geod( lat2 * SGD_DEGREES_TO_RADIANS,
                       lon2 * SGD_DEGREES_TO_RADIANS,
                       location_geod[ALTITUDE] - NED_distance[DOWN] );
//  set_heading_rad( az2 * SGD_DEGREES_TO_RADIANS );
#else
    location_geod += NED_distance;
    set_altitude_agl_ft(location_geod[DOWN]);
#endif
printf("GEOD:  %7.2f, %7.2f, %7.2f\n", location_geod[0], location_geod[1], location_geod[2]);

    /* angle of body wrt earth: phi (pitch), theta (roll), psi (heading) */
    float sin_p = std::sin(euler_body[PHI]);
    float cos_p = std::cos(euler_body[PHI]);
    float sin_t = std::sin(euler_body[THETA]);
    float cos_t = std::cos(euler_body[THETA]);
    if (std::abs(cos_t) < 0.00001f) cos_t = std::copysign(0.00001f,cos_t);

    euler_dot[PSI] =     (q*sin_p - r*cos_p)/cos_t;
    euler_dot[THETA] =    q*cos_p - r*sin_p;
//  euler_dot[PHI] = p + (q*sin_p + r*cos_p)*sin_t/cos_t;
    euler_dot[PHI] = p + euler_dot[PSI]*sin_t;
    euler_body += euler_dot;
printf("euler: %7.2f, %7.2f, %7.2f\n", euler_body[PHI], euler_body[THETA], euler_body[PSI]);

#ifdef ENABLE_SP_FDM
    copy_from_AISim();
#endif
}

#ifdef ENABLE_SP_FDM
bool
FGAISim::copy_to_AISim()
{
    set_rudder_norm(globals->get_controls()->get_rudder());
    set_elevator_norm(globals->get_controls()->get_elevator());
    set_aileron_norm(globals->get_controls()->get_aileron());
    set_flaps_norm(globals->get_controls()->get_flaps());
    set_throttle_norm(globals->get_controls()->get_throttle(0));
    set_brake_norm(0.5f*(globals->get_controls()->get_brake_left()
                         +globals->get_controls()->get_brake_right()));

    set_altitude_asl_ft(get_Altitude());
    set_altitude_agl_ft(get_Altitude_AGL());
//  set_location_geod(get_Latitude(), get_Longitude(), location_geod[ALTITUDE]);
//  set_velocity_fps(get_V_calibrated_kts());

    return true;
}

bool
FGAISim::copy_from_AISim()
{
    // Positions
    _set_Geodetic_Position( location_geod[LATITUDE], location_geod[LONGITUDE]);

    double sl_radius, lat_geoc;
    sgGeodToGeoc(location_geod[LATITUDE], location_geod[ALTITUDE], &sl_radius,&lat_geoc);
    _set_Geocentric_Position( lat_geoc, location_geod[LONGITUDE], sl_radius);

    _update_ground_elev_at_pos();
    _set_Sea_level_radius( sl_radius * SG_METER_TO_FEET);
    _set_Altitude( location_geod[ALTITUDE] );
    _set_Altitude_AGL( location_geod[ALTITUDE] - get_Runway_altitude() );

    _set_Euler_Angles( euler_body[PHI], euler_body[THETA], euler_body[PSI] );
    _set_Alpha( AOA_body[ALPHA] );
    _set_Beta(  AOA_body[BETA] );

    // Velocities
    _set_V_equiv_kts( velocity*std::sqrt(sigma) * SG_FPS_TO_KT );
    _set_V_calibrated_kts( std::sqrt( 2*qbar*sigma/rho) * SG_FPS_TO_KT );
    set_V_ground_speed_kt( simd4::magnitude(NED_body) * SG_FPS_TO_KT );
    _set_Mach_number( mach );

    _set_Velocities_Local( NED_body[NORTH], NED_body[EAST], NED_body[DOWN] );
//  _set_Velocities_Local_Airmass( UVW_aero[U], UVW_aero[V], UVW_aero[W] );
    _set_Velocities_Body( UVW_body[U], UVW_body[V], UVW_body[W] );
    _set_Omega_Body( PQR_body[P], PQR_body[Q], PQR_body[R] );
    _set_Euler_Rates( euler_dot[PHI], euler_dot[THETA], euler_dot[PSI] );

    // Accelerations
//  set_Accels_Omega( PQR_dot[P], PQR_dot[Q], PQR_dot[R] );
    _set_Accels_Body( UVW_dot[U], UVW_dot[V], UVW_dot[W] );

    return true;
}
#endif

// ----------------------------------------------------------------------------

#define INCHES_TO_FEET	0.08333333333f
#ifndef _MINMAX
# define _MINMAX(a,b,c) (((a)>(c)) ? (c) : (((a)<(b)) ? (b) : (a)))
#endif

// 1976 Standard Atmosphere - Density (slugs/ft2): 0 - 101,000 ft
float FGAISim::density[101] = {
   0.0023771699, 0.0023083901, 0.0022411400, 0.0021753900, 0.0021111399,
   0.0020483399, 0.0019869800, 0.0019270401, 0.0018685000, 0.0018113200,
   0.0017554900, 0.0017009900, 0.0016477900, 0.0015958800, 0.0015452200,
   0.0014958100, 0.0014476100, 0.0014006100, 0.0013547899, 0.0013101200,
   0.0012665900, 0.0012241700, 0.0011828500, 0.0011426000, 0.0011034100,
   0.0010652600, 0.0010281201, 0.0009919840, 0.0009568270, 0.0009226310,
   0.0008893780, 0.0008570500, 0.0008256280, 0.0007950960, 0.0007654340,
   0.0007366270, 0.0007086570, 0.0006759540, 0.0006442340, 0.0006140020,
   0.0005851890, 0.0005577280, 0.0005315560, 0.0005066120, 0.0004828380,
   0.0004601800, 0.0004385860, 0.0004180040, 0.0003983890, 0.0003796940,
   0.0003618760, 0.0003448940, 0.0003287090, 0.0003132840, 0.0002985830,
   0.0002845710, 0.0002712170, 0.0002584900, 0.0002463600, 0.0002347990,
   0.0002237810, 0.0002132790, 0.0002032710, 0.0001937320, 0.0001846410,
   0.0001759760, 0.0001676290, 0.0001595480, 0.0001518670, 0.0001445660,
   0.0001376250, 0.0001310260, 0.0001247530, 0.0001187880, 0.0001131160,
   0.0001077220, 0.0001025920, 0.0000977131, 0.0000930725, 0.0000886582,
   0.0000844590, 0.0000804641, 0.0000766632, 0.0000730467, 0.0000696054,
   0.0000663307, 0.0000632142, 0.0000602481, 0.0000574249, 0.0000547376,
   0.0000521794, 0.0000497441, 0.0000474254, 0.0000452178, 0.0000431158,
   0.0000411140, 0.0000392078, 0.0000373923, 0.0000356632, 0.0000340162,
   0.0000324473
};

// 1976 Standard Atmosphere - Speed of sound (ft/s): 0 - 101,000 ft
float FGAISim::vsound[101] = {
   1116.450, 1112.610, 1108.750, 1104.880, 1100.990, 1097.090, 1093.180,
   1089.250, 1085.310, 1081.360, 1077.390, 1073.400, 1069.400, 1065.390,
   1061.360, 1057.310, 1053.250, 1049.180, 1045.080, 1040.970, 1036.850,
   1032.710, 1028.550, 1024.380, 1020.190, 1015.980, 1011.750, 1007.510,
   1003.240,  998.963,  994.664,  990.347,  986.010,  981.655,  977.280,
    972.885,  968.471,  968.076,  968.076,  968.076,  968.076,  968.076,
    968.076,  968.076,  968.076,  968.076,  968.076,  968.076,  968.076,
    968.076,  968.076,  968.076,  968.076,  968.076,  968.076,  968.076,
    968.076,  968.076,  968.076,  968.076,  968.076,  968.076,  968.076,
    968.076,  968.076,  968.076,  968.337,  969.017,  969.698,  970.377,
    971.056,  971.735,  972.413,  973.091,  973.768,  974.445,  975.121,
    975.797,  976.472,  977.147,  977.822,  978.496,  979.169,  979.842,
    980.515,  981.187,  981.858,  982.530,  983.200,  983.871,  984.541,
    985.210,  985.879,  986.547,  987.215,  987.883,  988.550,  989.217,
    989.883,  990.549,  991.214
};

void
FGAISim::update_velocity(float v)
{
    velocity = v;

    /* altitude related */
    float alt_idx = _MINMAX(location_geod[ALTITUDE]/1000.0f, 0, 100);
    int idx = std::floor(alt_idx);
    float fract = alt_idx - idx;
    float ifract = 1.0f - fract;

    /* linear interpolation for density */ 
    rho = ifract*density[idx] + fract*density[idx+1];
    qbar = 0.5f*rho*v*v;
    sigma = rho/density[0];

    float Sqbar = S*qbar;
    float Sbqbar = Sqbar*b;
    float Sqbarcbar = Sqbar*cbar;

                          /* Drag, Side,  Lift */
    C2F = simd4_t<float,3>(-Sqbar, Sqbar, -Sqbar);

                          /* Roll, Pitch,     Yaw */
    C2M = simd4_t<float,3>(Sbqbar, Sqbarcbar, Sbqbar);

    /* linear interpolation for speed of sound */
    float vs = ifract*vsound[idx] + fract*vsound[idx+1];
    mach = v/vs;

    /*  useful semi-constants */
    if (v < 0.0000001f) v = 0.0000001f;
    b_2U = 0.5f*b/v;
    cbar_2U = 0.5f*cbar/v;
}

bool
FGAISim::load(std::string path)
{
    /* defaults for the Cessna 172p */
    S = 174.0f;
    cbar = 4.90f;
    b = 35.8f;

    m = 2267.0f/AISIM_G;	// max: 2650.0f;
    cg[X] =  0.0f;
    cg[Y] =  0.0f;
    cg[Z] = -8.1f*INCHES_TO_FEET;

    I[XX] = 948.0f;
    I[YY] = 1346.0f;
    I[ZZ] = 1967.0f;

    // gear ground contact points relative tot aero ref. pt. at (0,0,0)
    no_gears = 3;
    /* nose */
    gear_pos[0][X] = -47.8f*INCHES_TO_FEET;
    gear_pos[0][Y] =   0.0f*INCHES_TO_FEET;
    gear_pos[0][Z] = -54.4f*INCHES_TO_FEET;
    Cg_spring[0] = 1800.0f;
    Cg_damp[0] = 600.0f;
    /* left */
    gear_pos[1][X] =  17.2f*INCHES_TO_FEET;
    gear_pos[1][Y] = -43.0f*INCHES_TO_FEET;
    gear_pos[1][Z] = -54.4f*INCHES_TO_FEET;
    Cg_spring[1] = 5400.0f;
    Cg_damp[1] = 1600.0f;
    /* right */
    gear_pos[2][X] =  17.2f*INCHES_TO_FEET;
    gear_pos[2][Y] =  43.0f*INCHES_TO_FEET;
    gear_pos[2][Z] = -54.4f*INCHES_TO_FEET;
    Cg_spring[2] = 5400.0f;
    Cg_damp[2] = 1600.0f;

    float de_max = 24.0f*SGD_DEGREES_TO_RADIANS;
    float dr_max = 16.0f*SGD_DEGREES_TO_RADIANS;
    float da_max = 17.5f*SGD_DEGREES_TO_RADIANS;
    float df_max = 30.0f*SGD_DEGREES_TO_RADIANS;

    /* thuster / propulsion */
    no_engines = 1;
    CTmax  =  0.057f/144;
    CTu    = -0.096f;

                     // (FWD,RIGHT,DOWN)
    simd4_t<float,3> dir(1.0f,0.0f,0.0f);
// if propeller driven
    float D = 75.0f*INCHES_TO_FEET;
    float RPS = 2700.0f/60.0f;
    float Ixx = 1.67f;	// propeller

    simd4::normalize(dir);
    FT[0]  = dir * (CTmax * RPS*RPS * D*D*D*D);	// Thrust force
    FTM[0] = dir * (CTu/(RPS*D))*vsound[0];	// Thrust force due to Mach
    MT[0]  = dir * (-Ixx*(2.0f*RPS*float(SGD_PI)));// Thrus moment
// if propeller driven

    /* aerodynamic coefficients */
    CLmin  =  0.307f;
    CLa    =  5.13f;
    CLadot =  1.70f;
    CLq    =  3.90f;
    CLdf_n =  0.705f*df_max;

    CDmin  =  0.027f;
    CDa    =  0.158f;
    CDb    =  0.192f;
    CDi    =  0.0446f;
    CDdf_n =  0.052f*df_max;

    CYb    = -0.31f;
    CYp    =  0.006f;
    CYr    =  0.262f;
    CYdr_n =  0.091f*dr_max;

    Clb    = -0.057f;
    Clp    = -0.613f;
    Clr    =  0.079f;
    Clda_n =  0.170f*da_max;
    Cldr_n =  0.01f*dr_max;

    Cma    = -1.0f;
    Cmadot = -4.42f;
    Cmq    = -10.5f;
    Cmde_n = -1.05f*de_max;
    Cmdf_n = -0.059f*df_max;

    Cnb    =  0.0630f;
    Cnp    = -0.0028f;
    Cnr    = -0.0681f;
    Cnda_n = -0.0100f*da_max;
    Cndr_n = -0.0398f*dr_max;

    return true;
}

