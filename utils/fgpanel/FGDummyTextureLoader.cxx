//
//  Written by David Megginson, started January 2000.
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

#include "FGDummyTextureLoader.hxx"

GLuint
FGDummyTextureLoader::loadTexture (const string& filename) {
  GLuint texture;
  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture);

  GLubyte image[ 2 * 2 * 3 ];

  /* Red and white chequerboard */
  image [ 0] = 255; image [ 1] =   0; image [ 2] =   0;
  image [ 3] = 255; image [ 4] = 255; image [ 5] = 255;
  image [ 6] = 255; image [ 7] = 255; image [ 8] = 255;
  image [ 9] = 255; image [10] =   0; image [11] =   0;

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0,
                GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*) image);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  return texture;
}
