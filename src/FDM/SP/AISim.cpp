// AISim.cxx -- interface to the AI Sim
//
// Written by Erik Hofman, started November 2016
//
// Copyright (C) 2016,2017  Erik Hofman <erik@ehofman.com>
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

#define INCHES_TO_FEET	0.08333333333f
#ifndef _MINMAX
# define _MINMAX(a,b,c)  (((a)>(c)) ? (c) : (((a)<(b)) ? (b) : (a)))
#endif


FGAISim::FGAISim(double dt) :
    br(0),
    location_geod(0.0),
    NEDdot(0.0f),
    vUVW(0.0f),
    vUVWdot(0.0f),
    vPQR(0.0f),
    vPQRdot(0.0f),
    AOA(0.0f),
    AOAdot(0.0f),
    euler(0.0f),
    euler_dot(0.0f),
    wind_ned(0.0f),
    vUVWaero(0.0f),
    b_2U(0.0f),
    cbar_2U(0.0f),
    velocity(0.0f),
    mach(0.0f),
    agl(0.0f),
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
    simd4x4::zeros(xCDYLT);
    simd4x4::zeros(xClmnT);

    xCq = xCadot = 0.0f;
    xCp = xCr = 0.0f;

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
    xCp[SIDE] = CYp;
    xCr[SIDE] = CYr;
    xCp[ROLL] = Clp;
    xCr[ROLL] = Clr;
    xCp[YAW]  = Cnp;
    xCr[YAW]  = Cnr;

    xCq[LIFT] = CLq;
    xCadot[LIFT] = CLadot;

    xCq[PITCH] = Cmq;
    xCadot[PITCH] = Cmadot;

    xCDYLT.ptr()[MIN][LIFT] = CLmin;
    xCDYLT.ptr()[MIN][DRAG] = CDmin;

    inv_m = 1.0f/m;

    // calculate the initial c.g. position above ground level to position
    // the aircraft on it's wheels.
    for (int i=0; i<no_gears; ++i) {
        // convert from structural frame to body frame
        gear_pos[i] = simd4_t<float,3>(cg[X]-gear_pos[i][X], gear_pos[i][Y]-cg[Y], cg[Z]-gear_pos[i][Z])*INCHES_TO_FEET;
        if (gear_pos[i][Z] < agl) agl = gear_pos[i][Z];
    }
    // Contact point at the Genter of Gravity
    if (no_gears > (AISIM_MAX-1)) no_gears = AISIM_MAX-1;
    gear_pos[no_gears] = cg*INCHES_TO_FEET;
    Cg_spring[no_gears] = 20000.0f;
    Cg_damp[no_gears] = 2000.0f;
    no_gears++;

    simd4x4_t<float,4> mcg;
    simd4x4::unit(mcg);
    simd4x4::translate(mcg, cg);

    mI = simd4x4_t<float,4>( I[XX],  0.0f, -I[XZ], 0.0f,
                              0.0f, I[YY],   0.0f, 0.0f,
                            -I[XZ],  0.0f,  I[ZZ], 0.0f,
                              0.0f,  0.0f,   0.0f, 0.0f);
    mIinv = invert_inertia(mI);
//  mI *= mcg;
//  mIinv *= matrix_inverse(mcg);
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
    float inv_dt = 1.0f/dt;

#ifdef ENABLE_SP_FDM
    copy_to_AISim();
#endif

    /* Earth-to-Body-Axis Transformation Matrix */
    /* matrices to compensate for pitch, roll and heading */
    simd4_t<float,3> vector = euler;
    float angle = simd4::normalize(vector);
    simd4x4_t<float,4> mNed2Body = simd4x4::rotation_matrix(angle, vector);
    simd4x4_t<float,4> mBody2Ned = simd4x4::transpose(mNed2Body);
    simd4_t<float,3> gravity_body = mNed2Body*gravity_ned;

    /* Air-Relative velocity vector */
    vUVWaero = vUVW + mNed2Body*wind_ned;
    update_velocity( simd4::magnitude( simd4_t<float,2>(vUVWaero) ) );

    simd4_t<float,3> WindAxis = AOA;
    float alpha = (vUVWaero[W] == 0) ? 0 : std::atan2(vUVWaero[W], vUVWaero[U]);
    set_alpha_rad( _MINMAX(alpha, -0.0873f, 0.349f) );	// -5 to 20 degrees

    float beta = (velocity == 0) ? 0 : std::asin(vUVWaero[V]/velocity);
    set_beta_rad( _MINMAX(beta, -0.2618f, 0.2618f) );	// -15 to 15 degrees
    AOAdot = (AOA - WindAxis)*inv_dt;

printf("velocity: %f, vUVWaero: %3.2f, %3.2f, %3.2f, mach: %3.2f\n", velocity, vUVWaero[0], vUVWaero[1], vUVWaero[2], mach);
printf("agl: %5.4f, alpha: %5.4f, beta: %5.4f, adot: %5.4f, bdot: %5.4f\n", agl, AOA[ALPHA], AOA[BETA], AOAdot[ALPHA], AOAdot[BETA]);

    /* Force and Moment Coefficients */
    /* Sum all Drag, Side, Lift, Roll, Pitch, Yaw and Thrust coefficients */
    float p = vPQR[P];
    float q = vPQR[Q];
    float r = vPQR[R];
    float adot = AOAdot[ALPHA];

    /*
     * CDYL[LIFT]  = (CLq*q + CLadot*adot)*cbar_2U;
     * Clmn[PITCH] = (Cmq*q + Cmadot*adot)*cbar_2U;
     *
     * CDYL[SIDE]  = (CYp*p       + CYr*r)*b_2U;
     * Clmn[ROLL]  = (Clp*p       + Clr*r)*b_2U;
     * Clmn[YAW]   = (Cnp*p       + Cnr*r)*b_2U;
     */
    simd4_t<float,4> Ccbar2U = (xCq*q + xCadot*adot)*cbar_2U;
    simd4_t<float,4> Cb2U    = (xCp*p + xCr*r      )*b_2U;

    simd4_t<float,4> CDYL(0.0f, Cb2U[SIDE], Ccbar2U[LIFT]);
    simd4_t<float,4> Clmn(Cb2U[ROLL], Ccbar2U[PITCH], Cb2U[YAW]);
    for (int i=0; i<4; ++i) {
        CDYL += simd4_t<float,4>(xCDYLT.m4x4()[i]);
        Clmn += simd4_t<float,4>(xClmnT.m4x4()[i]);
    }
    float CL = CDYL[LIFT];
    CDYL += simd4_t<float,3>(CDi*CL*CL, 0.0f, 0.0f);
printf("CDYL:    %7.2f, %7.2f, %7.2f\n", CDYL[DRAG], CDYL[SIDE], CDYL[LIFT]);
#if 0
printf(" CLa: %6.3f, CLadot: %6.3f, CLq: %6.3f\n", xCDYLT.ptr()[ALPHA][LIFT],CLadot*adot,CLq*q);
printf(" CDa: %6.3f, CDb:    %6.3f, CDi: %6.3f\n", xCDYLT.ptr()[ALPHA][DRAG],xCDYLT.ptr()[BETA][DRAG],CDi*CL*CL);
printf(" CYb: %6.3f, CYp:    %6.3f, CYr: %6.3f\n", xCDYLT.ptr()[BETA][SIDE],CYp*p,CYr*r);
printf(" Cma: %6.3f, Cmadot: %6.3f, Cmq: %6.3f\n", xClmnT.ptr()[ALPHA][PITCH],Cmadot*adot,Cmq*q);
printf(" Clb: %6.3f, Clp:    %6.3f, Clr: %6.3f\n", xClmnT.ptr()[BETA][ROLL],Clp*p,Clr*r);
printf(" Cnb: %6.3f, Cnp:    %6.3f, Cnr: %6.3f\n", xClmnT.ptr()[BETA][YAW],Cnp*p,Cnr*r);

printf(" Cmde: %6.3f\n", xClmnT.ptr()[ELEVATOR][PITCH]);
printf(" CYdr: %6.3f, Cldr:  %6.3f, Cndr: %6.3f\n", xCDYLT.ptr()[RUDDER][SIDE], xClmnT.ptr()[RUDDER][ROLL], xClmnT.ptr()[RUDDER][YAW]);
printf(" Clda: %6.3f, CYda:  %6.3f\n", xClmnT.ptr()[AILERON][ROLL], xClmnT.ptr()[AILERON][YAW]);
printf(" Cldf: %6.3f, CDdf:  %6.3f, Cmdf: %6.3f\n", xCDYLT.ptr()[FLAPS][LIFT], xCDYLT.ptr()[FLAPS][DRAG], xClmnT.ptr()[FLAPS][PITCH]);
#endif

    /* State Accelerations (convert coefficients to forces and moments) */
    simd4_t<float,3> FDYL = CDYL*Coef2Force;
    simd4_t<float,3> Mlmn = Clmn*Coef2Moment;

    /* convert from wind axes to body axes */
    vector = AOA;
    angle = simd4::normalize(vector);
    simd4x4_t<float,4> mWindBody = simd4x4::rotation_matrix(angle, vector);
    simd4_t<float,3> FXYZ = mWindBody*FDYL;

    /* Thrust */
    for (int i=0; i<no_engines; ++i) {
        FXYZ += FT[i]*th + FTM[i]*mach;
//      Mlmn += MT*th;

printf("FDYL:    %7.2f, %7.2f, %7.2f\n", FDYL[DRAG], FDYL[SIDE], FDYL[LIFT]);
printf("FT:   %10.2f, %7.2f, MT: %7.2f\n", FT[i][X]*th, FTM[i][X]*mach, MT[i][X]*th);
    }

    /* gear forces */
    WoW = false;
    if (agl < 100.0f) {
        int WoW_main = 0;
        simd4_t<float,3> cg_agl_neu(0.0f, 0.0f, agl);
        for (int i=0; i<no_gears; ++i) {
            simd4_t<float,3> gear_ned = mBody2Ned*gear_pos[i];
            simd4_t<float,3> lg_ground_neu = gear_ned + cg_agl_neu;
            if (lg_ground_neu[Z] < 0.0f) {         // weight on wheel
                simd4_t<float,3> lg_vrot = simd4::cross(vPQR, gear_pos[i]);
                simd4_t<float,3> lg_cg_vned = mBody2Ned*lg_vrot;
                simd4_t<float,3> lg_vned = NEDdot + lg_cg_vned;
                float Fn;

                Fn = Cg_spring[i]*lg_ground_neu[Z] - Cg_damp[i]*lg_vned[Z];
                if (Fn > 0.0f) Fn = 0.0f;

                simd4_t<float,3> Fn_lg(0.0f, 0.0f, Fn);
                simd4_t<float,3> mu_body(-0.02f-0.7f*br, 0.8f, 0.0f);

                simd4_t<float,3> FLGear = mNed2Body*Fn_lg + vUVWaero*mu_body;
//              simd4_t<float,3> MLGear = simd4::cross(gear_pos[i], FLGear);
#if 0
printf("FLGear[%i]: %10.2f %10.2f %10.2f\n",i,FLGear[0], FLGear[1], FLGear[2]);
printf("MLGear[%i]: %10.2f %10.2f %10.2f\n",i,MLGear[0], MLGear[1], MLGear[2]);
#endif

                FXYZ += FLGear;
//              Mlmn += MLGear;
                if (i<3) WoW_main++;
            }
            WoW = (WoW_main == 3);
        }
    }

    /* local body accelrations */
    aXYZ = FXYZ*inv_m + gravity_body;
printf("aXYZ:    %7.2f, %7.2f, %7.2f\n", aXYZ[X]/m, aXYZ[Y]/m, aXYZ[Z]/m);
printf("Mlmn:    %7.2f, %7.2f, %7.2f\n", Mlmn[ROLL], Mlmn[PITCH], Mlmn[YAW]);

    /* Dynamic Equations */

    /* body-axis translational accelerations: forward, sideward, upward */
    vUVWdot = aXYZ - simd4::cross(vPQR, vUVW);
    vUVW += vUVWdot*dt;
 
    /* body-axis rotational accelerations: rolling, pitching, yawing */
    vPQRdot = mIinv*(Mlmn - vPQR*(mI*vPQR));
    vPQR += vPQRdot*dt;

printf("PQRdot:  %7.2f, %7.2f, %7.2f\n", vPQRdot[P], vPQRdot[Q], vPQRdot[R]);
printf("PQR:     %7.2f, %7.2f, %7.2f\n", vPQR[P], vPQR[Q], vPQR[R]);
printf("UVWdot:  %7.2f, %7.2f, %7.2f\n", vUVWdot[U], vUVWdot[V], vUVWdot[W]);
printf("UVW:     %7.2f, %7.2f, %7.2f\n", vUVW[U], vUVW[V], vUVW[W]);

    /* position of center of mass wrt earth: north, east, down */
    NEDdot = mBody2Ned*vUVW;
    simd4_t<float,3> NEDdist = NEDdot*dt;
printf("NEDdot:  %7.2f, %7.2f, %7.2f\n", NEDdot[0], NEDdot[1], NEDdot[2]);
printf("NEDdist: %7.2f, %7.2f, %7.2f\n", NEDdist[0], NEDdist[1], NEDdist[2]);

#ifdef ENABLE_SP_FDM
    double dist = simd4::magnitude( simd4_t<float,2>(NEDdist) );

    double lat2 = 0.0, lon2 = 0.0, az2 = 0.0;
    geo_direct_wgs_84( 0.0, location_geod[LATITUDE] * SGD_RADIANS_TO_DEGREES,
                            location_geod[LONGITUDE] * SGD_RADIANS_TO_DEGREES,
                            euler[PSI] * SGD_RADIANS_TO_DEGREES,
                            dist * SG_FEET_TO_METER, &lat2, &lon2, &az2 );
    set_location_geod( lat2 * SGD_DEGREES_TO_RADIANS,
                       lon2 * SGD_DEGREES_TO_RADIANS,
                       location_geod[ALTITUDE] - NEDdist[DOWN] );
//  set_heading_rad( az2 * SGD_DEGREES_TO_RADIANS );
#else
    location_geod[X] += NEDdist[X];
    location_geod[Y] += NEDdist[Y];
    location_geod[Z] -= NEDdist[Z];
    set_altitude_agl_ft(location_geod[DOWN]);
#endif
printf("GEOD:    %7.2f, %7.2f, %7.2f\n", location_geod[0], location_geod[1], location_geod[2]);

    /* angle of body wrt earth: phi (pitch), theta (roll), psi (heading) */
    float sin_p = std::sin(euler[PHI]);
    float cos_p = std::cos(euler[PHI]);
    float sin_t = std::sin(euler[THETA]);
    float cos_t = std::cos(euler[THETA]);
    if (std::abs(cos_t) < 0.00001f) cos_t = std::copysign(0.00001f,cos_t);

    euler_dot[PSI] =     (q*sin_p + r*cos_p)/cos_t;
    euler_dot[THETA] =    q*cos_p - r*sin_p;
//  euler_dot[PHI] = p + (q*sin_p + r*cos_p)*sin_t/cos_t;
    euler_dot[PHI] = p + euler_dot[PSI]*sin_t;
    euler += euler_dot*dt;
printf("euler:   %7.2f, %7.2f, %7.2f\n", euler[PHI], euler[THETA], euler[PSI]);

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

//  _set_Euler_Angles( roll, pitch, heading );
    float heading = euler[PSI];
    if (heading < 0) heading += SGD_2PI;
    _set_Euler_Angles( euler[PHI], euler[THETA], heading );
    _set_Alpha( AOA[ALPHA] );
    _set_Beta(  AOA[BETA] );

    // Velocities
    _set_V_equiv_kts( velocity*std::sqrt(sigma) * SG_FPS_TO_KT );
    _set_V_calibrated_kts( std::sqrt( 2*qbar*sigma/rho) * SG_FPS_TO_KT );
    set_V_ground_speed_kt( simd4::magnitude(NEDdot) * SG_FPS_TO_KT );
    _set_Mach_number( mach );

    _set_Velocities_Local( NEDdot[NORTH], NEDdot[EAST], NEDdot[DOWN] );
//  _set_Velocities_Local_Airmass( vUVWaero[U], vUVWaero[V], vUVWaero[W] );
    _set_Velocities_Body( vUVW[U], vUVW[V], vUVW[W] );
    _set_Omega_Body( vPQR[P], vPQR[Q], vPQR[R] );
    _set_Euler_Rates( euler_dot[PHI], euler_dot[THETA], euler_dot[PSI] );

    // Accelerations
//  set_Accels_Omega( vPQRdot[P], vPQRdot[Q], vPQRdot[R] );
    _set_Accels_Body( vUVWdot[U], vUVWdot[V], vUVWdot[W] );

    return true;
}
#endif

// ----------------------------------------------------------------------------

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
    Coef2Force = simd4_t<float,3>(-Sqbar, Sqbar, -Sqbar, Sqbar);

                                  /* Roll, Pitch,     Yaw */
    Coef2Moment = simd4_t<float,3>(Sbqbar, Sqbarcbar, Sbqbar, Sbqbar);

    /* linear interpolation for speed of sound */
    float vs = ifract*vsound[idx] + fract*vsound[idx+1];
    mach = v/vs;

    /*  useful semi-constants */
    if (v < 0.0000001f) v = 0.0000001f;
    b_2U = 0.5f*b/v;
    cbar_2U = 0.5f*cbar/v;
}

simd4x4_t<float,4>
FGAISim::matrix_inverse(simd4x4_t<float,4> mtx)
{
    simd4x4_t<float,4> dst;
    simd4_t<float,4> v1, v2;

    dst = simd4x4::transpose(mtx);

    v1 = simd4_t<float,4>(mtx.m4x4()[3]);
    v2 = simd4_t<float,4>(mtx.m4x4()[0]);
    dst.ptr()[3][0] = -simd4::dot(v1, v2);

    v2 = simd4_t<float,4>(mtx.m4x4()[1]);
    dst.ptr()[3][1] = -simd4::dot(v1, v2);

    v2 = simd4_t<float,4>(mtx.m4x4()[2]);
    dst.ptr()[3][2] = -simd4::dot(v1, v2);

    return dst;
}

simd4x4_t<float,4>
FGAISim::invert_inertia(simd4x4_t<float,4> mtx)
{
    float Ixx, Iyy, Izz, Ixz;
    float k1, k3, k4, k6;
    float denom;

    Ixx = mtx.ptr()[0][0];
    Iyy = mtx.ptr()[1][1];
    Izz = mtx.ptr()[2][2];
    Ixz = -mtx.ptr()[0][2];

    k1 = Iyy*Izz;
    k3 = Iyy*Ixz;
    denom = 1.0f/(Ixx*k1 - Ixz*k3);

    k1 *= denom;
    k3 *= denom;
    k4 = (Izz*Ixx - Ixz*Ixz)*denom;
    k6 = Ixx*Iyy*denom;

    return simd4x4_t<float,4>(   k1, 0.0f,   k3, 0.0f,
                               0.0f,   k4, 0.0f, 0.0f,
                                 k3, 0.0f,   k6, 0.0f,
                               0.0f, 0.0f, 0.0f, 0.0f );
}


bool
FGAISim::load(std::string path)
{
    /* defaults for the Cessna 172p */
    S = 174.0f;
    cbar = 4.90f;
    b = 35.8f;

    m = 2267.0f/AISIM_G;	// max: 2650.0f;

    I[XX] = 948.0f;
    I[YY] = 1346.0f;
    I[ZZ] = 1967.0f;

    // Center of Gravity and gears are in the structural frame
    //  X-axis is directed afterwards,
    //  Y-axis is directed towards the right,
    //  Z-axis is directed upwards
    //  positions are in inches
    cg[X] =  -2.2f;
    cg[Y] =   0.0f;
    cg[Z] = -22.5f;

    // gear ground contact points relative tot aero ref. pt. at (0,0,0)
    no_gears = 3;
    /* nose */
    gear_pos[0][X] = -50.0f;
    gear_pos[0][Y] =   0.0f;
    gear_pos[0][Z] = -78.9f;
    Cg_spring[0] = 1800.0f;
    Cg_damp[0] = 600.0f;
    /* left */
    gear_pos[1][X] =  15.0f;
    gear_pos[1][Y] = -43.0f;
    gear_pos[1][Z] = -74.9f;
    Cg_spring[1] = 5400.0f;
    Cg_damp[1] = 1600.0f;
    /* right */
    gear_pos[2][X] =  15.0f;
    gear_pos[2][Y] =  43.0f;
    gear_pos[2][Z] = -74.9f;
    Cg_spring[2] = 5400.0f;
    Cg_damp[2] = 1600.0f;

    float de_max = 24.0f*SGD_DEGREES_TO_RADIANS;
    float dr_max = 16.0f*SGD_DEGREES_TO_RADIANS;
    float da_max = 17.5f*SGD_DEGREES_TO_RADIANS;
    float df_max = 30.0f*SGD_DEGREES_TO_RADIANS;

    /* thuster / propulsion */
                     // (FWD,RIGHT,DOWN)
    simd4_t<float,3> dir(1.0f,0.0f,0.0f);
    no_engines = 1;

    CTmax  =  0.057f/144;
    CTu    = -0.096f;

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

#if 0
    Clb    = -0.057f;
    Clp    = -0.613f;
    Clr    =  0.079f;
    Clda_n =  0.170f*da_max;
    Cldr_n =  0.01f*dr_max;
#else
    Clb    =  0.0;
    Clp    =  0.0;
    Clr    =  0.0f;
    Clda_n =  0.0f*da_max;
    Cldr_n =  0.0f*dr_max;
#endif

    Cma    = -1.0f;
    Cmadot = -4.42f;
    Cmq    = -10.5f;
    Cmde_n = -1.05f*de_max;
    Cmdf_n = -0.059f*df_max;

#if 0
    Cnb    =  0.0630f;
    Cnp    = -0.0028f;
    Cnr    = -0.0681f;
    Cnda_n = -0.0100f*da_max;
    Cndr_n = -0.0398f*dr_max;
#else
    Cnb    =  0.0f;
    Cnp    =  0.0f;
    Cnr    =  0.0f;
    Cnda_n =  0.0f*da_max;
    Cndr_n =  0.0f*dr_max;
#endif

    return true;
}

