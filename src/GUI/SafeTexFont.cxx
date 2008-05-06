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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "SafeTexFont.hxx"

using namespace flightgear;

int SafeTexFont::load(const char *fname, GLenum mag, GLenum min)
{
    struct stat buf;
    if (stat(fname, &buf) == -1) {
        _status = ERROR;
        return FNT_FALSE;
    }
    _name = fname;
    _mag = mag;
    _min = min;
    return FNT_TRUE;
}

bool SafeTexFont::ensureTextureLoaded()
{
    if (_status != ERROR) {
        if (_status == LOADED) {
            return true;
        } else {
            int loadStatus = fntTexFont::load(_name.c_str(), _mag, _min);
            if (loadStatus == FNT_TRUE) {
                _status = LOADED;
                return true;
            } else {
                _status = ERROR;
                _error = ulGetError();
                return false;
            }    
        }
    } else {
        return false;
    }
}

void SafeTexFont::begin()
{
    if (ensureTextureLoaded())
        fntTexFont::begin();
}

void SafeTexFont::putch(sgVec3 curpos, float pointsize, float italic, char c)
{
    if (ensureTextureLoaded())
        fntTexFont::putch(curpos, pointsize, italic, c);
}

void SafeTexFont::puts(sgVec3 curpos, float pointsize, float italic,
                       const char *s)
{
    if (ensureTextureLoaded())
        fntTexFont::puts(curpos, pointsize, italic, s);
}
