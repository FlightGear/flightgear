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
#ifndef __FGFONTCACHE_HXX
#define __FGFONTCACHE_HXX

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

#if defined (SG_MAC)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#elif defined (_GLES2)
#include <GLES2/gl2.h>
#else
#include <GL/glew.h> // Must be included before <GL/gl.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

using namespace std;

/**
 * A class to keep all fonts available for future use.
 * This also assures a font isn't resident more than once.
 */
class FGFontCache {
public:
  FGFontCache ();
  ~FGFontCache ();
  bool Set_Font (const string& Font_Name,
                 const float Size,
                 GLuint &Glyph_Texture);
  bool Get_Char (const char Char,
                 int &X,     int &Y,
                 int &Left,  int &Bottom,
                 int &W,     int &H,
                 double &X1, double &Y1, // Top (Y1) left (X1)
                 double &X2, double &Y2) const; // Bottom (Y2) right (X2)
private:
  void Get_Pos (const unsigned short ASCII_Code,
                const unsigned short Row,
                const unsigned short Width,
                unsigned int &Line,
                unsigned int &Column) const;
  void Get_Relative_Pos (const unsigned short ASCII_Code,
                         const unsigned short Row,
                         const unsigned short Width,
                         double &X,
                         double &Y) const;
  static string Get_Size (const float Size);
  static const unsigned short First_Printable_Char;
  static const unsigned short Last_Printable_Char;
  static const unsigned int Texture_Size = 1024;
  FT_Library m_Ft;
  typedef map <string, FT_Face *> Face_Map_Type;
  Face_Map_Type m_Face_Map;
  FT_Face *m_Current_Face_Ptr;
  char m_Texture[Texture_Size * Texture_Size];
  typedef map <string, unsigned int> Pos_Map_Type;
  Pos_Map_Type m_Pos_Map;
  unsigned int m_Current_Pos;
  GLuint m_Glyph_Texture;
};

#endif
