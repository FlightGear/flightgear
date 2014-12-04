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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <cstdio>
#include <boost/foreach.hpp>

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
string_list
FGLocale::getUserLanguage()
{
    string_list result;
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
        {
            result.push_back(locale);
            return result;
        }
        
        locale[--i] = 0;
        SG_LOG(SG_GENERAL, SG_WARN, "Failed to detected locale's country setting.");
        result.push_back(locale);
        return result;
    }

    return result;
}
#elif __APPLE__
//  implemented in CocoaHelpers.mm
#else
/**
 * Determine locale/language settings on Linux/Unix.
 */
string_list
FGLocale::getUserLanguage()
{
    string_list result;
    const char* langEnv = ::getenv("LANG");
    if (langEnv) {
        result.push_back(langEnv);
    }
    
    return result;
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
    string_list languages = getUserLanguage();
    if (languages.empty()) {
        // Use plain C locale if nothing is available.
        SG_LOG(SG_GENERAL, SG_WARN, "Unable to detect system language" );
        languages.push_back("C");
    }
    
    // if we were passed a language option, try it first
    if ((language != NULL) && (strlen(language) > 0)) {
        languages.insert(languages.begin(), string(language));
    }

    
    _currentLocale = NULL;
    BOOST_FOREACH(string lang, languages) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "trying to find locale for " << lang );
        _currentLocale = findLocaleNode(lang);
        if (_currentLocale) {
            SG_LOG(SG_GENERAL, SG_DEBUG, "found locale for " << lang << " at " << _currentLocale->getPath() );
            break;
        }
    }
    
    // load resource for system messages (translations for fgfs internal messages)
    loadResource("sys");

    // load resource for atc messages
    loadResource("atc");

    if (!_currentLocale)
    {
       SG_LOG(SG_GENERAL, SG_ALERT,
              "System locale not found or no internationalization settings specified in preferences.xml. Using default (en)." );
       return false;
    }

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
        return false;
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
            if (! s.empty())
                return s;
        }

        if (_defaultLocale)
        {
            simgear::PropertyList s = getLocalizedStrings(_defaultLocale, id, resource);
            if (! s.empty())
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

std::string FGLocale::localizedPrintf(const char* id, const char* resource, ... )
{
    va_list args;
    va_start(args, resource);
    string r = vlocalizedPrintf(id, resource, args);
    va_end(args);
    return r;
}

std::string FGLocale::vlocalizedPrintf(const char* id, const char* resource, va_list args)
{
    const char* format = getLocalizedString(id, resource);
    int len = ::vsprintf(NULL, format, args);
    char* buf = (char*) alloca(len);
    ::vsprintf(buf, format, args);
    return std::string(buf);
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

const char* fgTrMsg(const char* key)
{
    return globals->get_locale()->getLocalizedString(key, "message");
}

std::string fgTrPrintfMsg(const char* key, ...)
{
    va_list args;
    va_start(args, key);
    string r = globals->get_locale()->vlocalizedPrintf(key, "message", args);
    va_end(args);
    return r;
    
}
