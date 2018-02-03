#ifndef _VERSION_HPP
#define _VERSION_HPP

#include "yasim-common.hpp"

namespace yasim {

class Version {
public:
  Version() : _version(YASIM_VERSION_ORIGINAL) {}
  virtual ~Version() {}

  enum YASIM_VERSION {
    YASIM_VERSION_ORIGINAL = 0,
    YASIM_VERSION_32,
    YASIM_VERSION_2017_2,
    YASIM_VERSION_2018_1,
    YASIM_VERSION_CURRENT = YASIM_VERSION_2018_1
  } ;

  void setVersion( const char * version );
  int getVersion() const { return _version; }
  bool isVersion( YASIM_VERSION version ) const;
  bool isVersionOrNewer( YASIM_VERSION version ) const;
  static YASIM_VERSION getByName(const std::string& name);
  static std::string getName(YASIM_VERSION v);
private:
  YASIM_VERSION _version;
};

inline bool Version::isVersion( YASIM_VERSION version ) const
{
  return _version == version;
}

inline bool Version::isVersionOrNewer( YASIM_VERSION version ) const
{
  return _version >= version;
}


}; // namespace yasim
#endif // _WING_HPP
