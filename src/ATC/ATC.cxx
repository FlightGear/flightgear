// Implementation of FGATC - ATC subsystem abstract base class.
// Nothing in here should ever get called.
//
// Written by David Luff, started February 2002.
//
// Copyright (C) 2002  David C Luff - david.luff@nottingham.ac.uk
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

#include "ATC.hxx"

FGATC::~FGATC() {
}

void FGATC::Update() {
}

void FGATC::AddPlane(string pid) {
}

int FGATC::RemovePlane() {
}

void FGATC::SetDisplay() {
}

void FGATC::SetNoDisplay() {
}

const char* FGATC::GetIdent() {
    return("Error - base class function called in FGATC...");
}

atc_type FGATC::GetType() {
    return INVALID;
}

ostream& operator << (ostream& os, atc_type atc) {
    switch(atc) {
    case(INVALID):
	return(os << "INVALID");
    case(ATIS):
 	return(os << "ATIS");
    case(GROUND):
	return(os << "GROUND");
    case(TOWER):
	return(os << "TOWER");
    case(APPROACH):
	return(os << "APPROACH");
    case(DEPARTURE):
	return(os << "DEPARTURE");
    case(ENROUTE):
	return(os << "ENROUTE");
    }
    return(os << "ERROR - Unknown switch in atc_type operator << ");
}
