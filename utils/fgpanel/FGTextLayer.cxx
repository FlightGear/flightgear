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

#include "ApplicationProperties.hxx"
#include "GL_utils.hxx"
#include "FGTextLayer.hxx"

SGPath FGTextLayer::The_Font_Path;

FGFontCache FGTextLayer::The_Font_Cache;

GLuint FGTextLayer::Text_Layer_Program_Object (0);
GLint FGTextLayer::Text_Layer_Position_Loc (0);
GLint FGTextLayer::Text_Layer_Tex_Coord_Loc (0);
GLint FGTextLayer::Text_Layer_MVP_Loc (0);
GLint FGTextLayer::Text_Layer_Sampler_Loc (0);
GLint FGTextLayer::Text_Layer_Color_Loc (0);

bool
FGTextLayer::Init () {
  const char V_Text_Layer_Shader_Str[] =
#ifdef _RPI
    "#version 100                               \n"
    "attribute vec4 a_position;                 \n"
    "attribute vec2 a_tex_coord;                \n"
    "varying vec2 v_tex_coord;                  \n"
#else
    "#version 130                               \n"
    "in vec4 a_position;                        \n"
    "in vec2 a_tex_coord;                       \n"
    "out vec2 v_tex_coord;                      \n"
#endif
    "uniform mat4 u_mvp_matrix;                 \n"
    "void main () {                             \n"
    "  gl_Position = u_mvp_matrix * a_position; \n"
    "  v_tex_coord = a_tex_coord;               \n"
    "}                                          \n";

  const char F_Text_Layer_Shader_Str[] =
#ifdef _RPI
    "#version 100                                                   \n"
    "precision mediump float;                                       \n"
    "varying vec2 v_tex_coord;                                      \n"
#else
    "#version 130                                                   \n"
    "in vec2 v_tex_coord;                                           \n"
#endif
    "uniform sampler2D u_texture;                                   \n"
    "uniform vec4 u_color;                                          \n"
    "void main () {                                                 \n"
    "  gl_FragColor = vec4 (1, 1, 1,                                \n"
    "                       texture2D (u_texture, v_tex_coord).a) * \n"
    "    u_color;                                                   \n"
    "}                                                              \n";

  Text_Layer_Program_Object = GL_utils::instance ().load_program (V_Text_Layer_Shader_Str,
                                                                  F_Text_Layer_Shader_Str);
  if (Text_Layer_Program_Object == 0) {
    return false;
  }

  // Get the attribute locations
  Text_Layer_Position_Loc  = glGetAttribLocation (Text_Layer_Program_Object, "a_position");
  Text_Layer_Tex_Coord_Loc = glGetAttribLocation (Text_Layer_Program_Object, "a_tex_coord");

  // Get the uniform locations
  Text_Layer_MVP_Loc = glGetUniformLocation (Text_Layer_Program_Object, "u_mvp_matrix");

  // Get the sampler location
  Text_Layer_Sampler_Loc = glGetUniformLocation (Text_Layer_Program_Object, "u_texture");

  // Get the color location
  Text_Layer_Color_Loc = glGetUniformLocation (Text_Layer_Program_Object, "u_color");

  return true;
}

FGTextLayer::FGTextLayer (const int w, const int h) :
  FGInstrumentLayer (w, h),
  m_pointSize (0.0),
  m_font_name () {
  m_then.stamp ();
  m_color[0] = m_color[1] = m_color[2] = 0.0;
  m_color[3] = 1.0;
}

FGTextLayer::~FGTextLayer () {
  for (chunk_list::iterator It = m_chunks.begin ();
       It != m_chunks.end ();
       ++It) {
    delete *It;
  }
}

void
FGTextLayer::draw () {
  if (test ()) {
    glUseProgram (Text_Layer_Program_Object);
    glUniform4fv (Text_Layer_Color_Loc, 1, m_color);

    transform ();

    GL_utils::instance ().glMatrixMode (GL_utils::GL_UTILS_PROJECTION);
    GL_utils::instance ().glPushMatrix ();
    GL_utils::instance ().glMultMatrixf
      (*reinterpret_cast <GLfloat (*)[4][4]>
       (GL_utils::instance ().get_top_matrix (GL_utils::GL_UTILS_MODELVIEW)));
    glUniformMatrix4fv (Text_Layer_MVP_Loc,
                        1,
                        GL_FALSE,
                        GL_utils::instance ().get_top_matrix (GL_utils::GL_UTILS_PROJECTION));
    GLuint Glyph_Texture;
    if (!The_Font_Cache.Set_Font (m_font_name, m_pointSize, Glyph_Texture)) {
      SG_LOG (SG_COCKPIT, SG_ALERT, "Missing font : " << m_font_name << " " << m_pointSize);
    }

    m_now.stamp ();
    const long diff ((m_now - m_then).toUSecs ());

    if (diff > 100000 || diff < 0 ) {
      // ( diff < 0 ) is a sanity check and indicates our time stamp
      // difference math probably overflowed.  We can handle a max
      // difference of 35.8 minutes since the returned value is in
      // usec.  So if the panel is left off longer than that we can
      // over flow the math with it is turned back on.  This (diff <
      // 0) catches that situation, get's us out of trouble, and
      // back on track.
      recalc_value ();
      m_then = m_now;
    }

    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, Glyph_Texture);
    glUniform1i (Text_Layer_Sampler_Loc, 0);

    int X (0);
    int Y (0);
    int Previous_X (0);
    int Previous_Y (0);
    int Left, Bottom, W, H;
    double X1, Y1, X2, Y2;

    for (string::iterator It = m_value.begin (); It != m_value.end (); ++It) {
      if (The_Font_Cache.Get_Char (*It,
                                   X, Y,
                                   Left, Bottom,
                                   W, H,
                                   X1, Y1,
                                   X2, Y2)) {
        const GLfloat v_Vertices[] = {
          float (Previous_X + Left),     float (Previous_Y + Bottom),     0.0f, // Position 0 (bottom left corner)
          float (X1),                    float (Y2),                            // TexCoord 0 (bottom left corner)
          float (Previous_X + Left + W), float (Previous_Y + Bottom),     0.0f, // Position 1 (bottom right corner)
          float (X2),                    float (Y2),                            // TexCoord 1 (bottom right corner)
          float (Previous_X + Left + W), float (Previous_Y + Bottom + H), 0.0f, // Position 2 (top right corner)
          float (X2),                    float (Y1),                            // TexCoord 2 (top right corner)
          float (Previous_X + Left),     float (Previous_Y + Bottom + H), 0.0f, // Position 3 (top left corner)
          float (X1),                    float (Y1) };                          // TexCoord 3 (top left corner)
        glVertexAttribPointer (Text_Layer_Position_Loc,
                               3,
                               GL_FLOAT,
                               GL_FALSE,
                               5 * sizeof (GLfloat),
                               v_Vertices);
        glVertexAttribPointer (Text_Layer_Tex_Coord_Loc,
                               2,
                               GL_FLOAT,
                               GL_FALSE,
                               5 * sizeof (GLfloat),
                               &v_Vertices[3]);

        glEnableVertexAttribArray (Text_Layer_Position_Loc);
        glEnableVertexAttribArray (Text_Layer_Tex_Coord_Loc);

        const GLushort indices[] = {0, 1, 2, 0, 2, 3};
        glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
        Previous_X = X;
        Previous_Y = Y;
      }
    }

    GL_utils::instance ().glPopMatrix ();
    GL_utils::instance ().glMatrixMode (GL_utils::GL_UTILS_MODELVIEW);
  }
}

void
FGTextLayer::addChunk (FGTextLayer::Chunk * const chunk) {
  m_chunks.push_back (chunk);
}

void
FGTextLayer::setColor (const float r,
                       const float g,
                       const float b) {
  m_color[0] = r;
  m_color[1] = g;
  m_color[2] = b;
  m_color[3] = 1.0;
}

void
FGTextLayer::setPointSize (const float size) {
  m_pointSize = size;
}

void
FGTextLayer::setFontName (const string &name) {
  if (The_Font_Path.isNull ()) {
    char *Env_Path = ::getenv ("FG_FONTS");
    if (Env_Path != NULL) {
      The_Font_Path = SGPath::fromEnv (Env_Path);
    } else {
      The_Font_Path = ApplicationProperties::GetRootPath ("Fonts");
    }
  }
  m_font_name = The_Font_Path.local8BitStr () + "/" + name + ".ttf";
}

void
FGTextLayer::recalc_value () const {
  m_value = "";
  for (chunk_list::const_iterator It = m_chunks.begin ();
       It != m_chunks.end ();
       ++It) {
    m_value += (*It)->getValue ();
  }
}

////////////////////////////////////////////////////////////////////////
// Implementation of FGTextLayer::Chunk.
////////////////////////////////////////////////////////////////////////

FGTextLayer::Chunk::Chunk (const string &text,
                           const string &fmt) :
  m_type (FGTextLayer::TEXT),
  m_text (text),
  m_fmt (fmt),
  m_mult (1.0),
  m_offs (0.0),
  m_trunc (false) {
  if (m_fmt.empty ()) {
    m_fmt = "%s";
  }
}

FGTextLayer::Chunk::Chunk (const ChunkType type,
                           const SGPropertyNode *node,
                           const string &fmt,
                           const float mult,
                           const float offs,
                           const bool truncation) :
  m_type (type),
  m_node (node),
  m_fmt (fmt),
  m_mult (mult),
  m_offs (offs),
  m_trunc (truncation) {
  if (m_fmt.empty ()) {
    if (type == TEXT_VALUE) {
      m_fmt = "%s";
    } else {
      m_fmt = "%.2f";
    }
  }
}

const char *
FGTextLayer::Chunk::getValue () const {
  if (test ()) {
    m_buf[0] = '\0';
    switch (m_type) {
    case TEXT:
      sprintf (m_buf, m_fmt.c_str (), m_text.c_str ());
      break;
    case TEXT_VALUE:
      sprintf (m_buf, m_fmt.c_str (), m_node->getStringValue ());
      break;
    case DOUBLE_VALUE:
      double d (m_offs + m_node->getFloatValue() * m_mult);
      if (m_trunc)  {
        d = (d < 0) ? -floor (-d) : floor (d);
      }
      sprintf (m_buf, m_fmt.c_str(), d);
      break;
    }
    return m_buf;
  } else {
    return "";
  }
}
