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

#ifndef GL_UTILS_HXX
#define GL_UTILS_HXX

#include <boost/utility.hpp>
#include <stack>

#if defined (SG_MAC)
#include <OpenGL/gl.h>
#elif defined (_GLES2)
#include <GLES2/gl2.h>
#else
#include <GL/glew.h> // Must be included before <GL/gl.h>
#include <GL/gl.h>
#endif

using namespace std;

class GL_utils : private boost::noncopyable {
public:
  static GL_utils& instance ();

  enum GLenum_Mode {
    GL_UTILS_MODELVIEW,
    GL_UTILS_PROJECTION,
    GL_UTILS_TEXTURE,
    GL_UTILS_COLOR,
    GL_UTILS_LAST,
    GL_UTILS_UNSET
  };

  GLuint load_program (const char *vert_shader_src, const char *frag_shader_src);

  void glMatrixMode (const GL_utils::GLenum_Mode mode);
  void glLoadIdentity ();
  void gluOrtho2D (const GLfloat left,
                   const GLfloat right,
                   const GLfloat bottom,
                   const GLfloat top);
  void glOrtho (const GLfloat left,
                const GLfloat right,
                const GLfloat bottom,
                const GLfloat top,
                const GLfloat nearVal,
                const GLfloat farVal);
  void glTranslatef (const GLfloat x, const GLfloat y, const GLfloat z);
  void glRotatef (const GLfloat angle, const GLfloat x, const GLfloat y, const GLfloat z);
  void glScalef (const GLfloat x, const GLfloat y, const GLfloat z);
  // C' = C X M
  void glMultMatrixf (const GLfloat m[4][4]);
  void glPushMatrix ();
  void glPopMatrix ();
  GLfloat* get_top_matrix (const GL_utils::GLenum_Mode mode);
  void Debug (const GL_utils::GLenum_Mode mode) const;

private:
  explicit GL_utils ();
  virtual ~GL_utils ();

  GLuint load_shader (GLenum type, const char *shader_src);

  typedef struct {
    GLfloat m[4][4];
  } Matrix;

  stack <Matrix> m_Matrix[GL_UTILS_LAST];
  GLenum_Mode m_Current_Matrix_Mode;
};

#endif
