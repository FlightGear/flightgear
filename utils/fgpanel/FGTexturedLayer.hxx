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

#ifndef FGTEXTUREDLAYER_HXX
#define FGTEXTUREDLAYER_HXX

#include "FGCroppedTexture.hxx"
#include "FGInstrumentLayer.hxx"

/**
 * A textured layer of an instrument.
 *
 * This is a layer holding a single texture.  Normally, the texture's
 * backgound should be transparent so that lower layers and the panel
 * background can show through.
 */
class FGTexturedLayer : public FGInstrumentLayer {
public:
  static void Init (const GLuint Program_Object,
                    const GLint Position_Loc,
                    const GLint Tex_Coord_Loc,
                    const GLint MVP_Loc,
                    const GLint Sampler_Loc);

  FGTexturedLayer (const int w = -1, const int h = -1);
  FGTexturedLayer (const FGCroppedTexture_ptr texture, const int w = -1, const int h = -1);
  virtual ~FGTexturedLayer ();

  virtual void draw ();

  virtual void setTexture (const FGCroppedTexture_ptr texture);
  FGCroppedTexture_ptr getTexture () const;

  void setEmissive (const bool emissive);

private:
  void getDisplayList ();

  FGCroppedTexture_ptr m_texture;
  bool m_emissive;

  static GLuint Textured_Layer_Program_Object;
  static GLint Textured_Layer_Position_Loc;
  static GLint Textured_Layer_Tex_Coord_Loc;
  static GLint Textured_Layer_MVP_Loc;
  static GLint Textured_Layer_Sampler_Loc;
};

#endif
