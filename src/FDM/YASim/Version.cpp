#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "Version.hpp"
#include <simgear/debug/logstream.hxx>
#include <string>

namespace yasim {
void Version::setVersion( const char * version )
{
  const std::string v(version);
  
  if( v ==  "YASIM_VERSION_ORIGINAL" ) {
    _version = YASIM_VERSION_ORIGINAL;
  } else if( v == "YASIM_VERSION_32" ) {
    _version = YASIM_VERSION_32;
  } else if( v == "YASIM_VERSION_CURRENT" ) {
    _version = YASIM_VERSION_CURRENT;
  } else {
    SG_LOG(SG_FLIGHT,SG_ALERT,"unknown yasim version '" << version << "' ignored, using YASIM_VERSION_ORIGINAL");
  }
}

} // namespace yasim
