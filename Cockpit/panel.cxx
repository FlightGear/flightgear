/**************************************************************************
 * panel.cxx -- routines to draw an instrument panel
 *
 * Written by Friedemann Reinhard, started June 1998.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Aircraft/aircraft.h>
#include <Debug/fg_debug.h>
#include <Main/options.hxx>

#include "panel.hxx"

  // Intriquing. Needs documentation.

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


IMAGE *img;

static GLuint panel_list;
static GLuint panel_tex_id;
static GLubyte tex[512][256][4];


extern double get_speed( void );


/* image.c ,temporary hack, I know*/
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


void fgPanelInit ( void ) {
    char tpath[256];
    int x, y;

#ifdef GL_VERSION_1_1
    xglGenTextures(1, &panel_tex_id);
    xglBindTexture(GL_TEXTURE_2D, panel_tex_id);
#elif GL_EXT_texture_object
    xglGenTexturesEXT(1, &panel_tex_id);
    xglBindTextureEXT(GL_TEXTURE_2D, panel_tex_id);
#else
#  error port me
#endif

    // xglPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 512);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /* load in the texture data */
    current_options.get_fg_root(tpath);
    strcat(tpath, "/Textures/");
    strcat(tpath, "panel1.rgb");

    if ( (img = ImageLoad(tpath)) == NULL ){
	fgPrintf( FG_COCKPIT, FG_EXIT, 
		  "Error loading cockpit texture %s\n", tpath );
    }

    for ( y = 0; y < 256; y++ ) {
	for ( x = 0; x < 512; x++ ) { 
	    tex[x][y][0]=img->data[(y+x*256)*3];
	    tex[x][y][1]=img->data[(y+x*256)*3+1];
	    tex[x][y][2]=img->data[(y+x*256)*3+2];
	    if ( (tex[x][y][0] == 0) && (tex[x][y][1] == 0) && 
		 (tex[x][y][2] == 0) ) {
		tex[x][y][3]=0;
	    } else {
		tex[x][y][3]=255;
	    }
	}
    }
    xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 256, 0, GL_RGBA, 
		  GL_UNSIGNED_BYTE, (GLvoid *)(tex));
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    printf("ALPHA=%d\n", tex[0][0][3]);
    printf("ALPHA=%d\n", tex[512][0][3]);
    printf("ALPHA=%d\n", tex[512][256][3]);
    printf("ALPHA=%d\n", tex[0][256][3]);

    panel_list = xglGenLists (1);
    xglNewList(panel_list, GL_COMPILE);
    xglBegin(GL_POLYGON);
    xglTexCoord2f(0.0,0.0); glVertex2f(0.0,0.0);
    xglTexCoord2f(1.0,0.0); glVertex2f(640.0,0.0);
    xglTexCoord2f(1.0,1.0); glVertex2f(640.0,330.0);
    // xglTexCoord2f(0.6,1.0); glVertex2f(384.0,330.0); 
    // xglTexCoord2f(0.166666,0.91111); glVertex2f(106.66666,303.182);
    // xglTexCoord2f(0.0, 0.75769231); glVertex2f(0.0, 279.61); 
    xglTexCoord2f(0.0,1.0); glVertex2f(0.0,330.0); 
    xglEnd();
    xglEndList ();
}


void fgPanelUpdate ( void ) {
    float alpha;
    double speed;

    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    // xglViewport(0, 0, 640, 480);
    gluOrtho2D(0, 640, 0, 480);
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
    xglEnable(GL_BLEND);
    xglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_BLEND);
    xglColor4f(1.0, 1.0, 1.0, 1.0);

    xglCallList(panel_list);

    xglPushMatrix();
    xglDisable(GL_TEXTURE_2D);
    speed = get_speed();
    alpha=((((float)(speed))/150)*270 + 20);
    xglTranslatef(130, 146, 0);
    xglRotatef(-alpha, 0.0, 0.0, 1.0);
    xglScalef(20, 23, 0.0);
    xglBegin(GL_POLYGON);
    xglColor4f(1.0, 1.0, 1.0, 1.0);
    xglVertex2f(0.0, 0.0);
    xglVertex2f(0.1, 0.2);
    xglVertex2f(0.0, 1.0);
    xglVertex2f(-0.1, 0.2);
    xglVertex2f(0.0, 0.0);
    xglEnd();  
    xglPopMatrix();   

    // xglFlush();

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


/* $Log$
/* Revision 1.3  1998/07/13 21:00:52  curt
/* Integrated Charlies latest HUD updates.
/* Wrote access functions for current fgOPTIONS.
/*
 * Revision 1.2  1998/07/03 11:55:37  curt
 * A few small rearrangements and tweaks.
 *
 * Revision 1.1  1998/06/27 16:47:54  curt
 * Incorporated Friedemann Reinhard's <mpt218@faupt212.physik.uni-erlangen.de>
 * first pass at an isntrument panel.
 *
 */
