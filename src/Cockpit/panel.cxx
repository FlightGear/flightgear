// panel.cxx -- routines to draw an instrument panel
//
// Written by Friedemann Reinhard, started June 1998.
//
//  Major code reorganization by David Megginson, November 1999.
// 
// Copyright(C)1998 Friedemann Reinhard-reinhard@theorie2.physik.uni-erlangen.de
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

#define FILLED true

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <math.h>

#include <Debug/logstream.hxx>
#include <Aircraft/aircraft.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Misc/fgpath.hxx>
#include <Objects/texload.h>

#include "panel.hxx"
#include "cockpit.hxx"
#include "hud.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanel.
////////////////////////////////////////////////////////////////////////


// Global panel.
FGPanel* FGPanel::OurPanel = 0;


// Constructor (ensures that the panel is a singleton).
FGPanel::FGPanel(void) {
    int x, y;
    FILE *f;
    char line[256];
    GLint test;
    GLubyte *tex = new GLubyte[262144];
    float Xzoom, Yzoom;
    
    if(OurPanel) {
        FG_LOG( FG_GENERAL, FG_ALERT, "Error: only one Panel allowed" );
        exit(-1);
    }
    
    OurPanel = this;   
    
    Xzoom = (float)((float)(current_view.get_winWidth())/1024);
    Yzoom = (float)((float)(current_view.get_winHeight())/768);
    
    airspeedIndicator = new FGAirspeedIndicator(144.375, 166.875);
    verticalSpeedIndicator = new FGVerticalSpeedIndicator(358, 52);
				// Each hand of the altimeter is a
				// separate instrument for now...
    altimeter = new FGAltimeter(357.5, 167);
    altimeter2 = new FGAltimeter2(357.5, 167);
    horizonIndicator = new FGHorizon(251, 166.75);
    turnCoordinator = new FGTurnCoordinator(143.75, 51.75);
    rpmIndicator = new FGRPMIndicator(462.5, 133);
    
#ifdef GL_VERSION_1_1
    xglGenTextures(2, panel_tex_id);
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id[1]);
#elif GL_EXT_texture_object
    xglGenTexturesEXT(2, panel_tex_id);
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id[1]);
#else
#  error port me
#endif
    
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    xglViewport(0, 0, 640, 480);
    xglOrtho(0, 640, 0, 480, 1, -1);
    xglMatrixMode(GL_MODELVIEW);
    xglPushMatrix();
    xglLoadIdentity();
    xglPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // load in the texture data 
    
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    FGPath tpath( current_options.get_fg_root() );
    tpath.append( "Textures/gauges.rgb" );
    if((img = read_rgb_texture( (char *)tpath.c_str(), &img_width,                      &img_height ))==NULL){
    }
    
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    tpath.set( current_options.get_fg_root() );
    tpath.append( "Textures/gauges2.rgb" );
    if((imag = read_rgb_texture( (char *)tpath.c_str(), &imag_width,                     &imag_height ))==NULL){
    }
    
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);
    
    tpath.set( current_options.get_fg_root() );
    tpath.append( "Textures/Fullone.rgb" );
    if ((background = read_rgb_texture( (char *)tpath.c_str(), &width,                         &height ))==NULL ){
    }
    
    xglPixelZoom(Xzoom, Yzoom);
    xglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);
    xglRasterPos2i(0,0);
    xglPixelZoom(Xzoom, Yzoom);
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB,                                   GL_UNSIGNED_BYTE, imag);
    
#ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id[0]);
#elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id[0]);
#else
#  error port me
#endif
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB,                                  GL_UNSIGNED_BYTE, (GLvoid *)(img)); 
    
    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
}


// Destructor.
FGPanel::~FGPanel ()
{
    delete airspeedIndicator;
    delete verticalSpeedIndicator;
    delete altimeter;
    delete altimeter2;
    delete horizonIndicator;
    delete turnCoordinator;
    delete rpmIndicator;
    OurPanel = 0;
}


// Reinitialize the panel.
void FGPanel::ReInit( int x, int y, int finx, int finy){
    GLint buffer;
    float Xzoom, Yzoom;
    
    xglDisable(GL_DEPTH_TEST);
    
    Xzoom = (float)((float)(current_view.get_winWidth())/1024);
    Yzoom = (float)((float)(current_view.get_winHeight())/768);
    
    // save the current buffer state
    xglGetIntegerv(GL_DRAW_BUFFER, &buffer);
    
    // and enable both buffers for writing
    xglDrawBuffer(GL_FRONT_AND_BACK);
    
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    xglViewport(0, 0, 640, 480);
    xglOrtho(0, 640, 0, 480, 1, -1);
    xglMatrixMode(GL_MODELVIEW);
    xglPushMatrix();
    xglLoadIdentity();
    xglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    xglPixelZoom(Xzoom, Yzoom);
    xglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);
    xglPixelStorei(GL_UNPACK_SKIP_PIXELS, x);
    xglPixelStorei(GL_UNPACK_SKIP_ROWS, y);
    xglRasterPos2i(x, y);
    xglPixelZoom(Xzoom, Yzoom);
    xglDrawPixels(finx - x, finy - y, GL_RGB, GL_UNSIGNED_BYTE,
                  (GLvoid *)(background));
    
    // restore original buffer state
    xglDrawBuffer( (GLenum)buffer );
    xglEnable(GL_DEPTH_TEST);
}


// Update the panel.
void FGPanel::Update ( void ) {
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    xglOrtho(0, 640, 0, 480, 10, -10);
    xglMatrixMode(GL_MODELVIEW);
    xglPushMatrix();
    xglLoadIdentity();
    xglDisable(GL_DEPTH_TEST);
    xglEnable(GL_LIGHTING);
    xglEnable(GL_TEXTURE_2D);
    xglDisable(GL_BLEND);
    
    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
    xglPushMatrix();
    
    xglDisable(GL_LIGHTING);
    airspeedIndicator->Render();
    verticalSpeedIndicator->Render();
    altimeter->Render();
    altimeter2->Render();

    xglPopMatrix();
    xglPushMatrix();
    
    turnCoordinator->Render();
    rpmIndicator->Render();
    
    xglEnable(GL_LIGHTING);
    
    horizonIndicator->Render();
    
    
    xglDisable(GL_TEXTURE_2D);
    xglPopMatrix();   
    xglEnable(GL_DEPTH_TEST);
    xglEnable(GL_LIGHTING);
    xglDisable(GL_TEXTURE_2D);
    xglDisable(GL_BLEND);
    xglMatrixMode(GL_PROJECTION);
    xglPopMatrix();
    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
}


// fgEraseArea - 'Erases' a drawn Polygon by overlaying it with a textured 
//                 area. Shall be a method of a panel class once.
// This should migrate into FGPanel somehow.
void fgEraseArea(GLfloat *array, int NumVerti, GLfloat texXPos,                                  GLfloat texYPos, GLfloat XPos, GLfloat YPos,                                    int Texid, float ScaleFactor){
    int i, j;
    int n;
    float a;
    float ififth;
    
    xglEnable(GL_TEXTURE_2D);
    xglEnable(GL_TEXTURE_GEN_S);
    xglEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    xglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    xglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    xglMatrixMode(GL_TEXTURE);
    xglLoadIdentity();
    
#ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, FGPanel::OurPanel->panel_tex_id[Texid]);
#elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, FGPanel::OurPanel->panel_tex_id[Texid]);
#else
#  error port me
#endif
    
    xglMatrixMode(GL_TEXTURE);
    xglLoadIdentity();
    xglTranslatef(-((float)((XPos/0.625)/256)),                                                   -((float)((YPos/0.625)/256)), 0.0);
    xglTranslatef(texXPos/256 , texYPos/256, 0.0);
    xglScalef(0.00625, 0.00625, 1.0);
    
    xglBegin(GL_POLYGON);
    for(n=0;n<NumVerti;n += 2){ 
        xglVertex2f(array[n] * ScaleFactor, array[n + 1] * ScaleFactor);
    } 
    xglEnd();
    
    xglLoadIdentity();
    xglMatrixMode(GL_MODELVIEW);
    xglPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    xglPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    xglDisable(GL_TEXTURE_2D);
    xglDisable(GL_TEXTURE_GEN_S);
    xglDisable(GL_TEXTURE_GEN_T);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGHorizon instrument.
////////////////////////////////////////////////////////////////////////


FGHorizon::FGHorizon (float inXPos, float inYPos)
{
  XPos = inXPos;
  YPos = inYPos;
  Init();
}


FGHorizon::~FGHorizon ()
{
}


void FGHorizon::Render(void){ 
    double pitch;
    double roll;
    float shifted, alpha, theta;
    float epsi = 360 / 180;
    GLboolean Light;
    GLfloat normal[2];
    
    int n1, n2, n, dn, rot, tmp1, tmp2;
    float a;
    
    GLfloat material[] = { 0.714844, 0.265625, 0.056875 ,1.0};
    GLfloat material2[] = {0.6640625, 0.921875, 0.984375, 1.0};
    GLfloat material3[] = {0.2, 0.2, 0.2, 1.0};
    GLfloat material4[] = {0.8, 0.8, 0.8, 1.0};
    GLfloat material5[] = {0.0, 0.0, 0.0, 1.0};
    GLfloat direction[] = {0.0, 0.0, 0.0};
    GLfloat light_position[4];
    GLfloat light_ambient[] = {0.7, 0.7, 0.7, 1.0};
    GLfloat light_ambient2[] = {0.7, 0.7, 0.7, 1.0};
    GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
    
    pitch = get_pitch() * RAD_TO_DEG;
    if(pitch > 45)
        pitch = 45;
    
    if(pitch < -45)
        pitch = -45;
    
    roll = get_roll() * RAD_TO_DEG;
    
    xglEnable(GL_NORMALIZE);
    xglEnable(GL_LIGHTING);
    xglEnable(GL_TEXTURE_2D);
    xglEnable(GL_LIGHT1);
    xglDisable(GL_LIGHT2);
    xglDisable(GL_LIGHT0);
    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    xglTranslatef(XPos, YPos, 0);
    xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    xglMatrixMode(GL_TEXTURE);
    xglPushMatrix();
    
    // computations for the non-textured parts of the AH
    
    shifted = -((pitch / 10) * 7.0588235);
    
    if(shifted > (bottom - radius)){
        theta = (180 - (acos((bottom - shifted) / radius)*RAD_TO_DEG));
        n = (int)(theta / epsi) - 1;
        n1 = n;
        n2 = (180 - n1) + 2;
        dn = n2 - n1;
        rot = (int)(roll / epsi);
        n1 += rot + 45;
        n2 += rot + 45;
    }
    
    if(shifted < (-top + radius)){
        theta = ((acos((-top - shifted) / radius)*RAD_TO_DEG));
        n = (int)(theta / epsi) + 1;
        n1 = n;
        n2 = (180 - n1) + 2;
        dn = n2 - n1;
        rot = (int)(roll / epsi);
        n1 += rot - 45;
        n2 += rot - 45;
        if(n1 < 0){ n1 += 180; n2 +=180;}
    }
    
    // end of computations  
    
    light_position[0] = 0.0;
    light_position[1] = 0.0; 
    light_position[2] = 1.5;
    light_position[3] = 0.0;
    xglLightfv(GL_LIGHT1, GL_POSITION, light_position);
    xglLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);  
    xglLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
    xglLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
    xglLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, direction);
    
#ifdef GL_VERSION_1_1
				// FIXME!!
    xglBindTexture(GL_TEXTURE_2D, FGPanel::OurPanel->panel_tex_id[1]);
#elif GL_EXT_texture_object
				// FIXME!!
    xglBindTextureEXT(GL_TEXTURE_2D, FGPanel::OurPanel->panel_tex_id[1]);
#else
#  error port me
#endif
    
    xglLoadIdentity();
    xglTranslatef(0.0, ((pitch / 10) * 0.046875), 0.0);
    xglTranslatef((texXPos/256), (texYPos/256), 0.0);
    xglRotatef(-roll, 0.0, 0.0, 1.0);
    xglScalef(1.7, 1.7, 0.0);
    
    // the following loop draws the textured part of the AH
    
    xglMaterialf(GL_FRONT, GL_SHININESS, 85.0);
    
    xglMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material4);
    xglMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material5);
    xglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material3);
    
    xglMatrixMode(GL_MODELVIEW);
    xglBegin(GL_TRIANGLES);

    int i;
    for(i=45; i < 225; i++){
        xglTexCoord2f(0.0, 0.0);
        xglNormal3f(0.0, 0.0, 0.6);
        xglVertex3f(0.0, 0.0, 0.0);
        xglTexCoord2f(texCoord[i % 180][0], texCoord[i % 180][1]);
        xglNormal3f(normals[i % 180][0], normals[i % 180][1], 0.6);
        xglVertex3f(vertices[i % 180][0], vertices[i % 180][1], 0.0);
        n = (i + 1) % 180;
        xglTexCoord2f(texCoord[n][0], texCoord[n][1]);
        xglNormal3f(normals[n][0], normals[n][1], 0.6);
        xglVertex3f(vertices[n][0], vertices[n][1], 0.0);
    }
    xglEnd();
    
    
    if((shifted > (bottom - radius)) && (n1 < 1000) && (n1 > 0)){
        
        a = sin(theta * DEG_TO_RAD) * sin(theta * DEG_TO_RAD);
        light_ambient2[0] = a;
        light_ambient2[1] = a;
        light_ambient2[2] = a;
        
        xglLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient2);
        xglLightfv(GL_LIGHT1, GL_DIFFUSE, light_ambient2); 
        xglLightfv(GL_LIGHT1, GL_SPECULAR, light_ambient2);
        
        xglBegin(GL_TRIANGLES);
        
        tmp1 = n1; tmp2 = n2;
        
        for(i = tmp1; i < tmp2 + 1; i++){
            n = i % 180;
            xglNormal3f(0.0, 0.0, 1.5);
            xglTexCoord2f((56 / 256), (140 / 256));
            xglVertex3f(((vertices[n1 % 180][0] + vertices[n2 % 180][0]) / 2),                            ((vertices[n1 % 180][1] + vertices[n2 % 180][1]) / 2),                             0.0);
            
            xglTexCoord2f((57 / 256), (139 / 256));        
            xglNormal3f(normals[n][0], normals[n][1], normals[n][3]);
            xglVertex3f(vertices[n][0], vertices[n][1], 0.0);
            
            n = (i + 1) % 180;
            xglTexCoord2f((57 / 256), (139 / 256));      
            xglNormal3f(normals[n][0], normals[n][1], normals[n][3]);
            xglVertex3f(vertices[n][0], vertices[n][1], 0.0);        
        }
        xglEnd();
    }
    
    if((shifted < (-top + radius)) && (n1 < 1000) && (n1 > 0)){
        a = sin(theta * DEG_TO_RAD) * sin(theta * DEG_TO_RAD);
        light_ambient2[0] = a;
        light_ambient2[1] = a;
        light_ambient2[2] = a;
        
        xglLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient2);
        xglLightfv(GL_LIGHT1, GL_DIFFUSE, light_ambient2); 
        xglLightfv(GL_LIGHT1, GL_SPECULAR, light_ambient2);
        xglMaterialf(GL_FRONT, GL_SHININESS, a * 85);
        xglBegin(GL_TRIANGLES);
        tmp1 = n1; tmp2 = n2;
        for(i = tmp1; i <= tmp2; i++){
            n = i % 180;
            xglNormal3f(0.0, 0.0, 1.5);
            xglTexCoord2f((73 / 256), (237 / 256));
            xglVertex3f(((vertices[n1 % 180][0] + vertices[n2 % 180][0]) / 2),                            ((vertices[n1 % 180][1] + vertices[n2 % 180][1]) / 2), 0.0); 
            
            xglTexCoord2f((73 / 256), (236 / 256));
            xglNormal3f(normals[n][0], normals[n][1], normals[n][2]);
            xglVertex3f(vertices[n][0], vertices[n][1], 0.0);
            
            n = (i + 1) % 180;
            xglTexCoord2f((73 / 256), (236 / 256)); 
            xglNormal3f(normals[n][0], normals[n][1], normals[n][2]);
            xglVertex3f(vertices[n][0], vertices[n][1], 0.0); 
        }
        xglEnd();
    }
    
    // Now we will have to draw the small triangle indicating the roll value
    
    xglDisable(GL_LIGHTING);
    xglDisable(GL_TEXTURE_2D);
    
    xglRotatef(roll, 0.0, 0.0, 1.0);
    
    xglBegin(GL_TRIANGLES);
    xglColor3f(1.0, 1.0, 1.0);
    xglVertex3f(0.0, radius, 0.0);
    xglVertex3f(-3.0, (radius - 7.0), 0.0);
    xglVertex3f(3.0, (radius - 7.0), 0.0);    
    xglEnd();
    
    xglLoadIdentity();
    
    xglBegin(GL_POLYGON);
    xglColor3f(0.2109375, 0.23046875, 0.203125);
    xglVertex2f(275.625, 138.0);
    xglVertex2f(275.625, 148.125);
    xglVertex2f(258.125, 151.25);
    xglVertex2f(246.875, 151.25);
    xglVertex2f(226.875, 147.5);
    xglVertex2f(226.875, 138.0);
    xglVertex2f(275.625, 138.0);
    xglEnd();
    
    xglLoadIdentity();
    
    xglMatrixMode(GL_TEXTURE);
    xglPopMatrix();
    xglMatrixMode(GL_PROJECTION);
    xglPopMatrix();
    xglDisable(GL_TEXTURE_2D);
    xglDisable(GL_NORMALIZE);
    xglDisable(GL_LIGHTING);
    xglDisable(GL_LIGHT1);
    xglEnable(GL_LIGHT0);
}

// fgHorizonInit - initialize values for the AH

void FGHorizon::Init(void){
    radius = 28.9;
    texXPos = 56;
    texYPos = 174;
    bottom = 36.5;
    top = 36.5;
    int n;
    
    float step = (360*DEG_TO_RAD)/180;
    
    for(n=0;n<180;n++){
        vertices[n][0] = cos(n * step) * radius;
        vertices[n][1] = sin(n * step) * radius;
        texCoord[n][0] = (cos(n * step) * radius)/256;
        texCoord[n][1] = (sin(n * step) * radius)/256;
        normals[n][0] = cos(n * step) * radius + sin(n * step);
        normals[n][1] = sin(n * step) * radius + cos(n * step);
        normals[n][2] = 0.0;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTurnCoordinator.
////////////////////////////////////////////////////////////////////////


// Static constants

GLfloat FGTurnCoordinator::wingArea[] = {
  -1.25, -28.125, 1.255, -28.125, 1.255, 28.125, -1.25, 28.125 };

GLfloat FGTurnCoordinator::elevatorArea[] = {
  3.0, -10.9375, 4.5, -10.9375, 4.5, 10.9375, 3.0, 10.9375 };
    
GLfloat FGTurnCoordinator::rudderArea[] = {
  2.0, -0.45, 10.625, -0.45, 10.625, 0.55, 2.0, 0.55};


FGTurnCoordinator::FGTurnCoordinator (float inXPos, float inYPos)
{
  XPos = inXPos;
  YPos = inYPos;
  Init();
}

FGTurnCoordinator::~FGTurnCoordinator ()
{
}


// fgUpdateTurnCoordinator - draws turn coordinator related stuff

void FGTurnCoordinator::Render(void){
    int n;
    
    xglDisable(GL_LIGHTING);
    xglDisable(GL_BLEND);
    xglEnable(GL_TEXTURE_2D);
    
    alpha = (get_sideslip() / 1.5) * 560;
    if(alpha > 56){
        alpha = 56;
    }
    if(alpha < -56){
        alpha = -56;
    }
    
    PlaneAlpha = get_roll();
    
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    
    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    xglTranslatef(BallXPos, BallYPos, 0.0);
    xglTranslatef(0.75 * sin(alphahist[0] * DEG_TO_RAD) * 31,                                     0.3 * (39 - (cos(alphahist[0] * DEG_TO_RAD) * 39)),                             0.0);
    fgEraseArea(vertices, 72, BallTexXPos +                                                     ((0.75 * sin(alphahist[0] * DEG_TO_RAD) * 31) / 0.625),                         BallTexYPos + ((0.3 * (39 - (cos(alphahist[0] * DEG_TO_RAD)                                  * 39))) / 0.625),                                                   BallXPos + (0.75 * sin(alphahist[0] * DEG_TO_RAD) * 31),                       BallYPos + (0.3 * (39 - (cos(alphahist[0] * DEG_TO_RAD)                         * 39))), 1, 1);
    xglDisable(GL_TEXTURE_2D);
    xglEnable(GL_BLEND);
    xglBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    xglTranslatef(BallXPos, BallYPos, 0.0);
    xglTranslatef(0.75 * sin(alpha * DEG_TO_RAD) * 31,                                            0.3 * (39 - (cos(alpha * DEG_TO_RAD) * 39)), 0.0);
    xglBegin(GL_POLYGON);
    xglColor3f(0.8, 0.8, 0.8);

    int i;
    for(i=0;i<36;i++){
        xglVertex2f(vertices[2 * i],                                                                vertices[(2 * i) + 1]);
    }
    xglEnd(); 
    
    xglDisable(GL_TEXTURE_2D);
    xglDisable(GL_BLEND);
    
    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    xglTranslatef(XPos, YPos, 0.0);
    xglRotatef(rollhist[0] * RAD_TO_DEG + 90, 0.0, 0.0, 1.0);
    
    fgEraseArea(wingArea, 8, PlaneTexXPos, PlaneTexYPos,                                                     XPos, YPos, 1, 1); 
    fgEraseArea(elevatorArea, 8, PlaneTexXPos, PlaneTexYPos,                                                     XPos, YPos, 1, 1); 
    fgEraseArea(rudderArea, 8, PlaneTexXPos, PlaneTexYPos,                                                     XPos, YPos, 1, 1); 
    
    xglLoadIdentity();
    xglTranslatef(XPos, YPos, 0.0);
    xglRotatef(-get_roll() * RAD_TO_DEG + 90, 0.0, 0.0, 1.0);
    
    xglBegin(GL_POLYGON);
    xglColor3f(1.0, 1.0, 1.0);
    for(i=0;i<90;i++){
        xglVertex2f(cos(i * 4 * DEG_TO_RAD) * 5, sin(i * 4 * DEG_TO_RAD) * 5);
    }
    xglEnd();
    
    xglBegin(GL_POLYGON);
    xglVertex2f(wingArea[0], wingArea[1]);
    xglVertex2f(wingArea[2], wingArea[3]);
    xglVertex2f(wingArea[4], wingArea[5]);
    xglVertex2f(wingArea[6], wingArea[7]);
    xglVertex2f(wingArea[0], wingArea[1]);
    xglEnd();
    
    xglBegin(GL_POLYGON);
    xglVertex2f(elevatorArea[0], elevatorArea[1]);
    xglVertex2f(elevatorArea[2], elevatorArea[3]);
    xglVertex2f(elevatorArea[4], elevatorArea[5]);
    xglVertex2f(elevatorArea[6], elevatorArea[7]);
    xglVertex2f(elevatorArea[0], elevatorArea[1]);
    xglEnd();
    
    xglBegin(GL_POLYGON);
    xglVertex2f(rudderArea[0], rudderArea[1]);
    xglVertex2f(rudderArea[2], rudderArea[3]);
    xglVertex2f(rudderArea[4], rudderArea[5]);
    xglVertex2f(rudderArea[6], rudderArea[7]);
    xglVertex2f(rudderArea[0], rudderArea[1]);
    xglEnd();
    
    
    alphahist[0] = alphahist[1];
    alphahist[1] = alpha;
    rollhist[0] = rollhist[1];
    rollhist[1] = -get_roll();
    
    xglDisable(GL_BLEND);
}

void FGTurnCoordinator::Init(void){
    int n;
    PlaneTexXPos = 49;
    PlaneTexYPos = 59.75;
    BallXPos = 145;
    BallYPos = 24;
    BallTexXPos = 49;
    BallTexYPos = 16;
    BallRadius = 3.5;
    
    for(n=0;n<36;n++){
        vertices[2 * n] = cos(10 * n * DEG_TO_RAD) * BallRadius;
        vertices[(2 * n) + 1] = sin(10 * n * DEG_TO_RAD) * BallRadius;
    }   
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTexInstrument base class.
////////////////////////////////////////////////////////////////////////

FGTexInstrument::FGTexInstrument (void)
{
}

FGTexInstrument::~FGTexInstrument (void)
{
}

void FGTexInstrument::UpdatePointer(void){
    float alpha;

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    
    alpha=((((float)(getValue() - (value1))) /
            (value2 - value1))* (alpha2 - alpha1) + alpha1);
    
    if (alpha < alpha1)
        alpha = alpha1;
    if (alpha > alpha2)
        alpha -= alpha2;
    xglMatrixMode(GL_MODELVIEW);  
    xglPushMatrix();
    xglLoadIdentity();
    xglDisable(GL_TEXTURE_2D);
    xglTranslatef(XPos, YPos, 0);
    xglRotatef(-alpha, 0.0, 0.0, 1.0);
    xglColor4f(1.0, 1.0, 1.0, 1.0);
    glDrawArrays(GL_POLYGON, 0, 10);
    tape[0] = tape[1];
    tape[1] = alpha;
    xglEnable(GL_TEXTURE_2D);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// CreatePointer - calculate the vertices of a pointer 

void FGTexInstrument::CreatePointer(void){
    int i;
    float alpha;
    float alphastep;
    float r = radius;
    
    vertices[0] = 0;
    vertices[1] = length;
    vertices[2] = -(width/2);
    vertices[3] = length - ((width/2)/(tan(angle*DEG_TO_RAD/2)));
    vertices[4] = -(width/2);
    vertices[5] = cos(asin((width/2)/r))*r;
    
    alphastep = (asin((width/2)/r)+asin((width/2)/r))/5;
    alpha = asin(-(width/2)/r);
    
    for(i=0;i<5;i++){
        alpha += alphastep;
        vertices[(i*2)+6] = sin(alpha)*r;
    }
    
    alpha = asin(-(width/2)/r);
    
    for(i=0;i<5;i++){
        alpha +=alphastep;
        vertices[(i*2)+7]= cos(alpha)*r;
    }
    
    vertices[16] = - vertices[4];
    vertices[17] = vertices[5];
    vertices[18] = - vertices[2];
    vertices[19] = vertices[3];
    
}

void FGTexInstrument::Init(void){
    CreatePointer();
}

void FGTexInstrument::Render(void){
    xglEnable(GL_TEXTURE_2D);
    xglLoadIdentity();
    xglTranslatef(XPos, YPos, 0.0);
    xglRotatef(-tape[0], 0.0, 0.0, 1.0);
    fgEraseArea(vertices, 20, (GLfloat)(textureXPos),                                               (GLfloat)(textureYPos), (GLfloat)(XPos),                                            (GLfloat)(YPos), 0, 1);
    
    UpdatePointer();
    
    xglDisable(GL_TEXTURE_2D);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAirspeedIndicator.
////////////////////////////////////////////////////////////////////////


FGAirspeedIndicator::FGAirspeedIndicator (int x, int y)
{
  XPos = x;
  YPos = y;
  radius = 4;
  length = 32;
  width = 3;
  angle = 30;
  value1 = 15.0;
  value2 = 260.0;
  alpha1 = -20.0;
  alpha2 = 360;
  textureXPos = 65;
  textureYPos = 193;
  Init();
}


FGAirspeedIndicator::~FGAirspeedIndicator ()
{
}


double
FGAirspeedIndicator::getValue () const
{
  return get_speed();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGVerticalSpeedIndicator.
////////////////////////////////////////////////////////////////////////


FGVerticalSpeedIndicator::FGVerticalSpeedIndicator (int x, int y)
{
  XPos = x;
  YPos = y;
  radius = 4;
  length = 30;
  width = 3;
  angle = 30;
  value1 = -3.0;
  value2 = 3.0;
  alpha1 = 100;
  alpha2 = 440;
  textureXPos = 66.15;
  textureYPos = 66;
  Init();
}


FGVerticalSpeedIndicator::~FGVerticalSpeedIndicator ()
{
}


double
FGVerticalSpeedIndicator::getValue () const
{
  return get_climb_rate() / 1000.0;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAltimeter.
////////////////////////////////////////////////////////////////////////


FGAltimeter::FGAltimeter (int x, int y)
{
  XPos = x;
  YPos = y;
  radius = 5;
  length = 25;
  width = 4;
  angle = 30;
  value1 = 0;
  value2 = 10000;
  alpha1 = 0;
  alpha2 = 360;
  textureXPos = 194;
  textureYPos = 191;
  Init();
}


FGAltimeter::~FGAltimeter ()
{
}


double
FGAltimeter::getValue () const
{
  return get_altitude();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAltimeter2.
////////////////////////////////////////////////////////////////////////


FGAltimeter2::FGAltimeter2 (int x, int y)
{
  XPos = x;
  YPos = y;
  radius = 5;
  length = 32;
  width = 3;
  angle = 30;
  value1 = 0;
  value2 = 3000;
  alpha1 = 0;
  alpha2 = 1080;
  textureXPos = 194;
  textureYPos = 191;
  Init();
}


FGAltimeter2::~FGAltimeter2 ()
{
}


double
FGAltimeter2::getValue () const
{
  return get_altitude();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGRPMIndicator.
////////////////////////////////////////////////////////////////////////


FGRPMIndicator::FGRPMIndicator (int x, int y)
{
  XPos = x;
  YPos = y;
  radius = 10;
  length = 20;
  width = 5.5;
  angle = 60;
  value1 = 0.0;
  value2 = 1.0;
  alpha1 = -67;
  alpha2 = 180;
  textureXPos = 174;
  textureYPos = 83;
  Init();
}


FGRPMIndicator::~FGRPMIndicator ()
{
}


double
FGRPMIndicator::getValue () const 
{
  return get_throttleval();
}
