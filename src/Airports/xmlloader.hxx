#ifndef _XML_LOADER_HXX_
#define _XML_LOADER_HXX_

#include <simgear/xml/easyxml.hxx>

class FGAirportDynamics;
class FGRunwayPreference;


class XMLLoader {
public:
  XMLLoader();
  ~XMLLoader();

  static void load(FGRunwayPreference* p);
  static void load(FGAirportDynamics* d);
  
};

#endif
