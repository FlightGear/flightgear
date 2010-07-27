// ADA.cxx -- interface to the "External"-ly driven ADA flight model
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
// $Id$

// Modified by Cdr. VS Renganthan <vsranga@ada.ernet.in>, 12 Oct 2K

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstring>

#include <simgear/io/iochannel.hxx>
#include <simgear/io/sg_socket.hxx>
#include <simgear/constants.h>

#include <Aircraft/controls.hxx>
#include <Scenery/scenery.hxx> //to pass ground elevation to FDM
#include <Main/globals.hxx>
#include <Main/fg_props.hxx> //to get ID of window (left/right or center)

#include "ADA.hxx"

#define numberofbytes 472 // from FDM to visuals
#define nbytes 8	//from visuals to FDM

static struct {
    double number_of_bytes;
    double lat_geoc;
    double lon_geoc;
    double altitude;
    double psirad;
    double thetrad;
    double phirad;
    double earth_posn_angle;
    double radius_to_vehicle;
    double sea_level_radius;
    double latitude;
    double longitude;
    double Vnorth;
    double Veast;
    double Vdown;
    double Vcas_kts;
    double prad;
    double qrad;
    double rrad;
    double alpharad;
    double betarad;
    double latitude_dot;
    double longitude_dot;
    double radius_dot;
    double Gamma_vert_rad;
    double Runway_altitude;
    double throttle;
    double pstick;
    double rstick;
    double rpedal;
    double U_local;
    double V_local;
    double W_local;
    double U_dot_local;
    double V_dot_local;
    double W_dot_local;
    double Machno;
    double anxg;
    double anyg;
    double anzg;
    double aux1;
    double aux2;
    double aux3;
    double aux4;
    double aux5;
    double aux6;
    double aux7;
    double aux8;
    int iaux1;
    int iaux2;
    int iaux3;
    int iaux4;
    int iaux5;
    int iaux6;
    int iaux7;
    int iaux8;
    int iaux9;
    int iaux10;
    int iaux11;
    int iaux12;
    float aux9;
    float aux10;
    float aux11;
    float aux12;
    float aux13;
    float aux14;
    float aux15;
    float aux16;
    float aux17;
    float aux18;
} sixdof_to_visuals;

double view_offset; //if this zero, means center window

static struct {
	double ground_elevation;
} visuals_to_sixdof;

#define ground_elevation visuals_to_sixdof.ground_elevation

#define number_of_bytes sixdof_to_visuals.number_of_bytes
#define U_dot_local sixdof_to_visuals.U_dot_local
#define V_dot_local sixdof_to_visuals.V_dot_local
#define W_dot_local sixdof_to_visuals.W_dot_local
#define U_local sixdof_to_visuals.U_local
#define V_local sixdof_to_visuals.V_local
#define W_local sixdof_to_visuals.W_local
#define throttle sixdof_to_visuals.throttle
#define pstick sixdof_to_visuals.pstick
#define rstick sixdof_to_visuals.rstick
#define rpedal sixdof_to_visuals.rpedal
#define V_north sixdof_to_visuals.Vnorth
#define V_east sixdof_to_visuals.Veast
#define V_down sixdof_to_visuals.Vdown
#define V_calibrated_kts sixdof_to_visuals.Vcas_kts
#define P_body sixdof_to_visuals.prad
#define Q_body sixdof_to_visuals.qrad
#define R_body sixdof_to_visuals.rrad
#define Latitude_dot sixdof_to_visuals.latitude_dot
#define Longitude_dot sixdof_to_visuals.longitude_dot
#define Radius_dot sixdof_to_visuals.radius_dot
#define Latitude sixdof_to_visuals.latitude
#define Longitude sixdof_to_visuals.longitude
#define Lat_geocentric sixdof_to_visuals.lat_geoc
#define Lon_geocentric sixdof_to_visuals.lon_geoc
#define Radius_to_vehicle sixdof_to_visuals.radius_to_vehicle
#define Altitude sixdof_to_visuals.altitude
#define Phi sixdof_to_visuals.phirad
#define Theta sixdof_to_visuals.thetrad
#define Psi sixdof_to_visuals.psirad
#define Alpha sixdof_to_visuals.alpharad
#define Beta sixdof_to_visuals.betarad
#define Sea_level_radius sixdof_to_visuals.sea_level_radius
#define Earth_position_angle sixdof_to_visuals.earth_posn_angle
#define Runway_altitude sixdof_to_visuals.Runway_altitude
#define Gamma_vert_rad sixdof_to_visuals.Gamma_vert_rad
#define Machno sixdof_to_visuals.Machno
#define anxg sixdof_to_visuals.anxg
#define anyg sixdof_to_visuals.anyg
#define anzg sixdof_to_visuals.anzg


FGADA::FGADA( double dt ) :
    fdmsock(0)
{
//     set_delta_t( dt );
}


FGADA::~FGADA() {
    delete fdmsock;
}


// Initialize the ADA flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGADA::init() {

    //do init common to all FDM"s
    common_init();
    
    //now do ADA-specific init.

    // cout << "FGADA::init()" << endl;

    char OutBuffer[nbytes];
    copy_to_FGADA();

    printf("\nInitialising UDP sockets\n");
    // initialise a "udp" socket
    fdmsock = new SGSocket( "fdm_pc", "5001", "udp" );

    // open as a client
    bool result = fdmsock->open(SG_IO_OUT);
    if (result == false) {
	printf ("Socket Open Error\n");
    } else {
    copy_to_FGADA();
	// Write FGExternal structure from socket to establish connection
	int result = fdmsock->write(OutBuffer, nbytes);
	printf("Connection established = %d.\n", result);
    }
}


// Run an iteration of the EOM.  This is essentially a NOP here
// because these values are getting filled in elsewhere based on
// external input.
void FGADA::update( double dt ) {
    // cout << "FGADA::update()" << endl;

    if (is_suspended())
      return;

    char Buffer[numberofbytes];
    char OutBuffer[nbytes];

    // Read FGExternal structure from socket
    while (1) {
      int result = fdmsock->read(Buffer, numberofbytes);
      if (result == numberofbytes) {
         // Copy buffer into FGExternal structure
         memcpy (&sixdof_to_visuals, &Buffer, sizeof (Buffer));
	     // Convert from the FGExternal struct to the FGInterface struct (input)
		 copy_from_FGADA();
      } else {
         break;
      }
    }

    copy_to_FGADA();
    fgGetDouble("/sim/view/offset",view_offset);
    if ( view_offset == 0.0) {
        memcpy (&OutBuffer, &visuals_to_sixdof, sizeof (OutBuffer));
        fdmsock->write(OutBuffer, nbytes);
    }
}

// Convert from the FGInterface struct to the FGADA struct (output)
bool FGADA::copy_to_FGADA () {
    ground_elevation = get_Runway_altitude_m();
    return true;
}


// Convert from the FGADA struct to the FGInterface struct (input)
bool FGADA::copy_from_FGADA() {

    //Positions and attitudes for The Rendering engine
    _set_Geodetic_Position( Latitude, Longitude, Altitude );
    _set_Euler_Angles( Phi, Theta, Psi );
    _set_Geocentric_Position( Lat_geocentric, Lon_geocentric,
			      Radius_to_vehicle );
    _set_Sea_level_radius( Sea_level_radius );

	_set_Geocentric_Rates( Latitude_dot, Longitude_dot, Radius_dot );
    _set_Earth_position_angle( Earth_position_angle );
    
	// Velocities and accelerations for the pitch ladder and velocity vector
    _set_Accels_Local( U_dot_local, V_dot_local, W_dot_local );
    _set_Velocities_Ground( U_local, V_local, W_local );//same as V_NED in mps
    _set_Velocities_Local( V_north, V_east, V_down ); //same as UVW_local in fps

    //Positions and attitude for ship
    
    fgSetDouble("/fdm-ada/ship-lat", sixdof_to_visuals.aux1);
    fgSetDouble("/fdm-ada/ship-lon", sixdof_to_visuals.aux2);
    fgSetDouble("/fdm-ada/ship-alt", sixdof_to_visuals.aux3);
    fgSetDouble("/fdm-ada/skijump-dist", sixdof_to_visuals.aux4);
    fgSetDouble("/fdm-ada/ship-pitch", sixdof_to_visuals.aux9); // faux1
    fgSetDouble("/fdm-ada/ship-roll", sixdof_to_visuals.aux10); // faux2
    fgSetDouble("/fdm-ada/ship-yaw", sixdof_to_visuals.aux11);  // faux3
    fgSetInt("/fdm-ada/draw-ship", sixdof_to_visuals.iaux1);

    // controls
    globals->get_controls()->set_throttle(0,throttle/131.0);
    globals->get_controls()->set_elevator(pstick);
    globals->get_controls()->set_aileron(rstick);
    globals->get_controls()->set_rudder(rpedal);

    // auxilliary parameters for HUD
    _set_V_calibrated_kts( V_calibrated_kts );
    _set_Alpha( Alpha );
    _set_Beta( Beta );
    _set_Accels_CG_Body_N( anxg,anyg,anzg);
    _set_Mach_number( Machno);
    _set_Climb_Rate( W_local*SG_METER_TO_FEET ); //pressure alt in feet for lca(navy)

    fgSetInt("/fdm-ada/iaux2", sixdof_to_visuals.iaux2); //control law mode switch posn
    fgSetInt("/fdm-ada/iaux3", sixdof_to_visuals.iaux3); //ldg gear posn
    fgSetInt("/fdm-ada/iaux4", sixdof_to_visuals.iaux4); // wow nose status
    fgSetInt("/fdm-ada/iaux5", sixdof_to_visuals.iaux5); // wow main status
    fgSetInt("/fdm-ada/iaux6", sixdof_to_visuals.iaux6); // arrester hook posn
    fgSetInt("/fdm-ada/iaux7", sixdof_to_visuals.iaux7);
    fgSetInt("/fdm-ada/iaux8", sixdof_to_visuals.iaux8);
    fgSetInt("/fdm-ada/iaux9", sixdof_to_visuals.iaux9);
    fgSetInt("/fdm-ada/iaux10", sixdof_to_visuals.iaux10);
    fgSetInt("/fdm-ada/iaux11", sixdof_to_visuals.iaux11);
    fgSetInt("/fdm-ada/iaux12", sixdof_to_visuals.iaux12);

    fgSetDouble("/fdm-ada/aux5", sixdof_to_visuals.aux5);
    fgSetDouble("/fdm-ada/aux6", sixdof_to_visuals.aux6);
    fgSetDouble("/fdm-ada/aux7", sixdof_to_visuals.aux7);
    fgSetDouble("/fdm-ada/aux8", sixdof_to_visuals.aux8);

    fgSetDouble("/fdm-ada/aux12", sixdof_to_visuals.aux12);
    fgSetDouble("/fdm-ada/aux13", sixdof_to_visuals.aux13);
    fgSetDouble("/fdm-ada/aux14", sixdof_to_visuals.aux14);
    fgSetDouble("/fdm-ada/aux15", sixdof_to_visuals.aux15);
    fgSetDouble("/fdm-ada/aux16", sixdof_to_visuals.aux16);
    fgSetDouble("/fdm-ada/aux17", sixdof_to_visuals.aux17);
    fgSetDouble("/fdm-ada/aux18", sixdof_to_visuals.aux18);

    // Angular rates 
    _set_Omega_Body( P_body, Q_body, R_body );

    // Miscellaneous quantities
    _set_Gamma_vert_rad( Gamma_vert_rad );
    _set_Runway_altitude( Runway_altitude );

    //    SG_LOG( SG_FLIGHT, SG_DEBUG, "lon = " << Longitude 
    //	    << " lat_geoc = " << Lat_geocentric << " lat_geod = " << Latitude 
    //	    << " alt = " << Altitude << " sl_radius = " << Sea_level_radius 
    //	    << " radius_to_vehicle = " << Radius_to_vehicle );
	    

    //    printf("sr=%f\n",Sea_level_radius);
    //    printf("psi = %f %f\n",Psi,Psi*SGD_RADIANS_TO_DEGREES);    
    
    return true;
}
