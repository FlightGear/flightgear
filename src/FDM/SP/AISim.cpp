// AISim.cxx -- interface to the AI Sim
//
// Written by Erik Hofman, started November 2016
//
// Copyright (C) 2016-2020 by Erik Hofman <erik@ehofman.com>
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

#include <fenv.h>

#include <cmath>
#include <limits>
#include <cstdio>

#include <string>
#include <fstream>
#include <streambuf>

#ifdef ENABLE_SP_FDM
# include <simgear/constants.h>
# include <simgear/math/simd.hxx>
# include <simgear/math/simd4x4.hxx>
# include <simgear/math/sg_geodesy.hxx>

# include <Aircraft/controls.hxx>
# include <Main/fg_props.hxx>
# include <Main/globals.hxx>
# include <FDM/flight.hxx>
# include <cJSON.h>
#else
# include "simd.hxx"
# include "simd4x4.hxx"
#endif

#define FEET_TO_INCHES       12.0f
#define INCHES_TO_FEET       (1.0f/FEET_TO_INCHES)

FGAISim::FGAISim(double dt)
{
    simd4x4::zeros(xCDYLT);
    simd4x4::zeros(xClmnT);

    for (size_t i=0; i<AISIM_MAX; ++i)
    {
        contact_pos[i] = 0.0f;
        contact_spring[i] = contact_damp[i] = 0.0f;
        FT[i] = MT[i] = 0.0f;
    }

#ifdef ENABLE_SP_FDM
    SGPath aircraft_path( fgGetString("/sim/fg-root") );
    SGPropertyNode_ptr aero = fgGetNode("sim/aero", true);
    aircraft_path.append("Aircraft-aisim");
    aircraft_path.append(aero->getStringValue());
    aircraft_path.concat(".json");

    load(aircraft_path.str());
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

    /* useful constants assigned to a vector */
    xCp[SIDE] = CYp;
    xCr[SIDE] = CYr;
    xCp[ROLL] = Clp;
    xCr[ROLL] = Clr;
    xCp[YAW]  = Cnp;
    xCr[YAW]  = Cnr;

    xCq[LIFT] = -CLq;
    xCadot[LIFT] = -CLadot;

    xCq[PITCH] = Cmq;
    xCadot[PITCH] = Cmadot;

    xCDYLT.ptr()[MIN][LIFT] = -CLmin;
    xCDYLT.ptr()[MIN][DRAG] = -CDmin;

    /* m is assigned in the load function */
    inv_mass = aiVec3(1.0f)/mass;

    // calculate aircraft position when resting on the landing gear.
    float cg_agl = -cg[Z];
    if (no_contacts)
    {
        for (size_t i=0; i<no_contacts; ++i) {
            cg_agl += contact_pos[i][Z];
        }
        cg_agl /= static_cast<float>(no_contacts);
    }
    set_altitude_agl_ft(cg_agl);

    // Contact point at the center of gravity
    contact_pos[no_contacts] = 0.0f;
    contact_spring[no_contacts] = -20000.0f;
    contact_damp[no_contacts] = -2000.0f;
    no_contacts++;

    aiMtx4 mcg;
    simd4x4::unit(mcg);
    simd4x4::translate(mcg, cg);

    mJ = aiMtx4( I[XX],  0.0f, -I[XZ], 0.0f,
                  0.0f, I[YY],   0.0f, 0.0f,
                -I[XZ],  0.0f,  I[ZZ], 0.0f,
                  0.0f,  0.0f,   0.0f, 0.0f);
    mJinv = invert_inertia(mJ);
    mJ *= mcg;
    mJinv *= matrix_inverse(mcg);
}

FGAISim::~FGAISim()
{
}

// Initialize the AISim flight model, dt is the time increment for
// each subsequent iteration through the EOM
void
FGAISim::init()
{
//  feenableexcept(FE_INVALID | FE_OVERFLOW);
#ifdef ENABLE_SP_FDM
    // do init common to all the FDM's
    common_init();

    // now do init specific to the AISim
    SG_LOG( SG_FLIGHT, SG_INFO, "Starting initializing AISim" );

    // cg_agl holds the lowest contact point of the aircraft
    set_Altitude( get_Altitude() + cg_agl );
    set_location_geod( get_Latitude(), get_Longitude(), get_Altitude() );
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
    aiVec3 dt(ddt);

#ifdef ENABLE_SP_FDM
    copy_to_AISim();
#endif

#if 0
 printf("pitch:      % 7.5f, roll: % 7.5f, heading: % 7.5f\n", euler[THETA], euler[PHI], euler[PSI]);
 printf("wind north: % 7.5f, east: % 7.5f, down:    % 7.5f\n", wind_ned[0], wind_ned[1], wind_ned[2]);
#endif

    /* Earth-to-Body-Axis Transformation Matrix */
    /* matrices to compensate for pitch, roll and heading */
    aiVec3 vector = euler; // simd4::normalize alters the vector
    float  len = simd4::normalize(vector);
    aiMtx4 mNed2Body = simd4x4::rotation_matrix(len, vector);
    aiMtx4 mBody2Ned = simd4x4::transpose(mNed2Body);
    aiVec3 wind = mNed2Body*wind_ned;

    /* Air-Relative velocity vector */
    vUVWaero = vUVW + wind;
    update_velocity( simd4::magnitude( vUVWaero ) );

    /* Wind angles */
    aiVec3 prevAOA = AOA;
    alpha = (vUVWaero[U] == 0.0f) ? 0.0f : std::atan2(vUVWaero[W], vUVWaero[U]);
    set_alpha_rad( alpha );

    beta = (velocity == 0.0f) ? 0.0f : std::asin(vUVWaero[V]/velocity);
    set_beta_rad( beta );

    /* set_alpha_rad and set_beta_rad set the new AOA */
    AOAdot = (AOA - prevAOA)/dt;

#if 0
 printf("velocity: %f, vUVWaero: %3.2f, %3.2f, %3.2f, mach: %3.2f\n", velocity, vUVWaero[U], vUVWaero[V], vUVWaero[W], mach);
 printf("cg_agl: %5.4f, alpha: %5.4f, beta: %5.4f, adot: %5.4f, bdot: %5.4f\n", cg_agl, AOA[ALPHA], AOA[BETA], AOAdot[ALPHA], AOAdot[BETA]);
#endif

    /* Force and Moment Coefficients */
    /* Sum all Drag, Side, Lift, Roll, Pitch and Yaw and Thrust coefficients */
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
    aiVec4 Ccbar2U = (xCq*q + xCadot*adot)*cbar_2U;
    aiVec4 Cb2U    = (xCp*p       + xCr*r)*b_2U;

    /* xCDYLT and xClmnT already have their factors applied */
    /* in the functions in the header file.                 */
    aiVec4 CDYL(0.0f, Cb2U[SIDE], Ccbar2U[LIFT]);
    aiVec4 Clmn(Cb2U[ROLL], Ccbar2U[PITCH], Cb2U[YAW]);
    size_t i = 3;
    do {
        CDYL += static_cast<aiVec4>(xCDYLT.m4x4()[i]);
        Clmn += static_cast<aiVec4>(xClmnT.m4x4()[i]);
    }
    while(i--);

    float CL = CDYL[LIFT];
    CDYL += aiVec3(CDi*CL*CL, 0.0f, 0.0f);
#if 0
 printf(" p: %6.3f, q: %6.3f, r: %6.3f, adot: %6.3f\n", p, q, r, adot);
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
 printf("\n");
#endif

    /* State Accelerations (convert coefficients to forces and moments) */
    aiVec3 FDYL = CDYL*Coef2Force;
    aiVec3 Mlmn = Clmn*Coef2Moment;

    /* convert from wind axes to body axes */
    vector = AOA;
    len = simd4::normalize(vector);
    aiMtx4 mWind2Body = simd4x4::rotation_matrix(len, vector);
    aiVec3 FXYZ_body = mWind2Body*FDYL;

    aiVec3 gravity_body = mNed2Body*gravity_ned;
    FXYZ_body += gravity_body*mass;

    /* Thrust */
    float Cth = rho*throttle*throttle;
    i = no_engines-1;
    do
    {
        aiVec3 FEngine = FT[i]*(Cth*n2[i]);
        aiVec3 MEngine = MT[i]*Cth;

        FXYZ_body += FEngine;
        Mlmn += MEngine;
#if 0
 printf("FT[%lu]: %5.4f, %5.4f, %5.4f\n", i, FEngine[X], FEngine[Y], FEngine[Z]);
 printf("MT[%lu]: %5.4f, % 7.5f, %5.4f\n", i, MEngine[ROLL], MEngine[PITCH], MEngine[YAW]);
#endif
    }
    while (i--);
#if 0
 printf("FXYZ: %5.4f, %5.4f, %5.4f\n", FXYZ_body[X], FXYZ_body[Y], FXYZ_body[Z]);
 printf("Mlmn: %5.4f, % 7.5f, %5.4f\n", Mlmn[ROLL], Mlmn[PITCH], Mlmn[YAW]);
#endif

    /* contact point (landing gear) forces and moments */
    WoW = false;
    if (no_contacts && cg_agl < 10.0f)
    {
        size_t WoW_main = 0;
        i = 0;
        do
        {
            aiVec3 lg_ground_ned = mBody2Ned*contact_pos[i];
            if (lg_ground_ned[Z] > cg_agl)
            {   // weight on wheel
                aiVec3 lg_vrot = simd4::cross(vPQR, contact_pos[i]);
                aiVec3 lg_cg_vned = mBody2Ned*lg_vrot;
                aiVec3 lg_vned = vNED + lg_cg_vned;
                float Fn = std::min((contact_spring[i]*lg_ground_ned[Z] +
                                     contact_damp[i]*lg_vned[Z]), 0.0f);

                aiVec3 Fgear(0.0f, 0.0f, Fn);
                aiVec3 Fbody = mNed2Body*Fgear;
                aiVec3 Fbrake = mu_body*Fbody;

                aiVec3 FLGear = Fbody + Fbrake;
                FXYZ_body += FLGear;

//              aiVec3 MLGear = simd4::cross(contact_pos[i], FLGear);
//              Mlmn += MLGear;
#if 0
 printf("gear: %lu: pos: % 3.2f % 3.2f % 3.2f\n", i,
         lg_ground_ned[0], lg_ground_ned[1], lg_ground_ned[2]);
 printf("   Fbody: % 7.2f % 7.2f % 7.2f\n", Fbody[0], Fbody[1], Fbody[2]);
 printf("  Fbrake: % 7.2f % 7.2f % 7.2f\n", Fbrake[0], Fbrake[1], Fbrake[2]);
 printf("   Fgear: % 7.2f % 7.2f % 7.2f\n", lg_ground_ned[0], lg_ground_ned[1], lg_ground_ned[2]);
 printf("   Mgear: % 7.2f % 7.2f % 7.2f\n", MLGear[0], MLGear[1], MLGear[2]);
#endif
                if (i<3) WoW_main++;
            }
            WoW = (WoW_main == 3);
        }
        while(++i < no_contacts);
    }

    /* local body accelrations */
    XYZdot = FXYZ_body*inv_mass;
#if 0
printf("AOAdot: %5.4f, AOA: %5.4f, up: XYZdot: % 7.5f, XYZ: % 7.5f, gravity: % 7.5f\n", AOAdot[ALPHA], AOA[ALPHA], XYZdot[Z], FXYZ_body[Z]/mass, gravity_body[DOWN]);
#endif

#if 0
 printf("XYZdot   % 7.5f, % 7.5f, % 7.5f\n", XYZdot[X], XYZdot[Y], XYZdot[Z]);
 printf("Mlmn:    % 7.5f, % 7.5f, % 7.5f\n", Mlmn[ROLL], Mlmn[PITCH], Mlmn[YAW]);
#endif

    /* Dynamic Equations */

    /* body-axis translational accelerations: forward, sideward, upward */
    vUVWdot = XYZdot - simd4::cross(vPQR, vUVW);
    vUVW += vUVWdot*dt;

    /* body-axis rotational accelerations: rolling, pitching, yawing */
    vPQRdot = mJinv*(Mlmn - vPQR*(mJ*vPQR));
    vPQR += vPQRdot*dt;
#if 0
 printf("PQRdot:  % 7.5f, % 7.5f, % 7.5f\n", vPQRdot[P], vPQRdot[Q], vPQRdot[R]);
 printf("PQR:     % 7.5f, % 7.5f, % 7.5f\n", vPQR[P], vPQR[Q], vPQR[R]);
 printf("UVWdot:  % 7.5f, % 7.5f, % 7.5f\n", vUVWdot[U], vUVWdot[V], vUVWdot[W]);
 printf("UVW:     % 7.5f, % 7.5f, %1.7f\n", vUVW[U], vUVW[V], vUVW[W]);
#endif

    /* position of center of mass wrt earth: north, east, down */
    vNED = mBody2Ned*vUVW;
    aiVec3 NEDdist = vNED*dt;
#if 0
 printf("vNED:    % 7.5f, % 7.5f, % 7.5f\n", vNED[NORTH], vNED[EAST], vNED[DOWN]);
 printf("NEDdist: % 7.5f, % 7.5f, % 7.5f\n", NEDdist[NORTH], NEDdist[EAST], NEDdist[DOWN]);
#endif

#ifdef ENABLE_SP_FDM
    double dist = simd4::magnitude( aiVec2(NEDdist) );

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
    set_altitude_cg_agl_ft(location_geod[DOWN]);
#endif
#if 0
 printf("GEOD:    % 7.5f, % 7.5f, % 7.5f\n", location_geod[0], location_geod[1], location_geod[2]);
#endif

#if 0
{
    vector = simd4::normalize(euler);
    float len = simd4::normalize(vector);
    aiMtx4 mWind2Body = simd4x4::rotation_matrix(len, vector);
    euler_dot = mWind2Body*vPQR;
    euler += euler_dot*dt;
}
#else
    /* angle of body wrt earth: phi (roll), theta (pitch), psi (heading) */
    float sin_p = std::sin(euler[PHI]);
    float cos_p = std::cos(euler[PHI]);
    float sin_t = std::sin(euler[THETA]);
    float cos_t = std::cos(euler[THETA]);
    if (std::abs(cos_t) < 0.00001f) cos_t = std::copysign(0.00001f,cos_t);

    if (cos_t == 0.0f) {
        euler_dot[PSI] =  0.0f;
    } else {
        euler_dot[PSI] = (q*sin_p + r*cos_p)/cos_t;
    }
    euler_dot[THETA] =    q*cos_p - r*sin_p;
/*  euler_dot[PHI] = p + (q*sin_p + r*cos_p)*sin_t/cos_t; */
    euler_dot[PHI] = p + euler_dot[PSI]*sin_t;
    euler += euler_dot*dt;
#if 0
 printf("euler:   % 7.5f, % 7.5f, % 7.5f\n", euler[PHI], euler[THETA], euler[PSI]);
#endif
#endif

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

//  set_location_geod(get_Latitude(), get_Longitude(), altitde);
//  set_velocity_fps(get_V_calibrated_kts());

    return true;
}

bool
FGAISim::copy_from_AISim()
{
    // Mass properties and geometry values
//  _set_Inertias( mass, I[XX], I[YY], I[ZZ], I[XZ] );
    _set_CG_Position( cg[X], cg[Y], cg[Z]);

    // Accelerations
    _set_Accels_Body( vUVWdot[U], vUVWdot[V], vUVWdot[W] );

    // Velocities
    _set_V_equiv_kts( velocity*std::sqrt(sigma) * SG_FPS_TO_KT );
    _set_V_calibrated_kts( std::sqrt( 2.0f*qbar*sigma/rho) * SG_FPS_TO_KT );
    _set_V_ground_speed( simd4::magnitude(aiVec2(vNED)) * SG_FPS_TO_KT );
    _set_Mach_number( mach );

    _set_Velocities_Local( vNED[NORTH], vNED[EAST], vNED[DOWN] );
//  _set_Velocities_Local_Airmass( vUVWaero[U], vUVWaero[V], vUVWaero[W] );
    _set_Velocities_Body( vUVW[U], vUVW[V], vUVW[W] );
    _set_Omega_Body( vPQR[P], vPQR[Q], vPQR[R] );
    _set_Euler_Rates( euler_dot[PHI], euler_dot[THETA], euler_dot[PSI] );

    // Positions
    double lon = location_geod[LONGITUDE];
    double lat_geod = location_geod[LATITUDE];
    double altitude = location_geod[ALTITUDE];
    double lat_geoc;

    sgGeodToGeoc( lat_geod, altitude, &sl_radius, &lat_geoc );
    _set_Geocentric_Position( lat_geoc, lon, sl_radius+altitude );

    _set_Geodetic_Position( lat_geod, lon, altitude );

    _set_Euler_Angles( euler[PHI], euler[THETA],
                       SGMiscd::normalizePeriodic(0, SGD_2PI, euler[PSI]) );

    set_altitude_agl_ft( altitude - get_Runway_altitude() );
    _set_Altitude_AGL( cg_agl );

//  _set_Alpha( alpha );
//  _set_Beta(  beta );

//  _set_Gamma_vert_rad( Gamma_vert_rad );

//  _set_Density( Density );

//  _set_Static_pressure( Static_pressure );

//  _set_Static_temperature( Static_temperature );

    _set_Sea_level_radius( sl_radius * SG_METER_TO_FEET );
//  _set_Earth_position_angle( Earth_position_angle );

//  _set_Runway_altitude( get_Runway_altitude() );

    _set_Climb_Rate( -vNED[DOWN] );

    _update_ground_elev_at_pos();

    return true;
}
#endif

// ----------------------------------------------------------------------------

#define OMEGA_EARTH 	0.00007272205217f
#define MAX_ALT		101

// 1976 Standard Atmosphere - Density (slugs/ft2): 0 - 101,000 ft
float FGAISim::density[MAX_ALT] = {
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
float FGAISim::vsound[MAX_ALT] = {
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
    float alt_kft = _MINMAX(location_geod[ALTITUDE]/1000.0f, 0, MAX_ALT);
    float alt_idx = std::floor(alt_kft);
    size_t idx = static_cast<size_t>(alt_idx);
    float fract = alt_kft - alt_idx;
    float ifract = 1.0f - fract;

    /* linear interpolation for density */
    rho = ifract*density[idx] + fract*density[idx+1];
    qbar = 0.5f*rho*v*v;
    sigma = rho/density[0];

    float Sqbar = Sw*qbar;
    float Sbqbar = Sqbar*span;
    float Sqbarcbar = Sqbar*cbar;

    Coef2Force = aiVec3(Sqbar);
    Coef2Moment = aiVec3(Sbqbar);
    Coef2Moment[PITCH] = Sqbarcbar;

    /* linear interpolation for speed of sound */
    float vs = ifract*vsound[idx] + fract*vsound[idx+1];
    mach = v/vs;

    /*  useful semi-constants */
    if (v == 0.0f) {
        cbar_2U = b_2U = 0.0f;
    }
    else
    {
        b_2U = 0.5f*span/v;
        cbar_2U = 0.5f*cbar/v;
    }
}

// structural: x is pos. aft., y is pos. right, z is pos. up. in inches.
// body:       x is pos. fwd., y is pos. right, z is pos. down in feet.
void
FGAISim::struct_to_body(aiVec3 &pos)
{
    pos *= INCHES_TO_FEET;
    pos[0] = -pos[0];
    pos[2] = -pos[2];
}

simd4x4_t<float,4>
FGAISim::matrix_inverse(aiMtx4 mtx)
{
    aiMtx4 dst;
    aiVec4 v1, v2;

    dst = simd4x4::transpose(mtx);

    v1 = static_cast<aiVec4>(mtx.m4x4()[3]);
    v2 = static_cast<aiVec4>(mtx.m4x4()[0]);
    dst.ptr()[3][0] = -simd4::dot(v1, v2);

    v2 = static_cast<aiVec4>(mtx.m4x4()[1]);
    dst.ptr()[3][1] = -simd4::dot(v1, v2);

    v2 = static_cast<aiVec4>(mtx.m4x4()[2]);
    dst.ptr()[3][2] = -simd4::dot(v1, v2);

    return dst;
}

simd4x4_t<float,4>
FGAISim::invert_inertia(aiMtx4 mtx)
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

    return aiMtx4(   k1, 0.0f,   k3, 0.0f,
                   0.0f,   k4, 0.0f, 0.0f,
                     k3, 0.0f,   k6, 0.0f,
                   0.0f, 0.0f, 0.0f, 0.0f );
}


std::map<std::string,float>
FGAISim::jsonParse(const char *str)
{
    cJSON *json = ::cJSON_Parse(str);
    jsonMap rv;
    if (json)
    {
        for (int i=0; i<::cJSON_GetArraySize(json); ++i)
        {
            cJSON* cj = ::cJSON_GetArrayItem(json, i);
            if (cj->string)
            {
                if (cj->valuedouble) {
                    rv.emplace(cj->string, cj->valuedouble);
                }
                else if (cj->type == cJSON_Array)
                {
                    cJSON* child = cj->child;
                    for (int j=0; child; child = child->next, ++j)
                    {
                        std::string str = cj->string;
                        str += '[';
                        str += std::to_string(j);
                        str += ']';
                        if (child->type == cJSON_Object)
                        {
                            cJSON* subchild = child->child;
                            for (int k=0; subchild; subchild = subchild->next, ++k)
                            {
                                std::string substr = str;
                                substr += '/';
                                substr += subchild->string;

                                if (subchild->type == cJSON_Array)
                                {
                                   cJSON* array = subchild->child;
                                   for (int l=0; array; array = array->next, ++l)
                                    {
                                        std::string arraystr = substr;
                                        arraystr += '[';
                                        arraystr += std::to_string(l);
                                        arraystr += ']';
                                        rv.emplace(arraystr, array->valuedouble);
                                    }
                                }
                                else {
                                    rv.emplace(substr, subchild->valuedouble);
                                }
                            }
                        }
                        else {
                            rv.emplace(str, child->valuedouble);
                        }
                    }
                }
            }
        }
        ::cJSON_Delete(json);
    }
    else
    {
        std::string err = ::cJSON_GetErrorPtr();
        err = err.substr(0, 16);
        err = "AISim: Can't parse Aircraft data around: "+err;
        SG_LOG(SG_FLIGHT, SG_ALERT, err );
    }
    return rv;
}

bool
FGAISim::load(std::string path)
{
    std::ifstream file(path);
    std::string jsonString;

    file.seekg(0, std::ios::end);
    jsonString.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    jsonString.assign((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    jsonMap data = jsonParse(jsonString.c_str());

    Sw   = data["Sw"];
    cbar = data["cbar"];
    span = data["b"];

    mass = data["mass"]/AISIM_G;

    I[XX] = data["Ixx"];
    I[YY] = data["Iyy"];
    I[ZZ] = data["Izz"];
    I[XZ] = data["Ixz"];

    // Center of gravity, gears and engines are in the structural frame
    //  positions are in inches
    //  0: X-axis is directed afterwards,
    //  1: Y-axis is directed towards the right,
    //  2: Z-axis is directed upwards
    //
    // c.g. is relative to the aero reference point
    cg[X] = data["cg[0]"];
    cg[Y] = data["cg[1]"];
    cg[Z] = data["cg[2]"];
    struct_to_body(cg);

    // Gear ground contact points relative to center of gravity.
    no_contacts = 0;
    do
    {
        size_t i = no_contacts;
        std::string gearstr = "gear[" + std::to_string(i) + "]";

        float spring = data[gearstr + "/spring"];
        if (!spring) break;

        contact_pos[i][X] = data[gearstr + "/pos[0]"];
        contact_pos[i][Y] = data[gearstr + "/pos[1]"];
        contact_pos[i][Z] = data[gearstr + "/pos[2]"];
        struct_to_body(contact_pos[i]);

        contact_spring[i] = -data[gearstr + "/spring"];
        contact_damp[i] = -data[gearstr + "/damp"];
#if 0
 printf("%lu: pos: % 3.2f, % 3.2f, % 3.2f, spring: % 7.1f, damp: % 7.1f\n", i, contact_pos[i][X], contact_pos[i][Y], contact_pos[i][Z], contact_spring[i], contact_damp[i]);
#endif
    }
    while (++no_contacts < AISIM_MAX);

    /* Thuster / propulsion locations relative to c.g. */
    no_engines = 0;
    do
    {
        size_t i = no_engines;
        std::string engstr = "engine[" + std::to_string(i) + "]";

        float FTmax = data[engstr + "/FT_max"];
        if (!FTmax) break;

        aiVec3 pos;
        pos[0] = data[engstr + "/pos[0]"];
        pos[1] = data[engstr + "/pos[1]"];
        pos[2] = data[engstr + "/pos[2]"];
        struct_to_body(pos);

        // Thruster orientation is in the following sequence: pitch, roll, yaw
        aiVec3 orientation(data[engstr + "/dir[1]"],  // roll (degrees)
                           data[engstr + "/dir[0]"],  // pitch (degrees)
                           data[engstr + "/dir[2]"]); // yaw (degrees)
        orientation *= SG_DEGREES_TO_RADIANS;

        float len = simd4::normalize(orientation);
        aiMtx4 mWind2Body = simd4x4::rotation_matrix(len, orientation);
        aiVec3 dir = mWind2Body*aiVec3(1.0f, 0.0f, 0.0f);
        aiVec3 rot = simd4::cross(pos, dir);

        float rho = 0.002379f;
//      float MTmax = data[engstr + "/MT_max"];

        float max_rpm = data[engstr + "/rpm_max"];
        if (max_rpm == 0.0f) {
             n2[i] = 1.0f;
        }
        else
        {
            n2[i] = max_rpm/60.0f;
            n2[i] *= n2[i];
        }

        FTmax /= (rho*n2[i]);
//      MTmax /= rho / max_rpm;

        FT[i] = dir * FTmax;
        MT[i] = rot * FT[i]; // + dir * MTmax;
    }
    while(++no_engines < AISIM_MAX);

    float de_max = data["de_max"]*SG_DEGREES_TO_RADIANS;
    float dr_max = data["dr_max"]*SG_DEGREES_TO_RADIANS;
    float da_max = data["da_max"]*SG_DEGREES_TO_RADIANS;
    float df_max = data["df_max"]*SG_DEGREES_TO_RADIANS;

    /* aerodynamic coefficients */
    CLmin  = data["CLmin"];
    CLa    = data["CLa"];
    CLadot = data["CLadot"];
    CLq    = data["CLq"];
    CLdf_n = data["CLdf"]*df_max;

    CDmin  = data["CDmin"];
    CDa    = data["CDa"];
    CDb    = data["CDb"];
    CDi    = data["CDi"];
    CDdf_n = data["CDdf"]*df_max;

    CYb    = data["CYb"];
    CYp    = data["CYp"];
    CYr    = data["CYr"];
    CYdr_n = data["CYdr"]*dr_max;

    Clb    = data["Clb"];
    Clp    = data["Clp"];
    Clr    = data["Clr"];
    Clda_n = data["Clda"]*da_max;
    Cldr_n = data["Cldr"]*dr_max;

    Cma    = data["Cma"];
    Cmadot = data["Cmadot"];
    Cmq    = data["Cmq"];
    Cmde_n = data["Cmde"]*de_max;
    Cmdf_n = data["Cmdf"]*df_max;

    Cnb    = data["Cnb "];
    Cnp    = data["Cnp"];
    Cnr    = data["Cnr"];
    Cnda_n = data["Cnda"]*da_max;
    Cndr_n = data["Cndr"]*dr_max;

    return true;
}


// Register the subsystem.
#if 0
SGSubsystemMgr::Registrant<FGAISim> registrantFGAISim;
#endif
