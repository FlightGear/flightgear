/**************************************************************************
 * panel.cxx -- routines to draw an instrument panel
 *
 * Written by Friedemann Reinhard, started June 1998.
 *
 * Copyright(C)1998 Friedemann Reinhard-reinhard@theorie2.physik.uni-erlangen.de
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

// #include <GL/gl.h>
// #include <GL/glu.h>
#include <GL/glut.h>
#include <XGL/xgl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <math.h>

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

static GLuint panel_tex_id;
static GLubyte tex[32][128][3];
static float alphahist;
static float Xzoom, Yzoom;
static Pointer pointer[20];
static int NumGyro = 1;
static int NumPoint = 4;
static int i = 0;
static GLdouble mvmatrix[16];
static GLdouble matrix[16];
static double var[20];
static double offset;
static float alpha;

/* image.c THIS FILE WAS COPIED FROM THE MESA GRAPHICS LIBRARY */


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

  if ((image->type & 0xFF00) == 0x0100) /* RLE image */
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

  if ((image->type & 0xFF00) == 0x0100)  /* RLE image */
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
  else /* verbatim image */
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

  ImageGetRawData(image, final->data);
  ImageClose(image);
  return final;
}

/* Beginning of the "panel-code" */
void fgPanelInit ( void ) {
    fgVIEW *v;
    string tpath;
    int x, y;

    v = &current_view;

    Xzoom = (float)((float)(current_view.winWidth)/1024);
    Yzoom = (float)((float)(current_view.winHeight)/768);

pointer[1].XPos = 357;
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
pointer[1].teXpos = 193;
pointer[1].texYpos = 191;

pointer[0].XPos = 357;
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
pointer[0].teXpos = 193;
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
pointer[3].teXpos = 173.8;
pointer[3].texYpos = 83;


for(i=0;i<NumPoint;i++){
CreatePointer(&pointer[i]);
}

#ifdef GL_VERSION_1_1
    xglGenTextures(1, &panel_tex_id);
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id);
#elif GL_EXT_texture_object
    xglGenTexturesEXT(1, &panel_tex_id);
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id);
#else
#  error port me
#endif
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    xglViewport(0, 0, 640, 480);
    gluOrtho2D(0, 640, 0, 480);
    xglMatrixMode(GL_MODELVIEW);
    xglPushMatrix();
    xglLoadIdentity();
    xglPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* load in the texture data */
    
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    tpath = current_options.get_fg_root() + "/Textures/gauges.rgb";
    if((img = ImageLoad( (char *)tpath.c_str() ))==NULL){
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
    xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, 
		  GL_UNSIGNED_BYTE, (GLvoid *)(img->data));
    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
}

void fgPanelReInit( int x, int y, int finx, int finy){
    fgVIEW *v;
    fgOPTIONS *o;
    int i;
    
    o = &current_options;
    v = &current_view;
    
    Xzoom = (float)((float)(current_view.winWidth)/1024);
    Yzoom = (float)((float)(current_view.winHeight)/768);
    
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    xglViewport(0, 0, 640, 480);
    gluOrtho2D(0, 640, 0, 480);
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
}

void fgPanelUpdate ( void ) {
    fgVIEW *v;
    float alpha;
    double pitch;
    double roll;
    float alpharad;
    double speed;
    int i;
    var[0] = get_speed();
    var[1] = get_altitude();
    var[2] = get_aoa(); // A placeholder. It should be the vertical speed once. 
    var[3] = get_throttleval();
    v = &current_view;
    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    glOrtho(0, 640, 0, 480, 1, -1);
    xglMatrixMode(GL_MODELVIEW);
    xglPushMatrix();
    xglLoadIdentity();
    xglDisable(GL_DEPTH_TEST);
    xglDisable(GL_LIGHTING);
    xglEnable(GL_TEXTURE_2D);

#ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id);
#elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id);
#else
#  error port me
#endif

    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
    xglPushMatrix();

    for(i=0;i<NumPoint;i++){
    xglLoadIdentity();
    xglTranslatef(pointer[i].XPos, pointer[i].YPos, 0.0);
    xglRotatef(-pointer[i].hist, 0.0, 0.0, 1.0);
    ErasePointer(pointer[i]);
    xglLoadIdentity();
    }

    for(i=0;i<NumPoint;i++){
    pointer[i].hist = UpdatePointer( pointer[i]);
    xglPopMatrix();
    xglPushMatrix();
    }

    xglDisable(GL_TEXTURE_2D);
    xglLoadIdentity();
    xglPopMatrix();   
   // xglRasterPos2i(0, 0);
  //  horizont();  
    xglEnable(GL_DEPTH_TEST);
    xglEnable(GL_LIGHTING);
    xglDisable(GL_TEXTURE_2D);
    xglDisable(GL_BLEND);
    xglDisable(GL_ALPHA_TEST);
    xglMatrixMode(GL_PROJECTION);
    xglPopMatrix();
    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
}

void horizont(void){ 
    double pitch;
    double roll;
    pitch = get_pitch() * RAD_TO_DEG;
    roll = get_roll() * RAD_TO_DEG;
    xglEnable(GL_TEXTURE_2D);
    xglMatrixMode(GL_MODELVIEW);
    xglLoadIdentity();
    xglTranslatef(200, 130, 0);
    xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);    
    #ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id);
    #elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id);
    #else
    #  error port me
    #endif
    xglMatrixMode(GL_TEXTURE);
    xglLoadIdentity();
    //xglTranslatef(0.0, 0.33, 0.0);
    xglScalef(0.5, 0.5, 0.0);
    xglTranslatef(0.0, (pitch / 45), 0.0);
    xglTranslatef(1.0, 1.0, 0.0);
    xglRotatef(-roll, 0.0, 0.0, 1.0);
    xglTranslatef(-0.5, -0.5, 0.0);
    xglBegin(GL_POLYGON);
    xglTexCoord2f(0.0, 0.1); xglVertex2f(0.0, 0.0);
    xglTexCoord2f(1.0, 0.1); xglVertex2f(70.0, 0.0);
    xglTexCoord2f(1.0, 0.9); xglVertex2f(70.0, 70.0);
    xglTexCoord2f(0.0, 0.9); xglVertex2f(0.0, 70.0);
    xglEnd();
    xglPopMatrix();
    xglMatrixMode(GL_PROJECTION);
    xglPopMatrix();
    xglDisable(GL_TEXTURE_2D);
    }

float UpdatePointer(Pointer pointer){
    double pitch;
    double roll;
    float alpharad;
    double speed;    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, pointer.vertices);
    alpha=((((float)((var[pointer.variable]) - (pointer.value1)))/(pointer.value2 - pointer.value1))*(pointer.alpha2 - pointer.alpha1) + pointer.alpha1);
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

void ErasePointer(Pointer pointer){
int i, j;
float a;
float ififth;

xglEnable(GL_TEXTURE_2D);
xglEnable(GL_TEXTURE_GEN_S);
xglEnable(GL_TEXTURE_GEN_T);
glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
//glTexGenfv(GL_S, GL_EYE_PLANE, currentCoeff);
glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
//glTexGenfv(GL_T, GL_EYE_PLANE, currentCoeff);
xglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
xglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
xglMatrixMode(GL_TEXTURE);
xglLoadIdentity();
    
    #ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id);
    #elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id);
    #else
    #  error port me
    #endif

xglMatrixMode(GL_TEXTURE);
xglLoadIdentity();
xglTranslatef(-((float)(((float)(pointer.XPos)/0.625)/256)) /* - 0.057143*/, -((float)(((float)(pointer.YPos)/0.625)/256)), 0.0);
xglTranslatef(pointer.teXpos/256 , pointer.texYpos/256, 0.0);
xglScalef(0.00625, 0.00625, 1.0);
//xglTranslatef(-pointer.teXpos , -pointer.texYpos, 0.0);
	
	xglBegin(GL_POLYGON);
xglVertex2f(pointer.vertices[0], pointer.vertices[1]);
xglVertex2f(pointer.vertices[2], pointer.vertices[3]);
xglVertex2f(pointer.vertices[4], pointer.vertices[5]);
xglVertex2f(pointer.vertices[16], pointer.vertices[17]);
xglVertex2f(pointer.vertices[18], pointer.vertices[19]); 
	xglEnd();
 
        xglLoadIdentity();
        xglMatrixMode(GL_MODELVIEW);
        xglPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        xglPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        xglDisable(GL_TEXTURE_2D);
	xglDisable(GL_TEXTURE_GEN_S);
	xglDisable(GL_TEXTURE_GEN_T);
        
}

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


void PrintMatrix( void){
glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
printf("matrix2 = %f %f %f %f \n", mvmatrix[0], mvmatrix[1], mvmatrix[2], mvmatrix[3]);
printf("         %f %f %f %f \n", mvmatrix[4], mvmatrix[5], mvmatrix[6], mvmatrix[7]);
printf("         %f %f %f %f \n", mvmatrix[8], mvmatrix[9], mvmatrix[10], mvmatrix[11]);
printf("         %f %f %f %f \n", mvmatrix[12], mvmatrix[13], mvmatrix[14], mvmatrix[15]);
}

/* $Log$
/* Revision 1.10  1998/11/09 23:38:52  curt
/* Panel updates from Friedemann.
/*
/* Revision 1.9  1998/11/06 21:18:01  curt
/* Converted to new logstream debugging facility.  This allows release
/* builds with no messages at all (and no performance impact) by using
/* the -DFG_NDEBUG flag.
/*
/* Revision 1.8  1998/10/16 23:27:37  curt
/* C++-ifying.
/*
 * Revision 1.7  1998/08/31 20:45:31  curt
 * Tweaks from Friedemann.
 *
 * Revision 1.6  1998/08/28 18:14:40  curt
 * Added new cockpit code from Friedemann Reinhard
 * <mpt218@faupt212.physik.uni-erlangen.de>
 *
 * Revision 1.1  1998/06/27 16:47:54  curt
 * Incorporated Friedemann Reinhard's <mpt218@faupt212.physik.uni-erlangen.de>
 * first pass at an isntrument panel.
 *
 */
