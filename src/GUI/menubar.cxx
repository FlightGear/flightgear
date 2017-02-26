#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include "menubar.hxx"
#include <Main/locale.hxx>
#include <Main/globals.hxx>

FGMenuBar::FGMenuBar()
{
    // load locale's menu resources (default and current language)
    globals->get_locale()->loadResource("menu");
}

FGMenuBar::~FGMenuBar ()
{
}

std::string
FGMenuBar::getLocalizedLabel(SGPropertyNode* node)
{
    const char* name = node->getStringValue("name", 0);

    std::string translated = globals->get_locale()->getLocalizedString(name, "menu");
    if (!translated.empty())
        return translated;

    // return default with fallback to name
    return node->getStringValue("label", name);
}

// end of menubar.cxx
