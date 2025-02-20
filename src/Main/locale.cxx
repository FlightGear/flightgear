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

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include "fg_props.hxx"
#include "locale.hxx"

#include <Add-ons/AddonManager.hxx>
#include <Add-ons/AddonMetadataParser.hxx>
#include <Translations/DefaultTranslationParser.hxx>
#include <Translations/XLIFFParser.hxx>

using std::string;
using std::vector;
namespace strutils = simgear::strutils;

using flightgear::DefaultTranslationParser;
using flightgear::TranslationResource;
using flightgear::addons::Addon;
using flightgear::TranslationDomain;

FGLocale::FGLocale(SGPropertyNode* root) :
	_intl(root->getNode("/sim/intl", 0, true)),
	_fallbackLocale(_intl->getChild("locale", 0, true))
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
    bool result = true;
    _domains.clear();
    // Default translation for 'atc', 'menu', 'options', etc.
    loadCoreResourcesForDefaultTranslation();

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
    // Record the current locale at /sim/intl/current-locale
    _intl->getChild("current-locale", 0, true)
         ->setStringValue(_currentLocaleString);

    _currentLocale.reset();

    if (_currentLocaleString != "default") {
        for (const string& lang : _languages) {
            SG_LOG(SG_GENERAL, SG_DEBUG,
                   "Trying to find locale for '" << lang << "'");
            _currentLocale = findLocaleNode(lang);

            if (_currentLocale) {
                SG_LOG(SG_GENERAL, SG_DEBUG,
                       "Found locale for '" << lang << "' at "
                       << _currentLocale->getPath());
                break;
            }
        }
    }

    if (_currentLocale) {
        if (_currentLocale->getNode("core", 0, true)->hasChild("xliff")) {
            // Load translation for the selected locale
            loadXLIFF(globals->get_fg_root(), _currentLocale, "core");
        }
    } else if (_currentLocaleString == "default") {
        SG_LOG(SG_GENERAL, SG_INFO,
               "Using the default translation (“engineering English”).");
    } else {
        SG_LOG(SG_GENERAL, SG_WARN,
               "System locale not found or no internationalization settings "
               "specified in defaults.xml. Using the fallback translation "
               "(English).");
        _currentLocale = _fallbackLocale;
        assert(_currentLocale != nullptr);
        result = false;
    }

    // From this point on, if (_currentLocale == nullptr), it means
    // --language=default was passed: the user wants “engineering English”,
    // so we won't load any XLIFF file (including from aircraft or add-ons).
    _inited = true;
    return result;
}

void FGLocale::loadCoreResourcesForDefaultTranslation()
{
    for (const string resource : {
            "atc", "menu", "options", "sys", "tips", "weather-scenarios"}) {
        const auto n = _intl->getChild("default-translation", 0, true);

        const auto resourceNode = n->getNode(resource);
        if (!resourceNode) {
            SG_LOG(SG_GENERAL, SG_ALERT, "No child node '" << resource << "' of "
                   << n->getPath() << "; presumably, FGData is not up-to-date.");
            return;
        }

        const string pathStr = resourceNode->getStringValue();
        if (pathStr.empty()) {
            SG_LOG(SG_GENERAL, SG_ALERT, "No path in " << resourceNode->getPath()
                   << " for resource '" << resource << "'.");
            return;
        }

        loadResourceForDefaultTranslation(globals->get_fg_root() / pathStr,
                                          "core", resource);
    }
}

void FGLocale::loadAircraftTranslations()
{
    loadResourcesFromAircraftOrAddonDir(fgGetString("/sim/aircraft-dir"),
                                        "aircraft");
}

void FGLocale::loadAddonTranslations()
{
    const auto& addonManager = flightgear::addons::AddonManager::instance();
    if (addonManager) {
        for (const Addon* addon : addonManager->registeredAddons()) {
            const string domain = "addons/" + addon->getId();
            loadResourcesFromAircraftOrAddonDir(addon->getBasePath(), domain);
        }
    } else {
        SG_LOG(SG_GENERAL, SG_WARN,
               "FGLocale: not loading add-on translations: AddonManager "
               "instance not found");
    }
}

void FGLocale::loadResourcesFromAircraftOrAddonDir(const SGPath& basePath,
                                                   const string& domain)
{
    const simgear::Dir d = simgear::Dir(basePath / "Translations" / "default");

    if (d.exists()) {
        loadDefaultTranslationFromAircraftOrAddonDir(d, domain);
    }

    if (_currentLocale != nullptr) { // if not “engineering English”
        loadXLIFFFromAircraftOrAddonDir(basePath, domain);
    }
}

void FGLocale::loadDefaultTranslationFromAircraftOrAddonDir(
    const simgear::Dir& defaultTranslationDir, const string& domain)
{
    const auto xmlFiles = defaultTranslationDir.children(
        simgear::Dir::TYPE_FILE | simgear::Dir::NO_DOT_OR_DOTDOT, ".xml");

    for (const SGPath& file : xmlFiles) {
        loadResourceForDefaultTranslation(file, domain, file.file_base());
    }
}

void FGLocale::loadXLIFFFromAircraftOrAddonDir(const SGPath& basePath,
                                               const string& domain)
{
    const simgear::Dir translDir = simgear::Dir(basePath / "Translations");
    if (!translDir.exists()) {
        return;
    }

    const auto subdirs = translDir.children(
        simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT);

    assert(_currentLocale != nullptr);
    const auto langNodes = _currentLocale->getChildren("lang");
    vector<string> foundSubdirs;

    for (const SGPath& subdir : subdirs) {
        const string name = subdir.file(); // name of subdir of 'Translations'
        if (name == "default") {
            continue;
        }

        for (const auto& n : langNodes) {
            if (n->getStringValue() != name) {
                continue;
            }

            // Subdir 'name' matches the current locale; check if we didn't
            // already find one before.
            foundSubdirs.push_back(name);

            if (foundSubdirs.size() > 1) {
                SG_LOG(SG_GENERAL, SG_WARN,
                       "Found several matching subdirectories of '"
                       << translDir.path().utf8Str() <<
                       "' for the current locale ("
                       << foundSubdirs[0] << ", " << foundSubdirs[1] <<
                       "). Incorrect Translations/locale.xml setup?");
                return;
            }

            // Load the XLIFF file
            SGPropertyNode* xliffNode = _currentLocale->getNode(
                domain + "/xliff", 0, true);
            xliffNode->setStringValue(
                "Translations/"  + name + "/FlightGear-nonQt.xlf");
            loadXLIFF(basePath, _currentLocale, domain);
        }
    }
}

void FGLocale::clear()
{
    _inited = false;
    _currentLocaleString.clear();
    _languages.clear();
    _domains.clear();

    if (_currentLocale) {
        _currentLocale->removeChild("aircraft");
        _currentLocale->removeChild("addons");
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

void FGLocale::loadXLIFF(const SGPath& basePath, SGPropertyNode* localeNode,
                         const string& domain)
{
    SGPropertyNode* domainNode = localeNode->getNode(domain, 0, true);
    const string relPath = domainNode->getStringValue("xliff");
    const SGPath xliffPath = basePath / relPath;
    const string languageId = localeNode->getStringValue("id");

    if (!xliffPath.exists()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "No XLIFF file at " << xliffPath);
    } else if (languageId.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Unable to load " << xliffPath
               << ": empty or missing subnode 'id' of "
               << localeNode->getPath());
    } else {
        SG_LOG(SG_GENERAL, SG_INFO, "Loading XLIFF file at " << xliffPath);
        try {
            flightgear::XLIFFParser visitor(languageId, &_domains[domain]);
            readXML(xliffPath, visitor);
        } catch (sg_io_exception& ex) {
            SG_LOG(SG_GENERAL, SG_WARN, "failure parsing XLIFF: " << xliffPath
                   << "\n\t" << ex.getMessage() << "\n\tat: "
                   << ex.getLocation().asString());
        } catch (sg_exception& ex) {
            SG_LOG(SG_GENERAL, SG_WARN, "failure parsing XLIFF: " << xliffPath
                   << "\n\t" << ex.getMessage());
        }
    }
}

void FGLocale::loadResourceForDefaultTranslation(
    const SGPath& xmlFile, const std::string& domain,
    const std::string& resource)
{
    // Automatically create the domain and resource if necessary
    auto resourcePtr = _domains[domain].getResourceCreate(resource);
    DefaultTranslationParser visitor(resourcePtr.get());

    SG_LOG(SG_GENERAL, SG_INFO, "Reading the default translation for " <<
           domain << "/" << resource << " from '" << xmlFile.utf8Str() << "'");

    try {
        readXML(xmlFile, visitor);
    } catch (const sg_io_exception& ex) {
        SG_LOG(SG_GENERAL, SG_WARN, "error parsing default translation from '"
               << xmlFile.utf8Str() << "':\n\t" << ex.getMessage()
               << "\n\tat: " << ex.getLocation().asString());
    } catch (const sg_exception& ex) {
        SG_LOG(SG_GENERAL, SG_WARN, "error parsing default translation from '"
               << xmlFile.utf8Str() << "':\n\t" << ex.getMessage());
    }
}

std::string
FGLocale::getLocalizedStringWithIndex(const string& id, const string& context,
                                      int index) const
{
    const string domainName = "core";
    auto elt = _domains.find(domainName);

    if (elt == _domains.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "While trying to retrieve a translation for " << context <<
               '/' << id << ':' << index << ": domain '" << domainName <<
               "' was not found");
        return {};
    } else {
        const auto resource = elt->second.getResource(context);
        return resource->getTranslation(id, index, 0); // XXX plural form index
    }
}

std::string
FGLocale::getLocalizedString(const string& id, const string& resource,
                             const std::string& defaultValue)
{
    assert(_inited);
    const std::string s = getLocalizedStringWithIndex(id, resource, 0);
    return (s.empty()) ? defaultValue : s;
}

vector<string>
FGLocale::getLocalizedStrings(const string& id, const string& context)
{
    const string domainName = "core";
    auto elt = _domains.find(domainName);

    if (elt == _domains.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "While trying to retrieve all translations for " << context <<
               '/' << id << ": domain '" << domainName << "' was not found");
        return {};
    } else {
        const auto resource = elt->second.getResource(context);
        return resource->getTranslations(id);
    }
}

std::size_t FGLocale::getLocalizedStringCount(const string& id,
                                              const string& context) const
{
    assert(_inited);

    const string domainName = "core";
    auto elt = _domains.find(domainName);

    if (elt == _domains.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "While trying to find the number of strings in " << context <<
               '/' << id << ": domain '" << domainName << "' was not found");
        return std::size_t(0);
    } else {
        const auto resource = elt->second.getResource(context);
        return resource->getNumberOfStringsWithId(id);
    }
}

// Check for localized font
std::string
FGLocale::getDefaultFont(const char* fallbackFont)
{
    assert(_inited);
    std::string font;
    if (_currentLocale)
    {
        font = _currentLocale->getStringValue("font", "");
        if (!font.empty())
            return font;
    }

    if (_fallbackLocale)
    {
        font = _fallbackLocale->getStringValue("font", "");
        if (!font.empty())
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
    ::vsnprintf(buf, len, format.c_str(), args);
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
