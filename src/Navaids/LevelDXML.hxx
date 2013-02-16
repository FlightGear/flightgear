#ifndef FG_NAV_LEVELDXML_HXX
#define FG_NAV_LEVELDXML_HXX

class FGAirport;
class SGPath;

#include <simgear/xml/easyxml.hxx>
#include <simgear/misc/sg_path.hxx>
#include <Navaids/procedure.hxx>

namespace flightgear
{

class NavdataVisitor : public XMLVisitor {
public:
  NavdataVisitor(FGAirport* aApt, const SGPath& aPath);

protected:
  virtual void startXML (); 
  virtual void endXML   ();
  virtual void startElement (const char * name, const XMLAttributes &atts);
  virtual void endElement (const char * name);
  virtual void data (const char * s, int len);
  virtual void pi (const char * target, const char * data);
  virtual void warning (const char * message, int line, int column);
  virtual void error (const char * message, int line, int column);

private:
  Waypt* buildWaypoint(RouteBase* owner);
  void processRunways(ArrivalDeparture* aProc, const XMLAttributes &atts);
 
  void finishApproach();
  void finishSid();
  void finishStar();
  
  FGAirport* _airport;
  SGPath _path;
  std::string _text; ///< last element text value
  
  SID* _sid;
  STAR* _star;
  Approach* _approach;
  Transition* _transition;
  Procedure* _procedure;
  
  WayptVec _waypoints; ///< waypoint list for current approach/sid/star
  WayptVec _transWaypts; ///< waypoint list for current transition
  
  std::string _wayptName;
  std::string _wayptType;
  std::string _ident; // id of segment under construction
  std::string _transIdent;
  double _longitude, _latitude, _altitude, _speed;
  RouteRestriction _altRestrict;
  
  double _holdRadial; // inbound hold radial, or -1 if radial is 'inbound'
  double _holdTD; ///< hold time (seconds) or distance (nm), based on flag below
  bool _holdRighthanded;
  bool _holdDistance; // true, TD is distance in nm; false, TD is time in seconds
  
  double _course, _radial, _dmeDistance;
};

}

#endif