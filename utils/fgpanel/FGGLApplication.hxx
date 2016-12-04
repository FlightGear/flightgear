//
//  Written and (c) Torsten Dreyer - Torsten(at)t3r_dot_de
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
#ifndef __FGGLAPPLICATION_HXX
#define __FGGLAPPLICATION_HXX

class FGGLApplication {
public:
  FGGLApplication (const char *a_name, int argc, char **argv);
  virtual ~FGGLApplication ();
  void Run (const int glutMode,
            const bool gameMode,
            int widht = -1,
            int height = -1,
            const int bpp = 32);
protected:
  virtual void Key (const unsigned char key, const int x, const int y) {}
  virtual void Idle () {}
  virtual void Display () {}
  virtual void Reshape (const int width, const int height) {}

  virtual void Init () {}

  int windowId;
  bool gameMode;

  const char *name;

  static FGGLApplication *application;
private:
  static void KeyCallback (const unsigned char key, const int x, const int y);
  static void IdleCallback ();
  static void DisplayCallback ();
  static void ReshapeCallback (const int width, const int height);

};

#endif
