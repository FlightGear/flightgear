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


#ifndef _FGAISim_HXX
#define _FGAISim_HXX

#include <string>

#include <simgear/math/SGVec3.hxx>
#include <FDM/flight.hxx>


#define G		32.174f
#define PI		3.1415926535f
#define INCHES_TO_FEET	0.0833333333f
#define FPS_TO_KTS	0.592484313161f
#ifndef _MINMAX
# define _MINMAX(a,b,c)	(((a)>(c)) ? (c) : (((a)<(b)) ? (b) : (a)))
#endif

class FGAISim : public FGInterface
{
private:
    enum { X=0, Y=1, Z=2 };
    enum { XX=0, YY=1, ZZ=2, XZ=3 };
    enum { LATITUDE=0, LONGITUDE=1, ALTITUDE=2 };
    enum { NORTH=0, EAST=2, DOWN=3, N=0, E=2, D=3 };
    enum { LEFT=0, RIGHT=1 };
    enum { MAX=0, VELOCITY=1 };
    enum { PROPULSION=0, FLAPS=2, RUDDER=2, MIN=3, AILERON=3, ELEVATOR=3 };
    enum { DRAG=0, SIDE=1, LIFT=2, THRUST=3 };
    enum { ROLL=0, PITCH=1, YAW=2 };
    enum { ALPHA=0, BETA=1 };
    enum { RHO=0, VSOUND=1 };
    enum { ALT=3, ASL=3 };
    enum { PHI, THETA, PSI };
    enum { P=0, Q=1, R=2 };
    enum { U=0, V=1, W=2 };

public:
    FGAISim(double dt);
    ~FGAISim();

    // reset flight params to a specific location 
    void init();

    // update location based on properties
    void update(double dt);

    bool load(std::string path);

    // copy FDM state to AISim structures
    bool copy_to_AISim();

    // copy AISim structures to FDM state
    bool copy_from_AISim();

    /* controls */
    inline void set_rudder_norm(float f) {
        xCDYLT[RUDDER][SIDE] = CYdr_n*f;
        xClmnT[RUDDER][ROLL] = Cldr_n*f;
        xClmnT[RUDDER][YAW] = Cndr_n*f;
    }
    inline void set_elevator_norm(float f) {
        xClmnT[ELEVATOR][PITCH] = Cmde_n*f;
    }
    inline void set_aileron_norm(float f) {
        xClmnT[AILERON][ROLL] = Clda_n*f;
        xClmnT[AILERON][YAW] = Cnda_n*f;
    }
    inline void set_flaps_norm(float f) { 
        xCDYLT[FLAPS][LIFT] = CLdf_n*f;
        xCDYLT[FLAPS][DRAG] = CDdf_n*f;
        xClmnT[FLAPS][PITCH] = Cmdf_n*f;
    }
    inline void set_throttle_norm(float f) {
        xCDYLT[MAX][THRUST] = f;
        xClmnT[PROPULSION][THRUST] = f;
    }
    inline void set_brake_norm(float f) { br = f; }

    /* (initial) state, local frame */
    inline void set_location_geod(const SGVec3f& p) {
        NED_cm = p.simd3(); update_qbar();
    }
    inline void set_location_geod(float lat, float lon, float alt) {
        NED_cm[LATITUDE] = lat;
        NED_cm[LONGITUDE] = lon;
        NED_cm[ALTITUDE] = alt;
        update_qbar();
    }
    inline void set_altitude_asl_ft(float f) { NED_cm[DOWN] = -f; };
    inline void set_altitude_agl_ft(float f) { agl = _MINMAX(f, 0.0f, 100000.0f); }

    inline void set_pitch_rad(float f) { euler[PHI] = f; }
    inline void set_roll_rad(float f) { euler[THETA] = f; }
    inline void set_yaw_rad(float f) { euler[PSI] = f; }
    inline void set_euler_angles_rad(const SGVec3f& o) { euler = o.simd3(); }
    inline void set_euler_angles_rad(float phi, float theta, float psi) {
        euler[PHI] = phi;
        euler[THETA] = theta;
        euler[PSI] = psi;
    }

    void set_velocity_fps(float f) {
        UVW_body = 0.0f; UVW_body[X] = f; update_UVW_body(f);
    }
    inline void set_wind_ned(const SGVec3f& w) { wind_ned = w.simd3(); }
    inline void set_wind_ned(float n, float e, float d) {
        wind_ned[NORTH] = n;
        wind_ned[EAST] = e;
        wind_ned[DOWN] = d;
    }

    inline void set_alpha_rad(float f) {
        xCDYLT[ALPHA][DRAG] = CDa*std::abs(f);
        xCDYLT[ALPHA][LIFT] = CLa*f;
        xClmnT[ALPHA][PITCH] = Cma*f;
        ABY_body[ALPHA] = f;
    }
    inline void set_beta_rad(float f) {
        xCDYLT[BETA][DRAG] = CDb*std::abs(f);
        xCDYLT[BETA][SIDE] = CYb*f;
        xClmnT[BETA][ROLL] = Clb*f; 
        xClmnT[BETA][YAW] = Cnb*f;
        ABY_body[BETA] = f;
    }
    inline float get_alpha_rad() {
        return ABY_body[ALPHA];
    }
    inline float get_beta_rad() {
        return ABY_body[BETA];
    }

private:
    void update_qbar();
    void update_UVW_body(float f);

    /* aircraft normalized controls */
    float  br;				/* brake			*/

    /* aircraft state */
    simd4_t<float,4> NED_cm;		/* lat, lon, altitude           */
    simd4_t<float,4> UVW_body;		/* fwd, up, side speed          */
    simd4_t<float,3> PQR_body;		/* P, Q, R rate			*/
    simd4_t<float,3> PQR_dot;		/* Pdot, Qdot, Rdot accel.	*/
    simd4_t<float,3> ABY_body;		/* alpha, beta, gamma		*/
    simd4_t<float,3> ABY_dot;		/* adot, bdot, ydot		*/
    simd4_t<float,3> euler;		/* phi, theta, psi		*/
    simd4_t<float,4> wind_ned;
    float agl, velocity;

    /* history, these change between every call to update() */
    simd4_t<float,3> PQR_body_prev, ABY_body_prev;


    /* ---------------------------------------------------------------- */
    /* This should reduce the time spent in update() since controls	*/
    /* change less often than the update function runs which  might	*/
    /* run 20 to 60 times (or more) per second				*/

    /* cache, values that don't change very often */
    simd4_t<float,3> FThrust, MThrust;
    float b_2U, cbar_2U;

    /* dynamic coefficients (already multiplied with their value) */
    simd4_t<float,4> xCDYLT[4];
    simd4_t<float,4> xClmnT[4];
    simd4_t<float,4> C2F, C2M;

    /* ---------------------------------------------------------------- */
    /* aircraft static data */
    int no_engines;
    simd4_t<float,3> gear;	/* NED_cms in structural frame	*/
    simd4_t<float,3> I;		/* inertia				*/
    float S, cbar, b;		/* wing area, mean average chord, span	*/
    float m;                    /* mass					*/
    float RPS2D4;		/* propeller diameter, ening RPS	*/

    /* static oefficients, *_n is for normalized surface deflection */
    float Cgear;		/* gear					*/
    float CTmax, CTu;		/* thrust max, due to speed		*/
    float CLmin, CLa, CLadot, CLq, CLdf_n;
    float CDmin, CDa, CDb, CDi, CDdf_n;
    float CYb, CYp, CYr, CYdr_n;
    float Clb, Clp, Clr, Clda_n, Cldr_n;
    float Cma, Cmadot, Cmq, Cmde_n, Cmdf_n;
    float Cnb, Cnp, Cnr, Cnda_n, Cndr_n;

    /* static environment  data */
    static float env[101][2];
    simd4_t<float,4> gravity;
    float dt2_2m;
};

#endif // _FGAISim_HXX

