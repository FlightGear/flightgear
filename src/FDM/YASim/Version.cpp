#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "Version.hpp"
#include <simgear/debug/logstream.hxx>
#include <iostream>

namespace yasim {
    
static const std::vector<std::string> VersionStrings = {
    "YASIM_VERSION_ORIGINAL",
    "YASIM_VERSION_32",
    "2017.2",
    "2018.1",
    "YASIM_VERSION_CURRENT",
};
    
Version::YASIM_VERSION Version::getByName(const std::string& name)
{
    auto it = std::find(VersionStrings.begin(), VersionStrings.end(), name);
    if (it == VersionStrings.end()) {
        SG_LOG(SG_FLIGHT,SG_ALERT,"Unknown yasim version '" << name << "' ignored, using YASIM_VERSION_ORIGINAL");        
        return YASIM_VERSION_ORIGINAL;
    }
    YASIM_VERSION v = static_cast<YASIM_VERSION>(std::distance(VersionStrings.begin(), it));
    if (v > YASIM_VERSION_CURRENT) return YASIM_VERSION_CURRENT;
    else return v;
}

std::string Version::getName(YASIM_VERSION v)
{
    return VersionStrings.at(static_cast<int>(v));
}

void Version::setVersion( const char * version )
{
    const std::string v(version);
    _version = getByName(v);
    SG_LOG(SG_FLIGHT,SG_ALERT, "This aircraft uses yasim version '" << v << "' (" << _version << ")\n");
}

} // namespace yasim
