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

#ifdef HAVE_VALUES_H
#  include <values.h>  // for MAXINT
#endif

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <GUI/gui.h>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Network/network.h>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>

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
  int font_size;

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

  font_size = (current_options.get_xsize() > 1000) ? LARGE : SMALL;
  
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

  return 0;  // For now. Later we may use this for an error code.

}

int fgHUDInit2( fgAIRCRAFT * /* current_aircraft */ )
{
//    instr_item *HIptr;
//    int index;
    int font_size;

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

    font_size = (current_options.get_xsize() > 1000) ? LARGE : SMALL;

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

#if 0
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

  glColor3f(1.0, 1.0, 1.0);
  glIndexi(7);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  // We can do translucency, so why not. :-)
//  glEnable    ( GL_BLEND ) ;
//  glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
  
  if( day_night_sw == DAY) {
      switch (brightness) {
          case BRT_LIGHT:
//            glColor4f (0.1, 0.9, 0.1, 0.75);
            glColor3f (0.1, 0.9, 0.1);
            break;

          case BRT_MEDIUM:
//            glColor4f (0.1, 0.7, 0.0, 0.75);
            glColor3f (0.1, 0.7, 0.0);
            break;

          case BRT_DARK:
//            glColor4f (0.0, 0.6, 0.0, 0.75);
            glColor3f(0.0, 0.6, 0.0);
            break;

          case BRT_BLACK:
//            glColor4f( 0.0, 0.0, 0.0, 0.75);
            glColor3f( 0.0, 0.0, 0.0);
            break;

          default:;
      }
  }
  else {
      if( day_night_sw == NIGHT) {
          switch (brightness) {
              case BRT_LIGHT:
//                glColor4f (0.9, 0.1, 0.1, 0.75);
                glColor3f (0.9, 0.1, 0.1);
                break;

              case BRT_MEDIUM:
//                glColor4f (0.7, 0.0, 0.1, 0.75);
                glColor3f (0.7, 0.0, 0.1);
                break;

              case BRT_DARK:
              default:
//                glColor4f (0.6, 0.0, 0.0, 0.75);
                  glColor3f (0.6, 0.0, 0.0);
          }
      }
          else {     // Just in case default
//            glColor4f (0.1, 0.9, 0.1, 0.75);
              glColor3f (0.1, 0.9, 0.1);
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
//        HUD_deque.at(i)->draw(); // Responsible for broken or fixed variants.
                              // No broken displays honored just now.
      }
  }

  char *gmt_str = get_formated_gmt_time();
  HUD_TextList.add( fgText( 40, 10, gmt_str) );

#ifdef FG_NETWORK_OLK
  if ( net_hud_display ) {
      net_hud_update();
  }
#endif

  HUD_TextList.draw();

  line_width = (current_options.get_xsize() > 1000) ? 1.0 : 0.5;
  glLineWidth(line_width);
  HUD_LineList.draw();

//  glEnable(GL_LINE_STIPPLE);
//  glLineStipple( 1, 0x00FF );
//  HUD_StippleLineList.draw();
//  glDisable(GL_LINE_STIPPLE);

//  glDisable( GL_BLEND );
  
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}
#endif


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

  glColor3f(1.0, 1.0, 1.0);
  glIndexi(7);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  // We can do translucency, so why not. :-)
//  glEnable    ( GL_BLEND ) ;
//  glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
  
  if( day_night_sw == DAY) {
	  switch (brightness) {
		  case BRT_LIGHT:
//            glColor4f (0.1, 0.9, 0.1, 0.75);
            glColor3f (0.1, 0.9, 0.1);
            break;

          case BRT_MEDIUM:
//            glColor4f (0.1, 0.7, 0.0, 0.75);
            glColor3f (0.1, 0.7, 0.0);
            break;

          case BRT_DARK:
//            glColor4f (0.0, 0.6, 0.0, 0.75);
            glColor3f(0.0, 0.6, 0.0);
            break;

          case BRT_BLACK:
//            glColor4f( 0.0, 0.0, 0.0, 0.75);
            glColor3f( 0.0, 0.0, 0.0);
            break;

          default:;
	  }
  }
  else {
	  if( day_night_sw == NIGHT) {
		  switch (brightness) {
			  case BRT_LIGHT:
//                glColor4f (0.9, 0.1, 0.1, 0.75);
                glColor3f (0.9, 0.1, 0.1);
                break;

              case BRT_MEDIUM:
//                glColor4f (0.7, 0.0, 0.1, 0.75);
                glColor3f (0.7, 0.0, 0.1);
                break;

			  case BRT_DARK:
			  default:
//				  glColor4f (0.6, 0.0, 0.0, 0.75);
				  glColor3f (0.6, 0.0, 0.0);
		  }
	  }
          else {     // Just in case default
//			  glColor4f (0.1, 0.9, 0.1, 0.75);
			  glColor3f (0.1, 0.9, 0.1);
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
//	      HUD_deque.at(i)->draw(); // Responsible for broken or fixed variants.
                              // No broken displays honored just now.
	  }
  }

  char *gmt_str = get_formated_gmt_time();
  HUD_TextList.add( fgText(40, 10, gmt_str) );

  // temporary
  extern bool fgAPAltitudeEnabled( void );
  extern bool fgAPHeadingEnabled( void );
  extern bool fgAPWayPointEnabled( void );
  extern char *fgAPget_TargetDistanceStr( void );
  extern char *fgAPget_TargetHeadingStr( void );
  extern char *fgAPget_TargetAltitudeStr( void );
  extern char *fgAPget_TargetLatLonStr( void );

  int apY = 480 - 80;
//  char scratch[128];
//  HUD_TextList.add( fgText( "AUTOPILOT", 20, apY) );
//  apY -= 15;
  if( fgAPHeadingEnabled() ) {
	  HUD_TextList.add( fgText( 40, apY, fgAPget_TargetHeadingStr()) );	  
	  apY -= 15;
  }
  if( fgAPAltitudeEnabled() ) {
	  HUD_TextList.add( fgText( 40, apY, fgAPget_TargetAltitudeStr()) );	  
	  apY -= 15;
  }
  if( fgAPWayPointEnabled() ) {
	  HUD_TextList.add( fgText( 40, apY, fgAPget_TargetLatLonStr()) );
	  apY -= 15;
	  HUD_TextList.add( fgText( 40, apY, fgAPget_TargetDistanceStr() ) );	  
	  apY -= 15;
  }
  
  HUD_TextList.draw();

  line_width = (current_options.get_xsize() > 1000) ? 1.0 : 0.5;
  glLineWidth(line_width);
  HUD_LineList.draw();

//  glEnable(GL_LINE_STIPPLE);
//  glLineStipple( 1, 0x00FF );
//  HUD_StippleLineList.draw();
//  glDisable(GL_LINE_STIPPLE);

//  glDisable( GL_BLEND );
  
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

