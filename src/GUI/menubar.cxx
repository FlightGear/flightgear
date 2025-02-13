#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include "menubar.hxx"
#include <Main/locale.hxx>
#include <Main/globals.hxx>

FGMenuBar::FGMenuBar()
{
}

FGMenuBar::~FGMenuBar ()
{
}

std::string
FGMenuBar::getLocalizedLabel(SGPropertyNode* node)
{
    if (!node)
        return {};

    std::string name = node->getStringValue("name", ""); 
    if (!name.empty()) {
        const auto translated = globals->get_locale()->getLocalizedString(name, "menu");
        if (!translated.empty())
            return translated;
    }

    // return default with fallback to name
    std::string label = node->getStringValue("label", name.c_str());

    // this can occur if the menu item is missing a <name>
    if (label.empty()) {
        SG_LOG(SG_GUI, SG_ALERT, "FGMenuBar::getLocalizedLabel: No <name> defined for:" << node->getPath());
        return std::string{"<unnamed>"};
    }

    return label;
}

// end of menubar.cxx
