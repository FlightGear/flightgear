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

// #include <GL/gl.h>
// #include <GL/glu.h>
#include <GL/glut.h>
#include <XGL/xgl.h>

#include "Include/compiler.h"
#ifdef FG_HAVE_STD_INCLUDES
#  include <cstdlib>
#  include <cstdio>
#  include <cstring>
#  include <cmath>
#else
#  include <stdlib.h>
#  include <stdio.h>
#  include <string.h>
#  include <math.h>
#endif

#include <string>
FG_USING_STD(string);

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>

#include "panel.hxx"
#include "cockpit.hxx"
#include "hud.hxx"

#define IMAGIC      0x01da
#define IMAGIC_SWAP 0xda01

#define SWAP_SHORT_BYTES(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define SWAP_LONG_BYTES(x) (((((x) & 0xff) << 24) | (((x) & 0xff00) << 8)) | \
((((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24)))

typedef struct {
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short sizeX, sizeY, sizeZ;
    unsigned long min, max;
    unsigned long wasteBytes;
    char name[80];
    unsigned long colorMap;
    FILE *file;
    unsigned char *tmp[5];
    unsigned long rleEnd;
    unsigned long *rowStart;
    unsigned long *rowSize;
} Image;


IMAGE *imag;
IMAGE *img2;
IMAGE *img;

static float value[4];
static GLuint panel_tex_id[2];
static GLubyte tex[32][128][3];
static float alphahist;
static float Xzoom, Yzoom;
static Pointer pointer[20];
static TurnCoordinator Turny;
static int NumPoint = 5;
static int i = 0;
static GLdouble mvmatrix[16];
static GLdouble matrix[16];
static double var[20];
static double offset;
static float alpha;
static float vertices[180][2];
static float normals[180][3];
static float texCoord[180][2];
static arthor myarthor;
static int n1;
static int n2;
static GLfloat Wings[] = {-1.25, -28.125, 1.255, -28.125, 1.255, 28.125,                                  -1.25, 28.125};
static GLfloat Elevator[] = { 3.0, -10.9375, 4.5, -10.9375, 4.5, 10.9375,                                  3.0, 10.9375};
static GLfloat Rudder[] = {2.0, -0.45, 10.625, -0.45, 10.625, 0.55,                                           2.0, 0.55};

// image.c THIS FILE WAS COPIED FROM THE MESA GRAPHICS LIBRARY


static Image *ImageOpen(char *fileName)
{
  Image *image;
  unsigned long *rowStart, *rowSize, ulTmp;
  int x, i;

  image = (Image *)malloc(sizeof(Image));
  if (image == NULL) 
    {
      fprintf(stderr, "Out of memory!\n");
      exit(-1);
    }
  if ((image->file = fopen(fileName, "rb")) == NULL) 
    {
      perror(fileName);
      exit(-1);
    }
  /*
   *	Read the image header
   */
  fread(image, 1, 12, image->file);
  /*
   *	Check byte order
   */
  if (image->imagic == IMAGIC_SWAP) 
    {
      image->type = SWAP_SHORT_BYTES(image->type);
      image->dim = SWAP_SHORT_BYTES(image->dim);
      image->sizeX = SWAP_SHORT_BYTES(image->sizeX);
      image->sizeY = SWAP_SHORT_BYTES(image->sizeY);
      image->sizeZ = SWAP_SHORT_BYTES(image->sizeZ);
    }

  for ( i = 0 ; i <= image->sizeZ ; i++ )
    {
      image->tmp[i] = (unsigned char *)malloc(image->sizeX*256);
      if (image->tmp[i] == NULL ) 
	{
	  fprintf(stderr, "Out of memory!\n");
	  exit(-1);
	}
    }

  if ((image->type & 0xFF00) == 0x0100) // RLE image
    {
      x = image->sizeY * image->sizeZ * sizeof(long);
      image->rowStart = (unsigned long *)malloc(x);
      image->rowSize = (unsigned long *)malloc(x);
      if (image->rowStart == NULL || image->rowSize == NULL) 
	{
	  fprintf(stderr, "Out of memory!\n");
	  exit(-1);
	}
      image->rleEnd = 512 + (2 * x);
      fseek(image->file, 512, SEEK_SET);
      fread(image->rowStart, 1, x, image->file);
      fread(image->rowSize, 1, x, image->file);
      if (image->imagic == IMAGIC_SWAP) 
	{
	  x /= sizeof(long);
	  rowStart = image->rowStart;
	  rowSize = image->rowSize;
	  while (x--) 
	    {
	      ulTmp = *rowStart;
	      *rowStart++ = SWAP_LONG_BYTES(ulTmp);
	      ulTmp = *rowSize;
	      *rowSize++ = SWAP_LONG_BYTES(ulTmp);
	    }
	}
    }
  return image;
}

static void ImageClose( Image *image)
{
  int i;

  fclose(image->file);
  for ( i = 0 ; i <= image->sizeZ ; i++ )
    free(image->tmp[i]);
  free(image);
}

static void ImageGetRow( Image *image, unsigned char *buf, int y, int z)
{
  unsigned char *iPtr, *oPtr, pixel;
  int count;

  if ((image->type & 0xFF00) == 0x0100)  // RLE image
    {
      fseek(image->file, image->rowStart[y+z*image->sizeY], SEEK_SET);
      fread(image->tmp[0], 1, (unsigned int)image->rowSize[y+z*image->sizeY],
	    image->file);

      iPtr = image->tmp[0];
      oPtr = buf;
      while (1) 
	{
	  pixel = *iPtr++;
	  count = (int)(pixel & 0x7F);
	  if (!count)
	    return;
	  if (pixel & 0x80) 
	    {
	      while (count--) 
		{
		  *oPtr++ = *iPtr++;
		}
	    } 
	  else 
	    {
	      pixel = *iPtr++;
	      while (count--) 
		{
		  *oPtr++ = pixel;
		}
	    }
	}
    }
  else // verbatim image
    {
      fseek(image->file, 512+(y*image->sizeX)+(z*image->sizeX*image->sizeY),
	    SEEK_SET);
      fread(buf, 1, image->sizeX, image->file);
    }
}

static void ImageGetRawData( Image *image, char *data)
{
  int i, j, k;
  int remain;

  switch ( image->sizeZ )
    {
    case 1:
      remain = image->sizeX % 4;
      break;
    case 2:
      remain = image->sizeX % 2;
      break;
    case 3:
      remain = (image->sizeX * 3) & 0x3;
      if (remain)
	remain = 4 - remain;
      break;
    case 4:
      remain = 0;
      break;
    }

  for (i = 0; i < image->sizeY; i++) 
    {
      for ( k = 0; k < image->sizeZ ; k++ )
	ImageGetRow(image, image->tmp[k+1], i, k);
      for (j = 0; j < image->sizeX; j++) 
	for ( k = 1; k <= image->sizeZ ; k++ )
	  *data++ = *(image->tmp[k] + j);
      data += remain;
    }
}

static IMAGE *ImageLoad(char *fileName)
{
  Image *image;
  IMAGE *final;
  int sx;

  image = ImageOpen(fileName);

  final = (IMAGE *)malloc(sizeof(IMAGE));
  if (final == NULL) 
    {
      fprintf(stderr, "Out of memory!\n");
      exit(-1);
    }
  final->imagic = image->imagic;
  final->type = image->type;
  final->dim = image->dim;
  final->sizeX = image->sizeX; 
  final->sizeY = image->sizeY;
  final->sizeZ = image->sizeZ;

  /* 
   * Round up so rows are long-word aligned 
   */
  sx = ( (image->sizeX) * (image->sizeZ) + 3) >> 2;

  final->data 
    = (unsigned char *)malloc( sx * image->sizeY * sizeof(unsigned int));

  if (final->data == NULL) 
    {
      fprintf(stderr, "Out of memory!\n");
      exit(-1);
    }

  ImageGetRawData(image, (char*)final->data);
  ImageClose(image);
  return final;
}

// Beginning of the "panel-code" 

// fgPanelInit() - routine to initialize a panel.                 
void fgPanelInit ( void ) {
    // fgVIEW *v;
    string tpath;
    int x, y;
    FILE *f;
    char line[256];
    GLint test;

    // v = &current_view;

    Xzoom = (float)((float)(current_view.get_winWidth())/1024);
    Yzoom = (float)((float)(current_view.get_winHeight())/768);

pointer[1].XPos = 357.5;
pointer[1].YPos = 167;
pointer[1].radius = 5;
pointer[1].length = 32;
pointer[1].width = 3;
pointer[1].angle = 30;
pointer[1].value1 = 0;
pointer[1].value2 = 3000;
pointer[1].alpha1 = 0;
pointer[1].alpha2 = 1080;
pointer[1].variable = 1;
pointer[1].teXpos = 194;
pointer[1].texYpos = 191;

pointer[0].XPos = 357.5;
pointer[0].YPos = 167;
pointer[0].radius = 5;
pointer[0].length = 25;
pointer[0].width = 4;
pointer[0].angle = 30;
pointer[0].value1 = 0;
pointer[0].value2 = 10000;
pointer[0].alpha1 = 0;
pointer[0].alpha2 = 360;
pointer[0].variable = 1;
pointer[0].teXpos = 194;
pointer[0].texYpos = 191;

pointer[2].XPos = 358;
pointer[2].YPos = 52;
pointer[2].radius = 4;
pointer[2].length = 30;
pointer[2].width = 3;
pointer[2].angle = 30;
pointer[2].value1 = -3;
pointer[2].value2 = 3;
pointer[2].alpha1 = 100;
pointer[2].alpha2 = 440;
pointer[2].variable = 2;
pointer[2].teXpos = 66.15;
pointer[2].texYpos = 66;


pointer[3].XPos = 462;
pointer[3].YPos = 133;
pointer[3].radius = 9;
pointer[3].length = 20;
pointer[3].width = 5;
pointer[3].angle = 50;
pointer[3].value1 = 0.0;
pointer[3].value2 = 1.0;
pointer[3].alpha1 = 280;
pointer[3].alpha2 = 540;
pointer[3].variable = 3;
pointer[3].teXpos = 173.6;
pointer[3].texYpos = 83;


// These values define the airspeed pointer. Please note: 
// As we have a Bonanza panel, but a navion airplane, this gauge 
// doesn't show the true values !!!    

pointer[4].XPos = 144.375;
pointer[4].YPos = 166.875;
pointer[4].radius = 4;
pointer[4].length = 32;
pointer[4].width = 3;
pointer[4].angle = 30;
pointer[4].value1 = 15.0;
pointer[4].value2 = 260.0;
pointer[4].alpha1 = -20.0;
pointer[4].alpha2 = 360.0;
pointer[4].variable = 0;
pointer[4].teXpos = 64;
pointer[4].texYpos = 193;


myarthor.XPos = 251;
myarthor.YPos = 168;
myarthor.radius = 29;
myarthor.texXPos = 56;
myarthor.texYPos = 174;
myarthor.bottom = 36.5;
myarthor.top = 36.5;

Turny.PlaneXPos = 143.75;
Turny.PlaneYPos = 51.75;
Turny.PlaneTexXPos = 49;
Turny.PlaneTexYPos = 59.75;
Turny.BallXPos = 145;
Turny.BallYPos = 24;
Turny.BallTexXPos = 49;
Turny.BallTexYPos = 16;
Turny.BallRadius = 3.5;
Turny.alphahist = 0;
Turny.PlaneAlphaHist = 0;

for(i=0;i<NumPoint;i++){
CreatePointer(&pointer[i]);
}

fgHorizonInit(myarthor);

fgInitTurnCoordinator(&Turny);

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
    glOrtho(0, 640, 0, 480, 1, -1);
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
    if((img = ImageLoad( (char *)tpath.c_str() ))==NULL){
    }

    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    tpath = current_options.get_fg_root() + "/Textures/gauges2.rgb";
    if((imag = ImageLoad( (char *)tpath.c_str() ))==NULL){
    }

    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);

    tpath = current_options.get_fg_root() + "/Textures/Fullone.rgb";
    if ((img2 = ImageLoad( (char *)tpath.c_str() ))==NULL ){
        }

    xglPixelZoom(Xzoom, Yzoom);
    xglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);
      xglPixelStorei(GL_UNPACK_ROW_LENGTH, 1024);
    xglRasterPos2i(0,0);
    xglPixelZoom(Xzoom, Yzoom);
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB,                                  GL_UNSIGNED_BYTE, (GLvoid *)(imag->data));
		  
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
   xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB,                                  GL_UNSIGNED_BYTE, (GLvoid *)(img->data)); 
		  
    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
}

void fgPanelReInit( int x, int y, int finx, int finy){
    // fgVIEW *v;
    fgOPTIONS *o;
    int i;
    GLint buffer;
    
    o = &current_options;
    // v = &current_view;
    
    Xzoom = (float)((float)(current_view.get_winWidth())/1024);
    Yzoom = (float)((float)(current_view.get_winHeight())/768);
    
    // save the current buffer state
    glGetIntegerv(GL_DRAW_BUFFER, &buffer);
    
    // and enable both buffers for writing
    glDrawBuffer(GL_FRONT_AND_BACK);
    
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    xglViewport(0, 0, 640, 480);
    glOrtho(0, 640, 0, 480, 1, -1);
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
    xglDrawPixels(finx - x, finy - y, GL_RGB, GL_UNSIGNED_BYTE,                     (GLvoid *)(img2->data));
    
    // restore original buffer state
    glDrawBuffer(buffer);
}

void fgPanelUpdate ( void ) {
    // fgVIEW *v;
    float alpha;
    double pitch;
    double roll;
    float alpharad;
    double speed;
    int i;
                          
    var[0] = get_speed() * 1.4; // We have to multiply the airspeed by a 
                                // factor, to simulate flying a Bonanza 
    var[1] = get_altitude();
    var[2] = get_climb_rate(); 
    var[3] = get_throttleval();
    // v = &current_view;
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    glOrtho(0, 640, 0, 480, 10, -10);
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

    for(i=0;i<NumPoint;i++){
    xglLoadIdentity();
    xglTranslatef(pointer[i].XPos, pointer[i].YPos, 0.0);
    xglRotatef(-pointer[i].hist, 0.0, 0.0, 1.0);
    fgEraseArea(pointer[i].vertices, 20, (GLfloat)(pointer[i].teXpos),                          (GLfloat)(pointer[i].texYpos), (GLfloat)(pointer[i].XPos),                      (GLfloat)(pointer[i].YPos), 0);
    xglLoadIdentity();
    }

    glDisable(GL_LIGHTING);
    for(i=0;i<NumPoint;i++){
    pointer[i].hist = UpdatePointer( pointer[i]);
    xglPopMatrix();
    xglPushMatrix();
    }
    
    fgUpdateTurnCoordinator(&Turny);
    
    glEnable(GL_LIGHTING);
            
    horizon(myarthor);  

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
 
void horizon(arthor hor){ 
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
    
    glEnable(GL_NORMALIZE);
    xglEnable(GL_LIGHTING);
    xglEnable(GL_TEXTURE_2D);
    xglEnable(GL_LIGHT1);
    xglDisable(GL_LIGHT2);
    xglDisable(GL_LIGHT0);
    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    xglTranslatef(hor.XPos, hor.YPos, 0);
    xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    xglMatrixMode(GL_TEXTURE);
    glPushMatrix();
        
    // computations for the non-textured parts of the AH
    
    shifted = -((pitch / 10) * 7.0588235);
        
    if(shifted > (hor.bottom - hor.radius)){
    theta = (180 - (acos((hor.bottom - shifted) / hor.radius)*RAD_TO_DEG));
    n = (int)(theta / epsi) - 1;
    n1 = n;
    n2 = (180 - n1) + 2;
    dn = n2 - n1;
    rot = (int)(roll / epsi);
    n1 += rot + 45;
    n2 += rot + 45;
    }
    
    if(shifted < (-hor.top + hor.radius)){
    theta = ((acos((-hor.top - shifted) / hor.radius)*RAD_TO_DEG));
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
    glLightfv(GL_LIGHT1, GL_POSITION, light_position);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);  
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, direction);

    #ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id[1]);
    #elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id[1]);
    #else
    #  error port me
    #endif

    xglLoadIdentity();
    xglTranslatef(0.0, ((pitch / 10) * 0.046875), 0.0);
    xglTranslatef((hor.texXPos/256), (hor.texYPos/256), 0.0);
    xglRotatef(-roll, 0.0, 0.0, 1.0);
    xglScalef(1.7, 1.7, 0.0);

    // the following loop draws the textured part of the AH
    
   glMaterialf(GL_FRONT, GL_SHININESS, 85.0);
    
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material4);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material5);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material3);
   
    glMatrixMode(GL_MODELVIEW);
    xglBegin(GL_TRIANGLES);
	   for(i=45;i<225;i++){
                     glTexCoord2f(0.0, 0.0);
                     glNormal3f(0.0, 0.0, 0.6);
                     glVertex3f(0.0, 0.0, 0.0);
	       	     glTexCoord2f(texCoord[i % 180][0], texCoord[i % 180][1]);
	             glNormal3f(normals[i % 180][0], normals[i % 180][1], 0.6);
		     glVertex3f(vertices[i % 180][0], vertices[i % 180][1], 0.0);
		     n = (i + 1) % 180;
		     glTexCoord2f(texCoord[n][0], texCoord[n][1]);
		     glNormal3f(normals[n][0], normals[n][1], 0.6);
		     glVertex3f(vertices[n][0], vertices[n][1], 0.0);
      		}
     xglEnd();
    
            
    if((shifted > (hor.bottom - hor.radius)) && (n1 < 1000) && (n1 > 0)){
    
    a = sin(theta * DEG_TO_RAD) * sin(theta * DEG_TO_RAD);
light_ambient2[0] = a;
light_ambient2[1] = a;
light_ambient2[2] = a;
    
 glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient2);
 glLightfv(GL_LIGHT1, GL_DIFFUSE, light_ambient2); 
 glLightfv(GL_LIGHT1, GL_SPECULAR, light_ambient2);
        
    xglBegin(GL_TRIANGLES);
    
    tmp1 = n1; tmp2 = n2;
        
	    for(i = tmp1; i < tmp2 + 1; i++){
	             n = i % 180;
            glNormal3f(0.0, 0.0, 1.5);
              glTexCoord2f((56 / 256), (140 / 256));
            glVertex3f(((vertices[n1 % 180][0] + vertices[n2 % 180][0]) / 2),                            ((vertices[n1 % 180][1] + vertices[n2 % 180][1]) / 2),                             0.0);
                                                
             glTexCoord2f((57 / 256), (139 / 256));        
	     glNormal3f(normals[n][0], normals[n][1], normals[n][3]);
		     glVertex3f(vertices[n][0], vertices[n][1], 0.0);
		     
		     n = (i + 1) % 180;
	glTexCoord2f((57 / 256), (139 / 256));	     
	glNormal3f(normals[n][0], normals[n][1], normals[n][3]);
		     glVertex3f(vertices[n][0], vertices[n][1], 0.0);	     
      		}
     xglEnd();
    }
            
    if((shifted < (-hor.top + hor.radius)) && (n1 < 1000) && (n1 > 0)){
    a = sin(theta * DEG_TO_RAD) * sin(theta * DEG_TO_RAD);
light_ambient2[0] = a;
light_ambient2[1] = a;
light_ambient2[2] = a;
     
 glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient2);
 glLightfv(GL_LIGHT1, GL_DIFFUSE, light_ambient2); 
 glLightfv(GL_LIGHT1, GL_SPECULAR, light_ambient2);
 glMaterialf(GL_FRONT, GL_SHININESS, a * 85);
    xglBegin(GL_TRIANGLES);
    tmp1 = n1; tmp2 = n2;
	    for(i = tmp1; i <= tmp2; i++){
	             n = i % 180;
          glNormal3f(0.0, 0.0, 1.5);
          glTexCoord2f((73 / 256), (237 / 256));
          glVertex3f(((vertices[n1 % 180][0] + vertices[n2 % 180][0]) / 2),                            ((vertices[n1 % 180][1] + vertices[n2 % 180][1]) / 2),                             0.0); 
                      
                     glTexCoord2f((73 / 256), (236 / 256));
		     glNormal3f(normals[n][0], normals[n][1], normals[n][2]);
		     glVertex3f(vertices[n][0], vertices[n][1], 0.0);
		     
		     n = (i + 1) % 180;
		    glTexCoord2f((73 / 256), (236 / 256)); 
		    glNormal3f(normals[n][0], normals[n][1], normals[n][2]);
		    glVertex3f(vertices[n][0], vertices[n][1], 0.0); 
      		}
     xglEnd();
    }
    
    // Now we will have to draw the small triangle indicating the roll value

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    
    glRotatef(roll, 0.0, 0.0, 1.0);
    
    glBegin(GL_TRIANGLES);
    glColor3f(1.0, 1.0, 1.0);
    glVertex3f(0.0, hor.radius, 0.0);
    glVertex3f(-3.0, (hor.radius - 7.0), 0.0);
    glVertex3f(3.0, (hor.radius - 7.0), 0.0);    
    glEnd();
    
    glLoadIdentity();

    glBegin(GL_POLYGON);
    glColor3f(0.2109375, 0.23046875, 0.203125);
    glVertex2f(275.625, 135.0);
    glVertex2f(275.625, 148.125);
    glVertex2f(258.125, 151.25);
    glVertex2f(246.875, 151.25);
    glVertex2f(226.875, 147.5);
    glVertex2f(226.875, 135.0);
    glVertex2f(275.625, 135.0);
    glEnd();

    glLoadIdentity();
    
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

void fgHorizonInit( arthor hor){
int n;
float step = (360*DEG_TO_RAD)/180;

for(n=0;n<180;n++){
	   vertices[n][0] = cos(n * step) * hor.radius;
	   vertices[n][1] = sin(n * step) * hor.radius;
           texCoord[n][0] = (cos(n * step) * hor.radius)/256;
           texCoord[n][1] = (sin(n * step) * hor.radius)/256;
         normals[n][0] = cos(n * step) * hor.radius + sin(n * step) * 50;
         normals[n][1] = sin(n * step) * hor.radius + cos(n * step) * 50;
           normals[n][2] = 0.1;
           }
}

float UpdatePointer(Pointer pointer){
    double pitch;
    double roll;
    float alpharad;
    double speed;    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, pointer.vertices);

    alpha=((((float)((var[pointer.variable]) - (pointer.value1))) /                       (pointer.value2 - pointer.value1))*(pointer.alpha2 - pointer.alpha1)            + pointer.alpha1);
    
    	if (alpha < pointer.alpha1)
       		alpha = pointer.alpha1;
    	if (alpha > pointer.alpha2)
    	        alpha = pointer.alpha2;
    xglMatrixMode(GL_MODELVIEW);  
    xglPushMatrix();
    xglLoadIdentity();
    xglDisable(GL_TEXTURE_2D);
    xglTranslatef(pointer.XPos, pointer.YPos, 0);
    xglRotatef(-alpha, 0.0, 0.0, 1.0);
    xglColor4f(1.0, 1.0, 1.0, 1.0);
    glDrawArrays(GL_POLYGON, 0, 10);
    return alpha;
    xglEnable(GL_TEXTURE_2D);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// fgEraseArea - 'Erases' a drawn Polygon by overlaying it with a textured 
//                 area. Shall be a method of a panel class once.

void fgEraseArea(GLfloat *array, int NumVerti, GLfloat texXPos,
		 GLfloat texYPos, GLfloat XPos, GLfloat YPos,
		 int Texid, float ScaleFactor)
{
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

void CreatePointer(Pointer *pointer){
int i;
float alpha;
float alphastep;
float r = pointer->radius;
float angle = pointer->angle;
float length = pointer->length;
float width = pointer->width;

pointer->vertices[0] = 0;
pointer->vertices[1] = length;
pointer->vertices[2] = -(width/2);
pointer->vertices[3] = length - ((width/2)/(tan(angle*DEG_TO_RAD/2)));
pointer->vertices[4] = -(width/2);
pointer->vertices[5] = cos(asin((width/2)/r))*r;
alphastep = (asin((width/2)/r)+asin((width/2)/r))/5;
alpha = asin(-(width/2)/r);
for(i=0;i<5;i++){
alpha += alphastep;
pointer->vertices[(i*2)+6] = sin(alpha)*r;
}
alpha = asin(-(width/2)/r);
for(i=0;i<5;i++){
alpha +=alphastep;
pointer->vertices[(i*2)+7]= cos(alpha)*r;
}
pointer->vertices[16] = - pointer->vertices[4];
pointer->vertices[17] = pointer->vertices[5];
pointer->vertices[18] = - pointer->vertices[2];
pointer->vertices[19] = pointer->vertices[3];
}

// fgUpdateTurnCoordinator - draws turn coordinator related stuff

void fgUpdateTurnCoordinator(TurnCoordinator *turn){
int n;

glDisable(GL_LIGHTING);
glDisable(GL_BLEND);
glEnable(GL_TEXTURE_2D);

 turn->alpha = (get_sideslip() / 1.5) * 56;
 turn->PlaneAlpha = get_roll();

    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
glTranslatef(turn->BallXPos, turn->BallYPos, 0.0);
glTranslatef(0.75 * sin(turn->alphahist * DEG_TO_RAD) * 31,                                  0.3 * (39 - (cos(turn->alphahist * DEG_TO_RAD) * 39)),                          0.0);
fgEraseArea(turn->vertices, 72,                                                 turn->BallTexXPos + ((0.75 * sin(turn->alphahist * DEG_TO_RAD) * 31) / 0.625),  turn->BallTexYPos + ((0.3 * (39 - (cos(turn->alphahist * DEG_TO_RAD)                                  * 39))) / 0.625),                                                   turn->BallXPos + (0.75 * sin(turn->alphahist * DEG_TO_RAD) * 31),               turn->BallYPos + (0.3 * (39 - (cos(turn->alphahist * DEG_TO_RAD)                                  * 39))), 1);
glDisable(GL_TEXTURE_2D);
glEnable(GL_BLEND);
glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
glTranslatef(turn->BallXPos, turn->BallYPos, 0.0);
glTranslatef(0.75 * sin(turn->alpha * DEG_TO_RAD) * 31,                                      0.3 * (39 - (cos(turn->alpha * DEG_TO_RAD) * 39)), 0.0);
glBegin(GL_POLYGON);
glColor3f(0.8, 0.8, 0.8);
for(i=0;i<36;i++){
glVertex2f(turn->vertices[2 * i],                                                          turn->vertices[(2 * i) + 1]);
}
glEnd(); 

glDisable(GL_TEXTURE_2D);
glDisable(GL_BLEND);

glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
glTranslatef(turn->PlaneXPos, turn->PlaneYPos, 0.0);
glRotatef(turn->rollhist * RAD_TO_DEG + 90, 0.0, 0.0, 1.0);

fgEraseArea(Wings, 8, turn->PlaneTexXPos, turn->PlaneTexYPos,                                         turn->PlaneXPos, turn->PlaneYPos, 1); 
fgEraseArea(Elevator, 8, turn->PlaneTexXPos, turn->PlaneTexYPos,                                         turn->PlaneXPos, turn->PlaneYPos, 1); 
fgEraseArea(Rudder, 8, turn->PlaneTexXPos, turn->PlaneTexYPos,                                         turn->PlaneXPos, turn->PlaneYPos, 1); 

glLoadIdentity();
glTranslatef(turn->PlaneXPos, turn->PlaneYPos, 0.0);
glRotatef(-get_roll() * RAD_TO_DEG + 90, 0.0, 0.0, 1.0);

glBegin(GL_POLYGON);
glColor3f(1.0, 1.0, 1.0);
for(i=0;i<90;i++){
glVertex2f(cos(i * 4 * DEG_TO_RAD) * 5, sin(i * 4 * DEG_TO_RAD) * 5);
}
glEnd();

glBegin(GL_POLYGON);
glVertex2f(Wings[0], Wings[1]);
glVertex2f(Wings[2], Wings[3]);
glVertex2f(Wings[4], Wings[5]);
glVertex2f(Wings[6], Wings[7]);
glVertex2f(Wings[0], Wings[1]);
glEnd();

glBegin(GL_POLYGON);
glVertex2f(Elevator[0], Elevator[1]);
glVertex2f(Elevator[2], Elevator[3]);
glVertex2f(Elevator[4], Elevator[5]);
glVertex2f(Elevator[6], Elevator[7]);
glVertex2f(Elevator[0], Elevator[1]);
glEnd();

glBegin(GL_POLYGON);
glVertex2f(Rudder[0], Rudder[1]);
glVertex2f(Rudder[2], Rudder[3]);
glVertex2f(Rudder[4], Rudder[5]);
glVertex2f(Rudder[6], Rudder[7]);
glVertex2f(Rudder[0], Rudder[1]);
glEnd();


turn->alphahist = turn->alpha;
turn->PlaneAlphaHist = turn->PlaneAlpha;
turn->rollhist = -get_roll();

glDisable(GL_BLEND);
}

void fgInitTurnCoordinator(TurnCoordinator *turn){
int n;

for(n=0;n<36;n++){
turn->vertices[2 * n] = cos(10 * n * DEG_TO_RAD) * turn->BallRadius;
turn->vertices[(2 * n) + 1] = sin(10 * n * DEG_TO_RAD) * turn->BallRadius;
	}	
}

// PrintMatrix - routine to print the current modelview matrix.                 

void PrintMatrix( void){
glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
printf("matrix2 = %f %f %f %f \n", mvmatrix[0], mvmatrix[1], mvmatrix[2], mvmatrix[3]);
printf("         %f %f %f %f \n", mvmatrix[4], mvmatrix[5], mvmatrix[6], mvmatrix[7]);
printf("         %f %f %f %f \n", mvmatrix[8], mvmatrix[9], mvmatrix[10], mvmatrix[11]);
printf("         %f %f %f %f \n", mvmatrix[12], mvmatrix[13], mvmatrix[14], mvmatrix[15]);
}

// $Log$
// Revision 1.14  1999/02/02 20:13:33  curt
// MSVC++ portability changes by Bernie Bright:
//
// Lib/Serial/serial.[ch]xx: Initial Windows support - incomplete.
// Simulator/Astro/stars.cxx: typo? included <stdio> instead of <cstdio>
// Simulator/Cockpit/hud.cxx: Added Standard headers
// Simulator/Cockpit/panel.cxx: Redefinition of default parameter
// Simulator/Flight/flight.cxx: Replaced cout with FG_LOG.  Deleted <stdio.h>
// Simulator/Main/fg_init.cxx:
// Simulator/Main/GLUTmain.cxx:
// Simulator/Main/options.hxx: Shuffled <fg_serial.hxx> dependency
// Simulator/Objects/material.hxx:
// Simulator/Time/timestamp.hxx: VC++ friend kludge
// Simulator/Scenery/tile.[ch]xx: Fixed using std::X declarations
// Simulator/Main/views.hxx: Added a constant
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
