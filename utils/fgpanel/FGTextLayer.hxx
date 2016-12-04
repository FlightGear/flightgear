//
//  Written by David Megginson, started January 2000.
//  Adopted for standalone fgpanel application by Torsten Dreyer, August 2009
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

#ifndef FGTEXTLAYER_HXX
#define FGTEXTLAYER_HXX

#include <string>
#include <simgear/timing/timestamp.hxx>

#include "FGFontCache.hxx"
#include "FGInstrumentLayer.hxx"

using namespace std;

/**
 * A text layer of an instrument.
 *
 * This is a layer holding a string of static and/or generated text.
 * It is useful for instruments that have text displays, such as
 * a chronometer, GPS, or NavCom radio.
 */

class FGTextLayer : public FGInstrumentLayer {
public:
  enum ChunkType {
    TEXT,
    TEXT_VALUE,
    DOUBLE_VALUE
  };

  class Chunk : public SGConditional {
  public:
    Chunk (const string &text,
           const string &fmt = "%s");
    Chunk (const ChunkType type,
           const SGPropertyNode *node,
           const string &fmt = "",
           const float mult = 1.0,
           const float offs = 0.0,
           const bool truncation = false);

    const char *getValue () const;
  private:
    ChunkType m_type;
    string m_text;
    SGConstPropertyNode_ptr m_node;
    string m_fmt;
    float m_mult;
    float m_offs;
    bool m_trunc;
    mutable char m_buf[1024];

  };

  static bool Init ();

  FGTextLayer (const int w = -1, const int h = -1);
  virtual ~FGTextLayer ();

  virtual void draw ();

  // Transfer pointer!!
  virtual void addChunk (Chunk * const chunk);
  virtual void setColor (const float r,
                         const float g,
                         const float b);
  virtual void setPointSize (const float size);
  virtual void setFontName (const string &name);

private:

  void recalc_value () const;

  typedef vector<Chunk *> chunk_list;
  chunk_list m_chunks;
  float m_color[4];

  float m_pointSize;
  static SGPath The_Font_Path;
  mutable string m_font_name;
  mutable string m_value;
  mutable SGTimeStamp m_then;
  mutable SGTimeStamp m_now;

  static FGFontCache The_Font_Cache;

  static GLuint Text_Layer_Program_Object;
  static GLint Text_Layer_Position_Loc;
  static GLint Text_Layer_Tex_Coord_Loc;
  static GLint Text_Layer_MVP_Loc;
  static GLint Text_Layer_Sampler_Loc;
  static GLint Text_Layer_Color_Loc;
};

#endif
