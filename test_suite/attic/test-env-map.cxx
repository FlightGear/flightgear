#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <simgear/compiler.h>
#if defined( __APPLE__)
# include <OpenGL/OpenGL.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# ifdef HAVE_GLUT_H
#  include <GL/glut.h>
# endif
#endif

#define TEXRES_X 256
#define TEXRES_Y 256

#ifdef HAVE_GLUT_H
unsigned char env_map[TEXRES_X][TEXRES_Y][4];
GLuint texName;
int window_x = 640, window_y = 480;
float alpha = 0.0, beta = 0.0;

  /*****************************************************************/
  /*****************************************************************/
  /*****************************************************************/
void setColor(float x, float y, float z, float angular_size, float r, float g, float b, float a)
{
  //normalize
  float inv_length = 1.0 / sqrt(x*x + y*y + z*z);
  x *= inv_length; y *= inv_length; z *= inv_length;
  printf("x = %f, y = %f, z = %f\n", x, y, z);

  float cos_angular_size = cos(angular_size*3.1415/180.0);
  printf("cos_angular_size = %f\n", cos_angular_size);

  for(float tz_sign = -1.0; tz_sign < 3.0; tz_sign += 2.0)
  {
    for(float tx = -1.0; tx <= 1.0; tx += 0.01)
    {
      for(float ty = -1.0; ty <= 1.0; ty += 0.01)
      {
        if ((1.0 - tx*tx - ty*ty)<0.0)
          continue;

        float tz = tz_sign * sqrt(1.0 - tx*tx - ty*ty);

        float cos_phi = x*tx + y*ty + z*tz;
       
        if (cos_angular_size < cos_phi)
        {
          float rx = tx;  //mirroring on the z=0 plane
          float ry = ty;
          float rz = -tz;

          float inv_m = 1.0 / (2.0 * sqrt(rx*rx + ry*ry + (rz + 1)*(rz + 1)));
          int s = (int)(TEXRES_X * (rx * inv_m + 0.5));
          int t = (int)(TEXRES_Y * (ry * inv_m + 0.5));

          //seg_fault protection:
          if (s<0) s=0; 
          if (s>=TEXRES_X) s=TEXRES_X-1; 

          if (t<0) t=0;
          if (t>=TEXRES_Y) t=TEXRES_Y-1;

          env_map[s][t][0] = (unsigned char)(r * 255);
          env_map[s][t][1] = (unsigned char)(g * 255);
          env_map[s][t][2] = (unsigned char)(b * 255);
          env_map[s][t][3] = (unsigned char)(a * 255);
        }
      }
    }
  }
}
  /*****************************************************************/
  /*****************************************************************/
  /*****************************************************************/

void init(void)
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glShadeModel(GL_FLAT);
  glEnable(GL_DEPTH_TEST);

  for(int x=0; x<TEXRES_X; x++)
  {
    for(int y=0; y<TEXRES_Y; y++)
    {
      env_map[x][y][0] = x;  //a few colors
      env_map[x][y][1] = y;
      env_map[x][y][2] = 128;
      env_map[x][y][3] = 128;
    }
  }

  /*****************************************************************/
  /*****************************************************************/
  /*****************************************************************/
  //       x          y    z           ang   r    g    b    a
  setColor(0.0,       0.0, -1.0      , 23.0, 1.0, 1.0, 1.0, 1.0);
  setColor(0.7071067, 0.0, -0.7071067, 23.0, 1.0, 0.0, 0.0, 1.0);
  /*****************************************************************/
  /*****************************************************************/
  /*****************************************************************/

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &texName);
  glBindTexture(GL_TEXTURE_2D, texName);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
}

void reshape(int w, int h)
{
  window_x = w; window_y = h;
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective((GLdouble) 60.0, ((GLdouble)w)/((GLdouble)h), (GLdouble) 10.0, (GLdouble) 1000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -20.0);
}

void display()
{
  glClear  ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

  //show light
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective((GLdouble) 60.0, (float)window_x/window_y, (GLdouble) 1.0, (GLdouble) 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glTranslatef(0.0, 0.0, -10.0);

  glRotatef(alpha, 1.0, 0.0, 0.0);
  glRotatef(beta, 0.0, 1.0, 0.0);

  glBegin(GL_LINES);
  glColor3f(1.0, 1.0, 1.0);
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(0.0, 0.0, 1.0);
  glEnd();

  /*****************************************************************/
  /*****************************************************************/
  /*****************************************************************/
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glBindTexture(GL_TEXTURE_2D, texName);

  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);

  glBegin(GL_QUADS);
  glNormal3f(0.0, 0.0, 1.0);
  glVertex3f(1.0, 1.0, 0.0);
  glVertex3f(-1.0, 1.0, 0.0);
  glVertex3f(-1.0, -1.0, 0.0);
  glVertex3f(1.0, -1.0, 0.0);
  glEnd();

  /*
  glPointSize(48.0);
  glBegin(GL_POINTS);
  glNormal3f(0.0, 0.0, 1.0);
  glVertex3f(0.0, 0.0, 0.0);
  glEnd();
  */

  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_2D);
  /*****************************************************************/
  /*****************************************************************/
  /*****************************************************************/

  //show how the map looks like
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //gluOrtho2D(0.0  -10.0, 1.0 +10.0, -(float)window_x/window_y -10.0,0.0 +10.0);
  //glOrtho(0.0, 5.0, -5.0, 0.0, -10.0, 10.0);
  glOrtho(0.0, 5.0*((float)window_x/window_y), -5.0, 0.0, -10.0, 10.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glBindTexture(GL_TEXTURE_2D, texName);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0); glVertex3f(0.0, 0.0, 0.0);
  glTexCoord2f(0.0, 1.0); glVertex3f(0.0, -1.0, 0.0);
  glTexCoord2f(1.0, 1.0); glVertex3f(1.0, -1.0, 0.0);
  glTexCoord2f(1.0, 0.0); glVertex3f(1.0, 0.0, 0.0);
  glEnd();
  glFlush();
  glDisable(GL_TEXTURE_2D);

  glutPostRedisplay () ;
  glutSwapBuffers () ;
}

void keyboard (unsigned char key, int x, int y)
{
  switch(key)
  {
  case 27:
    exit(0);

  case '8':
    alpha += 1.0;
    if (alpha >= 360.0) alpha -= 360.0;
    break;

  case '2':
    alpha -= 1.0;
    if (alpha < 0.0) alpha += 360.0;
    break;

  case '4':
    beta -= 1.0;
    if (beta <= -90.0) beta = -90.0;
    break;

  case '6':
    beta += 1.0;
    if (beta >= 90.0) beta = 90.0;
    break;

  case '5':
    alpha = 0.0; beta = 0.0;
    break;
  }
}
#endif /* HAVE_GLUT_H */

int main(int argc, char** argv)
{
#ifdef HAVE_GLUT_H
  glutInitWindowSize(window_x, window_y);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow(argv[0]);
  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  glutMainLoop();
#else

  printf("GL Utility Toolkit (glut) was not found on this system.\n");
#endif

  return 0;
}
