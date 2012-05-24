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
#ifndef __FGFONTCACHE_HXX
#define __FGFONTCACHE_HXX

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>


// forward declare only!
class puFont;
class fntTexFont;

/**
 * A class to keep all fonts available for future use.
 * This also assures a font isn't resident more than once.
 */
class FGFontCache {
private:
    // The parameters of a request to the cache.
    struct FntParams
    {
        const std::string name;
        const float size;
        const float slant;
        FntParams() : size(0.0f), slant(0.0f) {}
        FntParams(const FntParams& rhs)
            : name(rhs.name), size(rhs.size), slant(rhs.slant)
        {
        }
        FntParams(const std::string& name_, float size_, float slant_)
            : name(name_), size(size_), slant(slant_)
        {
        }
    };
    struct FntParamsLess
        : public std::binary_function<const FntParams, const FntParams, bool>
    {
        bool operator() (const FntParams& f1, const FntParams& f2) const;
    };
    struct fnt {
        fnt(puFont *pu = 0) : pufont(pu), texfont(0) {}
        ~fnt();
        // Font used by plib GUI code
        puFont *pufont;
        // TXF font
        fntTexFont *texfont;
    };
    // Path to the font directory
    SGPath _path;

    typedef std::map<const std::string, fntTexFont*> TexFontMap;
    typedef std::map<const FntParams, fnt*, FntParamsLess> PuFontMap;
    TexFontMap _texFonts;
    PuFontMap _puFonts;

    bool _initialized;
    struct fnt *getfnt(const char *name, float size, float slant);
    void init();

public:
    FGFontCache();
    ~FGFontCache();

    puFont *get(const char *name, float size=15.0, float slant=0.0);
    puFont *get(SGPropertyNode *node);

    fntTexFont *getTexFont(const char *name, float size=15.0, float slant=0.0);

    SGPath getfntpath(const char *name);
    /**
     * Preload all the fonts in the FlightGear font directory. It is
     * important to load the font textures early, with the proper
     * graphics context current, so that no plib (or our own) code
     * tries to load a font from disk when there's no current graphics
     * context.
     */
    bool initializeFonts();
};
#endif
