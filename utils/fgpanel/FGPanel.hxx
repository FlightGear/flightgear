//  FGPanel.hxx - generic support classes for a 2D panel.
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
//  $Id: FGPanel.hxx,v 1.1 2016/07/20 22:01:30 allaert Exp $

#ifndef FGPANEL_HXX
#define FGPANEL_HXX

#ifndef __cplusplus
# error This library requires C++
#endif


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/props/propsfwd.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "FGCroppedTexture.hxx"
#include "FGPanelInstrument.hxx"

using namespace std;

////////////////////////////////////////////////////////////////////////
// Top-level panel.
////////////////////////////////////////////////////////////////////////

/**
 * Instrument panel class.
 *
 * The panel is a container that has a background texture and holds
 * zero or more instruments.  The panel will order the instruments to
 * redraw themselves when necessary, and will pass mouse clicks on to
 * the appropriate instruments for processing.
 */
class FGPanel : public SGSubsystem {
public:
  FGPanel (const SGPropertyNode_ptr root);
  virtual ~FGPanel ();

  // Update the panel (every frame).
  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  //  virtual void draw ();
  virtual void update (const double dt);

  // transfer pointer ownership!!!
  virtual void addInstrument (FGPanelInstrument * const instrument);

  // Background texture.
  virtual void setBackground (const FGCroppedTexture_ptr texture);
  void setBackgroundWidth (const double d);
  void setBackgroundHeight (const double d);

  // Background multiple textures.
  virtual void setMultiBackground (const FGCroppedTexture_ptr texture , const int idx);

  // Full width of panel.
  virtual void setWidth (const int width);
  virtual int getWidth () const;

  // Full height of panel.
  virtual void setHeight (const int height);
  virtual int getHeight () const;

private:

  typedef vector <FGPanelInstrument *> instrument_list_type;
  int m_width;
  int m_height;

  SGPropertyNode_ptr m_flipx;

  FGCroppedTexture_ptr m_bg;
  double m_bg_width;
  double m_bg_height;
  FGCroppedTexture_ptr m_mbg[8];
  // List of instruments in panel.
  instrument_list_type m_instruments;

  void getInitDisplayList ();

  static GLuint Textured_Layer_Program_Object;
  static GLint Textured_Layer_Position_Loc;
  static GLint Textured_Layer_Tex_Coord_Loc;
  static GLint Textured_Layer_MVP_Loc;
  static GLint Textured_Layer_Sampler_Loc;
};

#endif
