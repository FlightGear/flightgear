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
    if (!node)
        return {};

    const char* name = node->getStringValue("name", 0);
    const auto translated = globals->get_locale()->getLocalizedString(name, "menu");
    if (!translated.empty())
        return translated;

    // return default with fallback to name
    const char* l = node->getStringValue("label", name);

    // this can occur if the menu item is missing a <name>
    if (l == nullptr) {
        SG_LOG(SG_GUI, SG_ALERT, "FGMenuBar::getLocalizedLabel: No <name> defined for:" << node->getPath());
        return string{"<unnamed>"};
    }

    return string{l};
}

// end of menubar.cxx
