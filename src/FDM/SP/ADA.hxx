// ADA.hxx -- interface to the "External"-ly driven ADA flight model
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


#ifndef _ADA_HXX
#define _ADA_HXX


class SGSocket;

#include <FDM/flight.hxx>


class FGADA: public FGInterface {

private:
	
    SGSocket *fdmsock;

    // Auxilliary Flight Model parameters, basically for HUD
    double        aux1;           // auxilliary flag
    double        aux2;           // auxilliary flag
    double        aux3;           // auxilliary flag
    double        aux4;           // auxilliary flag
    double        aux5;           // auxilliary flag
    double        aux6;           // auxilliary flag
    double        aux7;           // auxilliary flag
    double        aux8;           // auxilliary flag
    float        aux9;            // auxilliary flag
    float        aux10;           // auxilliary flag
    float        aux11;           // auxilliary flag
    float        aux12;           // auxilliary flag
    float        aux13;           // auxilliary flag
    float        aux14;           // auxilliary flag
    float        aux15;           // auxilliary flag
    float        aux16;           // auxilliary flag
    float        aux17;           // auxilliary flag
    float        aux18;           // auxilliary flag
    int          iaux1;           // auxilliary flag
    int          iaux2;           // auxilliary flag
    int          iaux3;           // auxilliary flag
    int          iaux4;           // auxilliary flag
    int          iaux5;           // auxilliary flag
    int          iaux6;           // auxilliary flag
    int          iaux7;           // auxilliary flag
    int          iaux8;           // auxilliary flag
    int          iaux9;           // auxilliary flag
    int         iaux10;           // auxilliary flag
    int         iaux11;           // auxilliary flag
    int         iaux12;           // auxilliary flag

    // copy FDM state to FGADA structures
    bool copy_to_FGADA();

    // copy FDM state from FGADA structures
    bool copy_from_FGADA();

public:

    FGADA( double dt );
    ~FGADA();

    // reset flight params to a specific position 
    void init();

    // update position based on inputs, positions, velocities, etc.
    void update(double dt);

};


#endif // _ADA_HXX
