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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _ADA_HXX
#define _ADA_HXX


#include <simgear/io/sg_socket.hxx>

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

    // reset flight params to a specific position 
    bool init( double dt );

    // update position based on inputs, positions, velocities, etc.
    bool update( int multiloop );

    // auxiliary data getters and setters
    inline double get_aux1() const { return aux1; }
    inline void set_aux1(double aux) { aux1 = aux; }

    inline double get_aux2() const { return aux2; }
    inline void set_aux2(double aux) { aux2 = aux; }

    inline double get_aux3() const { return aux3; }
    inline void set_aux3(double aux) { aux3 = aux; }

    inline double get_aux4() const { return aux4; }
    inline void set_aux4(double aux) { aux4 = aux; }

    inline double get_aux5() const { return aux5; }
    inline void set_aux5(double aux) { aux5 = aux; }

    inline double get_aux6() const { return aux6; }
    inline void set_aux6(double aux) { aux6 = aux; }

    inline double get_aux7() const { return aux7; }
    inline void set_aux7(double aux) { aux7 = aux; }

    inline double get_aux8() const { return aux8; }
    inline void set_aux8(double aux) { aux8 = aux; }

    inline float get_aux9() const { return aux9; }
    inline void set_aux9(float aux) { aux9 = aux; }

    inline float get_aux10() const { return aux10; }
    inline void set_aux10(float aux) { aux10 = aux; }

    inline float get_aux11() const { return aux11; }
    inline void set_aux11(float aux) { aux11 = aux; }

    inline float get_aux12() const { return aux12; }
    inline void set_aux12(float aux) { aux12 = aux; }

    inline float get_aux13() const { return aux13; }
    inline void set_aux13(float aux) { aux13 = aux; }

    inline float get_aux14() const { return aux14; }
    inline void set_aux14(float aux) { aux14 = aux; }

    inline float get_aux15() const { return aux15; }
    inline void set_aux15(float aux) { aux15 = aux; }

    inline float get_aux16() const { return aux16; }
    inline void set_aux16(float aux) { aux16 = aux; }

    inline float get_aux17() const { return aux17; }
    inline void set_aux17(float aux) { aux17 = aux; }

    inline float get_aux18() const { return aux18; }
    inline void set_aux18(float aux) { aux18 = aux; }

    inline int get_iaux1() const { return iaux1; }
    inline void set_iaux1(int iaux) { iaux1 = iaux; }

    inline int get_iaux2() const { return iaux2; }
    inline void set_iaux2(int iaux) { iaux2 = iaux; }

    inline int get_iaux3() const { return iaux3; }
    inline void set_iaux3(int iaux) { iaux3 = iaux; }

    inline int get_iaux4() const { return iaux4; }
    inline void set_iaux4(int iaux) { iaux4 = iaux; }

    inline int get_iaux5() const { return iaux5; }
    inline void set_iaux5(int iaux) { iaux5 = iaux; }

    inline int get_iaux6() const { return iaux6; }
    inline void set_iaux6(int iaux) { iaux6 = iaux; }

    inline int get_iaux7() const { return iaux7; }
    inline void set_iaux7(int iaux) { iaux7 = iaux; }

    inline int get_iaux8() const { return iaux8; }
    inline void set_iaux8(int iaux) { iaux8 = iaux; }

    inline int get_iaux9() const { return iaux9; }
    inline void set_iaux9(int iaux) { iaux9 = iaux; }

    inline int get_iaux10() const { return iaux10; }
    inline void set_iaux10(int iaux) { iaux10 = iaux; }

    inline int get_iaux11() const { return iaux11; }
    inline void set_iaux11(int iaux) { iaux11 = iaux; }

    inline int get_iaux12() const { return iaux12; }
    inline void set_iaux12(int iaux) { iaux12 = iaux; }
};


#endif // _ADA_HXX
