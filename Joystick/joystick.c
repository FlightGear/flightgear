/**************************************************************************
 * joystick.h -- joystick support
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


#ifdef HAVE_JOYSTICK

#include <linux/joystick.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#endif

#include <Main/fg_debug.h>

static joy_x_min=0, joy_x_ctr=0, joy_x_max=0;
static joy_y_min=0, joy_y_ctr=0, joy_y_max=0;
static joy_x_dead_min=1000, joy_x_dead_max=-1000;
static joy_y_dead_min=1000, joy_y_dead_max=-1000;


static int joystick_fd;

int fgJoystickInit( int joy_num ) {

    fgPrintf( FG_INPUT, FG_INFO, "Initializing joystick\n");

#ifdef HAVE_JOYSTICK
	int status;
	char *fname;
	struct JS_DATA_TYPE js;
	int button;

	/* argument should be 0 or 1 */
	if( joy_num != 1 && joy_num != 0 ) {
		perror( "js" );
		return( 1 );
	}

	/* pick appropriate device file */
	if (joy_num == 0)
		fname = "/dev/js0";
	if (joy_num == 1)
		fname = "/dev/js1";

	/* open device file */
	joystick_fd = open(fname, O_RDONLY);
	if (joystick_fd < 0) {
		perror ("js");
		return( 1 );
	}
	
	fgPrintf( FG_INPUT,FG_ALERT,
		  "\nMove joystick around dead spot and press any joystick button.\n" );
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}
	button = js.buttons & 1 || js.buttons & 2;
	while(button == 0 ) {
		status = read(joystick_fd, &js, JS_RETURN);
		if (status != JS_RETURN) {
			perror("js");
			return( 1 );
		}
		button = js.buttons & 1 || js.buttons & 2;
		if( js.x > joy_x_dead_max )
			joy_x_dead_max = js.x;
		if( js.x < joy_x_dead_min )
			joy_x_dead_min = js.x;
		if( js.y > joy_y_dead_max )
			joy_y_dead_max = js.y;
		if( js.y < joy_y_dead_min )
			joy_y_dead_min = js.y;
		
		/* printf( "Xmin %d Xmax %d Ymin %d Ymax %d", 
			   joy_x_dead_min, joy_x_dead_max,
			   joy_y_dead_min, joy_y_dead_max ); */
	}
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}
	
	fgPrintf( FG_INPUT, FG_DEBUG, 
		  "\nJoystick calibration: X_dead_min = %d, X_dead_max = %d\n", 
		  joy_x_dead_min, joy_x_dead_max );
	fgPrintf( FG_INPUT, FG_DEBUG,
		  "                      Y_dead_min = %d, Y_dead_max = %d\n", 
		  joy_y_dead_min, joy_y_dead_max );
	
	sleep( 1 );
	
	fgPrintf( FG_INPUT, FG_DEBUG,
		  "\nCenter joystick and press any joystick button.\n" );
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}
	button = js.buttons & 1 || js.buttons & 2;
	while(button == 0 ) {
		status = read(joystick_fd, &js, JS_RETURN);
		if (status != JS_RETURN) {
			perror("js");
			return( 1 );
		}
		button = js.buttons & 1 || js.buttons & 2;
	}
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}
	joy_x_ctr = js.x;
	joy_y_ctr = js.y;
	
	fgPrintf( FG_INPUT, FG_DEBUG,
		  "Joystick calibration: X_ctr = %d, Y_ctr = %d\n", 
		  joy_x_ctr, joy_y_ctr );
	
	sleep( 1 );
	
	fgPrintf( FG_INPUT, FG_DEBUG,
		  "\nMove joystick to upper left and press any joystick button.\n" );
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}
	button = js.buttons & 1 || js.buttons & 2;
	while(button == 0 ) {
		status = read(joystick_fd, &js, JS_RETURN);
		if (status != JS_RETURN) {
			perror("js");
			return( 1 );
		}
		button = js.buttons & 1 || js.buttons & 2;
	}
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}
	joy_x_min = js.x;
	joy_y_min = js.y;
       	fgPrintf( FG_INPUT, FG_DEBUG,
		  "Joystick calibration: X_min = %d, Y_min = %d\n", 
		  joy_x_min, joy_y_min );
	
	sleep( 1 );
	
	fgPrintf( FG_INPUT, FG_DEBUG,
		  "\nMove joystick to lower right and press any joystick button.\n" );
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}
	button = js.buttons & 1 || js.buttons & 2;
	while(button == 0 ) {
		status = read(joystick_fd, &js, JS_RETURN);
		if (status != JS_RETURN) {
			perror("js");
			return( 1 );
		}
		button = js.buttons & 1 || js.buttons & 2;
	}
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}
	joy_x_max = js.x;
	joy_y_max = js.y;
	
	fgPrintf( FG_INPUT, FG_DEBUG,
		  "Joystick calibration: X_max = %d, Y_max = %d\n", 
		  joy_x_max, joy_y_max );
	
	// joy_x_ctr = (joy_x_max-joy_x_min)/2;
	// joy_y_ctr = (joy_y_max-joy_y_min)/2;
	// printf("Joystick calibration: X_ctr = %d, Y_ctr = %d\n", joy_x_ctr, joy_y_ctr );
	
	return( joystick_fd );
#else
	return( 0 );
#endif
}

/* void fgJoystickCalibrate( int joy_fd )
{

} */

int fgJoystickRead( double *joy_x, double *joy_y, int *joy_b1, int *joy_b2 )
{
#ifdef HAVE_JOYSTICK
	struct JS_DATA_TYPE js;
	int status;
	
	status = read(joystick_fd, &js, JS_RETURN);
	if (status != JS_RETURN) {
		perror("js");
		return( 1 );
	}

	/* printf("\n button 0: %s  button 1: %s  X position: %4d  Y position: %4d\n",
		 (js.buttons & 1) ? "on " : "off",
		 (js.buttons & 2) ? "on " : "off",
		 js.x,
		 js.y ); */
		 
	if( js.x >= joy_x_dead_min && js.x <= joy_x_dead_max )
		*joy_x = 0.5;
	else
		*joy_x = (double)js.x/(double)(joy_x_max-joy_x_min);
	*joy_x = *joy_x*2-1;
	
	if( js.y >= joy_y_dead_min && js.y <= joy_y_dead_max )
		*joy_y = 0.5;
	else
		*joy_y = (double)js.y/(double)(joy_y_max-joy_y_min);
	*joy_y = *joy_y*2-1;
	
	*joy_b1 = js.buttons & 1;
	*joy_b2 = js.buttons & 2;

#endif
	return( 0 );
}


/* $Log$
/* Revision 1.4  1998/02/03 23:20:20  curt
/* Lots of little tweaks to fix various consistency problems discovered by
/* Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
/* passed arguments along to the real printf().  Also incorporated HUD changes
/* by Michele America.
/*
 * Revision 1.3  1998/01/27 00:47:54  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.2  1997/12/30 20:47:40  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.1  1997/08/29 18:06:54  curt
 * Initial revision.
 *
 */
