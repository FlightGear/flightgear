// jsinput.cxx -- wait for and identify input from joystick
//
// Written by Tony Peden, started May 2001
//
// Copyright (C) 2001  Tony Peden (apeden@earthlink.net)
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

#include "jsinput.h"

jsInput::jsInput(jsSuper *j) {
  jss=j;
  pretty_display=true;
  joystick=axis=button=-1;
  axis_threshold=0.2;
}

jsInput::~jsInput(void) {}

int jsInput::getInput(){
      
      bool gotit=false;
      
      float delta;
      int i, current_button = 0, button_bits = 0;

      joystick=axis=button=-1;
      axis_positive=false;
      
      if(pretty_display) {
          printf ( "+----------------------------------------------\n" ) ;
          printf ( "| Btns " ) ;

          for ( i = 0 ; i < jss->getJoystick()->getNumAxes() ; i++ )
            printf ( "Ax:%1d ", i ) ;

          for ( ; i < 8 ; i++ )
            printf ( "     " ) ;

          printf ( "|\n" ) ;

          printf ( "+----------------------------------------------\n" ) ;
      }
      

      jss->firstJoystick();
      do {
        jss->getJoystick()->read ( &button_iv[jss->getCurrentJoystickId()],
                                       axes_iv[jss->getCurrentJoystickId()] ) ;
      } while( jss->nextJoystick() );  
      
      
      
      while(!gotit) {
        jss->firstJoystick();
        do {

          jss->getJoystick()->read ( &current_button, axes ) ;

          if(pretty_display) printf ( "| %04x ", current_button ) ;

	        for ( i = 0 ; i < jss->getJoystick()->getNumAxes(); i++ ) {

            delta = axes[i] - axes_iv[jss->getCurrentJoystickId()][i]; 
            if(pretty_display) printf ( "%+.1f ", delta ) ; 
            if(!gotit) {
              if( fabs(delta) > axis_threshold ) {
                gotit=true;
                joystick=jss->getCurrentJoystickId();
                axis=i;
		axis_positive=(delta>0);
              } else if( current_button != 0 ) {
                gotit=true;
                joystick=jss->getCurrentJoystickId();  
                button_bits=current_button;
              } 
            }            
          }
	        
          if(pretty_display) {
            for ( ; i < 8 ; i++ )
	            printf ( "  .  " ) ;
          }


        } while( jss->nextJoystick() && !gotit); 
        if(pretty_display) {
          printf ( "|\r" ) ;
          fflush ( stdout ) ;
        }

        ulMilliSecondSleep(1);
      }
      if(button_bits != 0) {
        for(int i=0;i<=31;i++) {
          if( ( button_bits & (1 << i) ) > 0 ) {
             button=i;
             break;
          } 
        }    
      } 

      return 0;
}
  
