// locale.cxx -- FlightGear Localization Support
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

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include "fg_props.hxx"
#include "locale.hxx"

using std::vector;
using std::string;

FGLocale::FGLocale(SGPropertyNode* root) :
    _intl(root->getNode("/sim/intl",0, true)),
    _defaultLocale(_intl->getChild("locale",0, true))
{
}

FGLocale::~FGLocale()
{
}

#ifdef _WIN32
/**
 * Determine locale/language settings on Windows.
 *
 * Copyright (C) 1997, 2002, 2003 Martin von Loewis
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies.
 *
 * This software comes with no warranty. Use at your own risk.
 */
const char*
FGLocale::getUserLanguage()
{
    static char locale[100] = {0};

    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_SISO639LANGNAME,
                      locale, sizeof(locale)))
    {
        SG_LOG(SG_GENERAL, SG_DEBUG, "Detected locale's language setting: " << locale);
        size_t i = strlen(locale);
        locale[i++] = '_';
        if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                          LOCALE_SISO3166CTRYNAME,
                          locale+i, (int)(sizeof(locale)-i)))
            return locale;

        locale[--i] = 0;
        SG_LOG(SG_GENERAL, SG_WARN, "Failed to detected locale's country setting.");
        return locale;
    }

    return NULL;
}
#else
/**
 * Determine locale/language settings on Linux (and Mac?).
 */
const char*
FGLocale::getUserLanguage()
{
    return ::getenv("LANG");
}
#endif

// Search property tree for matching locale description
SGPropertyNode*
FGLocale::findLocaleNode(const string& language)
{
    SGPropertyNode* node = NULL;

    // remove character encoding from the locale spec, i.e. "de_DE.utf8" => "de_DE"
    size_t pos = language.find(".");
    if ((pos != string::npos)&&(pos>0))
    {
        node = findLocaleNode(language.substr(0, pos));
        if (node)
            return node;
    }

    SG_LOG(SG_GENERAL, SG_DEBUG, "Searching language resource for locale: " << language);
    // search locale using full string
    vector<SGPropertyNode_ptr> localeList = _intl->getChildren("locale");

    for (size_t i = 0; i < localeList.size(); i++)
    {
       vector<SGPropertyNode_ptr> langList = localeList[i]->getChildren("lang");

       for (size_t j = 0; j < langList.size(); j++)
       {
           if (!language.compare(langList[j]->getStringValue()))
           {
               SG_LOG(SG_GENERAL, SG_INFO, "Found language resource for: " << language);
               return localeList[i];
           }
       }
    }

    // try country's default resource, i.e. "de_DE" => "de"
    pos = language.find("_");
    if ((pos != string::npos)&&(pos>0))
    {
        node = findLocaleNode(language.substr(0, pos));
        if (node)
            return node;
    }

    return NULL;
}

// Select the language. When no language is given (NULL),
// a default is determined matching the system locale.
bool
FGLocale::selectLanguage(const char *language)
{
    // Use system setting when no language is given.
    if ((language == NULL)||(language[0]==0))
    {
        language = getUserLanguage();
        SG_LOG(SG_GENERAL, SG_INFO, "System language: " << ((language) ? language : "<unavailable>"));
    }

    // Use plain C locale if nothing is available.
    if ((language == NULL)||(language[0]==0))
    {
        SG_LOG(SG_GENERAL, SG_WARN, "Unable to detect system language" );
        language = "C";
    }

    SGPropertyNode *locale = findLocaleNode(language);
    if (!locale)
    {
       SG_LOG(SG_GENERAL, SG_ALERT,
              "No internationalization settings specified in preferences.xml" );
       return false;
    }

    _currentLocale = locale;
    return true;
}

// Load strings for requested resource and locale.
// Result is stored below "strings" in the property tree of the given locale.
bool
FGLocale::loadResource(SGPropertyNode* localeNode, const char* resource)
{
    SGPath path( globals->get_fg_root() );

    SGPropertyNode* stringNode = localeNode->getNode("strings", 0, true);

    const char *path_str = stringNode->getStringValue(resource, NULL);
    if (!path_str)
    {
        SG_LOG(SG_GENERAL, SG_WARN, "No path in " << stringNode->getPath() << "/" << resource << ".");
        return NULL;
    }

    path.append(path_str);
    SG_LOG(SG_GENERAL, SG_INFO, "Reading localized strings for '" <<
           localeNode->getStringValue("lang", "<none>")
           <<"' from " << path.str());

    // load the actual file
    try
    {
        readProperties(path.str(), stringNode->getNode(resource, 0, true));
    } catch (const sg_exception &e)
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "Unable to read the localized strings from " << path.str() <<
               ". Error: " << e.getFormattedMessage());
        return false;
    }

   return true;
}

// Load strings for requested resource (for current and default locale).
// Result is stored below "strings" in the property tree of the locales.
bool
FGLocale::loadResource(const char* resource)
{
    // load defaults first
    bool Ok = loadResource(_defaultLocale, resource);

    // also load language specific resource, unless identical
    if ((_currentLocale!=0)&&
        (_defaultLocale != _currentLocale))
    {
        Ok &= loadResource(_currentLocale, resource);
    }

    return Ok;
}

const char*
FGLocale::getLocalizedString(SGPropertyNode *localeNode, const char* id, const char* context)
{
    SGPropertyNode *n = localeNode->getNode("strings",0, true)->getNode(context);
    if (n)
        return n->getStringValue(id, NULL);
    return NULL;
}

const char*
FGLocale::getLocalizedString(const char* id, const char* resource, const char* Default)
{
    if (id && resource)
    {
        const char* s = NULL;
        if (_currentLocale)
            s = getLocalizedString(_currentLocale, id, resource);
        if (s && s[0]!=0)
            return s;

        if (_defaultLocale)
            s = getLocalizedString(_defaultLocale, id, resource);
        if (s && s[0]!=0)
            return s;
    }
    return Default;
}

simgear::PropertyList
FGLocale::getLocalizedStrings(SGPropertyNode *localeNode, const char* id, const char* context)
{
    SGPropertyNode *n = localeNode->getNode("strings",0, true)->getNode(context);
    if (n)
    {
        return n->getChildren(id);
    }
    return simgear::PropertyList();
}

simgear::PropertyList
FGLocale::getLocalizedStrings(const char* id, const char* resource)
{
    if (id && resource)
    {
        if (_currentLocale)
        {
            simgear::PropertyList s = getLocalizedStrings(_currentLocale, id, resource);
            if (s.size())
                return s;
        }

        if (_defaultLocale)
        {
            simgear::PropertyList s = getLocalizedStrings(_defaultLocale, id, resource);
            if (s.size())
                return s;
        }
    }
    return simgear::PropertyList();
}

// Check for localized font
const char*
FGLocale::getDefaultFont(const char* fallbackFont)
{
    const char* font = NULL;
    if (_currentLocale)
    {
        font = _currentLocale->getStringValue("font", "");
        if (font[0] != 0)
            return font;
    }
    if (_defaultLocale)
    {
        font = _defaultLocale->getStringValue("font", "");
        if (font[0] != 0)
            return font;
    }

    return fallbackFont;
}

// Simple UTF8 to Latin1 encoder.
void FGLocale::utf8toLatin1(string& s)
{
    size_t pos = 0;

    // map '0xc3..' utf8 characters to Latin1
    while ((string::npos != (pos = s.find('\xc3',pos)))&&
           (pos+1 < s.size()))
    {
        char c='*';
        unsigned char p = s[pos+1];
        if ((p>=0x80)&&(p<0xc0))
            c = 0x40 + p;
        char v[2];
        v[0]=c;
        v[1]=0;
        s.replace(pos, 2, v);
        pos++;
    }

#ifdef DEBUG_ENCODING
    printf("'%s': ", s.c_str());
    for (pos = 0;pos<s.size();pos++)
        printf("%02x ", (unsigned char) s[pos]);
    printf("\n");
#endif

    // hack: also map some Latin2 characters to plain-text ASCII
    pos = 0;
    while ((string::npos != (pos = s.find('\xc5',pos)))&&
           (pos+1 < s.size()))
    {
        char c='*';
        unsigned char p = s[pos+1];
        switch(p)
        {
            case 0x82:c='l';break;
            case 0x9a:c='S';break;
            case 0x9b:c='s';break;
            case 0xba:c='z';break;
            case 0xbc:c='z';break;
        }
        char v[2];
        v[0]=c;
        v[1]=0;
        s.replace(pos, 2, v);
        pos++;
    }
}
