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

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
//#include <simgear/math/fg_random.h>
//#include <simgear/math/polar3d.hxx>

#include <Aircraft/aircraft.hxx>
#include <Autopilot/newauto.hxx>
#include <GUI/gui.h>
#include <Main/options.hxx>
#ifdef FG_NETWORK_OLK
#include <NetworkOLK/network.h>
#endif
#include <Scenery/scenery.hxx>
//#include <Time/fg_timer.hxx>

#if defined ( __sun__ ) || defined ( __sgi )
extern "C" {
  extern void *memmove(void *, const void *, size_t);
}
#endif

#include "hud.hxx"

static char units[5];

// The following routines obtain information concerning the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

deque< instr_item * > HUD_deque;

fgTextList         HUD_TextList;
fgLineList         HUD_LineList;
fgLineList         HUD_StippleLineList;

fntRenderer *HUDtext = 0;
float  HUD_TextSize = 0;
int HUD_style = 0;

float HUD_matrix[16];
static float hud_trans_alpha = 0.67f;

void fgHUDalphaInit( void );

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

#ifdef OLD_CODE
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

    if(*msg)
    {
//      puDrawString (  NULL, msg, x, y );
        glRasterPos2f(x, y);
        while (*msg) {
            glutBitmapCharacter(font, *msg);
            msg++;
        }
    }
}


/* strokeString - Stroke font string */
void strokeString(int x, int y, char *msg, void *font, float theta)
{
    int xx;
    int yy;
    int c;
    float sintheta,costheta;
    

    if(*msg)
    {
    glPushMatrix();
    glRotatef(theta * RAD_TO_DEG, 0.0, 0.0, 1.0);
    sintheta = sin(theta);
    costheta = cos(theta);
    xx = (int)(x * costheta + y * sintheta);
    yy = (int)(y * costheta - x * sintheta);
    glTranslatef( xx, yy, 0);
    glScalef(.1, .1, 0.0);
    while( (c=*msg++) ) {
        glutStrokeCharacter(font, c);
    }
    glPopMatrix();
    }
}

int getStringWidth ( char *str )
{
    if ( HUDtext && str )
    {
        float r, l ;
        guiFntHandle->getBBox ( str, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
        return FloatToInt( r - l );
    }
    return 0 ;
}
#endif // OLD_CODE

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
    //  int index;

    // $$$ begin - added VS Renganathan
#ifdef FIGHTER_HUD
    //  int off = 400;
    int min_x = 200; 
    int max_x = 440; 
    //  int min_y = 100;
    int max_y = 380; 
    int cen_x = 320;
    int cen_y = 240;
    unsigned int text_h = 10;
    unsigned int ladr_w2 = 60;
    int ladr_h2 = 90;
    int ladr_t = 35;
    int compass_w = 120;
    int gap = 10;
#else
    // $$$ end - added VS Renganathan

    //  int off = 50;
    int min_x = 25; //off/2;
    int max_x = 615; //640-(off/2);
    //  int min_y = off;
    int max_y = 430; //480-off;
    int cen_x = 320;
    int cen_y = 240;
    unsigned int text_h = 10;
    unsigned int ladr_w2 = 60;
    int ladr_h2 = 90;
    int ladr_t = 35;
    int compass_w = 200;
    int gap = 10;
#endif

//  int font_size = current_options.get_xsize() / 60;
  int font_size = (current_options.get_xsize() > 1000) ? LARGE : SMALL;
  
  HUD_style = 1;

  FG_LOG( FG_COCKPIT, FG_INFO, "Initializing current aircraft HUD" );

//  deque < instr_item * > :: iterator first = HUD_deque.begin();
//  deque < instr_item * > :: iterator last = HUD_deque.end();
//  HUD_deque.erase( first, last);  // empty the HUD deque  

  HUD_deque.erase( HUD_deque.begin(), HUD_deque.end());  // empty the HUD deque

//  hud->code = 1;
//  hud->status = 0;

  // For now lets just hardcode the hud here.
  // In the future, hud information has to come from the same place
  // aircraft information came from.

//  fgHUDSetTimeMode( hud, NIGHT );
//  fgHUDSetBrightness( hud, BRT_LIGHT );

//      case 0:     // TBI
//  int x = 290; /*cen_x-30*/
//  int y = 45;  /*off-5*/
//  HIptr = (instr_item *) new fgTBI_instr( x, y, ladr_w2, text_h );
// $$$ begin - added, VS Renganathan 13 Oct 2K
#ifdef FIGHTER_HUD
//      case 1:     // Artificial Horizon
  HIptr = (instr_item *) new HudLadder( cen_x-ladr_w2, cen_y-ladr_h2,
                                        2*ladr_w2, 2*ladr_h2 );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 4:    // GYRO COMPASS
  HIptr = (instr_item *) new hud_card( cen_x-(compass_w/2),
                                       cen_y+8.0,  //CENTER_DIAMOND_SIZE
                                       compass_w,
                                       28,
                                       get_heading,
                                       HUDS_TOP,
                                       360, 0,
                                       1.0,
                                       10,   1,
                                       360,
                                       0,
                                       25,
                                       true);
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 10:    // Digital KIAS
        HIptr = (instr_item *) new instr_label ( cen_x-190,
                                                 cen_y+25,
                                                  40,
                                                  30,
                                                 get_speed,
                                                 "%5.0f",
                                                 NULL,
                                                 " ",
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
// Mach no
        HIptr = (instr_item *) new instr_label ( cen_x-180,
                                                 cen_y+5,
                                                  40,
                                                  30,
                                                 get_mach,
                                                 "%5.2f",
                                                 NULL,
                                                 " ",
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
// Pressure Altitude
        HIptr = (instr_item *) new instr_label ( cen_x+110,
                                                 cen_y+25,
                                                  40,
                                                  30,
                                                 get_altitude,
                                                 "%5.0f",
                                                 NULL,
                                                 " ",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
// Radio Altitude
        HIptr = (instr_item *) new instr_label ( cen_x+110,
                                                 cen_y+5,
                                                  40,
                                                  30,
                                                 get_agl,
                                                 "%5.0f",
                                                 NULL,
                                                 " R",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 2:    // AOA
  HIptr = (instr_item *) new hud_card( cen_x-145.0, //min_x +18,
                                       cen_y-190,
                                       28,
                                       120,
                                       get_aoa,
                                       HUDS_LEFT | HUDS_VERT,
//                                       HUDS_RIGHT | HUDS_VERT,                                     
                                       30.0, -15.0,
                                       1.0,
                                       10,  2,
                                       0,
                                       0,
                                       60.0,
                                       true);
  HUD_deque.insert( HUD_deque.begin(), HIptr);
//      case 2:    // normal acceleration at cg, g's
  HIptr = (instr_item *) new hud_card( cen_x-185, //min_x +18,
                                       cen_y-220,
                                       18,
                                       130,
                                       get_anzg,
                                       HUDS_LEFT | HUDS_VERT,
//                                       HUDS_RIGHT | HUDS_VERT,                                     
                                       10.0, -5.0,
                                       1.0,
                                       2,  1,
                                       0,
                                       0,
                                       20.0,
                                       true);
  HUD_deque.insert( HUD_deque.begin(), HIptr);
//      case 2:    // VSI
  HIptr = (instr_item *) new hud_card( (2*cen_x)-195.0, //min_x +18,
                                       cen_y-190,
                                       28,
                                       120,
                                       get_climb_rate, //fix
//                                       HUDS_LEFT | HUDS_VERT,
                                       HUDS_RIGHT | HUDS_VERT,                                     
                                       500.0, -500.0,
                                       1.0,
                                       5,  1,
                                       0,
                                       0,
                                       15.0,
                                       true);
  HUD_deque.insert( HUD_deque.begin(), HIptr);


// Aux parameter 16 - xposn
        HIptr = (instr_item *) new instr_label ( cen_x+170,
                                                 cen_y+200,
                                                  40,
                                                  30,
                                                 get_aux16,
                                                 "%5.1f",
                                                 NULL,
                                                 " pstick",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
  

// Aux parameter 17 - xposn
        HIptr = (instr_item *) new instr_label ( cen_x+170,
                                                 cen_y+190,
                                                  40,
                                                  30,
                                                 get_aux17,
                                                 "%5.1f",
                                                 NULL,
                                                 " rstick",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
  
  
  
  
  
  
  
// Aux parameter 1 - xposn
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+200,
                                                  40,
                                                  30,
                                                 get_aux1,
                                                 "%5.0f",
                                                 NULL,
                                                 " m",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 2 - pla
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+190,
                                                  40,
                                                  30,
                                                 get_aux9,
                                                 "%5.0f",
                                                 NULL,
                                                 " pla",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 3 - xtd
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+170,
                                                  40,
                                                  30,
                                                 get_aux11,
                                                 "%5.1f",
                                                 NULL,
                                                 " xtd",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 4 - ytd
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+160,
                                                  40,
                                                  30,
                                                 get_aux12,
                                                 "%5.1f",
                                                 NULL,
                                                 " ytd",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 5 - nztd
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+150,
                                                  40,
                                                  30,
                                                 get_aux10,
                                                 "%5.1f",
                                                 NULL,
                                                 " nztd",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 6 - vvtd
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+140,
                                                  40,
                                                  30,
                                                 get_aux13,
                                                 "%5.1f",
                                                 NULL,
                                                 " vvtd",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 7 - vtd
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+130,
                                                  40,
                                                  30,
                                                 get_aux14,
                                                 "%5.1f",
                                                 NULL,
                                                 " vtd",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 8 - alftd
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+120,
                                                  40,
                                                  30,
                                                 get_aux15,
                                                 "%5.1f",
                                                 NULL,
                                                 " alftd",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 9 - fnr
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+100,
                                                  40,
                                                  30,
                                                 get_aux8,
                                                 "%5.1f",
                                                 NULL,
                                                 " fnose",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

// Aux parameter 10 - Ax
        HIptr = (instr_item *) new instr_label ( cen_x+240,
                                                 cen_y+90,
                                                  40,
                                                  30,
                                                 get_Ax,
                                                 "%5.2f",
                                                 NULL,
                                                 " Ax",
                                                 1.0,
                                                 HUDS_TOP,
                                                 LEFT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
#else
// $$$ end - added , VS Renganathan 13 Oct 2K

  HIptr = (instr_item *) new fgTBI_instr( 290, 45, 60, 10 );  
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 1:     // Artificial Horizon
  HIptr = (instr_item *) new HudLadder( cen_x-ladr_w2, cen_y-ladr_h2,
                                        2*ladr_w2, 2*ladr_h2 );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 4:    // GYRO COMPASS
  HIptr = (instr_item *) new hud_card( cen_x-(compass_w/2),
                                       max_y,
                                       compass_w,
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
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 5:    // AMSL
  HIptr = (instr_item *) new hud_card( max_x - 35 -15, // 15 to balance speed card
                                       cen_y-(compass_w/2),
                                       35,
                                       compass_w,
                                       get_altitude,
//                                     HUDS_RIGHT | HUDS_VERT,
                                       HUDS_LEFT | HUDS_VERT,
                                       5000, -1000,
                                       1.0,
                                       100,  25,
                                       0,
                                       0,
                                       250,
                                       true);
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 6:
  HIptr = (instr_item *) new  guage_instr( cen_x-50,            // x
                                           cen_y + ladr_h2 -20,  // y
                                           100,            // width
                                           20,            // height
                                           get_aileronval, // data source
                                           HUDS_BOTTOM | HUDS_NOTEXT,
                                           100.0,
                                           +1.0,
                                           -1.0);
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 3:    // Radio Altimeter
  HIptr = (instr_item *) new hud_card( cen_x + ladr_w2 + gap + 13 + ladr_t,
                                       cen_y-75,
                                       25,
                                       150,
                                       get_agl,
                                       HUDS_LEFT | HUDS_VERT,
                                       1000, 0,
                                       1.0,
                                       25, 5,
                                       0,
                                       0,
                                       200.0,
                                       true);
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 7:
  HIptr = (instr_item *) new  guage_instr( cen_x -ladr_w2 -gap -13 -20 -ladr_t,
                                           cen_y-50,             // y
                                           20,             // width
                                           100,             // height
                                           get_elevatorval, // data source
                                           HUDS_RIGHT | HUDS_VERT | HUDS_NOTEXT,
                                           -100.0,           // Scale data
                                           +1.0,           // Data Range
                                           -1.0);
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 8:
  HIptr = (instr_item *) new  guage_instr( cen_x-50,             // x
                                           cen_y -gap -ladr_w2 -20, //-85   // y
                                           100,             // width
                                           20,             // height
                                           get_rudderval,   // data source
                                           HUDS_TOP | HUDS_NOTEXT,
                                           100.0,
                                           +1.0,
                                           -1.0);
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 2:    // KIAS
  HIptr = (instr_item *) new hud_card( min_x +10 +5, //min_x +18,
                                       cen_y-(compass_w/2),
                                       28,
                                       compass_w,
                                       get_speed,
//                                     HUDS_LEFT | HUDS_VERT,
                                       HUDS_RIGHT | HUDS_VERT,                                     
                                       200.0, 0.0,
                                       1.0,
                                       10,  5,
                                       0,
                                       0,
                                       50.0,
                                       true);

  
 
  HUD_deque.insert( HUD_deque.begin(), HIptr);
    

//      case 10:    // Digital Mach number
        HIptr = (instr_item *) new instr_label ( min_x , //same as speed tape
                                                 cen_y-(compass_w/2) -10, //below speed tape
                                                  40,
                                                  30,
                                                 get_mach,
                                                 "%4.2f",
                                                 "",
                                                 NULL,
                                                 1.0,
                                                 HUDS_TOP,
                                                 RIGHT_JUST,
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 9:
  HIptr = (instr_item *) new  guage_instr( min_x-10,           // x
                                           cen_y -75,       // y 
                                           20,              // width
                                           150,             // height
                                           get_throttleval, // data source
//                                         HUDS_VERT | HUDS_RIGHT | HUDS_NOTEXT,
                                           HUDS_VERT | HUDS_LEFT | HUDS_NOTEXT,                                        100.0,
                                           1.0,
                                           0.0
                                          );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
#endif
// Remove this when below uncommented       
//      case 10:
  HIptr = (instr_item *) new instr_label( 10,
                                          25,
                                          60,
                                          10,
                                          get_frame_rate,
                                          "%5.1f",
                                          "",
                                          NULL,
                                          1.0,
                                          HUDS_TOP,
                                          RIGHT_JUST,
                                          font_size,
                                          0,
                                          TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
  
// $$$ begin - added VS Renganthan 19 Oct 2K
#ifdef FIGHTER_HUD
  HIptr = (instr_item *) new lat_label(  70,
                                          40,    
                                          1,
                                          text_h,
                                          get_latitude,
                                          "%s%", //"%.0f",
                                          "", //"Lat ",
                                          "",
                                          1.0,
                                          HUDS_TOP,
                                          CENTER_JUST,
                                          font_size,
                                          0,
                                          TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
    
    HIptr = (instr_item *) new lon_label( 475,
                                          40,
                                          1, text_h,
                                          get_longitude,
                                          "%s%",//"%.0f",
                                          "", //"Lon ", 
                                          "",
                                          1.0,
                                          HUDS_TOP,
                                          CENTER_JUST,
                                          font_size,
                                          0,
                                          TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
#else
    HIptr = (instr_item *) new lat_label(  (cen_x - (compass_w/2))/2,
                                          max_y,    
                                          1,
                                          text_h,
                                          get_latitude,
                                          "%s%", //"%.0f",
                                          "", //"Lat ",
                                          "",
                                          1.0,
                                          HUDS_TOP,
                                          CENTER_JUST,
                                          font_size,
                                          0,
                                          TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
    
    HIptr = (instr_item *) new lon_label(((cen_x+compass_w/2)+(2*cen_x))/2,
                                          max_y,
                                          1, text_h,
                                          get_longitude,
                                          "%s%",//"%.0f",
                                          "", //"Lon ", 
                                          "",
                                          1.0,
                                          HUDS_TOP,
                                          CENTER_JUST,
                                          font_size,
                                          0,
                                          TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
#endif
// $$$ end - added VS Renganthan 19 Oct 2K
/*
//      case 10:    // Digital KIAS
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
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 11:    // Digital Rate of Climb
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
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 12:    // Roll indication diagnostic
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
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 13:    // Angle of attack diagnostic
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
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 14:
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
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 15:
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
                                                 font_size,
                                                 0,
                                                 TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 16:
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
                                                font_size,
                                                0,
                                                TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 17:
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
                                                font_size,
                                                0,
                                                TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 18:
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
                                                font_size,
                                                0,
                                                TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 19:
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
                                                font_size,
                                                0,
                                                TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);

//      case 20:
      switch( current_options.get_tris_or_culled() ) {
      case 0:
          HIptr = (instr_item *) new instr_label( 10,
                              25,
                              120,
                              10,
                              get_vfc_tris_drawn,
                              "%.0f",
                              "Tris Rendered = ",
                              NULL,
                              1.0,
                              HUDS_TOP,
                              RIGHT_JUST,
                              font_size,
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
                              font_size,
                              0,
                              TRUE );
          break;
      }
      break;

//      case 21:
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
                                                font_size,
                                                0,
                                                TRUE );
  HUD_deque.insert( HUD_deque.begin(), HIptr);
*/
//      default:
//        HIptr = 0;;
//      }
//    if( HIptr ) {                   // Anything to install?
//      HUD_deque.insert( HUD_deque.begin(), HIptr);
//      }
//    index++;
//    }
//  while( HIptr );

  fgHUDalphaInit();
  fgHUDReshape();
   return 0;  // For now. Later we may use this for an error code.

}

int fgHUDInit2( fgAIRCRAFT * /* current_aircraft */ )
{
//    instr_item *HIptr;
//    int index;

    int off = 50;
//  int min_x = off;
//  int max_x = 640-off;
//  int min_y = off;
    int max_y = 480-off;
    int cen_x = 640 / 2;
    int cen_y = 480 / 2;
    int text_h = 10;
    int ladr_w2 = 60;
    int ladr_h2 = 90;
//  int ladr_t = 35;
    int compass_w = 200;
//  int gap = 10;

//	int font_size = current_options.get_xsize() / 60;
    int font_size = (current_options.get_xsize() > 1000) ? LARGE : SMALL;

    HUD_style = 2;

    FG_LOG( FG_COCKPIT, FG_INFO, "Initializing current aircraft HUD" );

//  deque < instr_item * > :: iterator first = HUD_deque.begin();
//  deque < instr_item * > :: iterator last = HUD_deque.end();
//  HUD_deque.erase( first, last);  // empty the HUD deque  
    HUD_deque.erase( HUD_deque.begin(), HUD_deque.end());

    //  hud->code = 1;
    //  hud->status = 0;

    // For now lets just hardcode the hud here.
    // In the future, hud information has to come from the same place
    // aircraft information came from.

    //  fgHUDSetTimeMode( hud, NIGHT );
    //  fgHUDSetBrightness( hud, BRT_LIGHT );

    //  index = 0;
//    index = 19;  

    instr_item* p;

    p = new HudLadder( cen_x-ladr_w2, cen_y-ladr_h2, 2*ladr_w2, 2*ladr_h2, 1 );
    HUD_deque.push_front( p );

//      case 4:    // GYRO COMPASS
    p =new hud_card( cen_x-(compass_w/2),
                     max_y,
                     compass_w,
                     28,
                     get_view_direction,
                     HUDS_TOP,
                     360, 0,
                     1.0,
                     5,   1,
                     360,
                     0,
                     25,
                     true);
    HUD_deque.push_front( p );

    p = new lat_label( (cen_x - compass_w/2)/2,
                       max_y,
                       0, text_h,
                       get_latitude,
                       "%s%", //"%.0f",
                       "", //"Lat ",
                       "",
                       1.0,
                       HUDS_TOP,
                       CENTER_JUST,
                       font_size,
                       0,
                       TRUE );
    HUD_deque.push_front( p );
    
//    p = new instr_label( 140, 450, 60, 10,
//           get_lat_min,
//           "%05.2f",
//           "",
//           NULL,
//           1.0,
//           HUDS_TOP,
//           CENTER_JUST,
//           font_size,
//           0,
//           TRUE );
//    HUD_deque.push_front( p );
    
    p = new lon_label(((cen_x+compass_w/2)+(2*cen_x))/2,
                       max_y,
                       1, text_h,
                       get_longitude,
                       "%s%",//"%.0f",
                       "", //"Lon ",
                       "",
                       1.0,
                       HUDS_TOP,
                       CENTER_JUST,
                       font_size,
                       0,
                       TRUE );
    HUD_deque.push_front( p );
    
    int x_pos = 40;
    
    p = new instr_label( x_pos, 25, 60, 10,
                         get_frame_rate,
                         "%7.1f",
                         "Frame rate =",
                         NULL,
                         1.0,
                         HUDS_TOP,
                         LEFT_JUST,
                         font_size,
                         0, 
                         TRUE );
    HUD_deque.push_front( p );
#if 0
    p = new instr_label( x_pos, 40, 120, 10,
                         get_vfc_tris_culled,
                         "%7.0f",
                         "Culled       =",
                         NULL,
                         1.0,
                         HUDS_TOP,
                         LEFT_JUST,
                         font_size,
                         0,
                         TRUE );
    HUD_deque.push_front( p );

    p = new instr_label( x_pos, 55, 120, 10,
                         get_vfc_tris_drawn,
                         "%7.0f",
                         "Rendered   =",
                         NULL,
                         1.0,
                         HUDS_TOP,
                         LEFT_JUST,
                         font_size,
                         0,
                         TRUE );
    HUD_deque.push_front( p );
#endif // 0
	
//    p = new instr_label( x_pos, 70, 90, 10,
    p = new instr_label( x_pos, 40, 90, 10,
                         get_fov,
                         "%7.1f",
                         "FOV          = ",
                         NULL,
                         1.0,
                         HUDS_TOP,
                         LEFT_JUST,
                         font_size,
                         0,
                         TRUE );
    HUD_deque.push_front( p );

    x_pos = 480;
    
    p = new instr_label ( x_pos,
                          70,
                          60,
                          10,
                          get_aoa,
                          "%7.2f",
                          "AOA      ",
                          " Deg",
                          1.0,
                          HUDS_TOP,
                          LEFT_JUST,
                          font_size,
                          0,
                          TRUE );
    HUD_deque.push_front( p );

    p = new instr_label( x_pos, 55, 40, 30,
                         get_speed,
                         "%5.0f",
                         "Airspeed ",
                         " Kts",
                         1.0,
                         HUDS_TOP,
                         LEFT_JUST,
                         font_size,
                         0,
                         TRUE );
    HUD_deque.push_front( p );

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
    strcpy(units, " ft");
    } else {
    strcpy(units, " m");
    }
    p = new instr_label( x_pos, 40, 40, 10,
             get_altitude,
             "%5.0f",
             "Altitude ",
             units,
             1.0,
             HUDS_TOP,
             LEFT_JUST,
             font_size,
             0,
             TRUE );
    HUD_deque.push_front( p );

    p = new instr_label( x_pos, 25, 40, 10,
             get_agl,
             "%5.0f",
             "Elvation ",
             units,
             1.0,
             HUDS_TOP,
             LEFT_JUST,
             font_size,
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
             LEFT_JUST,
             font_size,
             0,
             TRUE );
    HUD_deque.push_front( p );

    p = new fgTBI_instr( 290, 55, 60, 10 ); // 270
    HUD_deque.push_front( p );
    
    p = new  guage_instr( 270, //250,            // x
                          390, //360, //400, //45, //420,            // y
                          100,            // width
                          20,            // height
                          get_aileronval, // data source
                          HUDS_BOTTOM | HUDS_NOTEXT,
                          100.0,
                          +1.0,
                          -1.0);
    HUD_deque.push_front( p );

    p = new  guage_instr( 20,             // x
                          240-50,             // y
                          20,             // width
                          100,             // height
                          get_elevatorval, // data source
                          HUDS_RIGHT | HUDS_VERT | HUDS_NOTEXT,
                          -100.0,           // Scale data
                          +1.0,           // Data Range
                          -1.0);
    HUD_deque.push_front( p );

    p = new  guage_instr( 270, //250,             // x
                          10+15,             // y
                          100,             // width
                          20,             // height
                          get_rudderval,   // data source
                          HUDS_TOP | HUDS_NOTEXT,
                          100.0,
                          +1.0,
                          -1.0);
    HUD_deque.push_front( p );

    p = new  guage_instr( 600,             // x
                          240-80,
                          20,
                          160,             // height
                          get_throttleval, // data source
                          HUDS_VERT | HUDS_LEFT | HUDS_NOTEXT,
                          100.0,
                          1.0,
                          0.0);
    HUD_deque.push_front( p );
    
    return 0;  // For now. Later we may use this for an error code.
}

int global_day_night_switch = DAY;

void HUD_masterswitch( bool incr )
{
    if ( current_options.get_hud_status() ) {
	if ( global_day_night_switch == DAY ) {
	    global_day_night_switch = NIGHT;
	} else {
	    current_options.set_hud_status( false );
	}
    } else {
	current_options.set_hud_status( true );
	global_day_night_switch = DAY;
    }	
}

void HUD_brightkey( bool incr_bright )
{
    instr_item *pHUDInstr = HUD_deque[0];
    int brightness        = pHUDInstr->get_brightness();

    if( current_options.get_hud_status() ) {
	if( incr_bright ) {
	    switch (brightness)
		{
		case BRT_LIGHT:
		    brightness = BRT_BLACK;
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
	} else {
	    switch (brightness)
		{
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
		    brightness = BRT_LIGHT;
		    break;

		default:
		    current_options.set_hud_status(0);
		}
	}
    } else {
	current_options.set_hud_status(true);
    }

    pHUDInstr->SetBrightness( brightness );
}


#define fgAP_CLAMP(val,min,max) ( (val) = (val) > (max) ? (max) : (val) < (min) ? (min) : (val) )

static puDialogBox *HUDalphaDialog;
static puText      *HUDalphaText;
static puSlider    *HUDalphaHS0;
//static puText      *HUDtextText;
//static puSlider    *HUDalphaHS1;
static char         SliderText[2][ 8 ];

static void alpha_adj( puObject *hs ) {
	float val ;

	hs-> getValue ( &val ) ;
	fgAP_CLAMP ( val, 0.1, 1.0 ) ;
	//    printf ( "maxroll_adj( %p ) %f %f\n", hs, val, MaxRollAdjust * val ) ;
	hud_trans_alpha = val;
	sprintf( SliderText[ 0 ], "%05.2f", hud_trans_alpha );
	HUDalphaText -> setLabel ( SliderText[ 0 ] ) ;
}

void fgHUDalphaAdjust( puObject * ) {
	current_options.set_anti_alias_hud(1);
	FG_PUSH_PUI_DIALOG( HUDalphaDialog );
}

static void goAwayHUDalphaAdjust (puObject *)
{
	FG_POP_PUI_DIALOG( HUDalphaDialog );
}

static void cancelHUDalphaAdjust (puObject *)
{
	current_options.set_anti_alias_hud(0);
	FG_POP_PUI_DIALOG( HUDalphaDialog );
}

// Done once at system initialization
void fgHUDalphaInit( void ) {

	//	printf("fgHUDalphaInit\n");
#define HORIZONTAL  FALSE

	int DialogX = 40;
	int DialogY = 100;
	int DialogWidth = 240;

	char Label[] =  "HUD Adjuster";
	char *s;

	int labelX = (DialogWidth / 2) -
				 (puGetStringWidth( puGetDefaultLabelFont(), Label ) / 2);
	
	int nSliders = 1;
	int slider_x = 10;
	int slider_y = 55;
	int slider_width = 220;
	int slider_title_x = 15;
	int slider_value_x = 160;
	float slider_delta = 0.05f;

	puFont HUDalphaLegendFont;
	puFont HUDalphaLabelFont;
	puGetDefaultFonts ( &HUDalphaLegendFont, &HUDalphaLabelFont );
	
	HUDalphaDialog = new puDialogBox ( DialogX, DialogY ); {
		int horiz_slider_height = puGetStringHeight (HUDalphaLabelFont) +
								  puGetStringDescender (HUDalphaLabelFont) +
								  PUSTR_TGAP + PUSTR_BGAP + 5;

		puFrame *
		HUDalphaFrame = new puFrame ( 0, 0,
									  DialogWidth,
									  85 + nSliders * horiz_slider_height );
		
		puText *
		HUDalphaDialogMessage = new puText ( labelX,
											 52 + nSliders
											 * horiz_slider_height );
		HUDalphaDialogMessage -> setDefaultValue ( Label );
		HUDalphaDialogMessage -> getDefaultValue ( &s );
		HUDalphaDialogMessage -> setLabel        ( s );

		HUDalphaHS0 = new puSlider ( slider_x, slider_y,
									 slider_width, HORIZONTAL ) ;
		HUDalphaHS0->     setDelta ( slider_delta ) ;
		HUDalphaHS0->     setValue ( hud_trans_alpha ) ;
		HUDalphaHS0->    setCBMode ( PUSLIDER_DELTA ) ;
		HUDalphaHS0->  setCallback ( alpha_adj ) ;

		puText *
		HUDalphaTitle =      new puText ( slider_title_x, slider_y ) ;
		HUDalphaTitle-> setDefaultValue ( "Alpha" ) ;
		HUDalphaTitle-> getDefaultValue ( &s ) ;
		HUDalphaTitle->        setLabel ( s ) ;
		
		HUDalphaText = new puText ( slider_value_x, slider_y ) ;
		sprintf( SliderText[ 0 ], "%05.2f", hud_trans_alpha );
		HUDalphaText-> setLabel ( SliderText[ 0 ] ) ;


		puOneShot *
		HUDalphaOkButton =     new puOneShot ( 10, 10, 60, 45 );
		HUDalphaOkButton->         setLegend ( gui_msg_OK );
		HUDalphaOkButton-> makeReturnDefault ( TRUE );
		HUDalphaOkButton->       setCallback ( goAwayHUDalphaAdjust );
		
		puOneShot *
		HUDalphaNoButton = new puOneShot ( 160, 10, 230, 45 );
		HUDalphaNoButton->     setLegend ( gui_msg_CANCEL );
		HUDalphaNoButton->   setCallback ( cancelHUDalphaAdjust );
	}
	FG_FINALIZE_PUI_DIALOG( HUDalphaDialog );

#undef HORIZONTAL
}

void fgHUDReshape(void) {
	if ( HUDtext )
		delete HUDtext;

	HUD_TextSize = current_options.get_xsize() / 60;
        HUD_TextSize = 10;
	HUDtext = new fntRenderer();
	HUDtext -> setFont      ( guiFntHandle ) ;
	HUDtext -> setPointSize ( HUD_TextSize ) ;
	HUD_TextList.setFont( HUDtext );
}


static void set_hud_color(float r, float g, float b) {
	current_options.get_anti_alias_hud() ?
			glColor4f(r,g,b,hud_trans_alpha) :
			glColor3f(r,g,b);
}

// fgUpdateHUD
//
// Performs a once around the list of calls to instruments installed in
// the HUD object with requests for redraw. Kinda. It will when this is
// all C++.
//
void fgUpdateHUD( void ) {
  int brightness;
//  int day_night_sw = current_aircraft.controls->day_night_switch;
  int day_night_sw = global_day_night_switch;
  int hud_displays = HUD_deque.size();
  instr_item *pHUDInstr;
  float line_width;

  if( !hud_displays ) {  // Trust everyone, but ALWAYS cut the cards!
    return;
    }

  HUD_TextList.erase();
  HUD_LineList.erase();
//  HUD_StippleLineList.erase();
  
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

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  if( current_options.get_anti_alias_hud() ) {
	  glEnable(GL_LINE_SMOOTH);
//	  glEnable(GL_BLEND);
	  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	  glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
	  glLineWidth(1.5);
  } else {
	  glLineWidth(1.0);
  }

  if( day_night_sw == DAY) {
      switch (brightness)
	  {
	  case BRT_LIGHT:
	      set_hud_color (0.1f, 0.9f, 0.1f);
	      break;

          case BRT_MEDIUM:
	      set_hud_color (0.1f, 0.7f, 0.0f);
	      break;

          case BRT_DARK:
	      set_hud_color (0.0f, 0.6f, 0.0f);
	      break;

          case BRT_BLACK:
	      set_hud_color( 0.0f, 0.0f, 0.0f);
	      break;

          default:
	      set_hud_color (0.1f, 0.9f, 0.1f);
	  }
  } else {
      if( day_night_sw == NIGHT) {
	  switch (brightness)
	      {
	      case BRT_LIGHT:
		  set_hud_color (0.9f, 0.1f, 0.1f);
		  break;

              case BRT_MEDIUM:
		  set_hud_color (0.7f, 0.0f, 0.1f);
		  break;

	      case BRT_DARK:
		  set_hud_color (0.6f, 0.0f, 0.0f);
		  break;

	      case BRT_BLACK:
		  set_hud_color( 0.0f, 0.0f, 0.0f);
		  break;

	      default:
		  set_hud_color (0.6f, 0.0f, 0.0f);
	      }
      } else {     // Just in case default
	  set_hud_color (0.1f, 0.9f, 0.1f);
      }
  }

  deque < instr_item * > :: iterator current = HUD_deque.begin();
  deque < instr_item * > :: iterator last = HUD_deque.end();

  for ( ; current != last; ++current ) {
	  pHUDInstr = *current;

	  if( pHUDInstr->enabled()) {
		  //  fgPrintf( FG_COCKPIT, FG_DEBUG, "HUD Code %d  Status %d\n",
		  //            hud->code, hud->status );
		  pHUDInstr->draw();
	  }
  }

  char *gmt_str = get_formated_gmt_time();
  HUD_TextList.add( fgText(40, 10, gmt_str) );

#ifdef FG_NETWORK_OLK
  if ( net_hud_display ) {
      net_hud_update();
  }
#endif


  // temporary
  // extern bool fgAPAltitudeEnabled( void );
  // extern bool fgAPHeadingEnabled( void );
  // extern bool fgAPWayPointEnabled( void );
  // extern char *fgAPget_TargetDistanceStr( void );
  // extern char *fgAPget_TargetHeadingStr( void );
  // extern char *fgAPget_TargetAltitudeStr( void );
  // extern char *fgAPget_TargetLatLonStr( void );

  int apY = 480 - 80;
//  char scratch[128];
//  HUD_TextList.add( fgText( "AUTOPILOT", 20, apY) );
//  apY -= 15;
  if( current_autopilot->get_HeadingEnabled() ) {
      HUD_TextList.add( fgText( 40, apY, 
				current_autopilot->get_TargetHeadingStr()) );
      apY -= 15;
  }
  if( current_autopilot->get_AltitudeEnabled() ) {
      HUD_TextList.add( fgText( 40, apY, 
				current_autopilot->get_TargetAltitudeStr()) );
      apY -= 15;
  }
  if( current_autopilot->get_HeadingMode() == 
      FGAutopilot::FG_HEADING_WAYPOINT )
  {
      char *wpstr;
      wpstr = current_autopilot->get_TargetWP1Str();
      if ( strlen( wpstr ) ) {
	  HUD_TextList.add( fgText( 40, apY, wpstr ) );
	  apY -= 15;
      }
      wpstr = current_autopilot->get_TargetWP2Str();
      if ( strlen( wpstr ) ) {
	  HUD_TextList.add( fgText( 40, apY, wpstr ) );
	  apY -= 15;
      }
      wpstr = current_autopilot->get_TargetWP3Str();
      if ( strlen( wpstr ) ) {
	  HUD_TextList.add( fgText( 40, apY, wpstr ) );
	  apY -= 15;
      }
  }
  
  HUD_TextList.draw();

  HUD_LineList.draw();

//  glEnable(GL_LINE_STIPPLE);
//  glLineStipple( 1, 0x00FF );
//  HUD_StippleLineList.draw();
//  glDisable(GL_LINE_STIPPLE);

  if( current_options.get_anti_alias_hud() ) {
//	  glDisable(GL_BLEND);
	  glDisable(GL_LINE_SMOOTH);
	  glLineWidth(1.0);
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

