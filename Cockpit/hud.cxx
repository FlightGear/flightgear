// hud.cxx -- hud defines and prototypes
//
// Written by Michele America, started September 1997.
//
// Copyright (C) 1997  Michele F. America  - micheleamerica@geocities.com
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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_VALUES_H
#  include <values.h>  // for MAXINT
#endif

#include <Aircraft/aircraft.hxx>
#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>
#include <Weather/weather.h>

#if defined ( __sun__ ) || defined ( __sgi )
extern "C" {
  extern void *memmove(void *, const void *, size_t);
}
#endif

#include "hud.hxx"



// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

deque< instr_item * > HUD_deque;

class locRECT {
  public:
    RECT rect;

    locRECT( UINT left, UINT top, UINT right, UINT bottom);
    RECT get_rect(void) { return rect;}
};

locRECT :: locRECT( UINT left, UINT top, UINT right, UINT bottom)
{
  rect.left   =  left;
  rect.top    =  top;
  rect.right  =  right;
  rect.bottom =  bottom;

}
// #define DEBUG

void drawOneLine( UINT x1, UINT y1, UINT x2, UINT y2)
{
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}

void drawOneLine( RECT &rect)
{
  glBegin(GL_LINES);
  glVertex2f(rect.left, rect.top);
  glVertex2f(rect.right, rect.bottom);
  glEnd();
}

//
// The following code deals with painting the "instrument" on the display
//
   /* textString - Bitmap font string */

void textString( int x, int y, char *msg, void *font ){
	glRasterPos2f(x, y);
	while (*msg) {
		glutBitmapCharacter(font, *msg);
		msg++;
    }
}

/* strokeString - Stroke font string */

void strokeString(int x, int y, char *msg, void *font, float theta)
{
int xx;
int yy;

	glPushMatrix();
  glRotatef(theta * RAD_TO_DEG, 0.0, 0.0, 1.0);
  xx = (int)(x * cos(theta) + y * sin( theta ));
  yy = (int)(y * cos(theta) - x * sin( theta ));
	glTranslatef( xx, yy, 0);
	glScalef(.1, .1, 0.0);
	while (*msg) {
		glutStrokeCharacter(font, *msg);
		msg++;
	}
	glPopMatrix();
}

//========================= End of Class Implementations===================
// fgHUDInit
//
// Constructs a HUD object and then adds in instruments. At the present
// the instruments are hard coded into the routine. Ultimately these need
// to be defined by the aircraft's instrumentation records so that the
// display for a Piper Cub doesn't show the speed range of a North American
// mustange and the engine readouts of a B36!
//

#define INSTRDEFS 21

int fgHUDInit( fgAIRCRAFT * /* current_aircraft */ )
{
  instr_item *HIptr;
  int index;

  fgPrintf( FG_COCKPIT, FG_INFO, "Initializing current aircraft HUD\n" );

  HUD_deque.erase( HUD_deque.begin(), HUD_deque.end());  // empty the HUD deque

//  hud->code = 1;
//  hud->status = 0;

  // For now lets just hardcode the hud here.
  // In the future, hud information has to come from the same place
  // aircraft information came from.

//  fgHUDSetTimeMode( hud, NIGHT );
//  fgHUDSetBrightness( hud, BRT_LIGHT );

  index = 0;

  do {
    switch ( index ) {
      case 0:     // TBI
        HIptr = (instr_item *) new fgTBI_instr( 270, 100, 60, 10 );
        break;

      case 1:     // Artificial Horizon
        HIptr = (instr_item *) new HudLadder( 240, 195, 120, 180 );
        break;

      case 2:    // KIAS
        HIptr = (instr_item *) new hud_card( 130,
                                             170,
                                              28,
                                             200,
                                             get_speed,
                                             HUDS_LEFT | HUDS_VERT,
                                             200.0, 0.0,
                                             1.0,
                                             10,  5,
                                             0,
                                             0,
                                             50.0,
                                             true);

        break;

      case 3:    // Radio Altimeter
        HIptr = (instr_item *) new hud_card( 420,
                                             195,
                                              25,
                                             150,
                                             get_agl,
                                             HUDS_LEFT | HUDS_VERT,
                                             1000, 0,
                                             1.0,
                                             25,    5,
                                             0,
                                             0,
                                             200.0,
                                             true);
        break;

      case 4:    // GYRO COMPASS
        HIptr = (instr_item *) new hud_card( 200,
                                             375,
                                             200,
                                              28,
                                             get_heading,
                                             HUDS_TOP,
                                             360, 0,
                                               1.0,
                                               5,   1,
                                             360,
                                               0,
                                              25,
                                             true);
        break;

      case 5:    // AMSL
        HIptr = (instr_item *) new hud_card( 460,
                                             170,
                                              35,
                                             200,
                                             get_altitude,
                                             HUDS_RIGHT | HUDS_VERT,
                                             15000, 0,
                                             1.0,
                                             100,  25,
                                             0,
                                             0,
                                             250,
                                             true);
        break;

      case 6:
        HIptr = (instr_item *) new  guage_instr( 250,            // x
                                                 350,            // y
                                                 100,            // width
                                                  20,            // height
                                                 get_aileronval, // data source
                                                 HUDS_BOTTOM | HUDS_NOTEXT,
                                                 100.0,
                                                 +1.0,
                                                 -1.0);
        break;

      case 7:
        HIptr = (instr_item *) new  guage_instr( 170,             // x
                                                 225,             // y
                                                  20,             // width
                                                 100,             // height
                                                 get_elevatorval, // data source
                                                 HUDS_RIGHT | HUDS_VERT | HUDS_NOTEXT,
                                                -100.0,           // Scale data
                                                  +1.0,           // Data Range
                                                  -1.0);
        break;

      case 8:
        HIptr = (instr_item *) new  guage_instr( 250,             // x
                                                 200,             // y
                                                 100,             // width
                                                  20,             // height
                                                 get_rudderval,   // data source
                                                 HUDS_TOP | HUDS_NOTEXT,
                                                 100.0,
                                                 +1.0,
                                                 -1.0);
        break;

      case 9:
        HIptr = (instr_item *) new  guage_instr( 100,             // x
                                                 190,
                                                  20,
                                                 160,             // height
                                                 get_throttleval, // data source
                                                 HUDS_VERT | HUDS_RIGHT | HUDS_NOTEXT,
                                                 100.0,
                                                   1.0,
                                                   0.0);
        break;

      case 10:    // Digital KIAS
        HIptr = (instr_item *) new instr_label ( 110,
                                                 150,
                                                  40,
                                                  30,
                                                 get_speed,
                                                 "%5.0f",
                                                 NULL,
                                                 " Kts",
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 11:    // Digital Rate of Climb
        HIptr = (instr_item *) new instr_label ( 110,
                                                 135,
                                                  40,
                                                  10,
                                                 get_climb_rate,
                                                 "%5.0f",
                                                 " Climb",
                                                 NULL,
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 12:    // Roll indication diagnostic
        HIptr = (instr_item *) new instr_label ( 110,
                                                 120,
                                                  40,
                                                  10,
                                                 get_roll,
                                                 "%5.2f",
                                                 " Roll",
                                                 " Deg",
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 13:    // Angle of attack diagnostic
        HIptr = (instr_item *) new instr_label ( 440,
                                                 150,
                                                  60,
                                                  10,
                                                 get_aoa,
                                                 "      %5.2f",
                                                 "AOA",
                                                 " Deg",
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 14:
        HIptr = (instr_item *) new instr_label ( 440,
                                                 135,
                                                  60,
                                                  10,
                                                 get_heading,
                                                 " %5.1f",
                                                 "Heading ",
                                                 " Deg",
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 15:
        HIptr = (instr_item *) new instr_label ( 440,
                                                 120,
                                                  60,
                                                  10,
                                                 get_sideslip,
                                                 "%5.2f",
                                                 "Sideslip ",
                                                 NULL,
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 16:
        HIptr = (instr_item *) new instr_label( 440,
                                                100,
                                                 60,
                                                 10,
                                                get_throttleval,
                                                "%5.2f",
                                                "Throttle ",
                                                NULL,
                                                 1.0,
                                                HUDS_TOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      case 17:
        HIptr = (instr_item *) new instr_label( 440,
                                                 85,
                                                 60,
                                                 10,
                                                get_elevatorval,
                                                "%5.2f",
                                                "Elevator ",
                                                NULL,
                                                 1.0,
                                                HUDS_TOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      case 18:
        HIptr = (instr_item *) new instr_label( 440,
                                                 60,
                                                 60,
                                                 10,
                                                get_aileronval,
                                                "%5.2f",
                                                "Aileron  ",
                                                NULL,
                                                 1.0,
                                                HUDS_TOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;


      case 19:
        HIptr = (instr_item *) new instr_label( 10,
                                                10,
                                                60,
                                                10,
                                                 get_frame_rate,
                                                "%.1f",
                                                "Frame rate = ",
                                                NULL,
                                                 1.0,
                                                HUDS_TOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      case 20:
	  switch( current_options.get_tris_or_culled() ) {
	  case 0:
	      HIptr = (instr_item *) new instr_label( 10,
						      25,
						      90,
						      10,
						      get_vfc_tris_drawn,
						      "%.0f",
						      "Tris Rendered = ",
						      NULL,
						      1.0,
						      HUDS_TOP,
						      RIGHT_JUST,
						      SMALL,
						      0,
						      TRUE );
	      break;
	  case 1:
	      HIptr = (instr_item *) new instr_label( 10,
						      25,
						      90,
						      10,
						      get_vfc_ratio,
						      "%.2f",
						      "VFC Ratio = ",
						      NULL,
						      1.0,
						      HUDS_TOP,
						      RIGHT_JUST,
						      SMALL,
						      0,
						      TRUE );
	      break;
	  }
	  break;

      case 21:
        HIptr = (instr_item *) new instr_label( 10,
                                                40,
                                                90,
                                                10,
                                                get_fov,
                                                "%.1f",
                                                "FOV = ",
                                                NULL,
						1.0,
                                                HUDS_TOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      default:
        HIptr = 0;;
      }
    if( HIptr ) {                   // Anything to install?
      HUD_deque.insert( HUD_deque.begin(), HIptr);
      }
    index++;
    }
  while( HIptr );

  return 0;  // For now. Later we may use this for an error code.
}

int fgHUDInit2( fgAIRCRAFT * /* current_aircraft */ )
{
    instr_item *HIptr;
    int index;

    fgPrintf( FG_COCKPIT, FG_INFO, "Initializing current aircraft HUD\n" );

    HUD_deque.erase( HUD_deque.begin(), HUD_deque.end());

    //  hud->code = 1;
    //  hud->status = 0;

    // For now lets just hardcode the hud here.
    // In the future, hud information has to come from the same place
    // aircraft information came from.

    //  fgHUDSetTimeMode( hud, NIGHT );
    //  fgHUDSetBrightness( hud, BRT_LIGHT );

    //  index = 0;
    index = 19;  

    instr_item* p;

    p = new instr_label( 10, 10, 60, 10,
			 get_frame_rate,
			 "%.1f",
			 "Frame rate = ",
			 NULL,
			 1.0,
			 HUDS_TOP,
			 RIGHT_JUST,
			 SMALL,
			 0,
			 TRUE );
    HUD_deque.push_front( p );

    if ( current_options.get_tris_or_culled() == 0 )
	p = new instr_label( 10, 25, 90, 10,
			     get_vfc_tris_drawn,
			     "%.0f",
			     "Tris Rendered = ",
			     NULL,
			     1.0,
			     HUDS_TOP,
			     RIGHT_JUST,
			     SMALL,
			     0,
			     TRUE );
    else
	p = new instr_label( 10, 25, 90, 10,
			     get_vfc_ratio,
			     "%.2f",
			     "VFC Ratio = ",
			     NULL,
			     1.0,
			     HUDS_TOP,
			     RIGHT_JUST,
			     SMALL,
			     0,
			     TRUE );
    HUD_deque.push_front( p );

    p = new instr_label( 10, 40, 90, 10,
			 get_fov,
			 "%.1f",
			 "FOV = ",
			 NULL,
			 1.0,
			 HUDS_TOP,
			 RIGHT_JUST,
			 SMALL,
			 0,
			 TRUE );
    HUD_deque.push_front( p );

    const int x_pos = 480;
    p = new instr_label( x_pos, 40, 40, 30,
			 get_speed,
			 "%5.0f",
			 "Airspeed ",
			 " Kts",
			 1.0,
			 HUDS_TOP,
			 RIGHT_JUST,
			 SMALL,
			 0,
			 TRUE );
    HUD_deque.push_front( p );

    p = new instr_label( x_pos, 25, 40, 10,
			 get_altitude,
			 "%5.0f",
			 "Altitude ",
			 " m",
			 1.0,
			 HUDS_TOP,
			 RIGHT_JUST,
			 SMALL,
			 0,
			 TRUE );
    HUD_deque.push_front( p );

    p = new instr_label( x_pos, 10, 60, 10,
			 get_heading,
			 "%5.1f",
			 "Heading  ",
			 " Deg",
			 1.0,
			 HUDS_TOP,
			 RIGHT_JUST,
			 SMALL,
			 0,
			 TRUE );
    HUD_deque.push_front( p );

    return 0;  // For now. Later we may use this for an error code.
}

int global_day_night_switch = DAY;

void HUD_brightkey( bool incr_bright )
{
instr_item *pHUDInstr = HUD_deque[0];
int brightness        = pHUDInstr->get_brightness();

  if( current_options.get_hud_status() ) {
    if( incr_bright ) {
      switch (brightness) {
        case BRT_LIGHT:
   	      current_options.set_hud_status(0);
          break;

        case BRT_MEDIUM:
          brightness = BRT_LIGHT;
          break;

        case BRT_DARK:
          brightness = BRT_MEDIUM;
          break;

        case BRT_BLACK:
          brightness = BRT_DARK;
          break;

        default:
          brightness = BRT_BLACK;
        }
      }
    else {
      switch (brightness) {
        case BRT_LIGHT:
          brightness = BRT_MEDIUM;
          break;

        case BRT_MEDIUM:
          brightness = BRT_DARK;
          break;

        case BRT_DARK:
          brightness = BRT_BLACK;
          break;

        case BRT_BLACK:
        default:
   	      current_options.set_hud_status(0);
        }
      }
    }
  else {
    current_options.set_hud_status(1);
    if( incr_bright ) {
      if( DAY == global_day_night_switch ) {
        brightness = BRT_BLACK;
        }
      else {
        brightness = BRT_DARK;
        global_day_night_switch = DAY;
        }
      }
    else {
      if( NIGHT == global_day_night_switch ) {
        brightness = BRT_DARK;
        }
      else {
        brightness = BRT_MEDIUM;
        global_day_night_switch = NIGHT;
        }
      }
    }
  pHUDInstr->SetBrightness( brightness );
}

// fgUpdateHUD
//
// Performs a once around the list of calls to instruments installed in
// the HUD object with requests for redraw. Kinda. It will when this is
// all C++.
//
void fgUpdateHUD( void ) {
  int i;
  int brightness;
//  int day_night_sw = current_aircraft.controls->day_night_switch;
  int day_night_sw = global_day_night_switch;
  int hud_displays = HUD_deque.size();
  instr_item *pHUDInstr;

  if( !hud_displays ) {  // Trust everyone, but ALWAYS cut the cards!
    return;
    }

  pHUDInstr = HUD_deque[0];
  brightness = pHUDInstr->get_brightness();
//  brightness = HUD_deque.at(0)->get_brightness();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glLoadIdentity();
  gluOrtho2D(0, 640, 0, 480);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor3f(1.0, 1.0, 1.0);
  glIndexi(7);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glLineWidth(1);

  deque < instr_item * > :: iterator current = HUD_deque.begin();
  deque < instr_item * > :: iterator last = HUD_deque.end();

  for ( ; current != last; ++current ) {
    pHUDInstr = *current;

    // for( i = hud_displays; i; --i) { // Draw everything
    // if( HUD_deque.at(i)->enabled()) {
    // pHUDInstr = HUD_deque[i - 1];
    if( pHUDInstr->enabled()) {
                                   // We should to respond to a dial instead
                                   // or as well to the of time of day. Of
                                   // course, we have no dial!
      if( day_night_sw == DAY) {
        switch (brightness) {
          case BRT_LIGHT:
            glColor3f (0.1, 0.9, 0.1);
            break;

          case BRT_MEDIUM:
            glColor3f (0.1, 0.7, 0.0);
            break;

          case BRT_DARK:
            glColor3f (0.0, 0.5, 0.0);
            break;

          case BRT_BLACK:
            glColor3f( 0.0, 0.0, 0.0);
            break;

          default:;
            }
          }
        else {
          if( day_night_sw == NIGHT) {
            switch (brightness) {
              case BRT_LIGHT:
                glColor3f (0.9, 0.1, 0.1);
                break;

              case BRT_MEDIUM:
                glColor3f (0.7, 0.0, 0.1);
                break;

              case BRT_DARK:
              default:
                glColor3f (0.5, 0.0, 0.0);
              }
            }
          else {     // Just in case default
            glColor3f (0.1, 0.9, 0.1);
            }
          }
    //  fgPrintf( FG_COCKPIT, FG_DEBUG, "HUD Code %d  Status %d\n",
    //            hud->code, hud->status );
      pHUDInstr->draw();
//      HUD_deque.at(i)->draw(); // Responsible for broken or fixed variants.
                              // No broken displays honored just now.
      }
    }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

// $Log$
// Revision 1.24  1998/10/16 23:27:25  curt
// C++-ifying.
//
// Revision 1.23  1998/10/16 00:53:00  curt
// Mods to display a bit more info when mini-hud is active.
//
// Revision 1.22  1998/09/29 14:56:31  curt
// c++-ified comments.
//
// Revision 1.21  1998/09/29 02:01:07  curt
// Added a "rate of climb" indicator.
//
// Revision 1.20  1998/08/24 20:05:16  curt
// Added a second minimalistic HUD.
// Added code to display the number of triangles rendered.
//
// Revision 1.19  1998/07/30 23:44:05  curt
// Tweaks for sgi building.
//
// Revision 1.18  1998/07/20 12:47:55  curt
// Replace the hud rendering for loop (which linearly searches the the hud
// list to find the entry with the proper position) with a simple linear
// traversal using an "iterator."
//
// Revision 1.17  1998/07/13 21:28:02  curt
// Converted the aoa scale to a radio altimeter.
//
// Revision 1.16  1998/07/13 21:00:47  curt
// Integrated Charlies latest HUD updates.
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.15  1998/07/08 14:41:08  curt
// Renamed polar3d.h to polar3d.hxx
//
// Revision 1.14  1998/07/06 21:31:20  curt
// Removed an extraneous ^M.
//
// Revision 1.13  1998/07/03 13:16:28  curt
// Added Charlie Hotchkiss's HUD updates and improvementes.
//
// Revision 1.11  1998/06/05 18:17:10  curt
// Added the declaration of memmove needed by the stl which apparently
// solaris only defines for cc compilations and not for c++ (__STDC__)
//
// Revision 1.10  1998/05/17 16:58:12  curt
// Added a View Frustum Culling ratio display to the hud.
//
// Revision 1.9  1998/05/16 13:04:14  curt
// New updates from Charlie Hotchkiss.
//
// Revision 1.8  1998/05/13 18:27:54  curt
// Added an fov to hud display.
//
// Revision 1.7  1998/05/11 18:13:11  curt
// Complete C++ rewrite of all cockpit code by Charlie Hotchkiss.
//
// Revision 1.22  1998/04/18 04:14:02  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.21  1998/04/03 21:55:28  curt
// Converting to Gnu autoconf system.
// Tweaks to hud.c
//
// Revision 1.20  1998/03/09 22:48:40  curt
// Minor "formatting" tweaks.
//
// Revision 1.19  1998/02/23 20:18:28  curt
// Incorporated Michele America's hud changes.
//
// Revision 1.18  1998/02/21 14:53:10  curt
// Added Charlie's HUD changes.
//
// Revision 1.17  1998/02/20 00:16:21  curt
// Thursday's tweaks.
//
// Revision 1.16  1998/02/19 13:05:49  curt
// Incorporated some HUD tweaks from Michelle America.
// Tweaked the sky's sunset/rise colors.
// Other misc. tweaks.
//
// Revision 1.15  1998/02/16 13:38:39  curt
// Integrated changes from Charlie Hotchkiss.
//
// Revision 1.14  1998/02/12 21:59:41  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.12  1998/02/09 15:07:48  curt
// Minor tweaks.
//
// Revision 1.11  1998/02/07 15:29:34  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.10  1998/02/03 23:20:14  curt
// Lots of little tweaks to fix various consistency problems discovered by
// Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
// passed arguments along to the real printf().  Also incorporated HUD changes
// by Michele America.
//
// Revision 1.9  1998/01/31 00:43:04  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.8  1998/01/27 00:47:51  curt
// Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
// system and commandline/config file processing code.
//
// Revision 1.7  1998/01/19 18:40:20  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.6  1997/12/15 23:54:34  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.5  1997/12/10 22:37:39  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.4  1997/09/23 00:29:32  curt
// Tweaks to get things to compile with gcc-win32.
//
// Revision 1.3  1997/09/05 14:17:26  curt
// More tweaking with stars.
//
// Revision 1.2  1997/09/04 02:17:30  curt
// Shufflin' stuff.
//
// Revision 1.1  1997/08/29 18:03:22  curt
// Initial revision.
//
