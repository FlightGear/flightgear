/*
// animassist.c
//
// Jim Wilson 5/8/2003
//
// Copyright (C) 2003  Jim Wilson - jimw@kelcomaine.com
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
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main( int argc, char **argv ) {
	float x1,y1,z1,x2,y2,z2 = 0;
	float center_x,center_y,center_z,axis_x,axis_y,axis_z;
	float vector_length;

	if (argc < 7) {
		printf("animassist - utility to calculate axis and center values for SimGear model animation\n\n");
		printf("Usage: animassist x1 y1 z1 x2 y2 z2\n\n");
		printf("Defining two vectors in SimGear coords (x1,y1,z1)  and (x2,y2,z2)\n\n");
		printf("Conversion from model to SimGear:\n");
		printf("SimGear Coord         AC3D Coordinate\n");
		printf("     X                   =              X\n");
		printf("     Y                   =              Z * -1\n");
		printf("     Z                   =              Y\n\n");
		printf("Note: no normalization done, so please make the 2nd vector the furthest out\n");
	} else {
    
		x1  = atof(argv[1]);
		y1  = atof(argv[2]); 
		z1  = atof(argv[3]);
		x2  = atof(argv[4]);
		y2  = atof(argv[5]);
		z2  = atof(argv[6]);

		center_x = (x1+x2)/2;
		center_y = (y1+y2)/2;
		center_z = (z1+z2)/2;

		vector_length = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1));

		/* arbitrary axis defined by cos(theta) where theta is angle from x,y or z axis */
		axis_x = (x2-x1)/vector_length;
		axis_y = (y2-y1)/vector_length;
		axis_z = (z2-z1)/vector_length;

		printf("Flightgear Model XML for:\n");
		printf("Axis from (%f,%f,%f) to (%f,%f,%f)\n", x1,y1,z1,x2,y2,z2);
                                            printf("Assuming units in meters!\n\n");
		printf("<center>\n");
                                            printf(" <x-m>%f</x-m>\n",center_x);
                                            printf(" <y-m>%f</y-m>\n",center_y);
                                            printf(" <z-m>%f</z-m>\n",center_z);
                                            printf("</center>\n\n");
		printf("<axis>\n");
                                            printf(" <x>%f</x>\n",axis_x);
                                            printf(" <y>%f</y>\n",axis_y);
                                            printf(" <z>%f</z>\n",axis_z);
                                            printf("</axis>\n\n");
	}

	return 0;
}
