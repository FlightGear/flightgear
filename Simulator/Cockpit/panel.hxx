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
 
#define LETTER_OFFSET 0.03515625 

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

class FGInstrument;

class FGPanel{

private:
int height, width;
GLuint FontList;

GLubyte *background;

// FGInstrument **instr_list;
FGInstrument *test_instr[7];

void GetData(void);

public:
static FGPanel *OurPanel;

	FGPanel(void);

float   get_height(void){
	return height;
	}

void    ReInit( int x, int y, int finx, int finy);
void    Update(void);

void    DrawLetter(void){
        glBegin(GL_POLYGON);
        glTexCoord2f(0.0, 0.0);
        glVertex2f(0.0, 0.0);
        glTexCoord2f(LETTER_OFFSET + 0.004, 0.0);
        glVertex2f(7.0, 0.0);
        glTexCoord2f(LETTER_OFFSET + 0.004, 0.0390625);
        glVertex2f(7.0, 9.0);
        glTexCoord2f(0.0, 0.0390625);
        glVertex2f(0.0, 9.0);
        glEnd();
        }
        
void DrawTestLetter(float X, float Y);
void InitLists(void);
void TexString(char *s, float XPos, float YPos, float size);

};

class FGInstrument{
friend class FGPanel;

protected:
float XPos, YPos;

public:
	FGInstrument(void){
	}
	
	virtual ~FGInstrument(void){}
	
virtual void Init(void) = 0;
virtual void Render(void) = 0;
};

class FGHorizon : public FGInstrument {
private:
    float texXPos;
    float texYPos;
    float radius;
    float bottom;   // tell the program the offset between midpoint and bottom 
    float top;      // guess what ;-)
    float vertices[180][2];
    float normals[180][3];
    float texCoord[180][2];
    
public:
	FGHorizon(void){
	XPos = 0.0; YPos = 0.0;
	Init();
	}

        FGHorizon(float inXPos, float inYPos){
        XPos = inXPos; YPos = inYPos;
        Init();
        }
        
virtual void Init(void);
virtual void Render(void);

};

class FGTurnCoordinator : public FGInstrument {
private:
    float PlaneTexXPos;
    float PlaneTexYPos;
    float alpha;
    float PlaneAlpha;
    float alphahist[2];
    float rollhist[2];
    float BallXPos;
    float BallYPos;
    float BallTexXPos;
    float BallTexYPos;
    float BallRadius;
  GLfloat vertices[72];

public:
	FGTurnCoordinator(void){
	XPos = 0.0; YPos = 0.0;
	Init();
	}

	FGTurnCoordinator(float inXPos, float inYPos){
	XPos = inXPos; YPos = inYPos;
	Init();
	}	

virtual void Init (void);
virtual void Render(void);

};

class FGRpmGauge : public FGInstrument {
private:
    GLuint list;
    
public:
	FGRpmGauge(void){
	XPos = 0.0; YPos = 0.0;
	Init();
	}

	FGRpmGauge(float inXPos, float inYPos){
	XPos = inXPos; YPos = inYPos;
	Init();
	}	

virtual void Init(void);
virtual void Render(void);
};

// temporary class until I get the software-only routines for the 
// instruments run

class FGTexInstrument : public FGInstrument {

private:
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
    
public:
	FGTexInstrument(void){
	XPos = 0.0; YPos = 0.0;
	Init();
	}

	FGTexInstrument(float inXPos, float inYPos, float inradius,                                     float inlength, float inwidth, float inangle,                                   float invalue1, float invalue2, float inalpha1,                                 float inalpha2, float intexXPos, float intexYPos,                               int invariable){

	XPos = inXPos; YPos = inYPos;
	radius = inradius; angle = inangle;
	length = inlength; width = inwidth;
	value1 = invalue1; value2 = invalue2;
	alpha1 = inalpha1; alpha2 = inalpha2;
	teXpos = intexXPos; texYpos = intexYPos;
	variable = invariable; 
	Init();
	}

   void CreatePointer(void);
   void UpdatePointer(void);

   void Init(void);
   void Render(void);
};

typedef struct{
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
}Pointer;

void fgEraseArea(GLfloat *array, int NumVerti, GLfloat texXPos,                                  GLfloat texYPos, GLfloat XPos, GLfloat YPos,                                    int Texid, float ScaleFactor);
void DrawScale(float XPos, float YPos, float InnerRadius, float OuterRadius,                   float alpha1, float alpha2, int steps, float LineWidth,                       float red, float green, float blue, bool filled);
void DrawBeechcraftLogo(float XPos, float YPos, float Width, float Height);

void PrintMatrix( void);

#endif // _PANEL_HXX 



