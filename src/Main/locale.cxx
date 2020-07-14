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

#include <config.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <cstdio>
#include <cstring>              // std::strlen()
#include <cstddef>              // std::size_t
#include <cassert>

#include <simgear/misc/strutils.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include "fg_props.hxx"
#include "locale.hxx"
#include "XLIFFParser.hxx"

#include <Add-ons/AddonMetadataParser.hxx>

using std::vector;
using std::string;
namespace strutils = simgear::strutils;

FGLocale::FGLocale(SGPropertyNode* root) :
	_intl(root->getNode("/sim/intl", 0, true)),
	_defaultLocale(_intl->getChild("locale", 0, true))
{
}

FGLocale::~FGLocale()
{
}

// Static method
string FGLocale::removeEncodingPart(const string& locale)
{
    string res;
    std::size_t pos = locale.find('.');

    if (pos != string::npos)
    {
        assert(pos > 0);
        res = locale.substr(0, pos);
    } else {
        res = locale;
    }

    return res;
}

string removeLocalePart(const string& locale)
{
    std::size_t pos = locale.find('_');
    if (pos == string::npos) {
        pos = locale.find('-');
    }

    if (pos == string::npos)
        return {};

    return locale.substr(0, pos);
}

#ifdef _WIN32


string_list
FGLocale::getUserLanguages()
{
	unsigned long bufSize = 128;
	wchar_t* localeNameBuf = reinterpret_cast<wchar_t*>(alloca(bufSize));
	unsigned long numLanguages = 0;

	bool ok = GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages,
		localeNameBuf, &bufSize);
	if (!ok) {
		// if we have a lot of languages, can fail, allocate a bigger
		// buffer
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			bufSize = 0;
			GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages,
				nullptr, &bufSize);
			localeNameBuf = reinterpret_cast<wchar_t*>(alloca(bufSize));
			ok = GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages,
				localeNameBuf, &bufSize);
		}
	}

	if (!ok) {
		SG_LOG(SG_GENERAL, SG_WARN, "Failed to detected user locale via GetUserPreferredUILanguages");
		return{};
	}

	string_list result;
	result.reserve(numLanguages);
	for (unsigned int l = 0; l < numLanguages; ++l) {
		std::wstring ws(localeNameBuf);
		if (ws.empty())
			break;

		// skip to next string, past this string and trailing NULL
		localeNameBuf += (ws.size() + 1);
		result.push_back(simgear::strutils::convertWStringToUtf8(ws));
		SG_LOG(SG_GENERAL, SG_INFO, "User langauge " << l << ":" << result.back());
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
FGLocale::getUserLanguages()
{
    string_list result;
    const char* langEnv = ::getenv("LANG");

    if (langEnv) {
        // Remove character encoding from the locale spec, i.e. "de_DE.UTF-8"
        // becomes "de_DE". This is for consistency with the Windows and MacOS
        // implementations of this method.
        result.push_back(removeEncodingPart(langEnv));
    }

    return result;
}
#endif

// Search property tree for matching locale description
SGPropertyNode*
FGLocale::findLocaleNode(const string& localeSpec)
{
    SGPropertyNode* node = nullptr;
    // Remove the character encoding part of the locale spec, i.e.,
    // "de_DE.utf8" => "de_DE"
    string language = removeEncodingPart(localeSpec);

    SG_LOG(SG_GENERAL, SG_DEBUG,
           "Searching language resource for locale: '" << language << "'");
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
    const auto justTheLanguage = removeLocalePart(language);
    if (!justTheLanguage.empty()) {
        node = findLocaleNode(justTheLanguage);
        if (node)
            return node;
    }

    return nullptr;
}

// Select the language. When no language is given (nullptr),
// a default is determined matching the system locale.
bool FGLocale::selectLanguage(const std::string& language)
{
    _languages = getUserLanguages();
    if (_languages.empty()) {
        // Use plain C locale if nothing is available.
        SG_LOG(SG_GENERAL, SG_WARN, "Unable to detect system language" );
        _languages.push_back("C");
    }

    // if we were passed a language option, try it first
    if (!language.empty()) {
        const auto normalizedLang = strutils::replace(language, "-", "_");
        _languages.insert(_languages.begin(), normalizedLang);
    }

    _currentLocaleString = removeEncodingPart(_languages.front());
    if (_currentLocaleString == "C") {
        _currentLocaleString.clear();
    }

    _currentLocale = nullptr;

    for (const string& lang : _languages) {
        SG_LOG(SG_GENERAL, SG_DEBUG,
               "Trying to find locale for '" << lang << "'");
        _currentLocale = findLocaleNode(lang);

        if (_currentLocale) {
            SG_LOG(SG_GENERAL, SG_DEBUG,
                   "Found locale for '" << lang << "' at '" <<
                   _currentLocale->getPath() << "'");
            break;
        }
    }

    if (_currentLocale && _currentLocale->hasChild("xliff")) {
        parseXLIFF(_currentLocale);
    }
   
    
    // load resource for system messages (translations for fgfs internal messages)
    loadResource("sys");
    loadResource("atc");
    loadResource("tips");
    loadResource("weather-scenarios");

    _inited = true;
    if (!_currentLocale && !_currentLocaleString.empty()) {
        SG_LOG(SG_GENERAL, SG_WARN,
               "System locale not found or no internationalization settings specified in defaults.xml. Using default (en).");
        return false;
    }

    return true;
}

void FGLocale::clear()
{
    _inited = false;
    _currentLocaleString.clear();
    _languages.clear();

    if (_currentLocale && (_currentLocale != _defaultLocale)) {
        // remove loaded strings, so we don't duplicate
        _currentLocale->removeChild("strings");
    }

    _currentLocale.clear();
}

// Return the preferred language according to user choice and/or settings
// (e.g., 'fr_FR', or the empty string if nothing could be found).
std::string
FGLocale::getPreferredLanguage() const
{
    return _currentLocaleString;
}

void FGLocale::parseXLIFF(SGPropertyNode* localeNode)
{
    SGPath path(globals->get_fg_root());
    SGPath xliffPath = path / localeNode->getStringValue("xliff");
    if (!xliffPath.exists()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "No XLIFF file at " << xliffPath);
    } else {
        SG_LOG(SG_GENERAL, SG_INFO, "Loading XLIFF file at " << xliffPath);
        SGPropertyNode_ptr stringNode = localeNode->getNode("strings", 0, true);
        try {
            flightgear::XLIFFParser visitor(stringNode);
            readXML(xliffPath, visitor);
        } catch (sg_io_exception& ex) {
            SG_LOG(SG_GENERAL, SG_WARN, "failure parsing XLIFF: " << path <<
                   "\n\t" << ex.getMessage() << "\n\tat:" << ex.getLocation().asString());
        } catch (sg_exception& ex) {
            SG_LOG(SG_GENERAL, SG_WARN, "failure parsing XLIFF: " << path <<
                   "\n\t" << ex.getMessage());
        }
    }
}

// Load strings for requested resource and locale.
// Result is stored below "strings" in the property tree of the given locale.
bool
FGLocale::loadResource(SGPropertyNode* localeNode, const char* resource)
{
    SGPath path( globals->get_fg_root() );

    // already loaded
    if (localeNode->hasChild("xliff")) {
        return true;
    }
    
    SGPropertyNode* stringNode = localeNode->getNode("strings", 0, true);
    SGPropertyNode* resourceNode = stringNode->getNode(resource);

    if (!resourceNode)
        return false;

    if (resourceNode->getBoolValue("__loaded")) {
        // already loaded previously
        return true;
    }

    const char* path_str = resourceNode->getStringValue();
    if (!path_str)
    {
        SG_LOG(SG_GENERAL, SG_WARN, "No path in " << stringNode->getPath() << "/" << resource << ".");
        return false;
    }

    path.append(path_str);
    SG_LOG(SG_GENERAL, SG_INFO, "Reading localized strings for '" <<
           localeNode->getStringValue("lang", "<none>")
           <<"' from " << path);

    // load the actual file
    try
    {
        readProperties(path, resourceNode);
    } catch (const sg_exception &e)
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "Unable to read the localized strings from " << path <<
               ". Error: " << e.getFormattedMessage());
        return false;
    }

    // set marker to indicate the data has been loaded
    resourceNode->setBoolValue("__loaded", true);
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
    if ((_currentLocale != nullptr) && (_defaultLocale != _currentLocale))
    {
        Ok &= loadResource(_currentLocale, resource);
    }

    return Ok;
}

std::string
FGLocale::innerGetLocalizedString(SGPropertyNode* localeNode, const char* id, const char* context, int index) const
{
    SGPropertyNode *n = localeNode->getNode("strings",0, true)->getNode(context);
    if (!n) {
        return std::string();
    }

    n = n->getNode(id, index);
    if (n && n->hasValue()) {
        return std::string(n->getStringValue());
    }

    return std::string();
}

std::string
FGLocale::getLocalizedString(const std::string& id, const char* resource, const std::string& defaultValue)
{
    return getLocalizedString(id.c_str(), resource, defaultValue.c_str());
}

std::string
FGLocale::getLocalizedString(const char* id, const char* resource, const char* Default)
{
    assert(_inited);
    if (id && resource)
    {
        std::string s;
        if (_currentLocale) {
            s = innerGetLocalizedString(_currentLocale, id, resource, 0);
            if (!s.empty()) {
                return s;
            }
        }

        if (_defaultLocale) {
            s = innerGetLocalizedString(_defaultLocale, id, resource, 0);
            if (!s.empty()) {
                return s;
            }
        }
    }

    return (Default == nullptr) ? std::string() : std::string(Default);
}

std::string
FGLocale::getLocalizedStringWithIndex(const char* id, const char* resource, unsigned int index) const
{
    assert(_inited);
    if (id && resource) {
        std::string s;
        if (_currentLocale) {
            s = innerGetLocalizedString(_currentLocale, id, resource, index);
            if (!s.empty()) {
                return s;
            }
        }

        if (_defaultLocale) {
            s = innerGetLocalizedString(_defaultLocale, id, resource, index);
            if (!s.empty()) {
                return s;
            }
        }
    }

    return std::string();
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

size_t FGLocale::getLocalizedStringCount(const char* id, const char* resource) const
{
    assert(_inited);
    if (_currentLocale) {
        SGPropertyNode* resourceNode = _currentLocale->getNode("strings",0, true)->getNode(resource);
        if (resourceNode) {
            const size_t count = resourceNode->getChildren(id).size();
            if (count > 0) {
                return count;
            }
        }
    }

    if (_defaultLocale) {
        auto resourceNode = _defaultLocale->getNode("strings", 0, true)->getNode(resource);
        if (!resourceNode)
            return 0;

        size_t count = resourceNode->getChildren(id).size();
        if (count > 0) {
            return count;
        }
    }

    return 0;
}

simgear::PropertyList
FGLocale::getLocalizedStrings(const char* id, const char* resource)
{
    assert(_inited);
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
    assert(_inited);
    const char* font = nullptr;
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
    assert(_inited);
    std::string format = getLocalizedString(id, resource);
    int len = ::vsnprintf(nullptr, 0, format.c_str(), args);
    char* buf = (char*) alloca(len);
    ::vsprintf(buf, format.c_str(), args);
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

std::string fgTrMsg(const char* key)
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

SGPropertyNode_ptr FGLocale::selectLanguageNode(SGPropertyNode* langs) const
{
    if (!langs)
        return {};

    for (auto l : _languages) {
        // Only accept the hyphen separator in PropertyList node names between
        // language and territory
        const auto langNoEncoding = strutils::replace(removeEncodingPart(l),
                                                      "_", "-");
        if (langs->hasChild(langNoEncoding)) {
            return langs->getChild(langNoEncoding);
        }

        const auto justLang = removeLocalePart(langNoEncoding);
        if (langs->hasChild(justLang)) {
            return langs->getChild(justLang);
        }
    }

    return {};
}
