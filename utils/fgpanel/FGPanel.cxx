//  FGPanel.cxx - default, 2D single-engine prop instrument panel
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
//  $Id: FGPanel.cxx,v 1.1 2016/07/20 22:01:30 allaert Exp $

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdio.h>      // sprintf
#include <string.h>

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

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

#include "GL_utils.hxx"
#include "FGPanel.hxx"
#include "FGTextLayer.hxx"
#include "FGTexturedLayer.hxx"

////////////////////////////////////////////////////////////////////////
// Local functions.
////////////////////////////////////////////////////////////////////////
GLuint FGPanel::Textured_Layer_Program_Object (0);
GLint FGPanel::Textured_Layer_Position_Loc (0);
GLint FGPanel::Textured_Layer_Tex_Coord_Loc (0);
GLint FGPanel::Textured_Layer_MVP_Loc (0);
GLint FGPanel::Textured_Layer_Sampler_Loc (0);

////////////////////////////////////////////////////////////////////////
// Implementation of FGPanel.
////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 */
FGPanel::FGPanel (const SGPropertyNode_ptr root) :
  m_flipx (root->getNode ("/sim/panel/flip-x", true)),
  m_bg_width (1.0), m_bg_height (1.0) {
}

/**
 * Destructor.
 */
FGPanel::~FGPanel () {
  for (instrument_list_type::iterator it = m_instruments.begin ();
       it != m_instruments.end ();
       ++it) {
    delete *it;
    *it = 0;
  }
}

/**
 * Initialize the panel.
 */
void
FGPanel::init () {
  // Textured Layer Shaders
  const char V_Textured_Layer_Shader_Str[] =
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

  const char F_Textured_Layer_Shader_Str[] =
#ifdef _RPI
    "#version 100                                            \n"
    "precision mediump float;                                \n"
    "varying vec2 v_tex_coord;                               \n"
#else
    "#version 130                                            \n"
    "in vec2 v_tex_coord;                                    \n"
#endif
    "uniform sampler2D u_texture;                            \n"
    "void main () {                                          \n"
    "  vec4 base_color = texture2D (u_texture, v_tex_coord); \n"
    "  if (base_color.a <= 0.1) {                            \n"
    "    discard;                                            \n"
    "  }                                                     \n"
    "  gl_FragColor = base_color;                            \n"
    "}                                                       \n";

  Textured_Layer_Program_Object = GL_utils::instance ().load_program (V_Textured_Layer_Shader_Str,
                                                                      F_Textured_Layer_Shader_Str);
  if (Textured_Layer_Program_Object == 0) {
    terminate ();
  }

  // Get the attribute locations
  Textured_Layer_Position_Loc  = glGetAttribLocation (Textured_Layer_Program_Object, "a_position");
  Textured_Layer_Tex_Coord_Loc = glGetAttribLocation (Textured_Layer_Program_Object, "a_tex_coord");

  // Get the uniform locations
  Textured_Layer_MVP_Loc = glGetUniformLocation (Textured_Layer_Program_Object, "u_mvp_matrix");

  // Get the sampler location
  Textured_Layer_Sampler_Loc = glGetUniformLocation (Textured_Layer_Program_Object, "u_texture");

  FGTexturedLayer::Init (Textured_Layer_Program_Object,
                         Textured_Layer_Position_Loc,
                         Textured_Layer_Tex_Coord_Loc,
                         Textured_Layer_MVP_Loc,
                         Textured_Layer_Sampler_Loc);

  // Text Layer Shaders
  if (!FGTextLayer::Init ()) {
    terminate ();
  }

  glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Bind panel properties.
 */
void
FGPanel::bind () {
}

/**
 * Unbind panel properties.
 */
void
FGPanel::unbind () {
}

void
FGPanel::update (double dt) {
  getInitDisplayList ();
  // Draw the instruments.
  // Syd Adams: added instrument clipping
  for (instrument_list_type::const_iterator current = m_instruments.begin ();
       current != m_instruments.end ();
       ++current) {
    FGPanelInstrument *instr = *current;
    GL_utils::instance ().glPushMatrix ();
    GL_utils::instance ().glTranslatef (instr->getXPos (), instr->getYPos (), 0);

    instr->draw();

    GL_utils::instance ().glPopMatrix ();
  }
#ifndef _GLES2
  // restore some original state
  //glPopAttrib ();
#endif
}

/**
 * Add an instrument to the panel.
 */
void
FGPanel::addInstrument (FGPanelInstrument * const instrument) {
  m_instruments.push_back (instrument);
}

/**
 * Set the panel's background texture.
 */
void
FGPanel::setBackground (const FGCroppedTexture_ptr texture) {
  m_bg = texture;
}

void
FGPanel::setBackgroundWidth (const double d) {
  m_bg_width = d;
}

void
FGPanel::setBackgroundHeight (const double d) {
  m_bg_height = d;
}

/**
 * Set the panel's multiple background textures.
 */
void
FGPanel::setMultiBackground (const FGCroppedTexture_ptr texture, const int idx) {
  m_bg = 0;
  m_mbg[idx] = texture;
}

void
FGPanel::setWidth (const int width) {
  m_width = width;
}

int
FGPanel::getWidth () const {
  return m_width;
}

void
FGPanel::setHeight (const int height) {
  m_height = height;
}

int
FGPanel::getHeight () const {
  return m_height;
}

void
FGPanel::getInitDisplayList () {
  glUseProgram (Textured_Layer_Program_Object);
  GL_utils::instance ().glMatrixMode (GL_utils::GL_UTILS_PROJECTION);
  GL_utils::instance ().glLoadIdentity ();
  if (m_flipx->getBoolValue ()) {
    GL_utils::instance ().gluOrtho2D (m_width, 0, m_height, 0);
  } else {
    GL_utils::instance ().gluOrtho2D (0, m_width, 0, m_height);
  }
  GL_utils::instance ().glTranslatef (-1.0f, -1.0f, 0.0f);
  GL_utils::instance ().glScalef (2.0f / GLfloat (m_width), 2.0f / GLfloat (m_height), 1.0f);

#ifndef _GLES2
  glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
#endif

  GL_utils::instance ().glMatrixMode (GL_utils::GL_UTILS_MODELVIEW);
  GL_utils::instance ().glLoadIdentity ();
  glClear (GL_COLOR_BUFFER_BIT);

  glUseProgram (Textured_Layer_Program_Object);
#ifndef _GLES2
  // save some state
  //glPushAttrib (GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT |
  //              GL_TEXTURE_BIT | GL_PIXEL_MODE_BIT | GL_CULL_FACE |
  //              GL_DEPTH_BUFFER_BIT );
#endif
  // Draw the background
  glEnable (GL_TEXTURE_2D);
#ifndef _GLES2
  glDisable (GL_LIGHTING);
#endif
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef _GLES2
  glEnable (GL_COLOR_MATERIAL);
#endif
  glEnable (GL_CULL_FACE);
  glCullFace (GL_BACK);
  glDisable (GL_DEPTH_TEST);

  if (m_bg != NULL) {
    m_bg->bind (Textured_Layer_Sampler_Loc);
    const GLfloat v_Vertices[] = {
      0.0f,               0.0f,                0.0f, // Position 0 (bottom left corner)
      0.0f,               0.0f,                      // TexCoord 0 (bottom left corner)
      float (m_width),    0.0f,                0.0f, // Position 1 (bottom right corner)
      float (m_bg_width), 0.0f,                      // TexCoord 1 (bottom right corner)
      float (m_width),    float (m_height),    0.0f, // Position 2 (top right corner)
      float (m_bg_width), float (m_bg_height),       // TexCoord 2 (top right corner)
      0.0f,               float (m_height),    0.0f, // Position 3 (top left corner)
      0.0f,               float (m_bg_height) };     // TexCoord 3 (top left corner)
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
  } else if (m_mbg[0] != NULL) {
    for (int i = 0; i < 4; ++i) {
      // top row of textures...(1,3,5,7)
      m_mbg[i*2]->bind (Textured_Layer_Sampler_Loc);
      const GLfloat v_Vertices_Top[] = {
        float (i * m_width / 4),       float (m_height / 2), 0.0f, // Position 0 (bottom left corner)
        0.0f,                          0.0f,                       // TexCoord 0 (bottom left corner)
        float ((i + 1) * m_width / 4), float (m_height / 2), 0.0f, // Position 1 (bottom right corner)
        1.0f,                          0.0f,                       // TexCoord 1 (bottom right corner)
        float ((i + 1) * m_width / 4), float (m_height),     0.0f, // Position 2 (top right corner)
        1.0f,                          1.0f,                       // TexCoord 2 (top right corner)
        float (i * m_width / 4),       float (m_height),     0.0f, // Position 3 (top left corner)
        0.0f,                          1.0f };                     // TexCoord 3 (top left corner)
      glVertexAttribPointer (Textured_Layer_Position_Loc,
                             3,
                             GL_FLOAT,
                             GL_FALSE,
                             5 * sizeof (GLfloat),
                             v_Vertices_Top);
      glVertexAttribPointer (Textured_Layer_Tex_Coord_Loc,
                             2,
                             GL_FLOAT,
                             GL_FALSE,
                             5 * sizeof (GLfloat),
                             &v_Vertices_Top[3]);

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

      // bottom row of textures...(2,4,6,8)
      m_mbg[i*2+1]->bind (Textured_Layer_Sampler_Loc);
      const GLfloat v_Vertices_Bot[] = {
        float (i * m_width / 4),       0.0f,                 0.0f, // Position 0 (bottom left corner)
        0.0f, 0.0f,                                                // TexCoord 0 (bottom left corner)
        float ((i + 1) * m_width / 4), 0.0f,                 0.0f, // Position 1 (bottom right corner)
        1.0f, 0.0f,                                                // TexCoord 1 (bottom right corner)
        float ((i + 1) * m_width / 4), float (m_height / 2), 0.0f, // Position 2 (top right corner)
        1.0f, 1.0f,                                                // TexCoord 2 (top right corner)
        float (i * m_width / 4),       float (m_height / 2), 0.0f, // Position 3 (top left corner)
        0.0f, 1.0f };                                              // TexCoord 3 (top left corner)
      glVertexAttribPointer (Textured_Layer_Position_Loc,
                             3,
                             GL_FLOAT,
                             GL_FALSE,
                             5 * sizeof (GLfloat),
                             v_Vertices_Bot);
      glVertexAttribPointer (Textured_Layer_Tex_Coord_Loc,
                             2,
                             GL_FLOAT,
                             GL_FALSE,
                             5 * sizeof (GLfloat),
                             &v_Vertices_Bot[3]);

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

      glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

      GL_utils::instance ().glPopMatrix ();
      GL_utils::instance ().glMatrixMode (GL_utils::GL_UTILS_MODELVIEW);
    }
  } else {
#ifndef _GLES2
    glUseProgram (0);
    float c[4];
    glGetFloatv (GL_CURRENT_COLOR, c);
    glColor4f (1.0, 0.0, 0.0, 1.0);
    glBegin (GL_QUADS);
    glVertex2f (0, 0);
    glVertex2f (m_width, 0);
    glVertex2f (m_width, m_height);
    glVertex2f (0, m_height);
    glEnd ();
    glColor4fv (c);
    glUseProgram (Textured_Layer_Program_Object);
#endif
  }
}


// Register the subsystem.
#if 0
SGSubsystemMgr::Registrant<FGPanel> registrantFGPanel;
#endif
