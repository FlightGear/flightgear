/**************************************************************************
 * hud.c -- hud defines and prototypes
 *
 * Written by Michele America, started September 1997.
 *
 * Copyright (C) 1997  Michele F. America  - micheleamerica@geocities.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/
 

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

#include <Aircraft/aircraft.h>
#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar.h>
#include <Scenery/scenery.h>
#include <Time/fg_timer.hxx>
#include <Weather/weather.h>

#include "hud.hxx"

// #define DEBUG

#define drawOneLine(x1,y1,x2,y2)  glBegin(GL_LINES);  \
	glVertex2f ((x1),(y1)); glVertex2f ((x2),(y2)); glEnd();


// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

double get_throttleval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->throttle[0];     // Hack limiting to one engine
}

double get_aileronval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->aileron;
}

double get_elevatorval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->elevator;
}

double get_elev_trimval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->elevator_trim;
}

double get_rudderval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->rudder;
}

double get_speed( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_V_equiv_kts );    // Make an explicit function call.
}

double get_aoa( void )
{
	fgFLIGHT *f;
              
	f = current_aircraft.flight;
	return( FG_Gamma_vert_rad * RAD_TO_DEG );
}

double get_roll( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_Phi );
}

double get_pitch( void )
{
	fgFLIGHT *f;
              
	f = current_aircraft.flight;
	return( FG_Theta );
}

double get_heading( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_Psi * RAD_TO_DEG );
}

double get_altitude( void )
{
	fgFLIGHT *f;
	// double rough_elev;

	f = current_aircraft.flight;
	// rough_elev = mesh_altitude(FG_Longitude * RAD_TO_ARCSEC,
	//		                   FG_Latitude  * RAD_TO_ARCSEC);

	return( FG_Altitude * FEET_TO_METER /* -rough_elev */ );
}

double get_sideslip( void )
{
        fgFLIGHT *f;
        
        f = current_aircraft.flight;
        
        return( FG_Beta );
}

/****************************************************************************/
/* Convert degrees to dd mm.mmm' (DMM-Format)                               */
/****************************************************************************/
#if 0
char *toDM(double a)
{
  short        neg = 0;
  double       d, m;
  static char  dm[13];
  
  if (a < 0.0) {
    a = -a;
    neg = 1;
  }

  d = (double) ( (int) a);
  m = (a - d) * 60.0;
  
  if (m > 59.5) {
    m  = 0.0;
    d += 1.0;
  }
  if (neg) d = -d;
  
  sprintf(dm, "%.0f°%06.3f'", d, m);
  return dm;
}
#endif // 0
double get_latitude( void )
{
	fgFLIGHT *f;
	f = current_aircraft.flight;

//	return( toDM(FG_Latitude * RAD_TO_DEG) );
	return((double)((int)( FG_Latitude * RAD_TO_DEG)) );	
}
double get_lat_min( void )
{
	fgFLIGHT *f;
	double      a, d;

	f = current_aircraft.flight;
	
	a = FG_Latitude * RAD_TO_DEG;	
	if (a < 0.0) {
		a = -a;
	}
	d = (double) ( (int) a);
	return( (a - d) * 60.0);
}


double get_longitude( void )
{
	fgFLIGHT *f;
	f = current_aircraft.flight;

//	return( toDM(FG_Longitude * RAD_TO_DEG) );
	return((double)((int) (FG_Longitude * RAD_TO_DEG)) );
}
double get_long_min( void )
{
	fgFLIGHT *f;
	double  a, d;

	f = current_aircraft.flight;
	
	a = FG_Longitude * RAD_TO_DEG;	
	if (a < 0.0) {
		a = -a;
	}
	d = (double) ( (int) a);
	return( (a - d) * 60.0);
}

//
// The following code deals with painting the "instrument" on the display
//
   /* textString - Bitmap font string */

static void textString(int x, int y, char *msg, void *font)
{
	glRasterPos2f(x, y);
	while (*msg) {
		glutBitmapCharacter(font, *msg);
		msg++;
    }
}

/* strokeString - Stroke font string */
/*
static void strokeString(int x, int y, char *msg, void *font)
{
	glPushMatrix();
	glTranslatef(x, y, 0);
	glScalef(.04, .04, .04);
	while (*msg) {
		glutStrokeCharacter(font, *msg);
		msg++;
	}
	glPopMatrix();
}
*/

/*

	Draws a measuring scale anywhere on the HUD


	Needs: HUD_scale struct

*/
static void drawscale( HUD_scale * pscale )
{
  double vmin, vmax;
  int marker_x;
  int marker_y;
  register i;
  char TextScale[80];
  int condition;
  double cur_value = (*(pscale->load_value))();
  int disp_val;

  vmin = cur_value - pscale->half_width_units; // width units == needle travel
  vmax = cur_value + pscale->half_width_units; // or picture unit span.


  if( pscale->type == VERTICAL )     // Vertical scale
    {
      drawOneLine( pscale->scrn_pos.right,    // Vertical scale bar
                   pscale->scrn_pos.bottom,
                   pscale->scrn_pos.right,
                   pscale->scrn_pos.top );

    if( pscale->orientation == LEFT )     // Calculate x marker offset
      marker_x = pscale->scrn_pos.left - 6;
    else
      if( pscale->orientation == RIGHT )
        marker_x = pscale->scrn_pos.right;

    // Draw the basic markings for the scale...

    if( pscale->orientation == LEFT )
      {

      drawOneLine( pscale->scrn_pos.right - 3,     // Bottom tick bar
                   pscale->scrn_pos.bottom,
                   pscale->scrn_pos.right,
                   pscale->scrn_pos.bottom );

      drawOneLine( pscale->scrn_pos.right - 3,     // Top tick bar
                   pscale->scrn_pos.top,
                   pscale->scrn_pos.right,
                   pscale->scrn_pos.top );

      drawOneLine( pscale->scrn_pos.right,       // Middle tick bar /Index
                   pscale->mid_scr,
                   pscale->scrn_pos.right + 6,
                   pscale->mid_scr );

      }
    else
      if( pscale->orientation == RIGHT )
        {
        drawOneLine( pscale->scrn_pos.right,
                     pscale->scrn_pos.bottom,
                     pscale->scrn_pos.right+3,
                     pscale->scrn_pos.bottom );

        drawOneLine( pscale->scrn_pos.right,
                     pscale->scrn_pos.top,
                     pscale->scrn_pos.right+3,
                     pscale->scrn_pos.top );

        drawOneLine( pscale->scrn_pos.right,
                     pscale->mid_scr,
                     pscale->scrn_pos.right-6,
                     pscale->mid_scr );
        }

    // Work through from bottom to top of scale. Calculating where to put
    // minor and major ticks.

    for( i = (int)(vmin); i <= (int)(vmax); i++ )
      {
      if( pscale->sub_type == LIMIT ) {           // Don't show ticks
        condition = (i >= pscale->minimum_value); // below Minimum value.
        }
      else {
        if( pscale->sub_type == NOLIMIT ) {
          condition = 1;
          }
        }
      if( condition )   // Show a tick if necessary
        {
        // Calculate the location of this tick
        marker_y = (int)(pscale->scrn_pos.bottom + (i - vmin) * pscale->factor);

        // Block calculation artifact from drawing ticks below min coordinate.
        // Calculation here accounts for text height.

        if( marker_y < (pscale->scrn_pos.bottom + 4)) {  // Magic number!!!
          continue;
          }
        if( (i%pscale->div_min) == 0) {
          if( pscale->orientation == LEFT )
            {
            drawOneLine( marker_x + 3, marker_y, marker_x + 6, marker_y );
            }
          else {
            if( pscale->orientation == RIGHT )
              {
              drawOneLine( marker_x, marker_y, marker_x + 3, marker_y );
              }
            }
          }
        if( (i%pscale->div_max) == 0 )            {
          drawOneLine( marker_x,     marker_y,
                       marker_x + 6, marker_y );
          if(pscale->modulo) {
            disp_val = i % pscale->modulo;
            if( !disp_val ) {
              disp_val = pscale->modulo;
              }
            if( disp_val < 0) {
              disp_val += pscale->modulo;
              }
            if( disp_val == pscale->modulo ) {
              disp_val = 0;
              }
            }
          else {
            disp_val = i;
            }
          sprintf( TextScale, "%d", disp_val );
          if( pscale->orientation == LEFT )              {
            textString( marker_x -  8 * strlen(TextScale) - 2, marker_y - 4,
                        TextScale, GLUT_BITMAP_8_BY_13 );
            }
          else  {
            if( pscale->orientation == RIGHT )              {
            textString( marker_x + 10,                         marker_y - 4,
                        TextScale, GLUT_BITMAP_8_BY_13 );
              }
            }
          }
        } // End if condition
      } // End for range of i from vmin to vmax
    }  // End if VERTICAL SCALE TYPE
  if( pscale->type == HORIZONTAL )     // Horizontal scale
    {
    if( pscale->orientation == TOP ) {
      marker_y = pscale->scrn_pos.bottom;
      }
    else {
      if( pscale->orientation == BOTTOM ) {
        marker_y = pscale->scrn_pos.bottom - 6;
        }
      }
    drawOneLine( pscale->scrn_pos.left,
                 pscale->scrn_pos.bottom,
                 pscale->scrn_pos.right,
                 pscale->scrn_pos.bottom );

    if( pscale->orientation == TOP )
      {
      drawOneLine( pscale->scrn_pos.left,
                   pscale->scrn_pos.bottom,
                   pscale->scrn_pos.left,
                   pscale->scrn_pos.bottom + 3 );

      drawOneLine( pscale->scrn_pos.right,
                   pscale->scrn_pos.bottom,
                   pscale->scrn_pos.right,
                   pscale->scrn_pos.bottom + 6 );

      drawOneLine( pscale->mid_scr,
                   pscale->scrn_pos.bottom,
                   pscale->mid_scr,
                   pscale->scrn_pos.bottom - 6 );

      }
    else {
      if( pscale->orientation == BOTTOM )
        {
        drawOneLine( pscale->scrn_pos.left,
                     pscale->scrn_pos.bottom,
                     pscale->scrn_pos.left,
                     pscale->scrn_pos.bottom - 6 );

        drawOneLine( pscale->scrn_pos.right,
                     pscale->scrn_pos.bottom,
                     pscale->scrn_pos.right,
                     pscale->scrn_pos.bottom - 6 );

        drawOneLine( pscale->mid_scr,
                     pscale->scrn_pos.bottom,
                     pscale->mid_scr,
                     pscale->scrn_pos.bottom + 6 );
        }
      }

    for( i = (int)(vmin); i <= (int)(vmax); i++ )  // increment is faster than addition
      {
      if( pscale->sub_type == LIMIT ) {
        condition = (i >= pscale->minimum_value);
        }
      else {
        if( pscale->sub_type == NOLIMIT ) {
          condition = 1;
          }
        }
      if( condition )        {
        marker_x = (int)(pscale->scrn_pos.left + (i - vmin) * pscale->factor);
        if( (i%pscale->div_min) == 0 ) {
          if( pscale->orientation == TOP )
            {
            drawOneLine( marker_x, marker_y, marker_x, marker_y + 3 );
            }
          else {
            if( pscale->orientation == BOTTOM )
              {
              drawOneLine( marker_x, marker_y + 3, marker_x, marker_y + 6 );
              }
            }
          }
        if( (i%pscale->div_max)==0 )
          {
          if(pscale->modulo) {
            disp_val = i % pscale->modulo;
            if( !disp_val ) {
              disp_val = pscale->modulo;
              }
            if( disp_val < 0) {
              disp_val += pscale->modulo;
              }
            if( disp_val == pscale->modulo ) {
              disp_val = 0;
              }
            }
          else {
            disp_val = i;
            }
            sprintf( TextScale, "%d", disp_val );
          if( pscale->orientation == TOP )
            {
            drawOneLine( marker_x, marker_y, marker_x, marker_y+6 );
            textString ( marker_x - 4 * strlen(TextScale), marker_y + 14,
                         TextScale, GLUT_BITMAP_8_BY_13 );
            }
          else {
            if( pscale->orientation == BOTTOM )
              {
              drawOneLine( marker_x, marker_y, marker_x, marker_y+6 );
              textString ( marker_x - 4 * strlen(TextScale), marker_y - 14,
                           TextScale, GLUT_BITMAP_8_BY_13 );
              }
            }
          }
        }
      }
    }

}

//
//	Draws a climb ladder in the center of the HUD
//

static void drawladder( HUD_ladder *ladder )
{
  double vmin, vmax;
  double roll_value, pitch_value;
  int marker_x, marker_y;
#ifdef DEBUGHUD
  int mid_scr;
#endif
  int scr_min, scr_max;
  int x_ini, x_end;
  int y_ini, y_end;
  int new_x_ini, new_x_end;
  int new_y_ini, new_y_end;
  register i;
  double factor;
  char TextLadder[80];
  int condition;

  double cos_roll_value, sin_roll_value;
//  double cos_pitch_value, sin_pitch_value;

  roll_value = (*ladder->load_roll)();
  sin_roll_value = sin(roll_value);
  cos_roll_value = cos(roll_value);
  
  pitch_value = (*ladder->load_pitch)()*RAD_TO_DEG;

  vmin = pitch_value - ladder->width_units/2;
  vmax = pitch_value + ladder->width_units/2;

  scr_min = ladder->scrn_pos.y - (ladder->scr_height/2);
  scr_max = scr_min       + ladder->scr_height;

#ifdef DEBUGHUD
  mid_scr = scr_min       + (scr_max-scr_min)/2;
#endif

  marker_x = ladder->scrn_pos.x - ladder->scr_width/2;

  factor = (scr_max-scr_min)/ladder->width_units;

  for( i=(int)(vmin); i<=(int)(vmax); i+=1 )
    {
    condition = 1;
    if( condition )
      {
      marker_y = (int)(scr_min+(i-vmin)*factor);
      if( i%ladder->div_units==0 )
        {
        sprintf( TextLadder, "%d", i );
        if( ladder->scr_hole == 0 )
          {
          if( i ) {
            x_ini = ladder->scrn_pos.x - ladder->scr_width/2;
            }
          else {
            x_ini = ladder->scrn_pos.x - ladder->scr_width/2 - 10;
            }
          y_ini = marker_y;
          x_end = ladder->scrn_pos.x + ladder->scr_width/2;
          y_end = marker_y;
          new_x_ini = (int)(ladder->scrn_pos.x +                             \
                      (x_ini - ladder->scrn_pos.x) * cos_roll_value - \
                      (y_ini - ladder->scrn_pos.y) * sin_roll_value);
          new_y_ini = (int)(ladder->scrn_pos.y +                             \
                      (x_ini - ladder->scrn_pos.x) * sin_roll_value + \
                      (y_ini - ladder->scrn_pos.y) * cos_roll_value);
          new_x_end = (int)(ladder->scrn_pos.x +                             \
                      (x_end - ladder->scrn_pos.x) * cos_roll_value - \
                      (y_end - ladder->scrn_pos.y) * sin_roll_value);
          new_y_end = (int)(ladder->scrn_pos.y +                             \
                      (x_end - ladder->scrn_pos.x) * sin_roll_value + \
                      (y_end - ladder->scrn_pos.y) * cos_roll_value);

          if( i >= 0 )
            {
            drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
            }
          else
            {
            glEnable(GL_LINE_STIPPLE);
            glLineStipple( 1, 0x00FF );
            drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
            glDisable(GL_LINE_STIPPLE);
            }
          textString( new_x_ini -  8 * strlen(TextLadder) - 8,
                      new_y_ini -  4,
                      TextLadder, GLUT_BITMAP_8_BY_13 );
          textString( new_x_end + 10,
                      new_y_end -  4,
                      TextLadder, GLUT_BITMAP_8_BY_13 );
          }
        else
          {
          if( i != 0 )  {
            x_ini = ladder->scrn_pos.x - ladder->scr_width/2;
            }
          else          {
            x_ini = ladder->scrn_pos.x - ladder->scr_width/2 - 10;
            }
          y_ini = marker_y;
          x_end = ladder->scrn_pos.x - ladder->scr_width/2 + ladder->scr_hole/2;
          y_end = marker_y;
          new_x_ini = (int)(ladder->scrn_pos.x+                             \
                      (x_ini - ladder->scrn_pos.x) * cos_roll_value -\
                      (y_ini - ladder->scrn_pos.y) * sin_roll_value);
          new_y_ini = (int)(ladder->scrn_pos.y+                             \
                      (x_ini - ladder->scrn_pos.x) * sin_roll_value +\
                      (y_ini - ladder->scrn_pos.y) * cos_roll_value);
          new_x_end = (int)(ladder->scrn_pos.x+                             \
                      (x_end - ladder->scrn_pos.x) * cos_roll_value -\
                      (y_end - ladder->scrn_pos.y) * sin_roll_value);
          new_y_end = (int)(ladder->scrn_pos.y+                             \
                      (x_end - ladder->scrn_pos.x) * sin_roll_value +\
                      (y_end - ladder->scrn_pos.y) * cos_roll_value);

          if( i >= 0 )
            {
            drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
            }
          else
            {
            glEnable(GL_LINE_STIPPLE);
            glLineStipple( 1, 0x00FF );
            drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
            glDisable(GL_LINE_STIPPLE);
            }
          textString( new_x_ini - 8 * strlen(TextLadder) - 8,
                      new_y_ini - 4,
                      TextLadder, GLUT_BITMAP_8_BY_13 );

          x_ini = ladder->scrn_pos.x + ladder->scr_width/2 - ladder->scr_hole/2;
          y_ini = marker_y;
          if( i != 0 )  {
            x_end = ladder->scrn_pos.x + ladder->scr_width/2;
            }
          else          {
            x_end = ladder->scrn_pos.x + ladder->scr_width/2 + 10;
            }
          y_end = marker_y;
          new_x_ini = (int)(ladder->scrn_pos.x +                        \
                      (x_ini-ladder->scrn_pos.x)*cos_roll_value -\
                      (y_ini-ladder->scrn_pos.y)*sin_roll_value);
          new_y_ini = (int)(ladder->scrn_pos.y +                        \
                      (x_ini-ladder->scrn_pos.x)*sin_roll_value +\
                      (y_ini-ladder->scrn_pos.y)*cos_roll_value);
          new_x_end = (int)(ladder->scrn_pos.x +                        \
                      (x_end-ladder->scrn_pos.x)*cos_roll_value -\
           	      (y_end-ladder->scrn_pos.y)*sin_roll_value);
          new_y_end = (int)(ladder->scrn_pos.y +                        \
                      (x_end-ladder->scrn_pos.x)*sin_roll_value +\
                      (y_end-ladder->scrn_pos.y)*cos_roll_value);

          if( i >= 0 )
            {
            drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
            }
          else
            {
            glEnable(GL_LINE_STIPPLE);
            glLineStipple( 1, 0x00FF );
            drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
            glDisable(GL_LINE_STIPPLE);
            }
          textString( new_x_end+10, new_y_end-4,
                      TextLadder, GLUT_BITMAP_8_BY_13 );
          }
        }
            /* if( i%pscale->div_max==0 )
            {
            	drawOneLine( marker_x, marker_y, marker_x+6, marker_y );
                sprintf( TextScale, "%d", i );
                if( pscale->orientation == LEFT )
                {
                	textString( marker_x-8*strlen(TextScale)-2, marker_y-4,
                              TextScale, GLUT_BITMAP_8_BY_13 );
                }
                else if( pscale->orientation == RIGHT )
                {
                	textString( marker_x+10, marker_y-4,
                              TextScale, GLUT_BITMAP_8_BY_13 );
                }
            } */
        }
    }

}

//
//	Draws an artificial horizon line in the center of the HUD
//	(with or without a center hole)
//
//	Needs: x_center, y_center, length, hole
//

static void drawhorizon( HUD_horizon *horizon )
{
  int x_inc1, y_inc1;
  int x_inc2, y_inc2;
  int x_t_inc1, y_t_inc1;
  
  int d_bottom_x, d_bottom_y;
  int d_right_x, d_right_y;
  int d_top_x, d_top_y;
  int d_left_x, d_left_y;
  
  int inc_b_x, inc_b_y;
  int inc_r_x, inc_r_y;
  int inc_t_x, inc_t_y;
  int inc_l_x, inc_l_y;
  
//	struct fgFLIGHT *f = &current_aircraft.flight;
  double sin_bank, cos_bank;
  double bank_angle, sideslip_angle;
  double ss_const; // sideslip angle pixels per rad

  bank_angle     = (*horizon->load_roll)();     // Roll limit +/- 30 degrees
  if( bank_angle < -FG_PI_2/3 ) {
    bank_angle = -FG_PI_2/3;
  }else if( bank_angle > FG_PI_2/3 ) {
    bank_angle = FG_PI_2/3;
  }
  sideslip_angle = (*horizon->load_sideslip)(); // Sideslip limit +/- 5 degrees
  if( sideslip_angle < -FG_PI/36.0 ) {
    sideslip_angle = -FG_PI/36.0;
  } else if( sideslip_angle > FG_PI/36.0 ) {
    sideslip_angle = FG_PI/36.0;
  }

	// sin_bank = sin( FG_2PI-FG_Phi );
	// cos_bank = cos( FG_2PI-FG_Phi );
  sin_bank = sin(FG_2PI-bank_angle);
  cos_bank = cos(FG_2PI-bank_angle);
  
  x_inc1 = (int)(horizon->scr_width * cos_bank);
  y_inc1 = (int)(horizon->scr_width * sin_bank);
  x_inc2 = (int)(horizon->scr_hole  * cos_bank);
  y_inc2 = (int)(horizon->scr_hole  * sin_bank);

  x_t_inc1 = (int)(horizon->tee_height * sin_bank);
  y_t_inc1 = (int)(horizon->tee_height * cos_bank);
  
  d_bottom_x = 0;
  d_bottom_y = (int)(-horizon->scr_hole);
  d_right_x  = (int)(horizon->scr_hole);
  d_right_y  = 0;
  d_top_x    = 0;
  d_top_y    = (int)(horizon->scr_hole);
  d_left_x   = (int)(-horizon->scr_hole);
  d_left_y   = 0;
  
  ss_const = (horizon->scr_width*2)/(FG_2PI/36.0);// width represents 10 degrees

  d_bottom_x += (int)(sideslip_angle*ss_const);
  d_right_x  += (int)(sideslip_angle*ss_const);
  d_left_x   += (int)(sideslip_angle*ss_const);
  d_top_x    += (int)(sideslip_angle*ss_const);
  
  inc_b_x = (int)(d_bottom_x*cos_bank-d_bottom_y*sin_bank);
  inc_b_y = (int)(d_bottom_x*sin_bank+d_bottom_y*cos_bank);
  inc_r_x = (int)(d_right_x*cos_bank-d_right_y*sin_bank);
  inc_r_y = (int)(d_right_x*sin_bank+d_right_y*cos_bank);
  inc_t_x = (int)(d_top_x*cos_bank-d_top_y*sin_bank);
  inc_t_y = (int)(d_top_x*sin_bank+d_top_y*cos_bank);
  inc_l_x = (int)(d_left_x*cos_bank-d_left_y*sin_bank);
  inc_l_y = (int)(d_left_x*sin_bank+d_left_y*cos_bank);
  
  if( horizon->scr_hole == 0 )
    {
    drawOneLine( horizon->scrn_pos.x - x_inc1, horizon->scrn_pos.y - y_inc1, \
                 horizon->scrn_pos.x + x_inc1, horizon->scrn_pos.y + y_inc1 );
    }
  else
    {
    drawOneLine( horizon->scrn_pos.x - x_inc1, horizon->scrn_pos.y - y_inc1, \
                 horizon->scrn_pos.x - x_inc2, horizon->scrn_pos.y - y_inc2 );
    drawOneLine( horizon->scrn_pos.x + x_inc2, horizon->scrn_pos.y + y_inc2, \
                 horizon->scrn_pos.x + x_inc1, horizon->scrn_pos.y + y_inc1 );
    }

  // draw teemarks (?)
  drawOneLine( horizon->scrn_pos.x + x_inc2, horizon->scrn_pos.y + y_inc2, \
               horizon->scrn_pos.x + x_inc2 + x_t_inc1, horizon->scrn_pos.y + y_inc2 - y_t_inc1 );
  drawOneLine( horizon->scrn_pos.x - x_inc2, horizon->scrn_pos.y - y_inc2, \
               horizon->scrn_pos.x - x_inc2 + x_t_inc1, horizon->scrn_pos.y - y_inc2 - y_t_inc1 );
               
  // draw sideslip diamond (it is not yet positioned correctly )
  drawOneLine( horizon->scrn_pos.x + inc_b_x, \
               horizon->scrn_pos.y + inc_b_y, \
               horizon->scrn_pos.x + inc_r_x, \
               horizon->scrn_pos.y + inc_r_y )
  drawOneLine( horizon->scrn_pos.x + inc_r_x, \
               horizon->scrn_pos.y + inc_r_y, \
               horizon->scrn_pos.x + inc_t_x, \
               horizon->scrn_pos.y + inc_t_y );
  drawOneLine( horizon->scrn_pos.x + inc_t_x, \
               horizon->scrn_pos.y + inc_t_y, \
               horizon->scrn_pos.x + inc_l_x, \
               horizon->scrn_pos.y + inc_l_y );
  drawOneLine( horizon->scrn_pos.x + inc_l_x, \
               horizon->scrn_pos.y + inc_l_y, \
               horizon->scrn_pos.x + inc_b_x, \
               horizon->scrn_pos.y + inc_b_y );
  
  /* drawOneLine( horizon->scrn_pos.x + inc_b_x, \
               horizon->scrn_pos.y + inc_b_y, \
               horizon->scrn_pos.x + inc_r_x, \
               horizon->scrn_pos.y + inc_r_y )
  drawOneLine( horizon->scrn_pos.x + inc_r_x, \
               horizon->scrn_pos.y + inc_r_y, \
               horizon->scrn_pos.x + inc_t_x, \
               horizon->scrn_pos.y + inc_t_y );
  drawOneLine( horizon->scrn_pos.x + inc_t_x, \
               horizon->scrn_pos.y + inc_t_y, \
               horizon->scrn_pos.x + inc_l_x, \
               horizon->scrn_pos.y + inc_l_y );
  drawOneLine( horizon->scrn_pos.x + inc_l_x, \
               horizon->scrn_pos.y + inc_l_y, \
               horizon->scrn_pos.x + inc_b_x, \
               horizon->scrn_pos.y + inc_b_y ); */
}

//  drawControlSurfaces()
//	Draws a representation of the control surfaces in their current state
//	anywhere in the HUD
//

static void drawControlSurfaces( HUD_control_surfaces *ctrl_surf )
{
	int x_ini, y_ini;
	int x_end, y_end;
	/* int x_1, y_1; */
	/* int x_2, y_2; */
	fgCONTROLS *pCtls;
	int tmp;

	x_ini = ctrl_surf->scrn_pos.x;
	y_ini = ctrl_surf->scrn_pos.y;
	x_end = x_ini + 150;
	y_end = y_ini + 60;

	drawOneLine( x_ini, y_ini, x_end, y_ini );
	drawOneLine( x_ini, y_ini, x_ini, y_end );
	drawOneLine( x_ini, y_end, x_end, y_end );
	drawOneLine( x_end, y_end, x_end, y_ini );
	drawOneLine( x_ini + 30, y_ini, x_ini + 30, y_end );
	drawOneLine( x_ini + 30, y_ini + 30, x_ini + 90, y_ini + 30 );
	drawOneLine( x_ini + 90, y_ini, x_ini + 90, y_end );
	drawOneLine( x_ini + 120, y_ini, x_ini + 120, y_end );

	pCtls = current_aircraft.controls;

	/* Draw elevator diagram */
	textString( x_ini + 1, y_end-11, "E", GLUT_BITMAP_8_BY_13 );
	drawOneLine( x_ini + 15, y_ini + 5, x_ini + 15, y_ini + 55 );
	drawOneLine( x_ini + 14, y_ini + 30, x_ini + 16, y_ini + 30 );
	tmp = y_ini + 5 + (int)(((pCtls->elevator + 1.0)/2)*50.0);
	if( pCtls->elevator <= -0.01 || pCtls->elevator >= 0.01 )
	{
		drawOneLine( x_ini + 10, tmp, x_ini + 20, tmp );
	}
	else
	{
		drawOneLine( x_ini + 7, tmp, x_ini + 23, tmp);
	}

	/* Draw aileron diagram */
	textString( x_ini + 30 + 1, y_end-11, "A", GLUT_BITMAP_8_BY_13 );
	drawOneLine( x_ini + 35, y_end-15, x_ini + 85, y_end-15 );
	drawOneLine( x_ini + 60, y_end-14, x_ini + 60, y_end-16 );
	tmp = x_ini + 35 + (int)(((pCtls->aileron + 1.0)/2)*50.0);
	if( pCtls->aileron <= -0.01 || pCtls->aileron >= 0.01 )
	{
		drawOneLine( tmp, y_end-20, tmp, y_end-10 );
	}
	else
	{
		drawOneLine( tmp, y_end - 25, tmp, y_end -  5 );
	}

	/* Draw rudder diagram */
	textString ( x_ini + 30 + 1, y_ini + 21, "R", GLUT_BITMAP_8_BY_13 );
	drawOneLine( x_ini + 35, y_ini + 15, x_ini + 85, y_ini + 15 );
	drawOneLine( x_ini + 60, y_ini + 14, x_ini + 60, y_ini + 16 );

	tmp = x_ini + 35 + (int)(((pCtls->rudder + 1.0) / 2) * 50.0);
	if( pCtls->rudder <= -0.01 || pCtls->rudder >= 0.01 )
	{
		drawOneLine( tmp, y_ini + 20, tmp, y_ini + 10 );
	}
	else
	{
		drawOneLine( tmp, y_ini + 25, tmp, y_ini +  5 );
	}


	/* Draw throttle diagram */
	textString( x_ini + 90 + 1, y_end-11, "T", GLUT_BITMAP_8_BY_13 );
	textString( x_ini + 90 + 1, y_end-21, "r", GLUT_BITMAP_8_BY_13 );
	drawOneLine( x_ini + 105, y_ini + 5, x_ini + 105, y_ini + 55 );
	tmp = y_ini + 5 + (int)(pCtls->throttle[0]*50.0);
	drawOneLine( x_ini + 100, tmp, x_ini + 110, tmp);


	/* Draw elevator trim diagram */
	textString( x_ini + 121, y_end-11, "T", GLUT_BITMAP_8_BY_13 );
	textString( x_ini + 121, y_end-22, "m", GLUT_BITMAP_8_BY_13 );
	drawOneLine( x_ini + 135, y_ini + 5, x_ini + 135, y_ini + 55 );
	drawOneLine( x_ini + 134, y_ini + 30, x_ini + 136, y_ini + 30 );

	tmp = y_ini + 5 + (int)(((pCtls->elevator_trim + 1)/2)*50.0);
	if( pCtls->elevator_trim <= -0.01 || pCtls->elevator_trim >= 0.01 )
	{
		drawOneLine( x_ini + 130, tmp, x_ini + 140, tmp);
	}
	else
	{
		drawOneLine( x_ini + 127, tmp, x_ini + 143, tmp);
	}

}

//
// Draws a label anywhere in the HUD
//
//

static void drawlabel( HUD_label *label )
{
  char buffer[80];
  char string[80];
  int posincr;
  int lenstr;

  if( !label ) { // Eliminate the possible, but absurd case.
    return;
    }

  if( label->pre_str != NULL) {
    if( label->post_str != NULL ) {
      sprintf( buffer, "%s%s%s", label->pre_str,  \
                                 label->format,   \
                                 label->post_str );
      }
    else {
      sprintf( buffer, "%s%s",   label->pre_str, \
                                 label->format );
      }
    }
  else {
    if( label->post_str != NULL ) {
      sprintf( buffer, "%s%s",   label->format,  \
                                 label->post_str );
      }
    } // else do nothing if both pre and post strings are nulls. Interesting.


  sprintf( string, buffer, (*label->load_value)() );
#ifdef DEBUGHUD
	fgPrintf( FG_COCKPIT, FG_DEBUG,  buffer );
	fgPrintf( FG_COCKPIT, FG_DEBUG,  "\n" );
	fgPrintf( FG_COCKPIT, FG_DEBUG, string );
	fgPrintf( FG_COCKPIT, FG_DEBUG, "\n" );
#endif
  lenstr = strlen( string );
  if( label->justify == LEFT_JUST ) {
   posincr = -lenstr*8;
   }
  else {
    if( label->justify == CENTER_JUST ) {
      posincr = -lenstr*4;
      }
    else {
      if( label->justify == RIGHT_JUST ) {
        posincr = 0;
        }
      }
    }

  if( label->size == SMALL ) {
    textString( label->scrn_pos.x + posincr, label->scrn_pos.y,
                string, GLUT_BITMAP_8_BY_13);
    }
  else  {
    if( label->size == LARGE ) {
      textString( label->scrn_pos.x + posincr, label->scrn_pos.y,
                  string, GLUT_BITMAP_9_BY_15);
      }
    }
}
// The following routines concern HUD object/component object construction
//

// fgHUDInit
//
// Constructs a HUD object and then adds in instruments. At the present
// the instruments are hard coded into the routine. Ultimately these need
// to be defined by the aircraft's instrumentation records so that the
// display for a Piper Cub doesn't show the speed range of a North American
// mustange and the engine readouts of a B36!
//
Hptr fgHUDInit( fgAIRCRAFT *current_aircraft )
{
  Hptr hud;

  fgPrintf( FG_COCKPIT, FG_INFO, "Initializing HUD\n" );

  hud = (Hptr)calloc(sizeof( HUD),1);
  if( hud == NULL )
    return( NULL );

  hud->code = 1;
  hud->status = 0;

  // For now lets just hardcode the hud here.
  // In the future, hud information has to come from the same place
  // aircraft information came from.

  fgHUDSetTimeMode( hud, NIGHT );
  fgHUDSetBrightness( hud, BRT_LIGHT );

     // TBI
  fgHUDAddHorizon( hud, 330, 100, 40, 5, 10, get_roll, get_sideslip );

  fgHUDAddLadder ( hud, 330, 285, 120, 180, 70, 10,
                   NONE, 45, get_roll, get_pitch );
     // KIAS
  fgHUDAddScale  ( hud, VERTICAL,     LIMIT, 200, 180, 380,  5,  10,
                      LEFT,     0,  100,   50,   0, get_speed );
     // Angle of Attack
  fgHUDAddScale  ( hud, HORIZONTAL, NOLIMIT, 180, 250, 410,  1,   5,
                      BOTTOM, -40,   50,   21,   0, get_aoa );
     // GYRO COMPASS
  fgHUDAddScale  ( hud, HORIZONTAL, NOLIMIT, 380, 200, 460,  5,  10,
                      TOP,      0,   50,   50, 360, get_heading );
     // AMSL
  fgHUDAddScale  ( hud, VERTICAL,     LIMIT, 460, 180, 380, 25, 100,
                      RIGHT,    0, 15000, 250,   0, get_altitude);

  fgHUDAddLabel  ( hud, 160, 150, SMALL, NOBLINK,
                      RIGHT_JUST, NULL, " Kts",      "%5.0f", get_speed );
  fgHUDAddLabel  ( hud, 160, 135, SMALL, NOBLINK,
                      RIGHT_JUST, NULL, " m",        "%5.0f", get_altitude );
  fgHUDAddLabel  ( hud, 160, 120, SMALL, NOBLINK,
                      RIGHT_JUST, NULL, " Roll",     "%5.2f", get_roll );
  fgHUDAddLabel  ( hud, 160, 105, SMALL, NOBLINK,
                      RIGHT_JUST, "Lat  ", "d",     "%03.0f", get_latitude );
  fgHUDAddLabel  ( hud, 160, 90,  SMALL, NOBLINK,
                      RIGHT_JUST, NULL, " m",        "%05.2f", get_lat_min );
  
  fgHUDAddLabel  ( hud, 440, 150, SMALL, NOBLINK,
                      RIGHT_JUST, NULL, " AOA",      "%5.2f", get_aoa );
  fgHUDAddLabel  ( hud, 440, 135, SMALL, NOBLINK,
                      RIGHT_JUST, NULL, " Heading",  "%5.0f", get_heading );
  fgHUDAddLabel  ( hud, 440, 120, SMALL, NOBLINK,
                      RIGHT_JUST, NULL, " Sideslip", "%5.2f", get_sideslip );
  fgHUDAddLabel  ( hud, 440, 105, SMALL, NOBLINK,
                      RIGHT_JUST, "Lon  ", "d",     "%04.0f", get_longitude );
  fgHUDAddLabel  ( hud, 440, 90,  SMALL, NOBLINK,
                      RIGHT_JUST, NULL, " m",        "%05.2f", get_long_min );

  fgHUDAddControlSurfaces( hud, 10, 10, NULL );

//  fgHUDAddControl( hud, HORIZONTAL, 50,  25, get_aileronval  ); // was 10, 10
//  fgHUDAddControl( hud, VERTICAL,   150, 25, get_elevatorval ); // was 10, 10
//  fgHUDAddControl( hud, HORIZONTAL, 250, 25, get_rudderval   ); // was 10, 10

  return( hud );
}


// add_instrument
//
// This is a stand in for linked list code that will get replaced later
// by some more elegant list handling code.

void add_instrument( Hptr hud, HIptr pinstrument )
{
    if( !hud || !pinstrument ) {
	return;
    }

    pinstrument->next = hud->instruments;
    hud->instruments = pinstrument;
}


// fgHUDAddHorizon
//
// Constructs a HUD_horizon "object" and installs it into the hud instrument
// list.

Hptr fgHUDAddHorizon( Hptr hud,     \
                      int x_pos,    \
                      int y_pos,    \
                      int length,   \
                      int hole_len, \
                      int tee_height,\
                      double (*load_roll)(),
                      double (*load_sideslip)() )
{
    HUD_horizon *phorizon;
    HUD_instr   *pinstrument;

    if( !hud ) {
	return NULL;
    }
                                       // construct the parent object
    pinstrument = (HIptr)calloc(sizeof(HUD_instr),1);
    if( pinstrument == NULL ) {
	return( NULL );
    }
    pinstrument->type    = HUDhorizon;  //  ARTIFICIAL_HORIZON;

                                      // Construct the horizon
    phorizon = (HUD_horizon *) calloc( sizeof(HUD_horizon),1);
    if( phorizon == NULL )   {
	return( NULL );
    }

    phorizon->scrn_pos.x    = x_pos;
    phorizon->scrn_pos.y    = y_pos;
    phorizon->scr_width     = length | 1;
    phorizon->scr_hole      = hole_len;
    phorizon->tee_height    = tee_height;
    phorizon->load_roll     = load_roll;
    phorizon->load_sideslip = load_sideslip;
    //  Install the horizon in the parent.
    pinstrument->instr   = phorizon;
    //  Install the instrument into hud.
    add_instrument( hud, pinstrument);

    return( hud );
}

// fgHUDAddScale
//
// Constructs a HUD_scale "object" and installs it into the hud instrument
// list.

Hptr fgHUDAddScale( Hptr hud,        \
                    int type,        \
                    int sub_type,    \
                    int scr_pos,     \
                    int scr_min,     \
                    int scr_max,     \
                    int div_min,     \
                    int div_max,     \
                    int orientation, \
                    int min_value,   \
                    int max_value,   \
                    int width_units, \
                    int modulus,     \
                    double (*load_value)() )
{
  HUD_scale *pscale;
  HUD_instr *pinstrument;

  if( !hud ) {
    return NULL;
    }

	pinstrument = (HIptr)calloc(sizeof(HUD_instr),1);
	if( pinstrument == NULL ) {
     return( NULL );
     }

  pinstrument->type = HUDscale;

  pscale = ( HUD_scale *)calloc(sizeof(HUD_scale),1);
  if( pscale == NULL )   {
     return( NULL );
    }

  pscale->type             = type;
  pscale->sub_type         = sub_type;
  pscale->div_min          = div_min;
  pscale->div_max          = div_max;
  pscale->orientation      = orientation;
  pscale->minimum_value    = min_value;
  pscale->maximum_value    = max_value;
  pscale->modulo           = modulus;
  pscale->load_value       = load_value;

  pscale->half_width_units = width_units / 2.0;
  pscale->scr_span = scr_max - scr_min; // Run of scan in pix coord
  pscale->scr_span |= 1;                // Force odd span of units.
             // If span is odd number of units, mid will be correct.
             // If not it will be high by one coordinate unit. This is
             // an artifact of integer division needed for screen loc's.

  pscale->mid_scr  = (pscale->scr_span >> 1) + scr_min;

             // Calculate the number of screen units per indicator unit
             // We must force floating point calculation or the factor
             // will be low and miss locate tics by several units.

  pscale->factor   = (double)pscale->scr_span / (double)width_units;

  switch( type ) {
    case HORIZONTAL:
      pscale->scrn_pos.left    = scr_min;
      pscale->scrn_pos.top     = scr_pos;
      pscale->scrn_pos.right   = scr_max;
      pscale->scrn_pos.bottom  = scr_pos;
      break;

    case VERTICAL:
    default:
      pscale->scrn_pos.left    = scr_pos;
      pscale->scrn_pos.top     = scr_max;
      pscale->scrn_pos.right   = scr_pos;
      pscale->scrn_pos.bottom  = scr_min;
    }

                                     // Install the scale
  pinstrument->instr = pscale;
                                      //  Install the instrument into hud.
  add_instrument( hud, pinstrument);

  return( hud );
}

// fgHUDAddLabel
//
// Constructs a HUD_Label object and installs it into the hud instrument
// list.
Hptr fgHUDAddLabel( Hptr hud,       \
                    int x_pos,      \
                    int y_pos,      \
                    int size,       \
                    int blink,      \
                    int justify,    \
					          char *pre_str,  \
                    char *post_str, \
                    char *format,   \
                    double (*load_value)() )
{
	HUD_label *plabel;
	HUD_instr *pinstrument;

  if( !hud ) {
    return NULL;
    }

	pinstrument = (HIptr)calloc(sizeof(HUD_instr),1);
	if( pinstrument == NULL ) {
    return NULL;
    }
	pinstrument->type = HUDlabel;

	plabel = (HUD_label *)calloc(sizeof(HUD_label),1);
	if( plabel == NULL ){
    return NULL;
    }

  plabel->scrn_pos.x      = x_pos;
  plabel->scrn_pos.y      = y_pos;
  plabel->size       = size;
  plabel->blink      = blink;
  plabel->justify    = justify;
  plabel->pre_str    = pre_str;
  plabel->post_str   = post_str;
  plabel->format     = format;
  plabel->load_value = load_value;
                                      // Install the label
	pinstrument->instr = plabel;
                                      //  Install the instrument into hud.
  add_instrument( hud, pinstrument);

	return( hud );
}

// fgHUDAddLadder
//
// Contains code that constructs a ladder "object" and installs it as
// a hud instrument in the hud instrument list.
//
Hptr fgHUDAddLadder( Hptr hud,        \
                     int x_pos,       \
                     int y_pos,       \
                     int scr_width,   \
                     int scr_height,  \
					           int hole_len,    \
                     int div_units,   \
                     int label_pos,   \
                     int width_units, \
					           double (*load_roll)(),
                     double (*load_pitch)() )
{
	HUD_ladder *pladder;
	HUD_instr  *pinstrument;

  if( !hud ) {
    return NULL;
    }

	pinstrument = (HIptr)calloc(sizeof(HUD_instr),1);
	if( pinstrument == NULL )
		return( NULL );

	pinstrument->type = HUDladder;

	pladder = (HUD_ladder *)calloc(sizeof(HUD_ladder),1);
	if( pladder == NULL )
		return( NULL );

  pladder->type           = 0; // Not used.
  pladder->scrn_pos.x          = x_pos;
  pladder->scrn_pos.y          = y_pos;
  pladder->scr_width      = scr_width;
  pladder->scr_height     = scr_height;
  pladder->scr_hole       = hole_len;
  pladder->div_units      = div_units;
  pladder->label_position = label_pos;
  pladder->width_units    = width_units;
  pladder->load_roll      = load_roll;
  pladder->load_pitch     = load_pitch;

  pinstrument->instr      = pladder;
                                      //  Install the instrument into hud.
  add_instrument( hud, pinstrument);
	return hud;
}

//   fgHUDAddControlSurfaces()
//
//   Adds the control surface indicators which make up for the lack of seat
//   of the pants feel. Should be unnecessary with joystick and pedals
//   enabled. But that is another improvement. Also, what of flaps? Spoilers?
//   This may need to be expanded or flattened into multiple indicators,
//   vertical and horizontal.

Hptr fgHUDAddControlSurfaces( Hptr hud,
                              int x_pos,
                              int y_pos,
                              double (*load_value)() )
{
	HUD_control_surfaces *pcontrol_surfaces;
	HUD_instr *pinstrument;

    if( !hud ) {
      return NULL;
    }

    // Construct shell
    pinstrument = (HIptr)calloc(sizeof(HUD_instr),1);
    if( !pinstrument ) {
      return NULL;
      }
    pinstrument->type = HUDcontrol_surfaces;

    // Construct core
    pcontrol_surfaces = (HUD_control_surfaces *)calloc(sizeof(HUD_control),1);
    if( !pcontrol_surfaces ) {
      return( NULL );
      }

    pcontrol_surfaces->scrn_pos.x = x_pos;
    pcontrol_surfaces->scrn_pos.y = y_pos;
    pcontrol_surfaces->load_value = load_value;

    pinstrument->instr     = pcontrol_surfaces;
                                                   // Install
    add_instrument( hud, pinstrument);

    return hud;
}

// fgHUDAddControl
//
//

Hptr fgHUDAddControl( Hptr hud,        \
                      int ctrl_x,      \
                      int ctrl_y,      \
                      int ctrl_length, \
                      int orientation, \
                      int alignment,   \
                      int min_value,   \
                      int max_value,   \
                      int width_units, \
                      double (*load_value)() )
{
    HUD_control *pcontrol;
    HUD_instr *pinstrument;

    if( !hud ) {
      return NULL;
    }

    // Construct shell
    pinstrument = (HIptr)calloc(sizeof(HUD_instr),1);
    if( !pinstrument ) {
      return NULL;
      }
    pinstrument->type = HUDcontrol;

    // Construct core
    pcontrol = (HUD_control *)calloc(sizeof(HUD_control),1);
    if( !(pcontrol == NULL) ) {
      return( NULL );
      }
    pcontrol->scrn_pos.x    = ctrl_x;
    pcontrol->scrn_pos.y    = ctrl_y;
    pcontrol->ctrl_length   = ctrl_length;
    pcontrol->orientation   = orientation;
    pcontrol->alignment     = alignment;
    pcontrol->min_value     = min_value;
    pcontrol->max_value     = max_value;
    pcontrol->width_units   = width_units;
    pcontrol->load_value    = load_value;
                                                   // Integrate
    pinstrument->instr     = pcontrol;
                                                   // Install
    add_instrument( hud, pinstrument);

    return hud;
}

/*
Hptr fgHUDAddMovingHorizon(  Hptr hud,     \
                             int x_pos,    \
                             int y_pos,    \
                             int length,   \
                             int hole_len, \
                             int color )
{

}

Hptr fgHUDAddCircularLadder( Hptr hud,    \
                             int scr_min, \
                             int scr_max, \
                             int div_min, \
                             int div_max, \
                             int max_value )
{

}

Hptr fgHUDAddNumDisp( Hptr hud,           \
                      int x_pos,          \
                      int y_pos,          \
                      int size,           \
                      int color,          \
                      int blink,          \
						          char *pre_str,      \
                      char *post_str )
{

}
*/

// fgUpdateHUD
//
// Performs a once around the list of calls to instruments installed in
// the HUD object with requests for redraw. Kinda. It will when this is
// all C++.
//

void fgUpdateHUD( Hptr hud ) {
    HIptr phud_instr;

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
                                   // This is a good improvement, but needs
                                   // to respond to a dial instead of time
                                   // of day. Of course, we have no dial!
    if( hud->time_of_day==DAY) {
      switch (hud->brightness) {
         case BRT_LIGHT:
           glColor3f (0.1, 0.9, 0.1);
           break;
         case BRT_MEDIUM:
           glColor3f (0.1, 0.7, 0.0);
           break;
         case BRT_DARK:
           glColor3f (0.0, 0.5, 0.0);
         }
      }
    else if( hud->time_of_day==NIGHT) {
      switch (hud->brightness) {
         case BRT_LIGHT:
           glColor3f (0.9, 0.1, 0.1);
           break;
         case BRT_MEDIUM:
           glColor3f (0.7, 0.0, 0.1);
           break;
         case BRT_DARK:
           glColor3f (0.5, 0.0, 0.0);
         }
      }
    else {
      glColor3f (0.1, 0.9, 0.1);
      }

    fgPrintf( FG_COCKPIT, FG_DEBUG, "HUD Code %d  Status %d\n",
              hud->code, hud->status );

    phud_instr = hud->instruments;
    while( phud_instr ) {
	/* printf("Drawing Instrument %d\n", phud_instr->type); */

	switch (phud_instr->type) {
    case HUDhorizon:   // ARTIFICIAL HORIZON
	    drawhorizon( (pHUDhorizon)phud_instr->instr );
	    break;

    case HUDscale:     // Need to simplify this call.
	    drawscale (  (pHUDscale)  phud_instr->instr  );
	    break;

    case HUDlabel:
	    drawlabel (  (pHUDlabel)  phud_instr->instr  );
	    break;

    case HUDladder:
	    drawladder(  (pHUDladder) phud_instr->instr  );
	    break;

//    case HUDcontrol:
//      drawControl( (pHUDcontrol) phud_instr->instr );
//      break;

    case HUDcontrol_surfaces:
	    drawControlSurfaces( (pHUDControlSurfaces) phud_instr->instr );
	    break;

    default:; // Ignore anything you don't know about.
    }

  phud_instr = phud_instr->next;
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void fgHUDSetTimeMode( Hptr hud, int time_of_day )
{

  hud->time_of_day = time_of_day;

}

void fgHUDSetBrightness( Hptr hud, int brightness )
{

  hud->brightness = brightness;

}

/* $Log$
/* Revision 1.1  1998/04/24 00:45:57  curt
/* C++-ifing the code a bit.
/*
 * Revision 1.22  1998/04/18 04:14:02  curt
 * Moved fg_debug.c to it's own library.
 *
 * Revision 1.21  1998/04/03 21:55:28  curt
 * Converting to Gnu autoconf system.
 * Tweaks to hud.c
 *
 * Revision 1.20  1998/03/09 22:48:40  curt
 * Minor "formatting" tweaks.
 *
 * Revision 1.19  1998/02/23 20:18:28  curt
 * Incorporated Michele America's hud changes.
 *
 * Revision 1.18  1998/02/21 14:53:10  curt
 * Added Charlie's HUD changes.
 *
 * Revision 1.17  1998/02/20 00:16:21  curt
 * Thursday's tweaks.
 *
 * Revision 1.16  1998/02/19 13:05:49  curt
 * Incorporated some HUD tweaks from Michelle America.
 * Tweaked the sky's sunset/rise colors.
 * Other misc. tweaks.
 *
 * Revision 1.15  1998/02/16 13:38:39  curt
 * Integrated changes from Charlie Hotchkiss.
 *
 * Revision 1.14  1998/02/12 21:59:41  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.12  1998/02/09 15:07:48  curt
 * Minor tweaks.
 *
 * Revision 1.11  1998/02/07 15:29:34  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.10  1998/02/03 23:20:14  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.9  1998/01/31 00:43:04  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.8  1998/01/27 00:47:51  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.7  1998/01/19 18:40:20  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.6  1997/12/15 23:54:34  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.5  1997/12/10 22:37:39  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.4  1997/09/23 00:29:32  curt
 * Tweaks to get things to compile with gcc-win32.
 *
 * Revision 1.3  1997/09/05 14:17:26  curt
 * More tweaking with stars.
 *
 * Revision 1.2  1997/09/04 02:17:30  curt
 * Shufflin' stuff.
 *
 * Revision 1.1  1997/08/29 18:03:22  curt
 * Initial revision.
 *
 */
