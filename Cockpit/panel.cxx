// panel.cxx -- routines to draw an instrument panel
//
// Written by Friedemann Reinhard, started June 1998.
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
// (Log is kept at end of this file)


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
#include <Objects/texload.h>

#include "panel.hxx"
#include "cockpit.hxx"
#include "hud.hxx"

GLubyte *imag;
int imag_width, imag_height;

GLubyte *img;
int img_width, img_height;

static float value[4];
static GLuint panel_tex_id[2];
static GLubyte tex[32][128][3];
static float alphahist;
static float Xzoom, Yzoom;
static Pointer pointer[20];
static int NumPoint = 4;
static int i = 0;
static GLdouble mvmatrix[16];
static GLdouble matrix[16];
static double var[20];
static double offset;
static float alpha;
static int n1;
static int n2;

static GLfloat Wings[] = {-1.25, -28.125, 1.255, -28.125, 1.255, 28.125,                                  -1.25, 28.125};
static GLfloat Elevator[] = { 3.0, -10.9375, 4.5, -10.9375, 4.5, 10.9375,                                  3.0, 10.9375};
static GLfloat Rudder[] = {2.0, -0.45, 10.625, -0.45, 10.625, 0.55,                                           2.0, 0.55};

FGPanel* FGPanel::OurPanel = 0;

// FGPanel::FGPanel() - constructor to initialize the panel.                 
FGPanel::FGPanel(void){

    string tpath;
    int x, y;
    FILE *f;
    char line[256];
    GLint test;
    GLubyte tex[262144];

OurPanel = this;   

    Xzoom = (float)((float)(current_view.get_winWidth())/1024);
    Yzoom = (float)((float)(current_view.get_winHeight())/768);

test_instr[3] = new FGTexInstrument(144.375, 166.875, 4, 32, 3, 30, 15.0,                                           260.0, -20.0, 360, 65, 193, 0);
test_instr[4] = new FGTexInstrument(358, 52, 4, 30, 3, 30, -3.0, 3.0, 100,                                          440, 66.15, 66, 2);
test_instr[5] = new FGTexInstrument(357.5, 167, 5, 25, 4, 30, 0, 10000, 0,                                          360, 194, 191, 1);
test_instr[6] = new FGTexInstrument(357.5, 167, 5, 32, 3, 30, 0, 3000, 0,                                           1080, 194, 191, 1);
test_instr[0] = new FGHorizon(251, 166.75);
test_instr[1] = new FGTurnCoordinator(143.75, 51.75);
//test_instr[2] = new FGRpmGauge(462.5, 133);
test_instr[2] = new FGTexInstrument(462.5, 133, 10, 20, 5.5, 60, 0.0, 1.0,                                            -67, 180, 174, 83, 3); 

// FontList = glGenLists (256);
// glListBase(FontList);
// InitLists();

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
    tpath = current_options.get_fg_root() + "/Textures/gauges.rgb";
    if((img = read_rgb_texture( (char *)tpath.c_str(), &img_width,                      &img_height ))==NULL){
    }

    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    tpath = current_options.get_fg_root() + "/Textures/gauges2.rgb";
    if((imag = read_rgb_texture( (char *)tpath.c_str(), &imag_width,                     &imag_height ))==NULL){
    }

    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);

    tpath = current_options.get_fg_root() + "/Textures/Fullone.rgb";
    if ((background = read_rgb_texture( (char *)tpath.c_str(), &width,                         &height ))==NULL ){
        }

//    for(y=0;y<256;y++){
//    	for(x=0;x<256;x++){
//    	tex[(y+x*256)*3] = imag[(y+x*256)*3];
//    	tex[(y+x*256)*3 + 1] = imag[(y+x*256)*3 + 1];
//    	tex[(y+x*256)*3 + 2] = imag[(y+x*256)*3 + 2];
//    	tex[(y+x*256)*3 + 3] = (imag[(y+x*256)*3 + 1] + imag[(y+x*256)*3 + 2]   //                               + imag[(y+x*256)*3 + 0])/3;
//    	
//	if((imag[(y+x*256)*3] < 150) && (imag[(y+x*256)*3 +1] < 150) &&         //             (imag[(y+x*256)*3 + 2] < 150) ){
//              tex[(y+x*256)*3 + 3] = 0x0;
//               }
//           else{
//             tex[(y+x*256)*3 + 3] = 0xff;
//             }
//    	 }
// }

    xglPixelZoom(Xzoom, Yzoom);
    xglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);
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

void FGPanel::ReInit( int x, int y, int finx, int finy){
    fgOPTIONS *o;
    int i;
    GLint buffer;
    
    o = &current_options;
    
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
    xglDrawPixels(finx - x, finy - y, GL_RGB, GL_UNSIGNED_BYTE,                     (GLvoid *)(background));

    // restore original buffer state
    xglDrawBuffer( buffer );
    xglEnable(GL_DEPTH_TEST);
}

void FGPanel::Update ( void ) {

    float alpha;
    double pitch;
    double roll;
    float alpharad;
    double speed;
    int i;
    
//    static bool beech_drawn = false;
//    char *test = "ALM 100";
                          
    var[0] = get_speed() /* * 1.4 */; // We have to multiply the airspeed by a 
                                // factor, to simulate flying a Bonanza 
    var[1] = get_altitude();
    var[2] = get_climb_rate() / 1000.0; 
    var[3] = get_throttleval();

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
     test_instr[3]->Render();
     test_instr[4]->Render();
     test_instr[5]->Render();
     test_instr[6]->Render();
    xglPopMatrix();
    xglPushMatrix();
    
    test_instr[1]->Render();
    test_instr[2]->Render();

//   DrawBeechcraftLogo(230, 235, 30, 10);
//   DrawScale(144.375, 166.875, 38, 41.0, 18, 340, 44, 2.0, 1.0, 1.0, 1.0);
    
    xglEnable(GL_LIGHTING);
            
    test_instr[0]->Render();
    
    
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

// horizon - Let's draw an artificial horizon using both texture mapping and 
//           primitive drawing
 
void FGHorizon::Render(void){ 
    double pitch;
    double roll;
    float shifted, alpha, theta;
    float epsi = 360 / 180;
    GLboolean Light;
    GLfloat normal[2];
    
   static int n, dn, rot, tmp1, tmp2;
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
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id[1]);
    #elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id[1]);
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
	   for(i=45;i<225;i++){
                     xglTexCoord2f(0.0, 0.0);
                     xglNormal3f(0.0, 0.0, 0.6);
                     xglVertex3f(0.0, 0.0, 0.0);
	       	     xglTexCoord2f(texCoord[i % 180][0], texCoord[i % 180][1]);
	             xglNormal3f(normals[i % 180][0], normals[i % 180][1], 0.6);
		     xglVertex3f(vertices[i % 180][0], vertices[i % 180][1],                                     0.0);
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
          xglVertex3f(((vertices[n1 % 180][0] + vertices[n2 % 180][0]) / 2),                            ((vertices[n1 % 180][1] + vertices[n2 % 180][1]) / 2),                             0.0); 
                      
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

void FGTexInstrument::UpdatePointer(void){
    double pitch;
    double roll;
    float alpharad;
    double speed;    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);

    alpha=((((float)((var[variable]) - (value1))) /                                        (value2 - value1))*                                                             (alpha2 - alpha1) + alpha1);
    
    	if (alpha < alpha1)
       		alpha = alpha1;
    	if (alpha > alpha2)
    	        alpha = alpha2;
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

// fgEraseArea - 'Erases' a drawn Polygon by overlaying it with a textured 
//                 area. Shall be a method of a panel class once.

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
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id[Texid]);
    #elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id[Texid]);
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

fgEraseArea(Wings, 8, PlaneTexXPos, PlaneTexYPos,                                                     XPos, YPos, 1, 1); 
fgEraseArea(Elevator, 8, PlaneTexXPos, PlaneTexYPos,                                                     XPos, YPos, 1, 1); 
fgEraseArea(Rudder, 8, PlaneTexXPos, PlaneTexYPos,                                                     XPos, YPos, 1, 1); 

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
xglVertex2f(Wings[0], Wings[1]);
xglVertex2f(Wings[2], Wings[3]);
xglVertex2f(Wings[4], Wings[5]);
xglVertex2f(Wings[6], Wings[7]);
xglVertex2f(Wings[0], Wings[1]);
xglEnd();

xglBegin(GL_POLYGON);
xglVertex2f(Elevator[0], Elevator[1]);
xglVertex2f(Elevator[2], Elevator[3]);
xglVertex2f(Elevator[4], Elevator[5]);
xglVertex2f(Elevator[6], Elevator[7]);
xglVertex2f(Elevator[0], Elevator[1]);
xglEnd();

xglBegin(GL_POLYGON);
xglVertex2f(Rudder[0], Rudder[1]);
xglVertex2f(Rudder[2], Rudder[3]);
xglVertex2f(Rudder[4], Rudder[5]);
xglVertex2f(Rudder[6], Rudder[7]);
xglVertex2f(Rudder[0], Rudder[1]);
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

void DrawScale(float XPos, float YPos, float InnerRadius, float OuterRadius,                   float alpha1, float alpha2, int steps, float LineWidth,                       float red, float green, float blue, bool filled){
     int i;
     float diff = (alpha2 - alpha1) / (float)(steps - 1);
     
     #define ANTIALIASED_INSTRUMENTS

     #ifdef ANTIALIASED_INSTRUMENTS
     xglEnable(GL_LINE_SMOOTH);
     xglEnable(GL_BLEND);
     xglHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
     #endif
     
     xglMatrixMode(GL_MODELVIEW);
     xglLoadIdentity();
     
     xglTranslatef(XPos, YPos, 0.0);
     xglRotatef(-alpha1, 0.0, 0.0, 1.0);
     
     xglLineWidth(LineWidth);
     xglColor3f(red, green, blue);
     
     if(!filled){
     xglBegin(GL_LINES);
     }
     else{
     xglBegin(GL_QUAD_STRIP);
     }
     
     for(i=0;i < steps; i++){
     xglVertex3f(sin(i * diff * DEG_TO_RAD) * OuterRadius,                                       cos(i * diff * DEG_TO_RAD) * OuterRadius, 0.0);
     xglVertex3f(sin(i * diff * DEG_TO_RAD) * InnerRadius,                                       cos(i * diff * DEG_TO_RAD) * InnerRadius, 0.0);
        }
     xglEnd();
     
     xglLoadIdentity();
     xglDisable(GL_LINE_SMOOTH);
     xglDisable(GL_BLEND);
     }

void DrawBeechcraftLogo(float XPos, float YPos, float width, float height){
     xglMatrixMode(GL_MODELVIEW);
     xglLoadIdentity();
     xglTranslatef(XPos, YPos, 0.0);
     xglEnable(GL_BLEND);
     xglEnable(GL_TEXTURE_2D);
//   xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
//   xglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    #ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id[1]);
    #elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id[1]);
    #else
    #  error port me
    #endif

    xglBegin(GL_POLYGON);
    
    xglTexCoord2f(.39844, .01953);
    xglVertex2f(0.0, 0.0);
    xglTexCoord2f(.58594, .01953);
    xglVertex2f(width, 0.0);
    xglTexCoord2f(.58594, .074219);
    xglVertex2f(width, height);
    xglTexCoord2f(.39844, .074219);
    xglVertex2f(0.0, height);
    
    xglEnd();
    
    xglDisable(GL_BLEND);
    xglDisable(GL_TEXTURE_2D);
 }
    

// PrintMatrix - routine to print the current modelview matrix.                 

void PrintMatrix( void){
xglGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
printf("matrix2 = %f %f %f %f \n", mvmatrix[0], mvmatrix[1], mvmatrix[2], mvmatrix[3]);
printf("         %f %f %f %f \n", mvmatrix[4], mvmatrix[5], mvmatrix[6], mvmatrix[7]);
printf("         %f %f %f %f \n", mvmatrix[8], mvmatrix[9], mvmatrix[10], mvmatrix[11]);
printf("         %f %f %f %f \n", mvmatrix[12], mvmatrix[13], mvmatrix[14], mvmatrix[15]);
}

void FGRpmGauge::Init(void){
     list = xglGenLists (1);
     int n;
     
     xglNewList(list, GL_COMPILE);
     
     xglColor3f(.26, .289, .3281);
     xglBegin(GL_POLYGON);
     for(n = 0; n < 180; n++){
     xglVertex2f(cos(n * 0.0349066) * 24.5, sin(n * 0.0349066) * 24.5);
     }
     xglEnd();
     
DrawScale(XPos, YPos, 22.5, 25.625, 50, 135, 10, 1.0, 0.0, 0.7,                           0.0,FILLED);
DrawScale(XPos, YPos, 21.0, 25.625, -70, 180, 8, 1.8, 0.88, 0.88, 0.88, false);
DrawScale(XPos, YPos, 22.5, 25.0, -70, 180, 40, 0.6, 0.5, 0.5, 0.5, false);
          
     xglEndList();
     }
     
void FGRpmGauge::Render(void){
     xglMatrixMode(GL_MODELVIEW);
     xglLoadIdentity();
     xglTranslatef(XPos, YPos, 0.0);
     
     xglCallList(list);
          
     }
     
void FGPanel::DrawTestLetter(float X, float Y){
     xglEnable(GL_TEXTURE_2D);
     xglEnable(GL_BLEND);
     
     xglMatrixMode(GL_TEXTURE);
     xglLoadIdentity();
     xglTranslatef(X, Y, 0.0);
     
    DrawLetter();
    
     xglMatrixMode(GL_MODELVIEW); 
     xglTranslatef(6.0, 0.0, 0.0);
     xglDisable(GL_TEXTURE_2D);
     xglDisable(GL_BLEND);
     }
    
void FGPanel::InitLists(void){
     xglNewList(FontList + 'A', GL_COMPILE);
     DrawTestLetter(0.391625, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'B', GL_COMPILE);
     DrawTestLetter(0.391625 + 1 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'C', GL_COMPILE);
     DrawTestLetter(0.391625 + 2 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'D', GL_COMPILE);
     DrawTestLetter(0.391625 + 3 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'E', GL_COMPILE);
     DrawTestLetter(0.391625 + 4 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'F', GL_COMPILE);
     DrawTestLetter(0.391625 + 5 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'G', GL_COMPILE);
     DrawTestLetter(0.391625 + 6 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'H', GL_COMPILE);
     DrawTestLetter(0.391625 + 7 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'I', GL_COMPILE);
     DrawTestLetter(0.391625 + 8 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'J', GL_COMPILE);
     DrawTestLetter(0.391625 + 9 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'K', GL_COMPILE);
     DrawTestLetter(0.391625 + 9.7 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'L', GL_COMPILE);
     DrawTestLetter(0.399625 + 10.6 * LETTER_OFFSET, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'M', GL_COMPILE);
     DrawTestLetter(0.80459375, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'N', GL_COMPILE);
     DrawTestLetter(0.83975, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'O', GL_COMPILE);
     DrawTestLetter(0.871, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'P', GL_COMPILE);
     DrawTestLetter(0.90715625, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + 'Q', GL_COMPILE);
     DrawTestLetter(0.9413125, 0.29296875);
     xglEndList();
     
     xglNewList(FontList + '1', GL_COMPILE);
     DrawTestLetter(0.390625, 0.35546875);
     xglEndList();
     
     xglNewList(FontList + '2', GL_COMPILE);
     DrawTestLetter(0.390625 + 1*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + '3', GL_COMPILE);
     DrawTestLetter(0.390625 + 2*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + '4', GL_COMPILE);
     DrawTestLetter(0.390625 + 3*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + '5', GL_COMPILE);
     DrawTestLetter(0.390625 + 4*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + '6', GL_COMPILE);
     DrawTestLetter(0.390625 + 5*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + '7', GL_COMPILE);
     DrawTestLetter(0.390625 + 6*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + '8', GL_COMPILE);
     DrawTestLetter(0.390625 + 7*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + '9', GL_COMPILE);
     DrawTestLetter(0.390625 + 8*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + '0', GL_COMPILE);
     DrawTestLetter(0.383625 + 9*LETTER_OFFSET, 0.3515625); 
     xglEndList();
     
     xglNewList(FontList + ' ', GL_COMPILE);
     xglTranslatef(8.0, 0.0, 0.0);
     xglEndList();
     }
    
void FGPanel::TexString(char *s, float XPos, float YPos, float size){
     xglMatrixMode(GL_MODELVIEW);
     xglLoadIdentity();
     xglTranslatef(XPos, YPos, 0.0);
     xglScalef(size, size, 1.0);
     
    #ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id[1]);
    #elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id[1]);
    #else
    #  error port me
    #endif
    
     while((*s) != '\0'){
     xglCallList(FontList + (*s));
     s++;
      }
      xglLoadIdentity();
     }
     
void FGTexInstrument::Init(void){
     CreatePointer();
     }
     
void FGTexInstrument::Render(void){
xglEnable(GL_TEXTURE_2D);
     xglLoadIdentity();
     xglTranslatef(XPos, YPos, 0.0);
     xglRotatef(-tape[0], 0.0, 0.0, 1.0);
    fgEraseArea(vertices, 20, (GLfloat)(teXpos),                                               (GLfloat)(texYpos), (GLfloat)(XPos),                                            (GLfloat)(YPos), 0, 1);
     
     UpdatePointer();
     
     xglDisable(GL_TEXTURE_2D);
     }
     
// $Log$
// Revision 1.18  1999/03/09 20:58:17  curt
// Tweaks for compiling under native Irix compilers.
//
// Revision 1.17  1999/03/08 21:56:09  curt
// Added panel changes sent in by Friedemann.
//
// Revision 1.13  1999/01/07 19:25:53  curt
// Updates from Friedemann Reinhard.
//
// Revision 1.11  1998/11/11 00:19:27  curt
// Updated comment delimeter to C++ style.
//
// Revision 1.10  1998/11/09 23:38:52  curt
// Panel updates from Friedemann.
//
// Revision 1.9  1998/11/06 21:18:01  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.8  1998/10/16 23:27:37  curt
// C++-ifying.
//
// Revision 1.7  1998/08/31 20:45:31  curt
// Tweaks from Friedemann.
//
// Revision 1.6  1998/08/28 18:14:40  curt
// Added new cockpit code from Friedemann Reinhard
// <mpt218@faupt212.physik.uni-erlangen.de>
//
// Revision 1.1  1998/06/27 16:47:54  curt
// Incorporated Friedemann Reinhard's <mpt218@faupt212.physik.uni-erlangen.de>
// first pass at an isntrument panel.
//


