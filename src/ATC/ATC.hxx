// FGATC - abstract base class for the various actual atc classes 
// such as FGATIS, FGTower etc.
//
// Written by David Luff, started Feburary 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

#ifndef _FG_ATC_HXX
#define _FG_ATC_HXX

// Possible types of ATC type that the radios may be tuned to.
// INVALID implies not tuned in to anything.
enum atc_type {
    INVALID,
    ATIS,
    GROUND,
    TOWER,
    APPROACH,
    DEPARTURE,
    ENROUTE
};  

class FGATC {

public:

    virtual ~FGATC();

    // Run the internal calculations
    virtual void Update();

    // Indicate that this instance should output to the display if appropriate 
    virtual void SetDisplay();

    // Indicate that this instance should not output to the display
    virtual void SetNoDisplay();

    // Return the ATC station ident (generally the ICAO code of the airport)
    virtual const char* GetIdent();

    // Return the type of ATC station that the class represents
    virtual atc_type GetType();
};

#endif  // _FG_ATC_HXX
