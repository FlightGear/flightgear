// FGTransmission - a class to provide transmission control at larger airports.
//
// Written by Alexander Kappes, started March 2002.
// Based on ground.cxx by David Luff, started March 2002.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "transmission.hxx"

#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>


//Constructor
FGTransmission::FGTransmission(){
}

//Destructor
FGTransmission::~FGTransmission(){
}

void FGTransmission::Init() {
}

// ============================================================================
// extract parameters from transmission
// ============================================================================
TransPar FGTransmission::Parse() {
  TransPar   tpar;
  string     tokens[20];
  int        msglen,toklen;
  //char       dum;
  int        i,j,k;
  const char *capl = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  msglen = strlen( TransText.c_str() );

  int tkn  = 0;
  for ( i=0; i < msglen; ++i ) {
    if ( TransText.c_str()[i] != ' ' ) {
      if ( TransText.c_str()[i] != ',' ) tokens[tkn] += TransText.c_str()[i];
    } else if ( !tokens[tkn].empty() ) {
      if ( tkn <= 20 ) {
	tkn += 1;
      } else {
        SG_LOG(SG_ATC, SG_WARN,"Too many tokens");
      }
    }
  }

  for ( i=0; i<20; ++i) {
    
    if ( tokens[i] == "request" ) {
      tpar.request = true;
    } else if ( tokens[i] == "approach"  ) { 
      tpar.station = "approach";
      tpar.airport = tokens[i-1];
    } else if ( tokens[i] == "landing"  ) { 
      tpar.intention = "landing";
      for ( j=i+1; j<=i+2; ++j ) {
	if ( !tokens[j].empty() ) {
	  toklen = strlen( tokens[j].c_str() );
	  bool aid = true;
	  for ( k=0; k<toklen; ++k )
	    if ( ! strpbrk( &tokens[j].c_str()[k], capl )) {
	      aid = false;
	      break;
	    }
	  if ( aid ) tpar.intid = tokens[j];
	}
      }
    } else if ( tokens[i] == "Player"  ) { 
      tpar.callsign = tokens[i];
    }
  }

  //cout << tpar.airport << endl;
  //cout << tpar.request << endl;
  //cout << tpar.intention << endl;
  //cout << tpar.intid << endl;

  return tpar;
}
