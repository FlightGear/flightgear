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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <math.h>

#include <GL/glut.h>
#include <stdlib.h>
#include <string.h>

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>

#if defined ( __sun__ ) || defined ( __sgi )
extern "C" {
  extern void *memmove(void *, const void *, size_t);
}
#endif

#include "hud.hxx"


static char units[5];

// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

HudContainerType HUD_deque;

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

  FG_LOG( FG_COCKPIT, FG_INFO, "Initializing current aircraft HUD" );

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
                                             15000, -500,
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
                                                "%.0f",
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
    int index;

    FG_LOG( FG_COCKPIT, FG_INFO, "Initializing current aircraft HUD" );

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
			 "%.0f",
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

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
	strcpy(units, " ft");
    } else {
	strcpy(units, " m");
    }
    p = new instr_label( x_pos, 25, 40, 10,
			 get_altitude,
			 "%5.0f",
			 "Altitude ",
			 units,
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

  HudIterator current = HUD_deque.begin();
  HudIterator last = HUD_deque.end();

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

