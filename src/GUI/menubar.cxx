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

const char*
FGMenuBar::getLocalizedLabel(SGPropertyNode* node)
{
    const char* name = node->getStringValue("name", 0);

    const char* translated = globals->get_locale()->getLocalizedString(name, "menu");
    if (translated)
        return translated;

    // return default
    return node->getStringValue("label");
}

// end of menubar.cxx
