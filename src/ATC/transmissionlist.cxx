// transmissionlist.cxx -- transmission management class
//
// Written by Alexander Kappes, started March 2002.
// Based on navlist.cxx by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_STRINGS_H
#  include <strings.h>   // bcopy()
#else
#  include <string.h>    // MSVC doesn't have strings.h
#endif


#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "transmissionlist.hxx"

#include <GUI/gui.h>


FGTransmissionList *current_transmissionlist;


FGTransmissionList::FGTransmissionList( void ) {
}


FGTransmissionList::~FGTransmissionList( void ) {
}


// load default.transmissions
bool FGTransmissionList::init( const SGPath& path ) {
    FGTransmission a;

    transmissionlist_station.erase( transmissionlist_station.begin(), transmissionlist_station.end() );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // read in each line of the file

    // in >> skipeol;
    // in >> skipcomment;

#ifdef __MWERKS__

    char c = 0;
    while ( in.get(c) && c != '\0' ) {
        in.putback(c);
        in >> a;
	if ( a.get_type() != '[' ) {
	    transmissionlist_code[a.get_station()].push_back(a);
	}
        in >> skipcomment;
    }

#else

    double min = 100000;
    double max = 0;

    while ( ! in.eof() ) {
        in >> a;
        transmissionlist_station[a.get_station()].push_back(a);

        in >> skipcomment;

        if ( a.get_station() < min ) {
            min = a.get_station();
        }
        if ( a.get_station() > max ) {
            max = a.get_station();
        }

        /*
        cout << a.get_station() << " " << a.get_code().c1 << " " << a.get_code().c2 << " "
             << a.get_code().c3 << " " << a.get_transtext() 
             << " " << a.get_menutext() << endl;
        */
    }
 
#endif

    // init ATC menu
    fgSetBool("/sim/atc/menu",false);

    return true;
}

// query the database for the specified station type; 
// for station see FlightGear/ATC/default.transmissions
bool FGTransmissionList::query_station( const atc_type &station, FGTransmission *t,
					int max_trans, int &num_trans ) 
{
  transmission_list_type     tmissions = transmissionlist_station[station];
  transmission_list_iterator current   = tmissions.begin();
  transmission_list_iterator last      = tmissions.end();

  for ( ; current != last ; ++current ) {
    if (num_trans < max_trans) {
      t[num_trans] = *current;
      num_trans += 1;
    }
    else {
      cout << "Transmissionlist error: Too many transmissions" << endl; 
    }
  }

  if ( num_trans != 0 ) return true;
  else {
    cout << "No transmission with station " << station << "found." << endl;
    string empty;
    return false;
  }
}

string FGTransmissionList::gen_text(const atc_type &station, const TransCode code, 
                                    const TransPar &tpars, const bool ttext )
{
	const int cmax = 300;
	string message;
	char tag[4];
	char crej = '@';
	char mes[cmax];
	char dum[cmax];
	//char buf[10];
	char *pos;
	int len;
	FGTransmission t;
	
	//  if (current_transmissionlist->query_station( station, &t ) ) { 
	transmission_list_type     tmissions = transmissionlist_station[station];
	transmission_list_iterator current   = tmissions.begin();
	transmission_list_iterator last      = tmissions.end();
	
	for ( ; current != last ; ++current ) {
		if ( current->get_code().c1 == code.c1 &&  
			current->get_code().c2 == code.c2 &&
		    current->get_code().c3 == code.c3 ) {
			
			if ( ttext ) message = current->get_transtext();
			else message = current->get_menutext();
			strcpy( &mes[0], message.c_str() ); 
			
			// Replace all the '@' parameters with the actual text.
			int check = 0;	// If mes gets overflowed the while loop can go infinite
			while ( strchr(&mes[0], crej) != NULL  ) {	// ie. loop until no more occurances of crej ('@') found
				pos = strchr( &mes[0], crej );
				memmove(&tag[0], pos, 3);
				tag[3] = '\0';
				int i;
				len = 0;
				for ( i=0; i<cmax; i++ ) {
					if ( mes[i] == crej ) {
						len = i; 
						break;
					}
				}
				strncpy( &dum[0], &mes[0], len );
				dum[len] = '\0';
				
				if ( strcmp ( tag, "@ST" ) == 0 )
					strcat( &dum[0], tpars.station.c_str() );
				else if ( strcmp ( tag, "@AP" ) == 0 )
					strcat( &dum[0], tpars.airport.c_str() );
				else if ( strcmp ( tag, "@CS" ) == 0 ) 
					strcat( &dum[0], tpars.callsign.c_str() );
				else if ( strcmp ( tag, "@TD" ) == 0 ) {
					if ( tpars.tdir == 1 ) {
						char buf[] = "left";
						strcat( &dum[0], &buf[0] );
					}
					else {
						char buf[] = "right";
						strcat( &dum[0], &buf[0] );
					}
				}
				else if ( strcmp ( tag, "@HE" ) == 0 ) {
					char buf[10];
					sprintf( buf, "%i", (int)(tpars.heading) );
					strcat( &dum[0], &buf[0] );
				}
				else if ( strcmp ( tag, "@VD" ) == 0 ) {
					if ( tpars.VDir == 1 ) {
						char buf[] = "Descend and maintain";
						strcat( &dum[0], &buf[0] );
					}
					else if ( tpars.VDir == 2 ) {
						char buf[] = "Maintain";
						strcat( &dum[0], &buf[0] );
					}
					else if ( tpars.VDir == 3 ) {
						char buf[] = "Climb and maintain";
						strcat( &dum[0], &buf[0] );
					}  
				}
				else if ( strcmp ( tag, "@AL" ) == 0 ) {
					char buf[10];
					sprintf( buf, "%i", (int)(tpars.alt) );
					strcat( &dum[0], &buf[0] );
				}
				else if ( strcmp ( tag, "@MI" ) == 0 ) {
					char buf[10];
					sprintf( buf, "%3.1f", tpars.miles );
					strcat( &dum[0], &buf[0] );
				}
				else if ( strcmp ( tag, "@FR" ) == 0 ) {
					char buf[10];
					sprintf( buf, "%6.2f", tpars.freq );
					strcat( &dum[0], &buf[0] );
				}
				else if ( strcmp ( tag, "@RW" ) == 0 )
					strcat( &dum[0], tpars.runway.c_str() );
				else {
					cout << "Tag " << tag << " not found" << endl;
					break;
				}
				strcat( &dum[0], &mes[len+3] );
				strcpy( &mes[0], &dum[0] );
				
				++check;
				if(check > 10) {
					SG_LOG(SG_ATC, SG_WARN, "WARNING: Possibly endless loop terminated in FGTransmissionlist::gen_text(...)"); 
					break;
				}
			}
			
			//cout << mes  << endl;  
			break;
		}
	}
	if ( mes != "" ) return mes;
	else return "No transmission found";
}


