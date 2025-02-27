// SPDX-FileCopyrightText: (C) 2012  Thorsten Brehm - brehmt (at) gmail com
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileComment: FlightGear Localization Support

#ifndef __FGLOCALE_HXX
#define __FGLOCALE_HXX

#include <cstdarg> // for va_start/_end
#include <string>
#include <vector>

#include <simgear/props/propsfwd.hxx>
#include <simgear/misc/strutils.hxx>

#include <Translations/TranslationDomain.hxx>

// forward decls
class SGPath;

namespace flightgear {
    class GetLocalizedStringsSetup;
    class TranslationDomain;
};

namespace simgear { class Dir; }


///////////////////////////////////////////////////////////////////////////////
// FGLocale  //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 * Main concepts: default translation vs. other translations, etc.
 *
 * The “default translation” is made of all source strings found in the
 * XML files present in $FG_ROOT/Translations/default/ (also in
 * ⟨base dir⟩/Translations/default inside an aircraft or add-on). This
 * translation corresponds to the /sim/intl/default-translation node (see
 * $FG_ROOT/Translations/locale.xml). It does not correspond to any of the
 * /sim/intl/locale[n] nodes.
 *
 * The default translation has only one form for any given string: the “source
 * text” (this is what translators translate). Each of its strings is loaded
 * in the _sourceText member of the corresponding TranslationUnit instance.
 * Therefore, it can't have variations like singular and plural forms. It
 * should use indeterminate style like “Here are %1 apple(s)”. Its language is
 * referred to here as “engineering English”.
 *
 * Other translations are proper ones, that can have different forms depending
 * on an integer that is optionally passed to the API functions
 * (cd. GetLocalizedStringsSetup::setCardinalNumber()). These translations are
 * all loaded from XLIFF files in Translations/⟨language dir⟩ where
 * ⟨language dir⟩ is not 'default' (for aircraft and add-ons, use the most
 * specific variant for ⟨language dir⟩, e.g. 'fr_FR' as opposed to just 'fr':
 * this will allow correct selection of the locale node according to
 * locale.xml). In such translations, each _sourceText of a TranslationUnit
 * instance has:
 *
 *   - no translation if the string hasn't been translated yet (an empty
 *     'target' text in the .xlf file counts as “no translation”);
 *
 *   - exactly one translation (_targetTexts[0] in the TranslationUnit) if the
 *     string has has-plural="false" or no such attribute in the default
 *     translation;
 *
 *   - a language-dependent number of translations that can be obtained by
 *     passing the “language id” (cf. locale.xml) to
 *     LanguageInfo::getNumberOfPluralForms() (these translations are loaded
 *     into the _targetTexts vector of TranslationUnit instances);
 *
 * Language selection
 * ------------------
 *
 * When fgfs is started with --language=default, it uses the default
 * translation (“engineering English”). This is normally only useful for
 * debugging. In this case, FGLocale::selectLanguage() sets _currentLocale to
 * the empty shared pointer. In all other cases, FGLocale::selectLanguage()
 * chooses one of the /sim/intl/locale[n] nodes according to --language, the
 * user-level and system-level language settings, and the
 * /sim/intl/locale[n]/lang nodes defined in locale.xml (if no match is found,
 * it uses the “fallback locale” which corresponds to /sim/intl/locale[0] and
 * is proper English with plural forms). This results in one XLIFF file to be
 * loaded if present (also for the current aircraft and for registered
 * add-ons).
 *
 * When FGLocale::selectLanguage() returns and FlightGear uses the default
 * locale:
 *
 *   - _currentLocale is the empty shared pointer;
 *   - _languageId is the "default" string;
 *   - FlightGear did not and won't load any XLIFF file.
 *
 * When FGLocale::selectLanguage() returns and FlightGear does not use the
 * default locale:
 *
 *   - _currentLocale points to the /sim/intl/locale[n] node corresponding to
 *      the selected language;
 *   - _languageId is the string value of /sim/intl/locale[n]/language-id;
 *   - FlightGear loaded at least one XLIFF file indicated by
 *     /sim/intl/locale[n]/core/xliff (unless none was there: abnormal
 *      situation), and will possibly load more later from the current
 *      aircraft and add-ons.
 *
 * XXX: FGLocale::selectLanguage() sets /sim/intl/current-locale to the value
 * of _currentLocaleString. I think the _languageId value would be more useful
 * for most uses, including locale-dependent font selection (e.g., for the
 * splash screen or Canvas GUI). For instance, if we make no difference
 * betweeen fr_FR, fr_BE, and fr_CA for the sake of translations, all of them
 * are mapped to the same /sim/intl/locale[n] node which is characterized by
 * its 'language-id' value; OTOH, all these “locales” would have their own
 * value of _currentLocaleString, despite using the same XLIFF files (using
 * the plural here because XLIFF files can be loaded from all these domains:
 * 'core', 'current-aircraft' and 'addons/⟨addonId⟩').
 *
 *
 * Resource vs. context
 * --------------------
 *
 * Both terms mean essentially the same thing here. A resource can more easily
 * be considered as something that _contains_ translation material, however
 * when these terms refer to a single string parameter, they mean the same
 * thing: the resource name such as “atc”, “menu”, “options”, “sys”, “tips”,
 * etc.
 ******************************************************************************/

class FGLocale
{
public:
    FGLocale(SGPropertyNode* root);
    virtual ~FGLocale();

    /**
     * Select the locale's primary language according to user-level,
     * system-level language settings and the @p language argument.
     *
     * @param language  locale specification such as fr, fr_FR or fr_FR.UTF-8;
     *                  it takes precedence over system settings (pass an
     *                  empty value if you want it to be ignored). The special
     *                  value 'default' causes FlightGear to use the “default
     *                  translation” (see above).
     *
     * Once this function returns, getLanguageId() is safe to call.
     */
    bool selectLanguage(const std::string& language = {});
    /**
     * Return the value of _languageId, which uniquely identifies the language
     * for the LanguageInfo class (handling of plural forms...).
     */
    std::string getLanguageId() const;

    /** Return the preferred language according to user choice and/or settings.
     *
     *  Examples: 'fr_CA', 'de_DE'... or the empty string if nothing could be
     *            found.
     *
     *  Note that this is not necessarily the same as the last value passed to
     *  selectLanguage(), assuming it was non-empty, because the latter may
     *  have an encoding specifier, while values returned by
     *  getPreferredLanguage() never have that.
     *
     *  XXX getLanguageId() is probably more useful; remove
     *  getPreferredLanguage() or change its semantics?
     */
    std::string getPreferredLanguage() const;

    void loadAircraftTranslations();
    void loadAddonTranslations();

    /**
     * Obtain a single translation with the given identifier, context and index.
     */
    std::string getLocalizedStringWithIndex(const std::string& id,
                                            const std::string& context,
                                            int index) const;
    /**
     * Obtain a single string matching the given id, with fallback.
     * Selected context refers to "menu", "options", "dialog" etc.
     *
     * @param defaultValue  returned if the requested translation is missing
     *                      or empty *and* the default translation (source
     *                      text) is empty.
     *
     * Due to these conditions, this only makes sense when FlightGear uses the
     * default translation and some translatable “strings” have been defined
     * as empty elements (so that their “source text” is empty).
     */
    std::string getLocalizedString(const std::string& id,
                                   const std::string& resource,
                                   const std::string& defaultValue = {});

    /**
      * Obtain a list of translations that share the same tag name (id stem).
      *
      * @param id       name of the tag in the default translation XML file
      * @param resource a string such as "menu", "options", "sys", etc.
      *
      * @return A vector of translated strings
      */
    std::vector<std::string> getLocalizedStrings(const std::string& id,
                                                 const std::string& resource);

    /**
     * Return the number of strings with a given id in the specified context
     *
     * @parmam context  a string such as "menu", "options", "sys", etc.
     */
    std::size_t getLocalizedStringCount(const std::string& id,
                                        const std::string& context) const;

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
        @brief Given a node with children corresponding to different language
               / locale codes, select one based on the user preferred language
     */
    SGPropertyNode_ptr selectLanguageNode(SGPropertyNode* langs) const;

protected:
    /**
     * Find a property node matching the given language.
     */
    SGPropertyNode* findLocaleNode(const std::string& language);

    /**
     * Load default strings for the requested resource ("atc", "menu",  etc.).
     *
     * To avoid confusing unrelated things, translatable strings from the
     * simulator core (FGData), from an add-on or from the current aircraft
     * are all stored in different *domains*. There are three kinds of domains:
     *   - 'core' for strings coming from FGData;
     *   - 'addons/⟨addonId⟩' for strings coming from an add-on;
     *   - 'current-aircraft' for strings coming from the current aircraft.
     */
    void loadResourceForDefaultTranslation(
        const SGPath& xmlFile, const std::string& domain,
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
     * Obtain user's default language settings.
     */
    string_list getUserLanguages() const;
    /**
     * Return the appropriate value for _languageId according to
     * _currentLocale. When _currentLocale isn't the empty shared pointer,
     * alert if the /sim/intl/locale[n]/language-id node doesn't exist.
     */
    std::string findLanguageId() const;

    SGPropertyNode_ptr _intl;
    SGPropertyNode_ptr _currentLocale;
    /**
     * This is used to fetch linguistic data such as the number of plural
     * forms for the selected locale. The value is that of the 'language-id'
     * (first and only) child of the _currentLocale node, except for the
     * default locale which is characterized by _currentLocale == nullptr
     * and _languageId == "default".
     */
    std::string _languageId;
    /**
     * Proper locale (corresponding to a /sim/intl/locale[n] node, as opposed
     * to the default translation) used when none of the
     * /sim/intl/locale[n]/lang nodes matches the --language value or other
     * user language settings. Contrary to the default translation, this is
     * normally proper English with two plural forms.
     */
    SGPropertyNode_ptr _fallbackLocale;
    /**
     * Corresponds to user's language settings, possibly overridden by the
     * --language value. Not sure this is very useful, contrary to
     * _languageId.
     */
    std::string _currentLocaleString;

    /**
     * Load an XLIFF 1.2 file.
     *
     * @param basePath base for the relative path to XLIFF file that is the
     *                 string value of node /sim/intl/locale[n]/⟨domain⟩/xliff.
     * @param localeNode pointer to the /sim/intl/locale[n] node for the
     *                 current locale
     * @param domain   a string such as 'core' or 'addons/⟨addonId⟩'
     */
    void loadXLIFF(const SGPath& basePath, SGPropertyNode* localeNode,
                   const std::string& domain);
private:
    /** Return a new string with the character encoding part of the locale
     *  spec removed., i.e., "de_DE.UTF-8" becomes "de_DE". If there is no
     *  such part, return a copy of the input string.
     */
    static std::string removeEncodingPart(const std::string& locale);
    const flightgear::TranslationDomain* getDomain(const std::string& domain)
        const;

    // this is the ordered list of languages to try. It's the same as
    // returned by getUserLanguages(), except if the user has used
    // --language to override, that will be the first item.

    string_list _languages;
    bool _inited = false;

    // Keys are domain names such as "core", "addons/⟨addonId⟩", etc.
    using DomainsMap = std::map<std::string, flightgear::TranslationDomain>;
    DomainsMap _domains;

    // GetLocalizedStringsSetup uses our getDomain(), which is private.
    friend class flightgear::GetLocalizedStringsSetup;
};

// global translation wrappers

std::string fgTrMsg(const char* key);
std::string fgTrPrintfMsg(const char* key, ...);


#endif // __FGLOCALE_HXX
