//  panel.hxx -- instrument panel defines and prototypes
// 
//  Written by Friedemann Reinhard, started June 1998.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
// 
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  $Id$
//  (Log is kept at end of this file)
 
 

#ifndef _PANEL_HXX
#define _PANEL_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>


typedef struct {
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short sizeX, sizeY, sizeZ;
    char name[128];
    unsigned char *data;
} IMAGE;

typedef struct {
    float XPos;
    float YPos;
    float radius;
    float length;
    float width;
    float angle;
    float tape[2];
    float value1;
    float value2;
    float alpha1;
    float alpha2;
    float teXpos;
    float texYpos;
    int variable;
    GLfloat vertices[20];
} Pointer;

typedef struct{
    float XPos;
    float YPos;
    float texXPos;
    float texYPos;
    float radius;
    float bottom;   // tell the program the offset between midpoint and bottom 
    float top;      // guess what ;-)
} arthor;

typedef struct{
    float PlaneXPos;
    float PlaneYPos;
    float PlaneTexXPos;
    float PlaneTexYPos;
    float alpha;
    float PlaneAlpha;
    float PlaneAlphaHist;
    float alphahist;
    float rollhist;
    float BallXPos;
    float BallYPos;
    float BallTexXPos;
    float BallTexYPos;
    float BallRadius;
  GLfloat vertices[72];
} TurnCoordinator;

void fgPanelInit( void);
void fgPanelReInit( int x, int y, int finx, int finy );
void fgPanelUpdate( void);
void horizon( arthor hor);
void fgHorizonInit( arthor hor);
void CreatePointer(Pointer *pointer);
void UpdatePointer(Pointer *pointer);
void fgEraseArea(GLfloat *array, int NumVerti, GLfloat texXPos,
		 GLfloat texYPos, GLfloat XPos, GLfloat YPos,
		 int Texid, float ScaleFactor = 1);
void fgUpdateTurnCoordinator(TurnCoordinator *turn); 
void fgInitTurnCoordinator(TurnCoordinator *turn);
void DrawScale(float XPos, float YPos, float InnerRadius, float OuterRadius,
	       float alpha1, float alpha2, int steps, float LineWidth,
	       float red, float green, float blue);

void PrintMatrix( void);

#endif // _PANEL_HXX 



// $Log$
// Revision 1.6  1999/02/12 01:46:30  curt
// Updates and fixes from Friedemann.
//
// Revision 1.5  1999/01/07 19:25:55  curt
// Updates from Friedemann Reinhard.
//
// Revision 1.4  1998/11/11 00:19:29  curt
// Updated comment delimeter to C++ style.
//
// Revision 1.3  1998/11/09 23:38:54  curt
// Panel updates from Friedemann.
//
// Revision 1.2  1998/08/28 18:14:41  curt
// Added new cockpit code from Friedemann Reinhard
// <mpt218@faupt212.physik.uni-erlangen.de>
//
// Revision 1.1  1998/06/27 16:47:55  curt
// Incorporated Friedemann Reinhard's <mpt218@faupt212.physik.uni-erlangen.de>
// first pass at an isntrument panel.


