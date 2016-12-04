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
#ifndef FGCROPPEDTEXTURE_HXX
#define FGCROPPEDTEXTURE_HXX

#include <map>
#include <simgear/structure/SGSharedPtr.hxx>
#include "FGDummyTextureLoader.hxx"

/**
 * Cropped texture (should migrate out into FGFS).
 *
 * This structure wraps an SSG texture with cropping information.
 */
class FGCroppedTexture : public SGReferenced {
public:
  FGCroppedTexture (const string &path,
                    const float minX = 0.0, const float minY = 0.0,
                    const float maxX = 1.0, const float maxY = 1.0);

  virtual ~FGCroppedTexture ();

  virtual void setPath (const string &path);

  virtual const string &getPath () const;

  virtual void setCrop (const float minX, const float minY, const float maxX, const float maxY);

  static void registerTextureLoader (const string &extension,
                                     FGTextureLoaderInterface * const loader);

  virtual float getMinX () const;
  virtual float getMinY () const;
  virtual float getMaxX () const;
  virtual float getMaxY () const;
  GLuint getTexture () const;

  virtual void bind (const GLint Textured_Layer_Sampler_Loc);

private:
  string m_path;
  float m_minX, m_minY, m_maxX, m_maxY;

  GLuint m_texture;
  static GLuint s_current_bound_texture;
  static map <string, GLuint> s_cache;
  static map <string, FGTextureLoaderInterface*> s_TextureLoader;
  static FGDummyTextureLoader s_DummyTextureLoader;
};

typedef SGSharedPtr <FGCroppedTexture> FGCroppedTexture_ptr;

#endif
