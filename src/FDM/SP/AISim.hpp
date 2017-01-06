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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <cmath>

#ifdef ENABLE_SP_FDM
# include <simgear/math/simd.hxx>
# include <simgear/constants.h>
# include <FDM/flight.hxx>
#else
# include "simd.hxx"
# define SG_METER_TO_FEET		3.2808399
# define SG_FEET_TO_METER		(1/SG_METER_TO_FEET)
# define SGD_DEGREES_TO_RADIANS		0.0174532925
# define SGD_RADIANS_TO_DEGREES		(1/SGD_DEGREES_TO_RADIANS)
# define SGD_PI				3.1415926535
#endif

// #define SG_DEGREES_TO_RADIANS 0.0174532925f

	// max. no. gears, maxi. no. engines
#define AISIM_MAX	4
#define AISIM_G		32.174f

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
    enum { ROLL=0, PITCH=1, YAW=2 };
    enum { ALPHA=0, BETA=1 };
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

#ifdef ENABLE_SP_FDM
    // copy FDM state to AISim structures
    bool copy_to_AISim();

    // copy AISim structures to FDM state
    bool copy_from_AISim();
#endif

    /* controls */
    inline void set_rudder_norm(float f) {
        xCDYL[RUDDER][SIDE] = CYdr_n*f;
        xClmn[RUDDER][ROLL] = Cldr_n*f;
        xClmn[RUDDER][YAW] = Cndr_n*f;
    }
    inline void set_elevator_norm(float f) {
        xClmn[ELEVATOR][PITCH] = Cmde_n*f;
    }
    inline void set_aileron_norm(float f) {
        xClmn[AILERON][ROLL] = Clda_n*f;
        xClmn[AILERON][YAW] = Cnda_n*f;
    }
    inline void set_flaps_norm(float f) { 
        xCDYL[FLAPS][LIFT] = CLdf_n*f;
        xCDYL[FLAPS][DRAG] = CDdf_n*f;
        xClmn[FLAPS][PITCH] = Cmdf_n*f;
    }
    inline void set_throttle_norm(float f) { th = f; }
    inline void set_brake_norm(float f) { br = f; }

    /* (initial) state, local frame */
    inline void set_location_geod(const simd4_t<double,3>& p) {
        location_geod = p;
    }
    inline void set_location_geod(double lat, double lon, double alt) {
        location_geod = simd4_t<double,3>(lat, lon, alt);
    }
    inline void set_altitude_asl_ft(float f) { location_geod[ALTITUDE] = f; };
    inline void set_altitude_agl_ft(float f) { agl = f; }

    inline void set_euler_angles_rad(const simd4_t<float,3>& e) {
        euler_body = e;
     }
    inline void set_euler_angles_rad(float phi, float theta, float psi) {
        euler_body = simd4_t<float,3>(phi, theta, psi);
    }
    inline void set_pitch_rad(float f) { euler_body[PHI] = f; }
    inline void set_roll_rad(float f) { euler_body[THETA] = f; }
    inline void set_heading_rad(float f) { euler_body[PSI] = f; }

    void set_velocity_fps(const simd4_t<float,3>& v) { UVW_body = v; }
    void set_velocity_fps(float u, float v, float w) {
        UVW_body = simd4_t<float,3>(u, v, w);
    }
    void set_velocity_fps(float u) {
        UVW_body = simd4_t<float,3>(u, 0.0f, 0.0f);
    }

    inline void set_wind_ned_fps(const simd4_t<float,3>& w) { wind_ned = w; }
    inline void set_wind_ned_fps(float n, float e, float d) {
        wind_ned = simd4_t<float,3>(n, e, d);
    }

    inline void set_alpha_rad(float f) {
        xCDYL[ALPHA][DRAG] = CDa*std::abs(f);
        xCDYL[ALPHA][LIFT] = CLa*f;
        xClmn[ALPHA][PITCH] = Cma*f;
        AOA_body[ALPHA] = f;
    }
    inline void set_beta_rad(float f) {
        xCDYL[BETA][DRAG] = CDb*std::abs(f);
        xCDYL[BETA][SIDE] = CYb*f;
        xClmn[BETA][ROLL] = Clb*f; 
        xClmn[BETA][YAW] = Cnb*f;
        AOA_body[BETA] = f;
    }
    inline float get_alpha_rad() {
        return AOA_body[ALPHA];
    }
    inline float get_beta_rad() {
        return AOA_body[BETA];
    }

private:
    void update_velocity(float v);

    /* aircraft normalized controls */
    float th;				/* throttle command		*/
    float br;				/* brake command		*/

    /* aircraft state */
    simd4_t<double,3> location_geod;	/* lat, lon, altitude		*/
    simd4_t<float,5> NED_distance;	/* North, East, Down rel. pos.	*/
    simd4_t<float,3> NED_body;		/* North, East, Down speeds	*/
    simd4_t<float,3> UVW_body;		/* fwd, up, side speed		*/
    simd4_t<float,3> UVW_dot;           /* fwd, up, side accelerations	*/
    simd4_t<float,3> PQR_body;		/* P, Q, R rate			*/
    simd4_t<float,3> PQR_dot;		/* P, Q, R accelerations 	*/
    simd4_t<float,2> AOA_body;		/* alpha, beta			*/
    simd4_t<float,2> AOA_dot;		/* adot, bdot      		*/
    simd4_t<float,3> euler_body;	/* phi, theta, psi		*/
    simd4_t<float,3> euler_dot;		/* change in phi, theta, psi	*/
    simd4_t<float,3> wind_ned;		/* wind north, east, down	*/

    /* ---------------------------------------------------------------- */
    /* This should reduce the time spent in update() since controls	*/
    /* change less often than the update function runs which  might	*/
    /* run 20 to 60 times (or more) per second				*/

    /* cache */
    simd4_t<float,3> UVW_aero;		/* airmass relative to the body */
    simd4_t<float,3> FT[AISIM_MAX];	/* thrust force			*/
    simd4_t<float,3> FTM[AISIM_MAX];	/* thrust due to mach force	*/
    simd4_t<float,3> MT[AISIM_MAX];	/* thrust moment		*/
    simd4_t<float,3> IQR_dot_fact;
    float agl, velocity, mach;
    float b_2U, cbar_2U;
    bool WoW;

    /* dynamic coefficients (already multiplied with their value) */
    simd4_t<float,3> xCqadot[2], xCpr[2];
    simd4_t<float,3> xCDYL[4];
    simd4_t<float,3> xClmn[4];
    simd4_t<float,3> C2F, C2M;

    /* ---------------------------------------------------------------- */
    /* aircraft static data */
    int no_engines, no_gears;
    simd4_t<float,3> gear_pos[AISIM_MAX]; /* pos in structural frame	*/
    simd4_t<float,3> cg;	/* center of gravity			*/
    simd4_t<float,3> I;		/* inertia				*/
    float S, cbar, b;		/* wing area, mean average chord, span	*/
    float m;                    /* mass					*/

    /* static coefficients, *_n is for normalized surface deflection	*/
    float Cg_spring[AISIM_MAX];	/* gear spring coeffients		*/
    float Cg_damp[AISIM_MAX];	/* gear damping coefficients		*/
    float CTmax, CTu;		/* thrust max, due to speed		*/
    float CLmin, CLa, CLadot, CLq, CLdf_n;
    float CDmin, CDa, CDb, CDi, CDdf_n;
    float CYb, CYp, CYr, CYdr_n;
    float Clb, Clp, Clr, Clda_n, Cldr_n;
    float Cma, Cmadot, Cmq, Cmde_n, Cmdf_n;
    float Cnb, Cnp, Cnr, Cnda_n, Cndr_n;

    /* environment data */
    static float density[101], vsound[101];
    simd4_t<float,3> gravity_ned;
    float rho, qbar, sigma;
};

#endif // _FGAISim_HXX

