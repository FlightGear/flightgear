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

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include "ApplicationProperties.hxx"
#include "FGCroppedTexture.hxx"

GLuint FGCroppedTexture::s_current_bound_texture = 0;
map <string, GLuint> FGCroppedTexture::s_cache;
map <string, FGTextureLoaderInterface*> FGCroppedTexture::s_TextureLoader;
FGDummyTextureLoader FGCroppedTexture::s_DummyTextureLoader;

FGCroppedTexture::FGCroppedTexture (const string &path,
                                    const float minX, const float minY,
                                    const float maxX, const float maxY) :
  m_path (path),
  m_minX (minX), m_minY (minY),
  m_maxX (maxX), m_maxY (maxY),
  m_texture(0) {
}

FGCroppedTexture::~FGCroppedTexture () {
}

void
FGCroppedTexture::setPath (const string &path) {
  m_path = path;
}

const string &
FGCroppedTexture::getPath () const {
  return m_path;
}

void
FGCroppedTexture::setCrop (const float minX, const float minY, const float maxX, const float maxY) {
  m_minX = minX; m_minY = minY; m_maxX = maxX; m_maxY = maxY;
}

void
FGCroppedTexture::registerTextureLoader (const string &extension,
                                         FGTextureLoaderInterface * const loader) {
  if (s_TextureLoader.count (extension) == 0) {
    s_TextureLoader[extension] = loader;
  }
}

float
FGCroppedTexture::getMinX () const {
  return m_minX;
}

float
FGCroppedTexture::getMinY () const {
  return m_minY;
}

float
FGCroppedTexture::getMaxX () const {
  return m_maxX;
}

float
FGCroppedTexture::getMaxY () const {
  return m_maxY;
}

GLuint
FGCroppedTexture::getTexture () const {
  return m_texture;
}

void
FGCroppedTexture::bind (const GLint Textured_Layer_Sampler_Loc) {
  if (m_texture == 0) {
    SG_LOG (SG_COCKPIT,
            SG_DEBUG,
            "First bind of texture " << m_path);
    if (s_cache.count (m_path) > 0 ) {
      m_texture = s_cache[m_path];
      SG_LOG (SG_COCKPIT,
              SG_DEBUG,
              "Using texture " << m_path << " from cache (#" << m_texture << ")");
    } else {
      const SGPath path (ApplicationProperties::GetRootPath (m_path.c_str ()));
      const string extension (path.extension ());
      FGTextureLoaderInterface *loader (&s_DummyTextureLoader);
      if (s_TextureLoader.count (extension) == 0) {
        SG_LOG (SG_COCKPIT,
                SG_ALERT,
                "Can't handle textures of type " << extension);
      } else {
        loader = s_TextureLoader[extension];
      }

      m_texture = loader->loadTexture (path.local8BitStr ());
      SG_LOG (SG_COCKPIT,
              SG_DEBUG,
              "Texture " << path << " loaded from file as #" << m_texture);
      s_cache[m_path] = m_texture;
    }
  }

  if (s_current_bound_texture != m_texture) {
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, m_texture);
    glUniform1i (Textured_Layer_Sampler_Loc, 0);
    s_current_bound_texture = m_texture;
  }
}
