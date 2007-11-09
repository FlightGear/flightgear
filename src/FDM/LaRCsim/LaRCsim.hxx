//*************************************************************************
// LaRCsim.hxx -- interface to the "LaRCsim" flight model
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//*************************************************************************/


#ifndef _LARCSIM_HXX
#define _LARCSIM_HXX


#include <FDM/flight.hxx>

#include "IO360.hxx"
#include "LaRCsimIC.hxx"

class FGLaRCsim: public FGInterface {

private:

    FGNewEngine eng;
    LaRCsimIC* lsic;
    void set_ls(void);
    void snap_shot(void);
    double time_step;
    SGPropertyNode_ptr speed_up;
    SGPropertyNode_ptr aero;
    SGPropertyNode_ptr uiuc_type;
    double mass, i_xx, i_yy, i_zz, i_xz;
    
public:

    FGLaRCsim( double dt );
    ~FGLaRCsim(void);
    
    // copy FDM state to LaRCsim structures
    bool copy_to_LaRCsim();

    // copy FDM state from LaRCsim structures
    bool copy_from_LaRCsim();

    // reset flight params to a specific position 
    void init();

    // update position based on inputs, positions, velocities, etc.
    void update( double dt );
    
    // Positions
    void set_Latitude(double lat);  //geocentric
    void set_Longitude(double lon);    
    void set_Altitude(double alt);        // triggers re-calc of AGL altitude
    void set_AltitudeAGL(double altagl); // and vice-versa
    
    // Speeds -- setting any of these will trigger a re-calc of the rest
    void set_V_calibrated_kts(double vc);
    void set_Mach_number(double mach);
    void set_Velocities_Local( double north, double east, double down );
    void set_Velocities_Wind_Body( double u, double v, double w);
    
    // Euler angles 
    void set_Euler_Angles( double phi, double theta, double psi );
    
    // Flight Path
    void set_Climb_Rate( double roc);
    void set_Gamma_vert_rad( double gamma);
    
    // Earth
    void set_Static_pressure(double p);
    void set_Static_temperature(double T); 
    void set_Density(double rho); 

    // Inertias
    double get_Mass() const { return mass; }
    double get_I_xx() const { return i_xx; }
    double get_I_yy() const { return i_yy; }
    double get_I_zz() const { return i_zz; }
    double get_I_xz() const { return i_xz; }

    void _set_Inertias( double m, double xx, double yy, double zz, double xz)
    {
	mass = m;
	i_xx = xx;
	i_yy = yy;
	i_zz = zz;
	i_xz = xz;
    }
};


#endif // _LARCSIM_HXX


