/**************************************************************************
 * hud.c -- hud defines and prototypes
 *
 * Written by Michele America, started September 1997.
 *
 * Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
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
 

#ifdef WIN32
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#  include <values.h>  /* for MAXINT */
#endif /* not WIN32 */

#include "hud.h"

#include <Aircraft/aircraft.h>
#include <Include/fg_constants.h>
#include <Main/fg_debug.h>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar.h>
#include <Scenery/scenery.h>
#include <Time/fg_timer.h>
#include <Weather/weather.h>

// #define DEBUG

#define drawOneLine(x1,y1,x2,y2)  glBegin(GL_LINES);  \
   glVertex2f ((x1),(y1)); glVertex2f ((x2),(y2)); glEnd();
   
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
/* static void strokeString(int x, int y, char *msg, void *font) */
/* { */
/* 	glPushMatrix(); */
/* 	glTranslatef(x, y, 0); */
/* 	glScalef(.04, .04, .04); */
/* 	while (*msg) { */
/* 		glutStrokeCharacter(font, *msg); */
/* 		msg++; */
/* 	} */
/* 	glPopMatrix(); */
/* } */

/*

	Draws a measuring scale anywhere on the HUD
		
	
	Needs: HUD_scale struct

*/
static void drawscale( int type, int sub_type, int min_value, int orientation, int scr_pos, \
		int scr_min, int scr_max, double total_amount, int min_div, int max_div, \
		double cur_value )
{
    double vmin, vmax;
    int marker_x, marker_y;
    int mid_scr;
    // int scale_min, scale_max;
    register i;
    double factor;
    char TextScale[80];
    int condition;

    vmin = cur_value-total_amount/2;
    vmax = cur_value+total_amount/2;
    
    mid_scr = scr_min+(scr_max-scr_min)/2;
    
    if( type == VERTICAL )     // Vertical scale
    {
        if( orientation == LEFT )
            marker_x = scr_pos-6;
        else if( orientation == RIGHT )
            marker_x = scr_pos;
        drawOneLine( scr_pos, scr_min, scr_pos, scr_max );
        if( orientation == LEFT )
        {
            drawOneLine( scr_pos-3, scr_min, scr_pos, scr_min );
            drawOneLine( scr_pos-3, scr_max, scr_pos, scr_max );
            drawOneLine( scr_pos, mid_scr, scr_pos+6, mid_scr );
        } else if( orientation == RIGHT )
        {
            drawOneLine( scr_pos, scr_min, scr_pos+3, scr_min );
            drawOneLine( scr_pos, scr_max, scr_pos+3, scr_max );
            drawOneLine( scr_pos, mid_scr, scr_pos-6, mid_scr );
        }

        factor = (scr_max-scr_min)/total_amount;

        for( i=vmin; i<=vmax; i+=1 )
        {
        	if( sub_type == LIMIT )
        		condition = i>= min_value;
        	else if( sub_type == NOLIMIT )
        		condition = 1;
        		
            if( condition )
            {
                marker_y = scr_min+(i-vmin)*factor;
                if( i%min_div==0 )
                    if( orientation == LEFT )
                    {
                    	drawOneLine( marker_x+3, marker_y, marker_x+6, marker_y );
                    }
                    else if( orientation == RIGHT )
                    {
                    	drawOneLine( marker_x, marker_y, marker_x+3, marker_y );
                    }
                if( i%max_div==0 )
                {
                	drawOneLine( marker_x, marker_y, marker_x+6, marker_y );
                    sprintf( TextScale, "%d", i );
                    if( orientation == LEFT )
                    {
                    	textString( marker_x-8*strlen(TextScale)-2, marker_y-4, TextScale, GLUT_BITMAP_8_BY_13 );
                    }
                    else if( orientation == RIGHT )
                    {
                    	textString( marker_x+10, marker_y-4, TextScale, GLUT_BITMAP_8_BY_13 );
                    }
                }
            }
        }
    }
    if( type == HORIZONTAL )     // Horizontal scale
    {
        if( orientation == TOP )
            marker_y = scr_pos;
        else if( orientation == BOTTOM )
            marker_y = scr_pos-6;
        drawOneLine( scr_min, scr_pos, scr_max, scr_pos );
        if( orientation == TOP )
        {
        	drawOneLine( scr_min, scr_pos, scr_min, scr_pos-3 );
            drawOneLine( scr_max, scr_pos, scr_max, scr_pos-3 );
            drawOneLine( mid_scr, scr_pos, mid_scr, scr_pos-6 );
        } else if( orientation == BOTTOM )
        {
        	drawOneLine( scr_min, scr_pos, scr_min, scr_pos+3 );
            drawOneLine( scr_max, scr_pos, scr_max, scr_pos+3 );
            drawOneLine( mid_scr, scr_pos, mid_scr, scr_pos+6 );
        }

        factor = (scr_max-scr_min)/total_amount;

        for( i=vmin; i<=vmax; i+=1 )
        {
        	if( sub_type == LIMIT )
        		condition = i>= min_value;
        	else if( sub_type == NOLIMIT )
        		condition = 1;
        		
            if( condition )
            {
                marker_x = scr_min+(i-vmin)*factor;
                if( i%min_div==0 )
                    if( orientation == TOP )
                    {
                    	drawOneLine( marker_x, marker_y, marker_x, marker_y+3 );
                   	}
                    else if( orientation == BOTTOM )
                    {
                    	drawOneLine( marker_x, marker_y+3, marker_x, marker_y+6 );
                    }
                if( i%max_div==0 )
                {
                    sprintf( TextScale, "%d", i );
                    if( orientation == TOP )
                    {
                    	drawOneLine( marker_x, marker_y, marker_x, marker_y+6 );
                    	textString( marker_x-4*strlen(TextScale), marker_y+14, TextScale, GLUT_BITMAP_8_BY_13 );
                    }
                    else if( orientation == BOTTOM )
                    {
                    	drawOneLine( marker_x, marker_y, marker_x, marker_y+6 );
                    	textString( marker_x+10, marker_y-4, TextScale, GLUT_BITMAP_8_BY_13 );
                    }
                }
            }
        }
    }

}              

/*

	Draws a climb ladder in the center of the HUD
		
	
	Needs: HUD_ladder struct

*/
static void drawladder( struct HUD_ladder ladder )
{
    double vmin, vmax;
    double roll_value, pitch_value;
    /* double cos_roll, sin_roll; */
    int marker_x, marker_y;
    int mid_scr;
    int scr_min, scr_max;
    int x_ini, x_end;
    int y_ini, y_end;
    int new_x_ini, new_x_end;
    int new_y_ini, new_y_end;
    register i;
    double factor;
    char TextLadder[80];
    int condition;

	roll_value = (*ladder.load_roll)();
	pitch_value = (*ladder.load_pitch)()*RAD_TO_DEG;
	
    vmin = pitch_value-ladder.width_units/2;
    vmax = pitch_value+ladder.width_units/2;
    
    scr_min = ladder.y_pos-(ladder.scr_height/2);
    scr_max = scr_min+ladder.scr_height;
    
    mid_scr = scr_min+(scr_max-scr_min)/2;

	marker_x = ladder.x_pos-ladder.scr_width/2;
	
    factor = (scr_max-scr_min)/ladder.width_units;

    for( i=vmin; i<=vmax; i+=1 )
    {
    	condition = 1;
        if( condition )
        {
            marker_y = scr_min+(i-vmin)*factor;
            if( i%ladder.div_units==0 )
            {
            	sprintf( TextLadder, "%d", i );
            	if( ladder.scr_hole == 0 )
            	{
            		if( i != 0 )
               			x_ini = ladder.x_pos-ladder.scr_width/2;
               		else
               			x_ini = ladder.x_pos-ladder.scr_width/2-10;
               		y_ini = marker_y;
               		x_end = ladder.x_pos+ladder.scr_width/2;
               		y_end = marker_y;
           			new_x_ini = ladder.x_pos+(x_ini-ladder.x_pos)*cos(roll_value)-\
           				(y_ini-ladder.y_pos)*sin(roll_value);
           			new_y_ini = ladder.y_pos+(x_ini-ladder.x_pos)*sin(roll_value)+\
           				(y_ini-ladder.y_pos)*cos(roll_value);
           			new_x_end = ladder.x_pos+(x_end-ladder.x_pos)*cos(roll_value)-\
           				(y_end-ladder.y_pos)*sin(roll_value);
           			new_y_end = ladder.y_pos+(x_end-ladder.x_pos)*sin(roll_value)+\
           				(y_end-ladder.y_pos)*cos(roll_value);
   
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
                	textString( new_x_ini-8*strlen(TextLadder)-8, new_y_ini-4, TextLadder, GLUT_BITMAP_8_BY_13 );
                	textString( new_x_end+10, new_y_end-4, TextLadder, GLUT_BITMAP_8_BY_13 );
            	}
            	else
            	{
            		if( i != 0 )
               			x_ini = ladder.x_pos-ladder.scr_width/2;
               		else
               			x_ini = ladder.x_pos-ladder.scr_width/2-10;
               		y_ini = marker_y;
               		x_end = ladder.x_pos-ladder.scr_width/2+ladder.scr_hole/2;
               		y_end = marker_y;
           			new_x_ini = ladder.x_pos+(x_ini-ladder.x_pos)*cos(roll_value)-\
           				(y_ini-ladder.y_pos)*sin(roll_value);
           			new_y_ini = ladder.y_pos+(x_ini-ladder.x_pos)*sin(roll_value)+\
           				(y_ini-ladder.y_pos)*cos(roll_value);
           			new_x_end = ladder.x_pos+(x_end-ladder.x_pos)*cos(roll_value)-\
           				(y_end-ladder.y_pos)*sin(roll_value);
           			new_y_end = ladder.y_pos+(x_end-ladder.x_pos)*sin(roll_value)+\
           				(y_end-ladder.y_pos)*cos(roll_value);
            
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
            		textString( new_x_ini-8*strlen(TextLadder)-8, new_y_ini-4, TextLadder, GLUT_BITMAP_8_BY_13 );
            		
            		x_ini = ladder.x_pos+ladder.scr_width/2-ladder.scr_hole/2;
               		y_ini = marker_y;
               		if( i != 0 )
               			x_end = ladder.x_pos+ladder.scr_width/2;
               		else
               			x_end = ladder.x_pos+ladder.scr_width/2+10;
               		y_end = marker_y;
           			new_x_ini = ladder.x_pos+(x_ini-ladder.x_pos)*cos(roll_value)-\
           				(y_ini-ladder.y_pos)*sin(roll_value);
           			new_y_ini = ladder.y_pos+(x_ini-ladder.x_pos)*sin(roll_value)+\
           				(y_ini-ladder.y_pos)*cos(roll_value);
           			new_x_end = ladder.x_pos+(x_end-ladder.x_pos)*cos(roll_value)-\
           				(y_end-ladder.y_pos)*sin(roll_value);
           			new_y_end = ladder.y_pos+(x_end-ladder.x_pos)*sin(roll_value)+\
           				(y_end-ladder.y_pos)*cos(roll_value);
            
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
            		textString( new_x_end+10, new_y_end-4, TextLadder, GLUT_BITMAP_8_BY_13 );
            	}
            }
            /* if( i%max_div==0 )
            {
            	drawOneLine( marker_x, marker_y, marker_x+6, marker_y );
                sprintf( TextScale, "%d", i );
                if( orientation == LEFT )
                {
                	textString( marker_x-8*strlen(TextScale)-2, marker_y-4, TextScale, GLUT_BITMAP_8_BY_13 );
                }
                else if( orientation == RIGHT )
                {
                	textString( marker_x+10, marker_y-4, TextScale, GLUT_BITMAP_8_BY_13 );
                }
            } */
        }
    }

}

/*

	Draws an artificial horizon line in the center of the HUD
		(with or without a center hole)
	
	Needs: x_center, y_center, length, hole

*/
static void drawhorizon( struct HUD_horizon horizon )
{
	int x_inc1, y_inc1;
	int x_inc2, y_inc2;
	/* struct fgFLIGHT *f; */
	double sin_bank, cos_bank;
	double bank_angle;
              
	/* f = &current_aircraft.flight; */
	
	bank_angle = (*horizon.load_value)();

	// sin_bank = sin( FG_2PI-FG_Phi );
	// cos_bank = cos( FG_2PI-FG_Phi );
	sin_bank = sin(FG_2PI-bank_angle);
	cos_bank = cos(FG_2PI-bank_angle);
	x_inc1 = (int)(horizon.scr_width*cos_bank);
	y_inc1 = (int)(horizon.scr_width*sin_bank);
	x_inc2 = (int)(horizon.scr_hole*cos_bank);
	y_inc2 = (int)(horizon.scr_hole*sin_bank);
	
	if( horizon.scr_hole == 0 )
	{
		drawOneLine( horizon.x_pos-x_inc1, horizon.y_pos-y_inc1, \
						horizon.x_pos+x_inc1, horizon.y_pos+y_inc1 );
	}
	else
	{
		drawOneLine( horizon.x_pos-x_inc1, horizon.y_pos-y_inc1, \
						horizon.x_pos-x_inc2, horizon.y_pos-y_inc2 );
		drawOneLine( horizon.x_pos+x_inc2, horizon.y_pos+y_inc2, \
						horizon.x_pos+x_inc1, horizon.y_pos+y_inc1 );
	}	
}

/*

	Draws a representation of the control surfaces in their current state
		anywhere in the HUD
	
	Needs: struct HUD_control_surfaces

*/
static void drawcontrolsurfaces( struct HUD_control_surfaces ctrl_surf )
{
	int x_ini, y_ini;
	int x_end, y_end;
	/* int x_1, y_1; */
	/* int x_2, y_2; */
	struct fgCONTROLS *c;
	
	x_ini = ctrl_surf.x_pos;
	y_ini = ctrl_surf.y_pos;
	x_end = x_ini+150;
	y_end = y_ini+60;
	
	drawOneLine( x_ini, y_ini, x_end, y_ini );
	drawOneLine( x_ini, y_ini, x_ini, y_end );
	drawOneLine( x_ini, y_end, x_end, y_end );
	drawOneLine( x_end, y_end, x_end, y_ini );
	drawOneLine( x_ini+30, y_ini, x_ini+30, y_end );
	drawOneLine( x_ini+30, y_ini+30, x_ini+90, y_ini+30 );
	drawOneLine( x_ini+90, y_ini, x_ini+90, y_end );
	drawOneLine( x_ini+120, y_ini, x_ini+120, y_end );
              
	c = &current_aircraft.controls;
	
	/* Draw elevator diagram */
	textString( x_ini+1, y_end-11, "E", GLUT_BITMAP_8_BY_13 );	
	drawOneLine( x_ini+15, y_ini+5, x_ini+15, y_ini+55 );
	drawOneLine( x_ini+14, y_ini+30, x_ini+16, y_ini+30 );
	if( FG_Elevator <= -0.01 || FG_Elevator >= 0.01 )
	{
		drawOneLine( x_ini+10, y_ini+5+(int)(((FG_Elevator+1.0)/2)*50.0), \
				x_ini+20, y_ini+5+(int)(((FG_Elevator+1.0)/2)*50.0) );
	}
	else
	{
		drawOneLine( x_ini+7, y_ini+5+(int)(((FG_Elevator+1.0)/2)*50.0), \
				x_ini+23, y_ini+5+(int)(((FG_Elevator+1.0)/2)*50.0) );
	}
	
	/* Draw aileron diagram */
	textString( x_ini+30+1, y_end-11, "A", GLUT_BITMAP_8_BY_13 );	
	drawOneLine( x_ini+35, y_end-15, x_ini+85, y_end-15 );
	drawOneLine( x_ini+60, y_end-14, x_ini+60, y_end-16 );
	if( FG_Aileron <= -0.01 || FG_Aileron >= 0.01 )
	{
		drawOneLine( x_ini+35+(int)(((FG_Aileron+1.0)/2)*50.0), y_end-20, \
				x_ini+35+(int)(((FG_Aileron+1.0)/2)*50.0), y_end-10 );
	}
	else
	{
		drawOneLine( x_ini+35+(int)(((FG_Aileron+1.0)/2)*50.0), y_end-25, \
				x_ini+35+(int)(((FG_Aileron+1.0)/2)*50.0), y_end-5 );
	}
	
	/* Draw rudder diagram */
	textString( x_ini+30+1, y_ini+21, "R", GLUT_BITMAP_8_BY_13 );	
	drawOneLine( x_ini+35, y_ini+15, x_ini+85, y_ini+15 );
	drawOneLine( x_ini+60, y_ini+14, x_ini+60, y_ini+16 );
	if( FG_Rudder <= -0.01 || FG_Rudder >= 0.01 )
	{
		drawOneLine( x_ini+35+(int)(((FG_Rudder+1.0)/2)*50.0), y_ini+20, \
				x_ini+35+(int)(((FG_Rudder+1.0)/2)*50.0), y_ini+10 );
	}
	else
	{
		drawOneLine( x_ini+35+(int)(((FG_Rudder+1.0)/2)*50.0), y_ini+25, \
				x_ini+35+(int)(((FG_Rudder+1.0)/2)*50.0), y_ini+5 );
	}
	
	
	/* Draw throttle diagram */
	textString( x_ini+90+1, y_end-11, "T", GLUT_BITMAP_8_BY_13 );	
	textString( x_ini+90+1, y_end-21, "r", GLUT_BITMAP_8_BY_13 );	
	drawOneLine( x_ini+105, y_ini+5, x_ini+105, y_ini+55 );
	drawOneLine( x_ini+100, y_ini+5+(int)(FG_Throttle[0]*50.0), \
			x_ini+110, y_ini+5+(int)(FG_Throttle[0]*50.0) );
	
	
	/* Draw elevator trim diagram */
	textString( x_ini+121, y_end-11, "T", GLUT_BITMAP_8_BY_13 );	
	textString( x_ini+121, y_end-22, "m", GLUT_BITMAP_8_BY_13 );	
	drawOneLine( x_ini+135, y_ini+5, x_ini+135, y_ini+55 );
	drawOneLine( x_ini+134, y_ini+30, x_ini+136, y_ini+30 );
	if( FG_Elev_Trim <= -0.01 || FG_Elev_Trim >= 0.01 )
	{
		drawOneLine( x_ini+130, y_ini+5+(int)(((FG_Elev_Trim+1)/2)*50.0), \
				x_ini+140, y_ini+5+(int)(((FG_Elev_Trim+1.0)/2)*50.0) );
	}
	else
	{
		drawOneLine( x_ini+127, y_ini+5+(int)(((FG_Elev_Trim+1.0)/2)*50.0), \
				x_ini+143, y_ini+5+(int)(((FG_Elev_Trim+1.0)/2)*50.0) );
	}
	
}

/*

	Draws a label anywhere in the HUD
	
	Needs: HUD_label struct

*/	
static void drawlabel( struct HUD_label label )
{
	char buffer[80];
	char string[80];
	int posincr;
	int lenstr;
	
	if( label.pre_str != NULL && label.post_str != NULL )
		sprintf( buffer, "%s%s%s", label.pre_str, label.format, label.post_str );
	else if( label.pre_str == NULL && label.post_str != NULL )
		sprintf( buffer, "%s%s", label.format, label.post_str );
	else if( label.pre_str != NULL && label.post_str == NULL )
		sprintf( buffer, "%s%s", label.pre_str, label.format );

	sprintf( string, buffer, (*label.load_value)() );

	fgPrintf( FG_COCKPIT, FG_DEBUG,  buffer );
	fgPrintf( FG_COCKPIT, FG_DEBUG,  "\n" );
	fgPrintf( FG_COCKPIT, FG_DEBUG, string );
	fgPrintf( FG_COCKPIT, FG_DEBUG, "\n" );

	lenstr = strlen( string );
	if( label.justify == LEFT_JUST )
		posincr = -lenstr*8;
	else if( label.justify == CENTER_JUST )
		posincr = -lenstr*4;
	else if( label.justify == RIGHT_JUST )
		posincr = 0;
	
	if( label.size == SMALL )
		textString( label.x_pos+posincr, label.y_pos, string, GLUT_BITMAP_8_BY_13);
	else if( label.size == LARGE )
		textString( label.x_pos+posincr, label.y_pos, string, GLUT_BITMAP_9_BY_15);
	
}

double get_speed( void )
{
	struct fgFLIGHT *f;
              
	f = &current_aircraft.flight;
	return( FG_V_equiv_kts );
}

double get_aoa( void )
{
	struct fgFLIGHT *f;
              
	f = &current_aircraft.flight;
	return( FG_Gamma_vert_rad*RAD_TO_DEG );
}

double get_roll( void )
{
	struct fgFLIGHT *f;
              
	f = &current_aircraft.flight;
	return( FG_Phi );
}

double get_pitch( void )
{
	struct fgFLIGHT *f;
              
	f = &current_aircraft.flight;
	return( FG_Theta );
}

double get_heading( void )
{
	struct fgFLIGHT *f;
              
	f = &current_aircraft.flight;
	return( FG_Psi*RAD_TO_DEG );
}

double get_altitude( void )
{
	struct fgFLIGHT *f;
	/* double rough_elev; */
              
	f = &current_aircraft.flight;
	/* rough_elev = mesh_altitude(FG_Longitude * RAD_TO_ARCSEC,
			                   FG_Latitude  * RAD_TO_ARCSEC); */
                                                   
	return( FG_Altitude * FEET_TO_METER /* -rough_elev */ );
}

void add_instrument( Hptr hud, HIptr instrument )
{
	HIptr instruments;
	
	instruments = hud->instruments;
	// while( ++instruments
}

Hptr fgHUDInit( struct fgAIRCRAFT current_aircraft, int color )
{
	Hptr hud;
	
	hud = (Hptr)calloc(sizeof(struct HUD),1);
	if( hud == NULL )
		return( NULL );
		
	hud->code = 1;		/* It will be aircraft dependent */
	hud->status = 0;
	
	// For now lets just hardcode a hud here .
	// In the future, hud information has to come from the same place 
	// aircraft information came
	
	fgHUDAddHorizon( hud, 590, 50, 40, 20, get_roll );
	fgHUDAddScale( hud, VERTICAL, 220, 100, 280, 5, 10, LEFT, LEFT, 0, 100, get_speed );
	fgHUDAddScale( hud, VERTICAL, 440, 100, 280, 1, 5, RIGHT, RIGHT, -400, 25, get_aoa );
	fgHUDAddScale( hud, HORIZONTAL, 280, 220, 440, 5, 10, TOP, TOP, 0, 50, get_heading );
	fgHUDAddLabel( hud, 180, 85, SMALL, NOBLINK, RIGHT_JUST, NULL, " Kts", "%5.0f", get_speed );
	fgHUDAddLabel( hud, 180, 73, SMALL, NOBLINK, RIGHT_JUST, NULL, " m", "%5.0f", get_altitude );
	fgHUDAddLadder( hud, 330, 190, 90, 180, 70, 10, NONE, 45, get_roll, get_pitch );
	fgHUDAddControlSurfaces( hud, 10, 10, get_heading );
	
	return( hud );
}

Hptr fgHUDAddHorizon( Hptr hud, int x_pos, int y_pos, int length, \
						int hole_len, double (*load_value)() )
{
	struct HUD_horizon *horizon;
	struct HUD_instr *instrument;
	HIptr tmp_first, tmp_next;
	
	tmp_first = hud->instruments;
	if( tmp_first != NULL )
		tmp_next = tmp_first->next;
	else
		tmp_next = NULL;
	
	instrument = (HIptr)calloc(sizeof(struct HUD_instr),1);
	if( instrument == NULL )
		return( NULL );
		
	horizon = (struct HUD_horizon *)calloc(sizeof(struct HUD_horizon),1);
	if( horizon == NULL )
		return( NULL );
	
	instrument->type = ARTIFICIAL_HORIZON;
	instrument->instr.horizon = *horizon;
	instrument->instr.horizon.x_pos = x_pos;
	instrument->instr.horizon.y_pos = y_pos;
	instrument->instr.horizon.scr_width = length;
	instrument->instr.horizon.scr_hole = hole_len;
	instrument->instr.horizon.load_value = load_value;
	instrument->next = tmp_first;

	hud->instruments = instrument;

	return( hud );
}

Hptr fgHUDAddScale( Hptr hud, int type, int scr_pos, int scr_min, int scr_max, int div_min, int div_max, \
					int orientation, int with_min, int min_value, int width_units, double (*load_value)() )
{
	struct HUD_scale *scale;
	struct HUD_instr *instrument;
	HIptr tmp_first, tmp_next;
	
	tmp_first = hud->instruments;
	if( tmp_first != NULL )
		tmp_next = tmp_first->next;
	else
		tmp_next = NULL;
	
	instrument = (HIptr)calloc(sizeof(struct HUD_instr),1);
	if( instrument == NULL )
		return( NULL );
		
	scale = (struct HUD_scale *)calloc(sizeof(struct HUD_scale),1);
	if( scale == NULL )
		return( NULL );
	
	instrument->type = SCALE;
	instrument->instr.scale = *scale;
	instrument->instr.scale.type = type;
	instrument->instr.scale.scr_pos = scr_pos;
	instrument->instr.scale.scr_min = scr_min;
	instrument->instr.scale.scr_max = scr_max;
	instrument->instr.scale.div_min = div_min;
	instrument->instr.scale.div_max = div_max;
	instrument->instr.scale.orientation = orientation;
	instrument->instr.scale.with_minimum = with_min;
	instrument->instr.scale.minimum_value = min_value;
	instrument->instr.scale.width_units = width_units;
	instrument->instr.scale.load_value = load_value;
	instrument->next = tmp_first;

	hud->instruments = instrument;

	return( hud );
}

Hptr fgHUDAddLabel( Hptr hud, int x_pos, int y_pos, int size, int blink, int justify, \
					char *pre_str, char *post_str, char *format, double (*load_value)() )
{
	struct HUD_label *label;
	struct HUD_instr *instrument;
	HIptr tmp_first, tmp_next;
	
	tmp_first = hud->instruments;
	if( tmp_first != NULL )
		tmp_next = tmp_first->next;
	else
		tmp_next = NULL;
	
	instrument = (HIptr)calloc(sizeof(struct HUD_instr),1);
	if( instrument == NULL )
		return( NULL );
		
	label = (struct HUD_label *)calloc(sizeof(struct HUD_label),1);
	if( label == NULL )
		return( NULL );
	
	instrument->type = LABEL;
	instrument->instr.label = *label;
	instrument->instr.label.x_pos = x_pos;
	instrument->instr.label.y_pos = y_pos;
	instrument->instr.label.size = size;
	instrument->instr.label.blink = blink;
	instrument->instr.label.justify = justify;
	instrument->instr.label.pre_str = pre_str;
	instrument->instr.label.post_str = post_str;
	instrument->instr.label.format = format;
	instrument->instr.label.load_value = load_value;
	instrument->next = tmp_first;

	hud->instruments = instrument;

	return( hud );
}

Hptr fgHUDAddLadder( Hptr hud, int x_pos, int y_pos, int scr_width, int scr_height, \
					int hole_len, int div_units, int label_pos, int width_units, \
					double (*load_roll)(), double (*load_pitch)() )
{
	struct HUD_ladder *ladder;
	struct HUD_instr *instrument;
	HIptr tmp_first, tmp_next;
	
	tmp_first = hud->instruments;
	if( tmp_first != NULL )
		tmp_next = tmp_first->next;
	else
		tmp_next = NULL;
	
	instrument = (HIptr)calloc(sizeof(struct HUD_instr),1);
	if( instrument == NULL )
		return( NULL );
		
	ladder = (struct HUD_ladder *)calloc(sizeof(struct HUD_ladder),1);
	if( ladder == NULL )
		return( NULL );

	instrument->type = LADDER;
	instrument->instr.ladder = *ladder;
	instrument->instr.ladder.type = 0;	// Not used.
	instrument->instr.ladder.x_pos = x_pos;
	instrument->instr.ladder.y_pos = y_pos;
	instrument->instr.ladder.scr_width = scr_width;
	instrument->instr.ladder.scr_height = scr_height;
	instrument->instr.ladder.scr_hole = hole_len;
	instrument->instr.ladder.div_units = div_units;
	instrument->instr.ladder.label_position = label_pos;
	instrument->instr.ladder.width_units = width_units;
	instrument->instr.ladder.load_roll = load_roll;
	instrument->instr.ladder.load_pitch = load_pitch;
	instrument->next = tmp_first;

	hud->instruments = instrument;

	return( hud );
}

Hptr fgHUDAddControlSurfaces( Hptr hud, int x_pos, int y_pos, double (*load_value)() )
{
	struct HUD_control_surfaces *ctrl_surf;
	struct HUD_instr *instrument;
	HIptr tmp_first, tmp_next;
	
	tmp_first = hud->instruments;
	if( tmp_first != NULL )
		tmp_next = tmp_first->next;
	else
		tmp_next = NULL;
	
	instrument = (HIptr)calloc(sizeof(struct HUD_instr),1);
	if( instrument == NULL )
		return( NULL );
		
	ctrl_surf = (struct HUD_control_surfaces *)calloc(sizeof(struct HUD_control_surfaces),1);
	if( ctrl_surf == NULL )
		return( NULL );
	
	instrument->type = CONTROL_SURFACES;
	instrument->instr.control_surfaces = *ctrl_surf;
	instrument->instr.control_surfaces.x_pos = x_pos;
	instrument->instr.control_surfaces.y_pos = y_pos;
	instrument->instr.horizon.load_value = load_value;
	instrument->next = tmp_first;

	hud->instruments = instrument;

	return( hud );
}

/*
Hptr fgHUDAddMovingHorizon( Hptr hud, int x_pos, int y_pos, int length, int hole_len, \
						int color )
{

}

Hptr fgHUDAddCircularLadder( Hptr hud, int scr_min, int scr_max, int div_min, int div_max, \
						int max_value )
{

}

Hptr fgHUDAddNumDisp( Hptr hud, int x_pos, int y_pos, int size, int color, int blink, \
						char *pre_str, char *post_str )
{

}
*/

void fgUpdateHUD( Hptr hud )
{
	HIptr hud_instr;
	union HUD_instr_data instr_data;
              
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
	glColor3f (0.1, 0.9, 0.1);
                      
    fgPrintf( FG_COCKPIT, FG_DEBUG,  "HUD Code %d  Status %d\n", 
	      hud->code, hud->status ); 
    hud_instr = hud->instruments;
	while( hud_instr != NULL )
	{
		instr_data = hud_instr->instr;
		fgPrintf( FG_COCKPIT, FG_DEBUG, 
			  "Instr Type %d   SubType %d  Orient %d\n", 
			  hud_instr->type, hud_instr->sub_type, hud_instr->orientation );
		if( hud_instr->type == ARTIFICIAL_HORIZON )
		{
			drawhorizon( instr_data.horizon );
		  	/* drawhorizon( instr_data.horizon.x_pos, instr_data.horizon.y_pos, \
    				instr_data.horizon.scr_width, instr_data.horizon.scr_hole ); */
    	}
    	else if( hud_instr->type == SCALE )
    	{
    		drawscale( instr_data.scale.type, instr_data.scale.with_minimum, \
    				instr_data.scale.minimum_value, instr_data.scale.orientation, \
    				instr_data.scale.scr_pos, instr_data.scale.scr_min, \
    				instr_data.scale.scr_max, instr_data.scale.width_units, \
    				instr_data.scale.div_min, instr_data.scale.div_max, \
					(*instr_data.scale.load_value)() );     				
    	}
    	else if( hud_instr->type == CONTROL_SURFACES )
    	{
    		drawcontrolsurfaces( instr_data.control_surfaces );
    	}
    	else if( hud_instr->type == LABEL )
    	{
    		drawlabel( instr_data.label );
    		/* drawlabel( instr_data.label.x_pos, instr_data.label.y_pos, instr_data.label.size, \
    				instr_data.label.blink, instr_data.label.pre_str, instr_data.label.post_str, \
    				instr_data.label.format, (*instr_data.label.load_value)() ); */
    	}
    	else if( hud_instr->type == LADDER )
    	{
    		drawladder( instr_data.ladder );
    	}
    	hud_instr = hud_instr->next;
    }
    
    
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
                
}


/* $Log$
/* Revision 1.10  1998/02/03 23:20:14  curt
/* Lots of little tweaks to fix various consistency problems discovered by
/* Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
/* passed arguments along to the real printf().  Also incorporated HUD changes
/* by Michele America.
/*
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
