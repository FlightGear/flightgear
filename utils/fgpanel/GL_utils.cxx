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

#include <iostream>
#include <math.h>
#include <string.h>

#include "GL_utils.hxx"

const float PI (3.1415926535897932384626433832795);

GL_utils::GL_utils () {
  Matrix Tmp;
  memset (&Tmp, 0x0, sizeof (Matrix));
  int i;
  for (i = GL_UTILS_MODELVIEW; i < GL_UTILS_LAST; ++i) {
    m_Current_Matrix_Mode = (GLenum_Mode) i;
    m_Matrix[m_Current_Matrix_Mode].push (Tmp);
    glLoadIdentity ();
  }
  m_Current_Matrix_Mode = GL_utils::GL_UTILS_UNSET;
}

GL_utils::~GL_utils () {}

GL_utils&
GL_utils::instance () {
  static GL_utils* const The_Instance (new GL_utils);
  return *The_Instance;
}

//
///
/// \brief Load a shader, check for compile errors, print error messages to std error
/// \param type Type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
/// \param shader_src Shader source string
/// \return A new shader object on success, 0 on failure
//
GLuint
GL_utils::load_shader (GLenum type, const char *shader_src) {
  GLint compiled;

  // Create the shader object
  GLuint shader (glCreateShader (type));

  if (shader == 0) {
    cerr << "Error creating shader" << endl;
    return 0;
  }

  // Load the shader source
  glShaderSource (shader, 1, &shader_src, NULL);

  // Compile the shader
  glCompileShader (shader);

  // Check the compile status
  glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);

  if (!compiled) {
    GLint info_len (0);

    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 1) {
      char* info_log ((char *) malloc (sizeof (char) * info_len));

      glGetShaderInfoLog (shader, info_len, NULL, info_log);
      cerr << "Error compiling shader:" << endl << info_log << endl;

      free (info_log);
    }

    glDeleteShader (shader);
    return 0;
  }

  return shader;
}

//
///
/// \brief Load a vertex and fragment shader, create a program object, link program.
//         Errors output to std error.
/// \param vertShaderSrc Vertex shader source code
/// \param fragShaderSrc Fragment shader source code
/// \return A new program object linked with the vertex/fragment shader pair, 0 on failure
//
GLuint
GL_utils::load_program (const char *vert_shader_src, const char *frag_shader_src) {
  GLint linked;

  // Load the vertex/fragment shaders
  GLuint vertex_shader (load_shader (GL_VERTEX_SHADER, vert_shader_src));
  if (vertex_shader == 0) {
    cerr << "Error loading vertex shader" << endl;
    return 0;
  }

  GLuint fragment_shader (load_shader (GL_FRAGMENT_SHADER, frag_shader_src));
  if (fragment_shader == 0) {
    cerr << "Error loading fragment shader" << endl;
    glDeleteShader (vertex_shader);
    return 0;
  }

  // Create the program object
  GLuint program_object (glCreateProgram ());

  if (program_object == 0) {
    cerr << "Error creating program" << endl;
    return 0;
  }

  glAttachShader (program_object, vertex_shader);
  glAttachShader (program_object, fragment_shader);

  // Link the program
  glLinkProgram (program_object);

  // Check the link status
  glGetProgramiv (program_object, GL_LINK_STATUS, &linked);

  if (!linked) {
    GLint info_len (0);

    glGetProgramiv (program_object, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 1) {
      char* info_log ((char *) malloc (sizeof (char) * info_len));

      glGetProgramInfoLog (program_object, info_len, NULL, info_log);
      cerr << "Error linking program:" << endl << info_log << endl;

      free (info_log);
    }

    glDeleteProgram (program_object);
    return 0;
  }

  // Free up no longer needed shader resources
  glDeleteShader (vertex_shader);
  glDeleteShader (fragment_shader);

  return program_object;
}

void
GL_utils::glMatrixMode (const GLenum_Mode mode) {
  if (mode < GL_UTILS_LAST) {
    m_Current_Matrix_Mode = mode;
  }
}

void
GL_utils::glLoadIdentity () {
  if (m_Current_Matrix_Mode < GL_UTILS_LAST) {
    memset (&(m_Matrix[m_Current_Matrix_Mode].top ()), 0x0, sizeof (Matrix));
    m_Matrix[m_Current_Matrix_Mode].top ().m[0][0] = 1.0f;
    m_Matrix[m_Current_Matrix_Mode].top ().m[1][1] = 1.0f;
    m_Matrix[m_Current_Matrix_Mode].top ().m[2][2] = 1.0f;
    m_Matrix[m_Current_Matrix_Mode].top ().m[3][3] = 1.0f;
  }
}

void
GL_utils::gluOrtho2D (const GLfloat left,
                      const GLfloat right,
                      const GLfloat bottom,
                      const GLfloat top) {
  GL_utils::glOrtho (left, right, bottom, top, -1.0f, 1.0f);
}

void
GL_utils::glOrtho (const GLfloat left,
                   const GLfloat right,
                   const GLfloat bottom,
                   const GLfloat top,
                   const GLfloat nearVal,
                   const GLfloat farVal) {
  const GLfloat Delta_X (right - left);
  const GLfloat Delta_Y (top - bottom);
  const GLfloat Delta_Z (farVal - nearVal);
  Matrix Ortho;

  if ((Delta_X == 0.0f) || (Delta_Y == 0.0f) || (Delta_Z == 0.0f) || (m_Current_Matrix_Mode < GL_UTILS_LAST)) {
    return;
  }

  memset (&Ortho, 0x0, sizeof (Matrix));
  Ortho.m[0][0] = 2.0f / Delta_X;
  Ortho.m[3][0] = -(right + left) / Delta_X;
  Ortho.m[1][1] = 2.0f / Delta_Y;
  Ortho.m[3][1] = -(top + bottom) / Delta_Y;
  Ortho.m[2][2] = -2.0f / Delta_Z;
  Ortho.m[3][2] = -(nearVal + farVal) / Delta_Z;
  Ortho.m[3][3] = 1.0f;

  GL_utils::glMultMatrixf (Ortho.m);
}

void
GL_utils::glTranslatef (const GLfloat x, const GLfloat y, const GLfloat z) {
  if (m_Current_Matrix_Mode < GL_UTILS_LAST) {
    m_Matrix[m_Current_Matrix_Mode].top ().m[3][0] += (m_Matrix[m_Current_Matrix_Mode].top ().m[0][0] * x +
                                                       m_Matrix[m_Current_Matrix_Mode].top ().m[1][0] * y +
                                                       m_Matrix[m_Current_Matrix_Mode].top ().m[2][0] * z);

    m_Matrix[m_Current_Matrix_Mode].top ().m[3][1] += (m_Matrix[m_Current_Matrix_Mode].top ().m[0][1] * x +
                                                       m_Matrix[m_Current_Matrix_Mode].top ().m[1][1] * y +
                                                       m_Matrix[m_Current_Matrix_Mode].top ().m[2][1] * z);

    m_Matrix[m_Current_Matrix_Mode].top ().m[3][2] += (m_Matrix[m_Current_Matrix_Mode].top ().m[0][2] * x +
                                                       m_Matrix[m_Current_Matrix_Mode].top ().m[1][2] * y +
                                                       m_Matrix[m_Current_Matrix_Mode].top ().m[2][2] * z);

    m_Matrix[m_Current_Matrix_Mode].top ().m[3][3] += (m_Matrix[m_Current_Matrix_Mode].top ().m[0][3] * x +
                                                       m_Matrix[m_Current_Matrix_Mode].top ().m[1][3] * y +
                                                       m_Matrix[m_Current_Matrix_Mode].top ().m[2][3] * z);
  }
}

void
GL_utils::glRotatef (const GLfloat angle, const GLfloat x, const GLfloat y, const GLfloat z) {
  const GLfloat xx (x * x);
  const GLfloat yy (y * y);
  const GLfloat zz (z * z);
  const GLfloat Mag (sqrtf (xx + yy + zz));

  if ((Mag > 0.0f) && (m_Current_Matrix_Mode < GL_UTILS_LAST)) {
    const GLfloat Sin_Angle (sinf (-angle * PI / 180.0f));
    const GLfloat Cos_Angle (cosf (angle * PI / 180.0f));
    const GLfloat xy (x * y);
    const GLfloat yz (y * z);
    const GLfloat zx (z * x);
    const GLfloat xs (x * Sin_Angle);
    const GLfloat ys (y * Sin_Angle);
    const GLfloat zs (z * Sin_Angle);
    const GLfloat One_Minus_Cos (1.0f - Cos_Angle);
    Matrix Rot;

    // x /= mag;
    // y /= mag;
    // z /= mag;

    Rot.m[0][0] = (One_Minus_Cos * xx) + Cos_Angle;
    Rot.m[0][1] = (One_Minus_Cos * xy) - zs;
    Rot.m[0][2] = (One_Minus_Cos * zx) + ys;
    Rot.m[0][3] = 0.0f;

    Rot.m[1][0] = (One_Minus_Cos * xy) + zs;
    Rot.m[1][1] = (One_Minus_Cos * yy) + Cos_Angle;
    Rot.m[1][2] = (One_Minus_Cos * yz) - xs;
    Rot.m[1][3] = 0.0f;

    Rot.m[2][0] = (One_Minus_Cos * zx) - ys;
    Rot.m[2][1] = (One_Minus_Cos * yz) + xs;
    Rot.m[2][2] = (One_Minus_Cos * zz) + Cos_Angle;
    Rot.m[2][3] = 0.0f;

    Rot.m[3][0] = 0.0f;
    Rot.m[3][1] = 0.0f;
    Rot.m[3][2] = 0.0f;
    Rot.m[3][3] = 1.0f;

    GL_utils::glMultMatrixf (Rot.m);
  }
}

void
GL_utils::glScalef (GLfloat x, GLfloat y, GLfloat z) {
  if (m_Current_Matrix_Mode < GL_UTILS_LAST) {
    Matrix Scale;
    Scale.m[0][0] = x;
    Scale.m[0][1] = 0.0f;
    Scale.m[0][2] = 0.0f;
    Scale.m[0][3] = 0.0f;

    Scale.m[1][0] = 0.0f;
    Scale.m[1][1] = y;
    Scale.m[1][2] = 0.0f;
    Scale.m[1][3] = 0.0f;

    Scale.m[2][0] = 0.0f;
    Scale.m[2][1] = 0.0f;
    Scale.m[2][2] = z;
    Scale.m[2][3] = 0.0f;

    Scale.m[3][0] = 0.0f;
    Scale.m[3][1] = 0.0f;
    Scale.m[3][2] = 0.0f;
    Scale.m[3][3] = 1.0f;

    GL_utils::glMultMatrixf (Scale.m);
  }
}

void
GL_utils::glMultMatrixf (const GLfloat m[4][4]) {
  Matrix Tmp;

  for (int i = 0; i < 4; ++i) {
    Tmp.m[i][0] = (m[i][0] * m_Matrix[m_Current_Matrix_Mode].top ().m[0][0] +
                   m[i][1] * m_Matrix[m_Current_Matrix_Mode].top ().m[1][0] +
                   m[i][2] * m_Matrix[m_Current_Matrix_Mode].top ().m[2][0] +
                   m[i][3] * m_Matrix[m_Current_Matrix_Mode].top ().m[3][0]);

    Tmp.m[i][1] = (m[i][0] * m_Matrix[m_Current_Matrix_Mode].top ().m[0][1] +
                   m[i][1] * m_Matrix[m_Current_Matrix_Mode].top ().m[1][1] +
                   m[i][2] * m_Matrix[m_Current_Matrix_Mode].top ().m[2][1] +
                   m[i][3] * m_Matrix[m_Current_Matrix_Mode].top ().m[3][1]);

    Tmp.m[i][2] = (m[i][0] * m_Matrix[m_Current_Matrix_Mode].top ().m[0][2] +
                   m[i][1] * m_Matrix[m_Current_Matrix_Mode].top ().m[1][2] +
                   m[i][2] * m_Matrix[m_Current_Matrix_Mode].top ().m[2][2] +
                   m[i][3] * m_Matrix[m_Current_Matrix_Mode].top ().m[3][2]);

    Tmp.m[i][3] = (m[i][0] * m_Matrix[m_Current_Matrix_Mode].top ().m[0][3] +
                   m[i][1] * m_Matrix[m_Current_Matrix_Mode].top ().m[1][3] +
                   m[i][2] * m_Matrix[m_Current_Matrix_Mode].top ().m[2][3] +
                   m[i][3] * m_Matrix[m_Current_Matrix_Mode].top ().m[3][3]);
  }
  memcpy (&(m_Matrix[m_Current_Matrix_Mode].top ()), &Tmp, sizeof (Matrix));
}

void
GL_utils::glPushMatrix () {
  if (m_Current_Matrix_Mode < GL_UTILS_LAST) {
    m_Matrix[m_Current_Matrix_Mode].push (m_Matrix[m_Current_Matrix_Mode].top ());
  }
}

void
GL_utils::glPopMatrix () {
  if (m_Current_Matrix_Mode < GL_UTILS_LAST) {
    m_Matrix[m_Current_Matrix_Mode].pop ();
  }
}

GLfloat*
GL_utils::get_top_matrix (const GL_utils::GLenum_Mode mode) {
  if (mode < GL_UTILS_LAST) {
    return &(m_Matrix[mode].top ().m[0][0]);
  } else {
    return NULL;
  }
}

void
GL_utils::Debug (const GL_utils::GLenum_Mode mode) const {
  if (mode < GL_UTILS_LAST) {
    for (int l = 0; l < 4; ++l) {
      cout << "        ";
      for (int c = 0; c < 4; ++c) {
        cout << m_Matrix[mode].top ().m[c][l] << "   ";
      }
      cout << endl;
    }
  }
}
