From curt@infoplane.com  Fri Nov 13 06:10:29 1998
X-VM-v5-Data: ([nil nil nil nil nil nil nil nil nil]
	["6615" "Fri" "13" "November" "1998" "06:10:23" "-0600" "curt@infoplane.com" "curt@infoplane.com" nil "271" "fuzzy-light-effect" "^From:" nil nil "11" nil nil nil nil nil]
	nil)
Received: from dorthy.state.net (dorthy.state.net [209.234.62.254])
	by meserv.me.umn.edu (8.9.1a/8.9.1) with ESMTP id GAA15847
	for <curt@me.umn.edu>; Fri, 13 Nov 1998 06:10:28 -0600 (CST)
Received: from sledge.infoplane.com (curt@sledge.infoplane.com [204.120.151.21]) by dorthy.state.net (8.8.8/8.7.2) with ESMTP id GAA25998 for <curt@me.umn.edu>; Fri, 13 Nov 1998 06:10:08 -0600 (CST)
Received: (from curt@localhost)
	by sledge.infoplane.com (8.8.8/8.8.8/Debian/GNU) id GAA24798
	for curt@me.umn.edu; Fri, 13 Nov 1998 06:10:23 -0600
Message-Id: <199811131210.GAA24798@sledge.infoplane.com>
From: curt@infoplane.com
To: curt@me.umn.edu
Subject: fuzzy-light-effect
Date: Fri, 13 Nov 1998 06:10:23 -0600

/* stars - draws a twisting sphere of eerie lights
   Copyright (C) 1998 James Bowman

stars is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpasm is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#include <assert.h>
#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <GL/glut.h>

static int phi;                 /* Global clock */
static int texWidth = 64;       /* 64x64 is plenty */

/************************************************************************/

/* Generate a pleasing, light-shaped texture. */

static void
setTexture(void)
{
  int texSize;
  void *textureBuf;
  GLubyte *p;
  int i,j;
  double radius;

  texSize = texWidth*texWidth;

  textureBuf = malloc(texSize);
  if (NULL == textureBuf) return;

  p = (GLubyte *)textureBuf;

  radius = (double)(texWidth / 2);

  for (i=0; i < texWidth; i++) {
    for (j=0; j < texWidth; j++) {
      double x, y, d;
            
      x = fabs((double)(i - (texWidth / 2)));
      y = fabs((double)(j - (texWidth / 2)));

      d = sqrt((x * x) + (y * y));
      if (d < radius) {
        double t = 1.0 - (d / radius); /* t is 1.0 at center,
                                          0.0 at edge */

        /* inverse square looks nice */
        *p = (int)((double)0xff * (t * t));
      } else
        *p = 0x00;
      p++;
    }
  }

  gluBuild2DMipmaps(GL_TEXTURE_2D, 1, texWidth, texWidth, 
                    GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, textureBuf);
  free(textureBuf);
}

/************************************************************************/

static int W, H;

#define NUM_STARS 100

#define frand()  ((float)(rand() & 0xfff) / (float)0x1000)  /* 0.0 - 1.0 */
#define frand2() (1.0f - (2.0f * frand()))                  /* -1 - +1   */

/* Normalise the input vector */
static void vecNormalise(float *vv)
{
  double mag;

  mag = sqrt((vv[0] * vv[0]) + (vv[1] * vv[1]) + (vv[2] * vv[2]));
  vv[0] /= mag;
  vv[1] /= mag;
  vv[2] /= mag;
}

void
redraw_viewing(void)
{
  static int cold = 1;
  static float stars[NUM_STARS * 3];
  GLfloat fb_buffer[NUM_STARS * 4];
  int i;

  if (cold) {
    /* Position all the stars more-or-less randomly on the surface
     * of a unit sphere. */
    for (i = 0; i < NUM_STARS; i++) {
      stars[3 * i + 0] = frand2();
      stars[3 * i + 1] = frand2();
      stars[3 * i + 2] = frand2();
      vecNormalise(&stars[3 * i]);
    }
    cold = 0;
  }

  glClear(GL_COLOR_BUFFER_BIT);

  /* First use feedback to determine the screen positions of the
     stars. */
  glFeedbackBuffer(NUM_STARS * 4, GL_3D, fb_buffer);
  glRenderMode(GL_FEEDBACK);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glRotatef(phi % 360, 0.0f, 1.0f, 0.0f); /* Constant spin */
  glRotatef(sin((double)phi / 300.0) * 150.0,   /* Periodic twist */
            0.1f, 0.6f, -0.5f);
  glBegin(GL_POINTS);
  for (i = 0; i < NUM_STARS; i++)
    glVertex3fv(&stars[3 * i]);
  glEnd();
  glPopMatrix();
  glRenderMode(GL_RENDER);      /* No more feedback */

  /* Now draw the stars as sprites. */
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);           /* Simple additive blend is fine */
  glBlendFunc(GL_ONE, GL_ONE);

  /* Choose a color triple like this so that when the sprites overlap
   * and the color saturates, red will clamp first, then green, then
   * blue. */
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glColor3f(1.0f, 0.75f, 0.5f);

  /* Set up projection and modelview matrix that gives us a 1:1
   * mapping to screen coordinates. */
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glScalef(2.0f / (GLfloat)W, 2.0f / (GLfloat)H, 1.0f);
  glTranslatef(0.5f * -(GLfloat)W, 0.5f * -(GLfloat)H, 0.0f);

  {
    float width;
    float x, y, z;
    int i;

    glBegin(GL_QUADS);

    for (i = 0; i < NUM_STARS; i++) {
      /* Skip the GL_POINT_TOKEN */
      x = fb_buffer[4 * i + 1];
      y = fb_buffer[4 * i + 2];
      z = fb_buffer[4 * i + 3];

      width = 10.0 * (2.0 - z); /* Arbitrary distance attenuation */

      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(x - width, y - width);

      glTexCoord2f(1.0f, 0.0f);
      glVertex2f(x + width, y - width);

      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(x + width, y + width);
      
      glTexCoord2f(0.0f, 1.0f);
      glVertex2f(x - width, y + width);
    }

    glEnd();
  }

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  
  glutSwapBuffers();
}

void
reshape_viewing(int w, int h)
{
  glViewport(0, 0, w, h);
  W = w;
  H = h;
}

/************************************************************************/

void
animate(void)
{
  phi++;
  redraw_viewing();
}

void
key(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
  case 'q':
    exit(0);
  }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(640, 480);

  glutCreateWindow("stars");  
  glutReshapeFunc(reshape_viewing);
  glutDisplayFunc(redraw_viewing);

  glutIdleFunc(animate);
  glutKeyboardFunc(key);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glMatrixMode(GL_MODELVIEW);

  setTexture();
  glTexParameteri(GL_TEXTURE_2D,
                  GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,
                  GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR);

  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0f,
                  /* aspect ratio             */ 1.0f,
                  /* Z near */ 3.0f,
                  /* Z far */ 5.0f);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0f, 0.0f, 4.0f,   /* eye position */
    0.0f, 0.0f, 0.0f,           /* looking at */
    0.0f, 1.0f, 0.0f);          /* up vector */

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}

