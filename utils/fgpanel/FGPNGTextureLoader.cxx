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
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#include "FGPNGTextureLoader.hxx"

#if defined (SG_MAC)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <png.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
using namespace std;
GLuint FGPNGTextureLoader::loadTexture( const string & filename )
{
  //header for testing if it is a png
  png_byte header[8];
  
  //open file as binary
  FILE *fp = fopen(filename.c_str(), "rb");
  if (!fp) {
    return NOTEXTURE;
  }
  
  //read the header
  fread(header, 1, 8, fp);
  
  //test if png
  int is_png = !png_sig_cmp(header, 0, 8);
  if (!is_png) {
    fclose(fp);
    return NOTEXTURE;
  }
  
  //create png struct
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
      NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    return (NOTEXTURE);
  }
  
  //create png info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
    fclose(fp);
    return (NOTEXTURE);
  }

  //create png info struct
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
    fclose(fp);
    return (NOTEXTURE);
  }

  //png error stuff, not sure libpng man suggests this.
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return (NOTEXTURE);
  }

  //init png reading
  png_init_io(png_ptr, fp);
  
  //let libpng know you already read the first 8 bytes
  png_set_sig_bytes(png_ptr, 8);

  // read all the info up to the image data
  png_read_info(png_ptr, info_ptr);

  //variables to pass to get info
  int bit_depth, color_type;
  png_uint_32 twidth, theight;

  // get info about png
  png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type,
      NULL, NULL, NULL);

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // Allocate the image_data as a big block, to be given to opengl
  png_byte *image_data = new png_byte[rowbytes * theight];
  if (!image_data) {
    //clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return NOTEXTURE;
  }

  //row_pointers is for pointing to image_data for reading the png with libpng
  png_bytep *row_pointers = new png_bytep[theight];
  if (!row_pointers) {
    //clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    delete[] image_data;
    fclose(fp);
    return NOTEXTURE;
  }
  // set the individual row_pointers to point at the correct offsets of image_data
  for (png_uint_32 i = 0; i < theight; ++i)
    row_pointers[theight - 1 - i] = image_data + i * rowbytes;

  //read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers);

  //Now generate the OpenGL texture object
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  gluBuild2DMipmaps( GL_TEXTURE_2D, 4, twidth, theight, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image_data );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
//  glTexImage2D(GL_TEXTURE_2D,0, GL_RGBA, twidth, theight, 0,
//      GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) image_data);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  //clean up memory and close stuff
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  delete[] image_data;
  delete[] row_pointers;
  fclose(fp);
  return texture;
}
