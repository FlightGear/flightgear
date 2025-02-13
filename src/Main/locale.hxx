// locale.hxx -- FlightGear Localization Support
//
// Written by Thorsten Brehm, started April 2012.
//
// Copyright (C) 2012 Thorsten Brehm - brehmt (at) gmail com
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef __FGLOCALE_HXX
#define __FGLOCALE_HXX

#include <string>
#include <cstdarg> // for va_start/_end

#include <simgear/props/propsfwd.hxx>
#include <simgear/misc/strutils.hxx>

// forward decls
class SGPath;
namespace simgear { class Dir; }


///////////////////////////////////////////////////////////////////////////////
// FGLocale  //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FGLocale
{
public:
    FGLocale(SGPropertyNode* root);
    virtual ~FGLocale();

    /**
     * Select the locale's primary language. When no language is given
     * (nullptr), a default is determined matching the system locale.
     */
    bool selectLanguage(const std::string& language = {});

    /** Return the preferred language according to user choice and/or settings.
     *
     *  Examples: 'fr_CA', 'de_DE'... or the empty string if nothing could be
     *            found.
     *
     *  Note that this is not necessarily the same as the last value passed to
     *  selectLanguage(), assuming it was non-null and non-empty, because the
     *  latter may have an encoding specifier, while values returned by
     *  getPreferredLanguage() never have that.
     */
    std::string getPreferredLanguage() const;

    void loadAircraftTranslations();
    void loadAddonTranslations();

    /**
     * Obtain a single string from the localized resource matching the given identifier.
     * Selected context refers to "menu", "options", "dialog" etc.
     */
    std::string getLocalizedString(const char* id, const char* resource,
                                   const char* Default = nullptr);

    std::string getLocalizedString(const std::string& id, const char* resource, const std::string& defaultValue = {});

    /**
      * Obtain a list of strings from the localized resource matching the given identifier.
      * Selected context refers to "menu", "options", "dialog" etc.
      * Returns a list of (string) properties.
      */
    simgear::PropertyList getLocalizedStrings(const char* id, const char* resource);


    /**
     * Obtain a single string from the resource matching an identifier and ID.
     */
    std::string getLocalizedStringWithIndex(const char* id, const char* resource, unsigned int index) const;

    /**
     * Return the number of strings matching a resource
     */
    size_t getLocalizedStringCount(const char* id, const char* resource) const;

    /**
     * Obtain default font for current locale.
     */
    std::string getDefaultFont      (const char* fallbackFont);

    /**
     * Obtain a message string, from a localized resource ID, and use it as
     * a printf format string.
     */
    std::string localizedPrintf(const char* id, const char* resource, ... );
    
    std::string vlocalizedPrintf(const char* id, const char* resource, va_list args);
    
    /**
     * Simple UTF8 to Latin1 encoder.
     */
    static void utf8toLatin1        (std::string& s);

    /**
        * reset all data in the locale. This is needed to allow the launcher to use the code,
                without disturbing the main behaviour. Afteer calling this you can do
                        selectLangauge again without problems.
     */
    void clear();

    /**
        @ brief given a node with children corresponding to different language / locale codes,
        select one based on the user preferred langauge
     */
    SGPropertyNode_ptr selectLanguageNode(SGPropertyNode* langs) const;

protected:
    /**
     * Find property node matching given language.
     */
    SGPropertyNode* findLocaleNode      (const std::string& language);

    /**
     * Load default strings for the requested resource ("atc", "menu",  etc.).
     *
     * The strings are stored in the property tree under
     * /sim/intl/locale[0]/⟨domain⟩/strings/⟨resource⟩.
     *
     * To avoid confusing unrelated things, translatable strings from the
     * simulator core (FGData), from an add-on or from the current aircraft
     * are all stored in different *domains*. There are three kinds of domains:
     *   - 'core' for strings coming from FGData;
     *   - 'addons/⟨addonId⟩' for strings coming from an add-on;
     *   - 'aircraft' for strings coming from the current aircraft.
     */
    bool loadResourceForDefaultTranslation(
        const SGPath& xmlFile, const std::string& domain,
        const std::string& resource);
    /**
     * Similar to loadResourceForDefaultTranslation(), except it gets the
     * resource path from the Property Tree.
     *
     * The path is the string value of property node
     * /sim/intl/locale[0]/⟨domain⟩/strings/⟨resource⟩; it is interpreted
     * relatively to basePath.
     */
    bool loadResourceForDefaultTranslation_indirect(
        const SGPath& basePath, const std::string& domain,
        const std::string& resource);
    /**
     * Load the default translation of core resources 'atc', 'menu',
     * 'options', 'sys', 'tips', etc.
     */
    void loadCoreResourcesForDefaultTranslation();

    /**
     * From an add-on or aircraft directory, load the default translation and,
     * if available, the XLIFF file for the current locale.
     */
    void loadResourcesFromAircraftOrAddonDir(const SGPath& basePath,
                                             const std::string& domain);
    void loadDefaultTranslationFromAircraftOrAddonDir(
        const simgear::Dir& defaultTranslationDir, const std::string& domain);
    void loadXLIFFFromAircraftOrAddonDir(const SGPath& basePath,
                                         const std::string& domain);

    /**
     * Obtain a single string from locale node matching the given identifier and context.
     */
    std::string innerGetLocalizedString(SGPropertyNode* localeNode, const char* id, const char* context, int index) const;

    /**
     * Obtain a list of strings from locale node matching the given identifier and context.
     */
    simgear::PropertyList getLocalizedStrings(SGPropertyNode *localeNode, const char* id, const char* context);

    /**
     * Obtain user's default language setting.
     */
    string_list getUserLanguages();

    SGPropertyNode_ptr _intl;
    SGPropertyNode_ptr _currentLocale;
    SGPropertyNode_ptr _defaultLocale;
    std::string _currentLocaleString;

    /**
     * Load an XLIFF 1.2 file into the Property Tree under
     * /sim/intl/locale[n]/⟨domain⟩/strings/⟨resource⟩.
     *
     * @param basePath base for the relative path to XLIFF file that is the
     *                 string value of node /sim/intl/locale[n]/⟨domain⟩/xliff.
     * @param localeNode pointer to the /sim/intl/locale[n] node for the
     *                 current locale
     * @param domain a string such as 'core' or 'addons/⟨addonId⟩'
     */
    void loadXLIFF(const SGPath& basePath, SGPropertyNode* localeNode,
                   const std::string& domain);
private:
    /** Return a new string with the character encoding part of the locale
     *  spec removed., i.e., "de_DE.UTF-8" becomes "de_DE". If there is no
     *  such part, return a copy of the input string.
     */
    static std::string removeEncodingPart(const std::string& locale);

    // this is the ordered list of languages to try. It's the same as
    // returned by getUserLanguages(), except if the user has used
    // --language to override, that will be the first item.

    string_list _languages;
    bool _inited = false;
};

// global translation wrappers

std::string fgTrMsg(const char* key);
std::string fgTrPrintfMsg(const char* key, ...);


#endif // __FGLOCALE_HXX
