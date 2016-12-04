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

#include "GL_utils.hxx"
#include "FGTexturedLayer.hxx"

GLuint FGTexturedLayer::Textured_Layer_Program_Object (0);
GLint FGTexturedLayer::Textured_Layer_Position_Loc (0);
GLint FGTexturedLayer::Textured_Layer_Tex_Coord_Loc (0);
GLint FGTexturedLayer::Textured_Layer_MVP_Loc (0);
GLint FGTexturedLayer::Textured_Layer_Sampler_Loc (0);

void
FGTexturedLayer::Init (const GLuint Program_Object,
                       const GLint Position_Loc,
                       const GLint Tex_Coord_Loc,
                       const GLint MVP_Loc,
                       const GLint Sampler_Loc) {
  Textured_Layer_Program_Object = Program_Object;
  Textured_Layer_Position_Loc = Position_Loc;
  Textured_Layer_Tex_Coord_Loc = Tex_Coord_Loc;
  Textured_Layer_MVP_Loc = MVP_Loc;
  Textured_Layer_Sampler_Loc = Sampler_Loc;
}

FGTexturedLayer::FGTexturedLayer (const int w, const int h) :
  FGInstrumentLayer (w, h) {
}

FGTexturedLayer::FGTexturedLayer (const FGCroppedTexture_ptr texture, const int w, const int h) :
  FGInstrumentLayer (w, h),
  m_texture (texture),
  m_emissive (false) {
}

FGTexturedLayer::~FGTexturedLayer () {
}

void
FGTexturedLayer::draw () {
  if (test ()) {
    transform ();
    getDisplayList ();
  }
}

void
FGTexturedLayer::setTexture (const FGCroppedTexture_ptr texture) {
  m_texture = texture;
}

FGCroppedTexture_ptr
FGTexturedLayer::getTexture () const {
  return m_texture;
}

void
FGTexturedLayer::setEmissive (const bool emissive) {
  m_emissive = emissive;
}

void
FGTexturedLayer::getDisplayList () {
  int w2 = m_w / 2;
  int h2 = m_h / 2;

  glUseProgram (Textured_Layer_Program_Object);
  m_texture->bind (Textured_Layer_Sampler_Loc);
  const GLfloat v_Vertices[] = {
    float (-w2),           float (-h2),           0.0f, // Position 0 (bottom left corner)
    m_texture->getMinX (), m_texture->getMinY (),       // TexCoord 0 (bottom left corner)
    float ( w2),           float (-h2),           0.0f, // Position 1 (bottom right corner)
    m_texture->getMaxX (), m_texture->getMinY (),       // TexCoord 1 (bottom right corner)
    float ( w2),           float ( h2),           0.0f, // Position 2 (top right corner)
    m_texture->getMaxX (), m_texture->getMaxY (),       // TexCoord 2 (top right corner)
    float (-w2),           float ( h2),           0.0f, // Position 3 (top left corner)
    m_texture->getMinX (), m_texture->getMaxY () };     // TexCoord 3 (top left corner)
  glVertexAttribPointer (Textured_Layer_Position_Loc,
                         3,
                         GL_FLOAT,
                         GL_FALSE,
                         5 * sizeof (GLfloat),
                         v_Vertices);
  glVertexAttribPointer (Textured_Layer_Tex_Coord_Loc,
                         2,
                         GL_FLOAT,
                         GL_FALSE,
                         5 * sizeof (GLfloat),
                         &v_Vertices[3]);

  glEnableVertexAttribArray (Textured_Layer_Position_Loc);
  glEnableVertexAttribArray (Textured_Layer_Tex_Coord_Loc);

  GL_utils::instance ().glMatrixMode (GL_utils::GL_UTILS_PROJECTION);
  GL_utils::instance ().glPushMatrix ();
  GL_utils::instance ().glMultMatrixf
    (*reinterpret_cast <GLfloat (*)[4][4]>
     (GL_utils::instance ().get_top_matrix (GL_utils::GL_UTILS_MODELVIEW)));
  glUniformMatrix4fv (Textured_Layer_MVP_Loc,
                      1,
                      GL_FALSE,
                      GL_utils::instance ().get_top_matrix (GL_utils::GL_UTILS_PROJECTION));

  const GLushort indices[] = {0, 1, 2, 0, 2, 3};
  glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
  GL_utils::instance ().glPopMatrix ();
  GL_utils::instance ().glMatrixMode (GL_utils::GL_UTILS_MODELVIEW);
}
