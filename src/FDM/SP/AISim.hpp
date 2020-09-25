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


#ifndef _FGAISim_HXX
#define _FGAISim_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <cmath>
#include <map>

#ifdef ENABLE_SP_FDM
# include <simgear/math/simd.hxx>
# include <simgear/constants.h>
# include <FDM/flight.hxx>
#else
# include "simd.hxx"
# include "simd4x4.hxx"
# define SG_METER_TO_FEET               3.2808399
# define SG_FEET_TO_METER               (1/SG_METER_TO_FEET)
# define SGD_DEGREES_TO_RADIANS         0.0174532925
# define SGD_RADIANS_TO_DEGREES         (1/SGD_DEGREES_TO_RADIANS)
# define SGD_PI                         3.1415926536
#endif

// #define SG_DEGREES_TO_RADIANS 0.0174532925f

// max. no. gears, max. no. engines
#define AISIM_MAX       4
#define AISIM_G         32.174f

#ifndef _MINMAX
# define _MINMAX(a,b,c)  (((a)>(c)) ? (c) : (((a)<(b)) ? (b) : (a)))
#endif

class FGAISim
#ifdef ENABLE_SP_FDM
        : public FGInterface
#endif
{
private:
    enum { X=0, Y=1, Z=2 };
    enum { XX=0, YY=1, ZZ=2, XZ=3 };
    enum { LATITUDE=0, LONGITUDE=1, ALTITUDE=2 };
    enum { NORTH=0, EAST=1, DOWN=2 };
    enum { LEFT=0, RIGHT=1, UP=2 };
    enum { MAX=0, VELOCITY=1, PROPULSION=2 };
    enum { FLAPS=2, RUDDER=2, MIN=3, AILERON=3, ELEVATOR=3 };
    enum { DRAG=0, SIDE=1, LIFT=2 };
    enum { ROLL=0, PITCH=1, YAW=2, THRUST=3 };
    enum { PHI=0, THETA, PSI };
    enum { ALPHA=0, BETA=1 };
    enum { P=0, Q=1, R=2 };
    enum { U=0, V=1, W=2 };


    using aiVec2 = simd4_t<float,2>;
    using aiVec3d = simd4_t<double,3>;
    using aiVec3 = simd4_t<float,3>;
    using aiVec4 = simd4_t<float,4>;
    using aiMtx4 = simd4x4_t<float,4>;

    using jsonMap = std::map<std::string,float>;

public:
    FGAISim(double dt);
    ~FGAISim();

    // Subsystem API.
    void init() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "aisim"; }

    bool load(std::string path);

#ifdef ENABLE_SP_FDM
    // copy FDM state to AISim structures
    bool copy_to_AISim();

    // copy AISim structures to FDM state
    bool copy_from_AISim();
#endif

    /* controls */
    inline void set_rudder_norm(float f) {
        xCDYLT.ptr()[RUDDER][SIDE] = CYdr_n*f;
        xClmnT.ptr()[RUDDER][ROLL] = Cldr_n*f;
        xClmnT.ptr()[RUDDER][YAW] = -Cndr_n*f;
    }
    inline void set_elevator_norm(float f) {
        xClmnT.ptr()[ELEVATOR][PITCH] = Cmde_n*f;
    }
    inline void set_aileron_norm(float f) {
        xClmnT.ptr()[AILERON][ROLL] = Clda_n*f;
        xClmnT.ptr()[AILERON][YAW] = Cnda_n*f;
    }
    inline void set_flaps_norm(float f) {
        xCDYLT.ptr()[FLAPS][LIFT] = -CLdf_n*f;
        xCDYLT.ptr()[FLAPS][DRAG] = -CDdf_n*std::abs(f);
        xClmnT.ptr()[FLAPS][PITCH] = Cmdf_n*f;
    }
    inline void set_throttle_norm(float f) { throttle = f; }
    inline void set_brake_norm(float f) { mu_body[0] = -0.02f-0.7f*f; }

    /* (initial) state, local frame */
    inline void set_location_geod(aiVec3d& p) {
        location_geod = p;
    }
    inline void set_location_geod(double lat, double lon, double alt) {
        location_geod = aiVec3d(lat, lon, alt);
    }
    inline void set_altitude_asl_ft(float f) { location_geod[ALTITUDE] = f; };
    inline void set_altitude_agl_ft(float f) { cg_agl = f; }

    inline void set_euler_angles_rad(const aiVec3& e) {
        euler = e;
     }
    inline void set_euler_angles_rad(float phi, float theta, float psi) {
        euler = aiVec3(phi, theta, psi);
    }
    inline void set_roll_rad(float f) { euler[PHI] = f; }
    inline void set_pitch_rad(float f) { euler[THETA] = f; }
    inline void set_heading_rad(float f) { euler[PSI] = f; }

    void set_velocity_fps(const aiVec3& v) { vUVW = v; }
    void set_velocity_fps(float u, float v, float w) {
        vUVW = aiVec3(u, v, w);
    }
    void set_velocity_fps(float u) {
        vUVW = aiVec3(u, 0.0f, 0.0f);
    }

    inline void set_wind_ned_fps(const aiVec3& w) { wind_ned = w; }
    inline void set_wind_ned_fps(float n, float e, float d) {
        wind_ned = aiVec3(n, e, d);
    }

    inline void set_alpha_rad(float f) {
        f = _MINMAX(f, -0.25f, 0.25f);		// -14 to 14 degrees
        xCDYLT.ptr()[ALPHA][LIFT] = -CLa*f;
        xCDYLT.ptr()[ALPHA][DRAG] = -CDa*std::abs(f);
        xClmnT.ptr()[ALPHA][PITCH] = Cma*f;
        AOA[ALPHA] = f;
    }
    inline void set_beta_rad(float f) {
        f = _MINMAX(f, -0.30f, 0.30f);		// -17 to 17 degrees
        xCDYLT.ptr()[BETA][DRAG] = -CDb*std::abs(f);
        xCDYLT.ptr()[BETA][SIDE] = CYb*f;
        xClmnT.ptr()[BETA][ROLL] = Clb*f;
        xClmnT.ptr()[BETA][YAW] = Cnb*f;
        AOA[BETA] = f;
    }
    inline float get_alpha_rad() {
        return AOA[ALPHA];
    }
    inline float get_beta_rad() {
        return AOA[BETA];
    }

private:
    std::map<std::string,float> jsonParse(const char *str);
    void update_velocity(float v);
    aiMtx4 matrix_inverse(aiMtx4 mtx);
    aiMtx4 invert_inertia(aiMtx4 mtx);
    void struct_to_body(aiVec3 &pos);

    /* aircraft normalized controls */
    float throttle = 0.0f;    /* throttle command              */

    /* aircraft state */
    aiVec3d location_geod = 0.0; /* lat, lon, altitude         */
    aiVec3 XYZdot = 0.0f;     /* local body accelrations       */
    aiVec3 vNED = 0.0f;       /* North, East, Down velocity    */
    aiVec3 vUVW = 0.0f;       /* fwd, side, down velocity      */
    aiVec3 vUVWdot = 0.0f;    /* fwd, side, down acceleration  */
    aiVec3 vPQR = 0.0f;       /* roll, pitch, yaw rate         */
    aiVec3 vPQRdot = 0.0f;    /* roll, pitch, yaw acceleration */
    aiVec3 AOA = 0.0f;        /* alpha, beta                   */
    aiVec3 AOAdot = 0.0f;     /* adot, bdot                    */
    aiVec3 euler = 0.0f;      /* phi, theta, psi               */
    aiVec3 euler_dot = 0.0f;  /* change in phi, theta, psi     */
    aiVec3 wind_ned = 0.0f;   /* wind north, east, down        */
    aiVec3 mu_body = { -0.02f, -0.8f, 0.0f };

    /* ---------------------------------------------------------------- */
    /* This should reduce the time spent in update() since controls     */
    /* change less often than the update function runs which  might     */
    /* run 20 to 60 times (or more) per second                          */

    /* cache */
    aiVec3 vUVWaero = 0.0f;   /* airmass relative to the body  */
    aiVec3 FT[AISIM_MAX];     /* thrust force                  */
    aiVec3 MT[AISIM_MAX];     /* thrust moment                 */
    float n2[AISIM_MAX];
    aiVec3 b_2U = 0.0f;
    aiVec3 cbar_2U = 0.0f;
    aiVec3 inv_mass;
    float cg_agl = 0.0f;
    float alpha = 0.0f;
    float beta = 0.0f;
    float velocity = 0.0f;
    float mach = 0.0f;
    double sl_radius = 0.0;
    bool WoW = true;

    /* dynamic coefficients (already multiplied with their value) */
    aiVec3 xCq = 0.0f;
    aiVec3 xCadot = 0.0f;
    aiVec3 xCp = 0.0f;
    aiVec3 xCr = 0.0f;
    aiMtx4 xCDYLT;
    aiMtx4 xClmnT;
    aiVec4 Coef2Force;
    aiVec4 Coef2Moment;

    /* ---------------------------------------------------------------- */
    /* aircraft static data */
    size_t no_engines = 0;
    size_t no_contacts = 0;
    aiMtx4 mJ, mJinv;                /* inertia matrix                  */
    aiVec3 contact_pos[AISIM_MAX+1]; /* pos in structural frame         */
    aiVec3 mass = 0.0f;              /* mass                            */
    aiVec3 cg = 0.0f;                /* center of gravity               */
    aiVec4 I = 0.0f;                 /* inertia                         */
    float Sw = 0.0f;		     /* wing area                       */
    float cbar = 0.0f;               /* mean average chord              */
    float span = 0.0f;               /* wing span                       */

    /* static coefficients, *_n is for normalized surface deflection    */
    float contact_spring[AISIM_MAX]; /* contact spring coeffients       */
    float contact_damp[AISIM_MAX];   /* contact damping coefficients    */
    float CLmin, CLa, CLadot, CLq, CLdf_n;
    float CDmin, CDa, CDb, CDi, CDdf_n;
    float CYb, CYp, CYr, CYdr_n;
    float Clb, Clp, Clr, Clda_n, Cldr_n;
    float Cma, Cmadot, Cmq, Cmde_n, Cmdf_n;
    float Cnb, Cnp, Cnr, Cnda_n, Cndr_n;

    /* environment data */
    static float density[101], vsound[101];
    aiVec3 gravity_ned = { 0.0f, 0.0f, AISIM_G };
    size_t alt_idx = -1;
    float rho = 0.0f;
    float qbar = 0.0f;
    float sigma = 0.0f;
};

#endif // _FGAISim_HXX

