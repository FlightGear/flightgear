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

#include <simgear/props/props.hxx>

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
    bool selectLanguage(const char* language = nullptr);

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

    /**
     * Load strings for requested resource, i.e. "menu", "options", "dialogs".
     * Loads data for current and default locale (the latter is the fallback).
     * Result is stored below the "strings" node in the property tree of the
     * respective locale.
     */
    bool        loadResource        (const char* resource);

    /**
     * Obtain a single string from the localized resource matching the given identifier.
     * Selected context refers to "menu", "options", "dialog" etc.
     */
    std::string getLocalizedString(const char* id, const char* resource,
                                   const char* Default = nullptr);

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
    const char* getDefaultFont      (const char* fallbackFont);

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



protected:
    /**
     * Find property node matching given language.
     */
    SGPropertyNode* findLocaleNode      (const std::string& language);

    /**
     * Load resource data for given locale node.
     */
    bool            loadResource        (SGPropertyNode* localeNode, const char* resource);

    /**
     * Obtain a single string from locale node matching the given identifier and context.
     */
    std::string     getLocalizedString  (SGPropertyNode *localeNode, const char* id, const char* context, int index) const;

    /**
     * Obtain a list of strings from locale node matching the given identifier and context.
     */
    simgear::PropertyList getLocalizedStrings(SGPropertyNode *localeNode, const char* id, const char* context);

    /**
     * Obtain user's default language setting.
     */
    string_list getUserLanguage();

    SGPropertyNode_ptr _intl;
    SGPropertyNode_ptr _currentLocale;
    SGPropertyNode_ptr _defaultLocale;
    std::string _currentLocaleString;

private:
    /** Return a new string with the character encoding part of the locale
     *  spec removed., i.e., "de_DE.UTF-8" becomes "de_DE". If there is no
     *  such part, return a copy of the input string.
     */
    static std::string removeEncodingPart(const std::string& locale);
};

// global translation wrappers

std::string fgTrMsg(const char* key);
std::string fgTrPrintfMsg(const char* key, ...);


#endif // __FGLOCALE_HXX
