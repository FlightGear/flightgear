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

#include <cmath>
#include <limits>
#include <stdio.h>

#include <simgear/constants.h>
#include <simgear/math/simd.hxx>
#include <simgear/math/simd4x4.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Aircraft/controls.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <FDM/flight.hxx>

#include "AISim.hpp"


FGAISim::FGAISim(double dt)
{
    SGPropertyNode_ptr aero = fgGetNode("sim/aero", true);
    load(aero->getStringValue());
    load("");

    for (int i=0; i<4; i++) {
        xCDYLT[i] = 0.0f;
        xClmnT[i] = 0.0f;
    }

    PQR_body_prev = 0.0f;
    ABY_body_prev = 0.0f;

    br = 0.0f;

    UVW_body = 0.0f;
    velocity = 0.0f;
    NED_cm = 0.0f;
    PQR_body = 0.0f;
    ABY_body = 0.0f;
    ABY_dot = 0.0f;
    euler = 0.0f;
    wind_ned = 0.0f;
    agl = 0.0f;

    copy_to_AISim();
    set_location_geod(get_Latitude(), get_Longitude(), get_Altitude());
    set_alpha_rad(0.0f);
    set_beta_rad(0.0f);
    set_velocity_fps(0.0f);

    /* useful constants */
    FThrust = 0.0f;
    MThrust = 0.0f;
    gravity = 0.0f; gravity[DOWN] = G;
    dt2_2m = 0.5f*dt*dt/m;
}

FGAISim::~FGAISim()
{
}

// Initialize the AISim flight model, dt is the time increment for
// each subsequent iteration through the EOM
void
FGAISim::init() {

    // do init common to all the FDM's           
    common_init();

    // now do init specific to the AISim
    SG_LOG( SG_FLIGHT, SG_INFO, "Starting initializing AISim" );

    set_location_geod( get_Latitude(), get_Longitude(), get_Altitude() );
    set_euler_angles_rad( get_Phi(), get_Theta(), get_Psi() );

    UVW_body[U] = fgGetFloat("sim/presets/uBody-fps");
    UVW_body[V] = fgGetFloat("sim/presets/vBody-fps");
    UVW_body[W] = fgGetFloat("sim/presets/wBody-fps");
    set_velocity_fps( UVW_body[U] );
}

void
FGAISim::update(double ddt)
{
    if (is_suspended())
        return;

    // initialize all of AISim vars
    float dt = float(ddt);

    copy_to_AISim();

    ABY_dot = (ABY_body - ABY_body_prev)*dt;
    PQR_dot = (PQR_body - PQR_body_prev)*dt;

    /* update the history */
    PQR_body_prev = PQR_body;
    ABY_body_prev = ABY_body;

    // Earth-to-Body-Axis Transformation Matrix
    // body = pitch, roll, yaw
    simd4_t<float,4> angles = euler;
    float angle = simd4::normalize(angles);
    simd4x4_t<float,4> EBM = simd4x4::rotation_matrix<float>(angle, angles);

    // Body-Axis Gravity Components
    simd4_t<float,4> gb = EBM*gravity;
    simd4_t<float,4> windb = EBM*wind_ned;

    // Air-Relative velocity vector
    simd4_t<float,3> Va = UVW_body + windb;
    set_velocity_fps(Va[U]);
    if (std::abs(velocity) > 1.0f) {
        set_alpha_rad( std::atan(Va[3]/std::abs(Va[1])) );
        set_beta_rad( std::asin(Va[2]/velocity) );
    }

    // Force and Moment Coefficients
    simd4_t<float,4> CDYLT = 0.0f;
    simd4_t<float,4> ClmnT = 0.0f;
    for (int i=0; i<4; i++) {
        CDYLT += xCDYLT[i];
        ClmnT += xClmnT[i];
    }

    float p = PQR_body[P];
    float q = PQR_body[Q];
    float r = PQR_body[R];
    do {
        float adot = ABY_dot[ALPHA];
        float CL = CDYLT[LIFT];

        CDYLT[DRAG] += CDi*CL*CL;
        CDYLT[LIFT] += (CLadot*adot + CLq*q)*cbar_2U;
        CDYLT[SIDE] += (CYp*p       + CYr*r)*b_2U;
        ClmnT[ROLL]  += (Clp*p       + Clr*r)*b_2U;
        ClmnT[YAW]   += (Cnp*p       + Cnr*r)*b_2U;
        ClmnT[PITCH] += (Cmadot*adot + Cmq*q)*cbar_2U;
    }
    while (0);

    // State Accelerations
    simd4_t<float,4> FDYLT = CDYLT*C2F;
    simd4_t<float,4> MlmnT = ClmnT*C2M;

    /* convert from wind axes to body axes */
    simd4_t<float,3> aby = ABY_body;
    angle = simd4::normalize(aby);

    simd4x4_t<float,4> WBM = simd4x4::rotation_matrix<float>(angle, aby);
    simd4_t<float,4>FXYZ = WBM*FDYLT;

     // Thrust -- todo: propulsion in non x-directions
    FThrust[X] = FDYLT[THRUST];
    MThrust[ROLL] = MlmnT[THRUST];

    FXYZ += FThrust;
//  MlmnT += MThrust;

    // Gear forces
    float gear_comp = 0.2f + _MINMAX(agl/gear[Z], -0.2f, 0.0f);
printf("agl: %7.6f, gear-z: %7.6f, comp: %f\n", agl, gear[Z], gear_comp);
    if (gear_comp > 0.1f) {
        simd4_t<float,4> FLGear = 0.0f; FLGear[Z] = Cgear*gear_comp;
//      simd4_t<float,4> MLGear = 0.0f;

        FXYZ += FLGear;
//      MlmnT +=  MLGear;
    }
    FXYZ /= m;

    // Dynamic Equations

    /* body-axis velocity: forward, sideward, upward */
    float u = UVW_body[U];
    float v = UVW_body[V];
    float w = UVW_body[W];
    simd4_t<float,3> dUVW = FXYZ + gb;
    dUVW[U] +=  r*v - q*w;
    dUVW[V] += -r*u + p*w;
    dUVW[W] +=  q*u - p*v;
    UVW_body += dUVW;
printf("UVW: %5.4f, %5.4f, %5.4f\n", dUVW[U], dUVW[V], dUVW[W]);

    /* position of center of mass wrt earth: north, east, down */
    simd4_t<float,4> dNED_cm;
    dNED_cm = simd4x4::transpose(EBM)*UVW_body;

    double az2, dist = simd4::magnitude(dNED_cm);
    SGGeod pos2, pos = getPosition();
    geo_direct_wgs_84 ( pos, euler[PSI] * SGD_RADIANS_TO_DEGREES,
                        dist, pos2, &az2 );
    set_location_geod( pos2.getLatitudeRad(),
                       pos2.getLongitudeRad(), pos.getElevationFt() );

    /* body-axis iniertial rates: pitching, rolling, yawing */
    simd4_t<float,3> dPQR;
    dPQR[P] = I[ZZ]*MlmnT[ROLL]-(I[ZZ]+I[YY])*r*q/I[XX];
    dPQR[Q] =      MlmnT[PITCH]-(I[XX]+I[ZZ])*p*r;
    dPQR[R] = I[XX]*MlmnT[YAW] +(I[XX]-I[YY])*p*q/I[ZZ];
//  PQR_body += dPQR*dt;

    /* angle of body wrt earth: phi (roll), theta (pitch), psi (yaw) */
    float cos_t = std::cos(euler[THETA]);
    float sin_t = std::sin(euler[THETA]);

    if (std::abs(cos_t) < 0.00001f) cos_t = std::copysign(0.00001f,cos_t);

    float sin_p = std::sin(euler[PHI]);
    float cos_p = std::cos(euler[PHI]);
    simd4_t<float,3> deuler;
    deuler[PHI] =   P+(sin_p*Q+cos_p*r)*sin_t/cos_t;
    deuler[THETA] =    cos_p*q-sin_p*r;
    deuler[PSI] =     (sin_p*q+cos_p*r)    / cos_t;
    euler += deuler;

    copy_from_AISim();
}

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

    set_altitude_agl_ft(get_Altitude_AGL());
//  set_location_geod(get_Latitude(), get_Longitude(), get_Altitude());
//  set_velocity_fps(get_V_calibrated_kts());

    return true;
}

bool
FGAISim::copy_from_AISim()
{
    // Accelerations
//  _set_Accels_Omega( PQR_body[P], PQR_body[Q], PQR_body[R] );

    // Velocities
    _set_Velocities_Local( UVW_body[U], UVW_body[V], UVW_body[W] );

    _set_Omega_Body( PQR_body[P], PQR_body[Q], PQR_body[R] );

    // Positions
    double sl_radius, lat_geoc;
    sgGeodToGeoc( NED_cm[LATITUDE], NED_cm[ALTITUDE], &sl_radius, &lat_geoc );
    _set_Geocentric_Position( lat_geoc, NED_cm[LONGITUDE], sl_radius);
//  _update_ground_elev_at_pos();
//  _set_Sea_level_radius( sl_radius * SG_METER_TO_FEET);

    _set_Euler_Angles( euler[PHI], euler[THETA], euler[PSI] );

    _set_Alpha( ABY_body[ALPHA] );
    _set_Beta(  ABY_body[BETA] );

    return true;
}

void
FGAISim::update_UVW_body(float f)
{
    velocity = f;
    xCDYLT[VELOCITY][THRUST] = CTu*f;

    if (std::abs(f) < 0.00001f) f = std::copysign(0.00001f,f);
    b_2U = 0.5f*b/f;
    cbar_2U = 0.5f*cbar/f;
    update_qbar();
}

// ----------------------------------------------------------------------------

void
FGAISim::update_qbar()
{
    unsigned int hi = _MINMAX(std::rint(NED_cm[ALTITUDE]/1000.0f), 0, 100);
    float rho = env[hi][RHO];

    float qbar = 0.5f*rho*velocity*velocity;
    float Sqbar = S*qbar;
    float Sbqbar = Sqbar*b;
    float Sqbarcbar = Sqbar*cbar;

    C2F = Sqbar;
    C2F[THRUST] = rho*RPS2D4*CTmax;

    C2M = Sbqbar;
    C2M[PITCH] = Sqbarcbar;
}

// 1976 Standard Atmosphere
//  Density     Speed of sound
// slugs/ft2       ft/s
float FGAISim::env[101][2] =
{
  { 0.00237717f,   1116.45f },
  { 0.00230839f,   1112.61f },
  { 0.00224114f,   1108.75f },
  { 0.00217539f,   1104.88f },
  { 0.00211114f,   1100.99f },
  { 0.00204834f,   1097.09f },
  { 0.00198698f,   1093.18f },
  { 0.00192704f,   1089.25f },
  { 0.00186850f,   1085.31f },
  { 0.00181132f,   1081.36f },
  { 0.00175549f,   1077.39f },
  { 0.00170099f,   1073.40f },
  { 0.00164779f,   1069.40f },
  { 0.00159588f,   1065.39f },
  { 0.00154522f,   1061.36f },
  { 0.00149581f,   1057.31f },
  { 0.00144761f,   1053.25f },
  { 0.00140061f,   1049.18f },
  { 0.00135479f,   1045.08f },
  { 0.00131012f,   1040.97f },
  { 0.00126659f,   1036.85f },
  { 0.00122417f,   1032.71f },
  { 0.00118285f,   1028.55f },
  { 0.00114260f,   1024.38f },
  { 0.00110341f,   1020.19f },
  { 0.00106526f,   1015.98f },
  { 0.00102812f,   1011.75f },
  { 0.000991984f,  1007.51f },
  { 0.000956827f,  1003.24f },
  { 0.000922631f,  998.963f },
  { 0.000889378f,  994.664f },
  { 0.000857050f,  990.347f },
  { 0.000825628f,  986.010f },
  { 0.000795096f,  981.655f },
  { 0.000765434f,  977.280f },
  { 0.000736627f,  972.885f },
  { 0.000708657f,  968.471f },
  { 0.000675954f,  968.076f },
  { 0.000644234f,  968.076f },
  { 0.000614002f,  968.076f },
  { 0.000585189f,  968.076f },
  { 0.000557728f,  968.076f },
  { 0.000531556f,  968.076f },
  { 0.000506612f,  968.076f },
  { 0.000482838f,  968.076f },
  { 0.000460180f,  968.076f },
  { 0.000438586f,  968.076f },
  { 0.000418004f,  968.076f },
  { 0.000398389f,  968.076f },
  { 0.000379694f,  968.076f },
  { 0.000361876f,  968.076f },
  { 0.000344894f,  968.076f },
  { 0.000328709f,  968.076f },
  { 0.000313284f,  968.076f },
  { 0.000298583f,  968.076f },
  { 0.000284571f,  968.076f },
  { 0.000271217f,  968.076f },
  { 0.000258490f,  968.076f },
  { 0.000246360f,  968.076f },
  { 0.000234799f,  968.076f },
  { 0.000223781f,  968.076f },
  { 0.000213279f,  968.076f },
  { 0.000203271f,  968.076f },
  { 0.000193732f,  968.076f },
  { 0.000184641f,  968.076f },
  { 0.000175976f,  968.076f },
  { 0.000167629f,  968.337f },
  { 0.000159548f,  969.017f },
  { 0.000151867f,  969.698f },
  { 0.000144566f,  970.377f },
  { 0.000137625f,  971.056f },
  { 0.000131026f,  971.735f },
  { 0.000124753f,  972.413f },
  { 0.000118788f,  973.091f },
  { 0.000113116f,  973.768f },
  { 0.000107722f,  974.445f },
  { 0.000102592f,  975.121f },
  { 0.0000977131f, 975.797f },
  { 0.0000930725f, 976.472f },
  { 0.0000886582f, 977.147f },
  { 0.0000844590f, 977.822f },
  { 0.0000804641f, 978.496f },
  { 0.0000766632f, 979.169f },
  { 0.0000730467f, 979.842f },
  { 0.0000696054f, 980.515f },
  { 0.0000663307f, 981.187f },
  { 0.0000632142f, 981.858f },
  { 0.0000602481f, 982.530f },
  { 0.0000574249f, 983.200f },
  { 0.0000547376f, 983.871f },
  { 0.0000521794f, 984.541f },
  { 0.0000497441f, 985.210f },
  { 0.0000474254f, 985.879f },
  { 0.0000452178f, 986.547f },
  { 0.0000431158f, 987.215f },
  { 0.0000411140f, 987.883f },
  { 0.0000392078f, 988.550f },
  { 0.0000373923f, 989.217f },
  { 0.0000356632f, 989.883f },
  { 0.0000340162f, 990.549f },
  { 0.0000324473f, 991.214f }
};

bool
FGAISim::load(std::string path)
{
    /* defaults for the Cessna 182, taken from Roskam */
    S = 172.0f;
    cbar = 4.90f;
    b = 36.0f;

    m = 2650.0f*G;
    I[XX] = 948.0f;
    I[YY] = 1346.0f;
    I[ZZ] = 1967.0f;

    // gear ground contact points relative tot cg at (0,0,0)
    float no_gears = 3.0f;
    Cgear = 5400.0f*no_gears;
#if 1
    gear[X] =   0.0f*INCHES_TO_FEET;
    gear[Y] =   0.0f*INCHES_TO_FEET;
    gear[Z] = -54.4f*INCHES_TO_FEET;
#else
    /* nose */
    gear[0][X] = -47.8f*INCHES_TO_FEET;
    gear[0][Y] =   0.0f*INCHES_TO_FEET;
    gear[0][Z] = -54.4f*INCHES_TO_FEET;
    /* left */
    gear[1][X] =  17.2f*INCHES_TO_FEET;
    gear[1][Y] = -43.0f*INCHES_TO_FEET;
    gear[1][Z] = -54.4f*INCHES_TO_FEET;
    /* right */
    gear[2][X] =  17.2f*INCHES_TO_FEET;
    gear[2][Y] =  43.0f*INCHES_TO_FEET;
    gear[2][Z] = -54.4f*INCHES_TO_FEET;
#endif

    float de_max = 24.0f*SG_DEGREES_TO_RADIANS;
    float dr_max = 16.0f*SG_DEGREES_TO_RADIANS;
    float da_max = 17.5f*SG_DEGREES_TO_RADIANS;
    float df_max = 30.0f*SG_DEGREES_TO_RADIANS;

    no_engines = 1.0f;
    CTmax = 0.073f*no_engines;
    CTu = -0.0960f*no_engines;

    CLmin = 0.307f;
    CLa = 4.410f;
    CLadot = 1.70f;
    CLq = 3.90f;
    CLdf_n = 0.6685f*df_max;

    CDmin = 0.0270f;
    CDa = 0.121f;
    CDb = 0.0f;
    CDi = 0.0f;
    CDdf_n = 0.0816f*df_max;

    CYb = -0.393f;
    CYp = -0.0750f;
    CYr = 0.214f;
    CYdr_n = 0.187f*dr_max;

    Clb = -0.0923f;
    Clp = -0.484f;
    Clr = 0.0798f;
    Clda_n = 0.2290f*da_max;
    Cldr_n = 0.0147f*dr_max;

    Cma = -0.613f;
    Cmadot = -7.27f;
    Cmq = -12.40f;
    Cmde_n = -1.122f*de_max;
    Cmdf_n = -0.2177f*df_max;

    Cnb = 0.0587f;
    Cnp = -0.0278f;
    Cnr = -0.0937;
    Cnda_n = -0.0216f*da_max;
    Cndr_n = -0.0645f*dr_max;

    /* thuster / propulsion */
    float D = 75.0f*INCHES_TO_FEET;
    float RPS = 2700.0f/60.0f;
    float prop_Ixx = 1.67f;

    C2M[THRUST] = prop_Ixx*(2.0f*RPS*PI)/no_engines;
    RPS2D4 = RPS*RPS * D*D*D*D;

    return true;
}

