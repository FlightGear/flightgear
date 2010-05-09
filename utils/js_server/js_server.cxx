/*
     Plib based joystick server based on PLIBs js_demo.cxx

     js_server is Copyright (C) 2003 
     by Stephen Lowry and Manuel Bessler

     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 2001  Steve Baker

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

     For further information visit http://plib.sourceforge.net

*/

#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include <plib/netSocket.h>
#include <plib/js.h>

void usage(char * progname)
{
    printf("This is an UDP based remote joystick server.\n");
    printf("usage: %s <hostname> <port>\n", progname);
}

int main ( int argc, char ** argv )
{
  jsJoystick * js ;
  float      * ax;
  int port = 16759;
  char * host; /* = "192.168.1.7"; */
  int activeaxes = 4;

  if( argc != 3 )
  {
    usage(argv[0]); 
    exit(1);
  }
  host = argv[1];
  port = atoi(argv[2]);

  jsInit () ;

  js = new jsJoystick ( 0 ) ;

  if ( js->notWorking () )
  {
    printf ( "no Joystick detected... exitting\n" ) ;
    exit(1);     
  }
  printf ( "Joystick is \"%s\"\n", js->getName() ) ;

  int numaxes = js->getNumAxes();
  ax = new float [ numaxes ] ;
  activeaxes = numaxes;
  
  if( numaxes > 4 )
  {
    printf("max 4 axes joysticks supported at the moment, however %i axes were detected\nWill only use the first 4 axes!\n", numaxes);
    activeaxes = 4;
  }

  // Must call this before any other net stuff
  netInit( &argc,argv );

  netSocket c;

  if ( ! c.open( false ) ) {	// open a UDP socket
	printf("error opening socket\n");
	return -1;
    }

    c.setBlocking( false );

    if ( c.connect( host, port ) == -1 ) {
	printf("error connecting to %s:%d\n", host, port);
	return -1;
    }


  char packet[256] = "Hello world!";
  while(1)
  {
        int b;
	int len = 0;
	int axis = 0;

        js->read( &b, ax );
	for ( axis = 0 ; axis < activeaxes ; axis++ )
	{
	  int32_t axisvalue = (int32_t)(ax[axis]*2147483647.0);
	  printf("axisval=%li\n", (long)axisvalue);
	  memcpy(packet+len, &axisvalue, sizeof(axisvalue));
	  len+=sizeof(axisvalue);
	}
	// fill emtpy values into packes when less than 4 axes
	for( ; axis < 4; axis++ )
	{
	  int32_t axisvalue = 0;
	  memcpy(packet+len, &axisvalue, sizeof(axisvalue));
	  len+=sizeof(axisvalue);
	}

	int32_t b_l = b;
        memcpy(packet+len, &b_l, sizeof(b_l));
        len+=sizeof(b_l);

	const char * termstr = "\0\0\r\n";
        memcpy(packet+len, termstr, 4);
	len += 4;

        c.send( packet, len, 0 );

    /* give other processes a chance */

#ifdef _WIN32
    Sleep ( 1 ) ;
#elif defined(sgi)
    sginap ( 1 ) ;
#else
    usleep ( 200 ) ;
#endif
    printf(".");
    fflush(stdout);
  }

  return 0 ;
}
