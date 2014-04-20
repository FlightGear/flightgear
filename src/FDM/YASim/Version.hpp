#ifndef _VERSION_HPP
#define _VERSION_HPP

namespace yasim {

class Version {
public:
  Version() : _version(YASIM_VERSION_ORIGINAL) {}
  virtual ~Version() {}

  typedef enum {
    YASIM_VERSION_ORIGINAL = 0,
    YASIM_VERSION_32,
    YASIM_VERSION_CURRENT = YASIM_VERSION_32
  } YASIM_VERSION;

  void setVersion( const char * version );
  bool isVersion( YASIM_VERSION version );
  bool isVersionOrNewer( YASIM_VERSION version );

private:
  YASIM_VERSION _version;
};

inline bool Version::isVersion( YASIM_VERSION version )
{
  return _version == version;
}

inline bool Version::isVersionOrNewer( YASIM_VERSION version )
{
  return _version >= version;
}


}; // namespace yasim
#endif // _WING_HPP
