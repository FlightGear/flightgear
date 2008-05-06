// Copyright (C) 2008  Tim Moore
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef FLIGHTGEAR_SAFETEXFONT_HXX
#define FLIGHTGEAR_SAFETEXFONT_HXX 1

#include <string>

#include <plib/fnt.h>

namespace flightgear
{
/**
 * Subclass of plib's fntTexFont that defers the loading of the font
 * texture until just before rendering, insuring that it happens in
 * the proper graphics context. 
 */
class SafeTexFont : public fntTexFont
{
public:
    SafeTexFont() : _status(NOT_LOADED) {}
    /** Load the texture for this font.
     * @param mag OpenGL texture magnification; default is GL_NEAREST
     * @param min OpenGL texture minification; default is
     * GL_LINEAR_MIPMAP_LINEAR.
     * @return the plib value FNT_FALSE if the font file doesn't
     * exist; otherwise returns FNT_TRUE.
     */
    int load(const char *fname, GLenum mag = GL_NEAREST, 
             GLenum min = GL_LINEAR_MIPMAP_LINEAR);
    void begin();
    void putch(sgVec3 curpos, float pointsize, float italic, char c);
    void puts(sgVec3 curpos, float pointsize, float italic, const char *s);
    enum FontStatus
    {
        ERROR = -1,
        NOT_LOADED = 0,
        LOADED = 1
    };
    FontStatus getStatus() { return _status; }
    std::string& fntError() { return _error; }
protected:
    bool ensureTextureLoaded();
    std::string _name;
    std::string _error;
    GLenum _mag;
    GLenum _min;
    FontStatus _status;
};
}
#endif
