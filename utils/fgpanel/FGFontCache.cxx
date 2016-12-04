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

#include <math.h>

#include "FGGLApplication.hxx"
#include "ApplicationProperties.hxx"
#include "FGFontCache.hxx"

////////////////////////////////////////////////////////////////////////
// FGFontCache class.
////////////////////////////////////////////////////////////////////////

const unsigned short FGFontCache::First_Printable_Char = 32;
const unsigned short FGFontCache::Last_Printable_Char = 127;

FGFontCache::FGFontCache () :
  m_Face_Map (),
  m_Current_Face_Ptr (NULL),
  m_Pos_Map (),
  m_Current_Pos (0) {
  if (FT_Init_FreeType (&m_Ft)) {
    SG_LOG (SG_COCKPIT, SG_ALERT, "Could not init freetype library");
  }
  for (unsigned int Index = 0; Index < Texture_Size * Texture_Size; ++Index) {
    m_Texture[Index] = char (0);
  }
  glGenTextures (1, &m_Glyph_Texture);
}

FGFontCache::~FGFontCache () {
  for (Face_Map_Type::iterator It = m_Face_Map.begin (); It != m_Face_Map.end (); ++It) {
    delete (It->second);
  }
  m_Face_Map.clear ();
  glDeleteTextures (1, &m_Glyph_Texture);
}

bool
FGFontCache::Set_Font (const string& Font_Name,
                       const float Size,
                       GLuint &Glyph_Texture) {
  if (m_Face_Map.find (Font_Name) != m_Face_Map.end ()) {
    m_Current_Face_Ptr = m_Face_Map[Font_Name];
  } else {
    FT_Face * const Face_Ptr (new FT_Face);
    if (FT_New_Face (m_Ft, Font_Name.c_str (), 0, Face_Ptr)) {
      SG_LOG (SG_COCKPIT, SG_ALERT, "Could not open font : " + Font_Name);
      return false;
    }
    m_Face_Map.insert (pair <string, FT_Face *> (Font_Name, Face_Ptr));
    m_Current_Face_Ptr = Face_Ptr;
  }
  if (m_Current_Face_Ptr != NULL) {
    FT_Set_Pixel_Sizes (*m_Current_Face_Ptr, 0, Size);
  } else {
    return false;
  }
  const string Key_Str (Font_Name + "_" + Get_Size (Size));
  if (m_Pos_Map.find (Key_Str) != m_Pos_Map.end ()) {
    m_Current_Pos = m_Pos_Map[Key_Str];
  } else {
    m_Current_Pos = m_Pos_Map.size ();
    for (unsigned short ASCII_Code = First_Printable_Char;
         ASCII_Code < Last_Printable_Char;
         ++ASCII_Code) {
      if (FT_Load_Char (*m_Current_Face_Ptr, char (ASCII_Code), FT_LOAD_RENDER)) {
        SG_LOG (SG_COCKPIT, SG_ALERT, "Could not load character : " << char (ASCII_Code));
      } else {
        unsigned int Line;
        unsigned int Column;
        const FT_GlyphSlot Glyph ((*m_Current_Face_Ptr)->glyph);
        for (unsigned int Row = 0; Row < Glyph->bitmap.rows; ++Row) {
          for (unsigned int Width = 0; Width < Glyph->bitmap.width; ++Width) {
            Get_Pos (ASCII_Code, Row, Width, Line, Column);
            m_Texture[Line * Texture_Size + Column] =
              Glyph->bitmap.buffer[Row * Glyph->bitmap.width + Width];
          }
        }
      }
    }
    glBindTexture (GL_TEXTURE_2D, m_Glyph_Texture);
    /* We require 1 byte alignment when uploading texture data */
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    /* Clamping to edges is important to prevent artifacts when scaling */
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    /* Linear filtering usually looks best for text */
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D,
                  0,
                  GL_ALPHA,
                  Texture_Size,
                  Texture_Size,
                  0,
                  GL_ALPHA,
                  GL_UNSIGNED_BYTE,
                  m_Texture);
    m_Pos_Map.insert (pair <string, unsigned int> (Key_Str, m_Current_Pos));
  }
  Glyph_Texture = m_Glyph_Texture;
  return true;
}

bool
FGFontCache::Get_Char (const char Char,
                       int &X,     int &Y,
                       int &Left,  int &Bottom,
                       int &W,     int &H,
                       double &X1, double &Y1,
                       double &X2, double &Y2) const {
  if (m_Current_Face_Ptr != NULL) {
    if (FT_Load_Char (*m_Current_Face_Ptr, Char, FT_LOAD_RENDER)) {
      SG_LOG (SG_COCKPIT, SG_ALERT, "Could not load character : " + Char);
      return false;
    } else {
      const FT_GlyphSlot Glyph ((*m_Current_Face_Ptr)->glyph);
      Left = Glyph->bitmap_left;
      W = Glyph->bitmap.width;
      H = Glyph->bitmap.rows;
      Bottom = Glyph->bitmap_top - H;
      X += (Glyph->advance.x >> 6);
      Y += (Glyph->advance.y >> 6);
      Get_Relative_Pos (int (Char), 0,                  0,                   X1, Y1);
      Get_Relative_Pos (int (Char), Glyph->bitmap.rows, Glyph->bitmap.width, X2, Y2);
      return true;
    }
  } else {
    return false;
  }
}

void
FGFontCache::Get_Pos (const unsigned short ASCII_Code,
                      const unsigned short Row,
                      const unsigned short Width,
                      unsigned int &Line,
                      unsigned int &Column) const {
  const unsigned short Font_Size (32);
  Line = (((((ASCII_Code - First_Printable_Char) * Font_Size) / Texture_Size) + 3 * m_Current_Pos) * Font_Size) + Row;
  Column = (((ASCII_Code - First_Printable_Char) * Font_Size) % Texture_Size) + Width;
}

void
FGFontCache::Get_Relative_Pos (const unsigned short ASCII_Code,
                               const unsigned short Row,
                               const unsigned short Width,
                               double &X,
                               double &Y) const {
  unsigned int Line;
  unsigned int Column;
  Get_Pos (ASCII_Code, Row, Width, Line, Column);
  X = double (Column) / double (Texture_Size);
  Y = double (Line) / double (Texture_Size);
}

string
FGFontCache::Get_Size (const float Size) {
  const int Half_Size (int (round (2.0 * Size)));
  const int Int_Part (Half_Size / 2);
  const int Dec_Part ((Half_Size % 2) ? 5 : 0);
  stringstream Result_SS;
  Result_SS << Int_Part << "." << Dec_Part;
  return Result_SS.str ();
}
