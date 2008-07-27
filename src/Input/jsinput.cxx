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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <simgear/compiler.h>

#include <iostream>

using std::cout;
using std::cin;
using std::endl;

#include "jsinput.h"

jsInput::jsInput(jsSuper *j) {
    jss=j;
    pretty_display=true;
    joystick=axis=button=-1;
    axis_threshold=0.2;
}

jsInput::~jsInput(void) {}

int jsInput::getInput() {

    bool gotit=false;

    float delta;
    int i, current_button = 0, button_bits = 0;

    joystick=axis=button=-1;
    axis_positive=false;

    if(pretty_display) {
        printf ( "+----------------------------------------------\n" ) ;
        printf ( "| Btns " ) ;

        for ( i = 0 ; i < jss->getJoystick()->getNumAxes() ; i++ )
            printf ( "Ax:%3d ", i ) ;

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
                if(pretty_display) printf ( "%+.3f ", delta ) ;
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

void jsInput::findDeadBand() {

    float delta;
    int i;
    float dead_band[MAX_JOYSTICKS][_JS_MAX_AXES];

    jss->firstJoystick();
    do {
        jss->getJoystick()->read ( NULL,
                axes_iv[jss->getCurrentJoystickId()] ) ;
        for ( i = 0; i <  jss->getJoystick()->getNumAxes(); i++ ) {
            dead_band[jss->getCurrentJoystickId()][i] = 0;
        }
    } while( jss->nextJoystick() );

    ulClock clock;
    cout << 10;
    cout.flush();

    for (int j = 9; j >= 0; j--) {
        double start_time = clock.getAbsTime();
        do {
            jss->firstJoystick();
            do {

                jss->getJoystick()->read ( NULL, axes ) ;

                for ( i = 0 ; i < jss->getJoystick()->getNumAxes(); i++ ) {

                    delta = axes[i] - axes_iv[jss->getCurrentJoystickId()][i];
                    if (fabs(delta) > dead_band[jss->getCurrentJoystickId()][i])
                        dead_band[jss->getCurrentJoystickId()][i] = delta;
                }

            } while( jss->nextJoystick());

            ulMilliSecondSleep(1);
            clock.update();
        } while (clock.getAbsTime() - start_time < 1.0);

        cout << " - " << j;
        cout.flush();
    }
    cout << endl << endl;

    jss->firstJoystick();
    do {
        for ( i = 0; i <  jss->getJoystick()->getNumAxes(); i++ ) {
            jss->getJoystick()->setDeadBand(i, dead_band[jss->getCurrentJoystickId()][i]);
            printf("Joystick %i, axis %i: %f\n", jss->getCurrentJoystickId(), i, dead_band[jss->getCurrentJoystickId()][i]);
        }
    } while( jss->nextJoystick() );
}
