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
     * Select the locale's primary language. When no language is given (NULL),
     * a default is determined matching the system locale.
     */
    bool        selectLanguage      (const char* language = NULL);

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
    const char* getLocalizedString  (const char* id, const char* resource, const char* Default=NULL);

    /**
      * Obtain a list of strings from the localized resource matching the given identifier.
      * Selected context refers to "menu", "options", "dialog" etc.
      * Returns a list of (string) properties.
      */
    simgear::PropertyList getLocalizedStrings(const char* id, const char* resource);

    /**
     * Obtain default font for current locale.
     */
    const char* getDefaultFont      (const char* fallbackFont);

    /**
     * Simple UTF8 to Latin1 encoder.
     */
    static void utf8toLatin1        (std::string& s);

    /**
     * Obtain user's default language setting.
     */
    const char* getUserLanguage();

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
    const char*     getLocalizedString  (SGPropertyNode *localeNode, const char* id, const char* context);

    /**
     * Obtain a list of strings from locale node matching the given identifier and context.
     */
    simgear::PropertyList getLocalizedStrings(SGPropertyNode *localeNode, const char* id, const char* context);

    SGPropertyNode_ptr _intl;
    SGPropertyNode_ptr _currentLocale;
    SGPropertyNode_ptr _defaultLocale;
};

#endif // __FGLOCALE_HXX
