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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "FGFontCache.hxx"

#include <plib/fnt.h>
#include <plib/pu.h>

#include <simgear/props/props.hxx>

#include <Main/globals.hxx>

////////////////////////////////////////////////////////////////////////
// FGFontCache class.
////////////////////////////////////////////////////////////////////////

extern puFont FONT_HELVETICA_14;
extern puFont FONT_SANS_12B;
extern puFont FONT_HELVETICA_12;

namespace
{
struct GuiFont
{
    const char *name;
    puFont *font;
    struct Predicate
        : public std::unary_function<const GuiFont, bool>
    {
        Predicate(const char* name_) : name(name_) {}
        bool operator() (const GuiFont& f1) const
        {
            return ::strcmp(f1.name, name) == 0;
        }
        const char* name;
    };
};

const GuiFont guifonts[] = {
    { "default",      &PUFONT_HELVETICA_12 },
    { "FIXED_8x13",   &PUFONT_8_BY_13 },
    { "FIXED_9x15",   &PUFONT_9_BY_15 },
    { "TIMES_10",     &PUFONT_TIMES_ROMAN_10 },
    { "TIMES_24",     &PUFONT_TIMES_ROMAN_24 },
    { "HELVETICA_10", &PUFONT_HELVETICA_10 },
    { "HELVETICA_12", &FONT_HELVETICA_12 },
    { "HELVETICA_14", &FONT_HELVETICA_14 },
    { "HELVETICA_18", &PUFONT_HELVETICA_18 },
    { "SANS_12B",     &FONT_SANS_12B }
};

const GuiFont* guifontsEnd = &guifonts[sizeof(guifonts)/ sizeof(guifonts[0])];
}

FGFontCache::FGFontCache() :
    _initialized(false)
{
}

FGFontCache::~FGFontCache()
{
   PuFontMap::iterator it, end = _puFonts.end();
   for (it = _puFonts.begin(); it != end; ++it)
       delete it->second;
}

inline bool FGFontCache::FntParamsLess::operator()(const FntParams& f1,
                                                   const FntParams& f2) const
{
    int comp = f1.name.compare(f2.name);
    if (comp < 0)
        return true;
    else if (comp > 0)
        return false;
    if (f1.size < f2.size)
        return true;
    else if (f1.size > f2.size)
        return false;
    return f1.slant < f2.slant;
}

struct FGFontCache::fnt *
FGFontCache::getfnt(const char *name, float size, float slant)
{
    std::string fontName(name);
    FntParams fntParams(fontName, size, slant);
    PuFontMap::iterator i = _puFonts.find(fntParams);
    if (i != _puFonts.end())
        return i->second;
    // fntTexFont s are all preloaded into the _texFonts map
    TexFontMap::iterator texi = _texFonts.find(fontName);
    fntTexFont* texfont = 0;
    puFont* pufont = 0;
    if (texi != _texFonts.end()) {
        texfont = texi->second;
    } else {
        const GuiFont* guifont = std::find_if(&guifonts[0], guifontsEnd,
                                              GuiFont::Predicate(name));
        if (guifont != guifontsEnd) {
            pufont = guifont->font;
        }
    }
    fnt* f = new fnt;
    if (pufont) {
        f->pufont = pufont;
    } else if (texfont) {
        f->texfont = texfont;
        f->pufont = new puFont;
        f->pufont->initialize(static_cast<fntFont *>(f->texfont), size, slant);
    } else {
        f->pufont = guifonts[0].font;
    }
    _puFonts[fntParams] = f;
    return f;
}

puFont *
FGFontCache::get(const char *name, float size, float slant)
{
    return getfnt(name, size, slant)->pufont;
}

fntTexFont *
FGFontCache::getTexFont(const char *name, float size, float slant)
{
    init();
    return getfnt(name, size, slant)->texfont;
}

puFont *
FGFontCache::get(SGPropertyNode *node)
{
    if (!node)
        return get("Helvetica.txf", 15.0, 0.0);

    const char *name = node->getStringValue("name", "Helvetica.txf");
    float size = node->getFloatValue("size", 15.0);
    float slant = node->getFloatValue("slant", 0.0);

    return get(name, size, slant);
}

void FGFontCache::init()
{
    if (!_initialized) {
        char *envp = ::getenv("FG_FONTS");
        if (envp != NULL) {
            _path.set(envp);
        } else {
            _path.set(globals->get_fg_root());
            _path.append("Fonts");
        }
        _initialized = true;
    }
}

SGPath
FGFontCache::getfntpath(const char *name)
{
    init();
    SGPath path(_path);
    if (name && std::string(name) != "") {
        path.append(name);
        if (path.exists())
            return path;
    }

    path = SGPath(_path);
    path.append("Helvetica.txf");
    
    return path;
}

bool FGFontCache::initializeFonts()
{
    static std::string fontext("txf");
    init();
    ulDir* fontdir = ulOpenDir(_path.c_str());
    if (!fontdir)
        return false;
    const ulDirEnt *dirEntry;
    while ((dirEntry = ulReadDir(fontdir)) != 0) {
        SGPath path(_path);
        path.append(dirEntry->d_name);
        if (path.extension() == fontext) {
            fntTexFont* f = new fntTexFont;
            if (f->load((char *)path.c_str()))
                _texFonts[std::string(dirEntry->d_name)] = f;
            else
                delete f;
        }
    }
    ulCloseDir(fontdir);
    return true;
}

FGFontCache::fnt::~fnt()
{
    if (texfont) { 
        delete pufont; 
        delete texfont;
    }
}

