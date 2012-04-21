#ifndef GUI_MAPWIDGET_HXX
#define GUI_MAPWIDGET_HXX

#include <map>
#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/props/props.hxx>

#include <plib/pu.h>

#include "dialog.hxx" // for GUI_ID

// forward decls
class FGRouteMgr;
class FGRunway;
class FGAirport;
class FGNavRecord;
class FGFix;
class MapData;
class SGMagVar;

class MapWidget : public puObject
{
public:
  MapWidget(int x, int y, int width, int height);
  virtual ~MapWidget();
  
  virtual void setSize(int width, int height);
  virtual void doHit( int button, int updown, int x, int y ) ;
  virtual void draw( int dx, int dy ) ;
  virtual int checkKey(int key, int updown);
    
  void setProperty(SGPropertyNode_ptr prop);
private:
  int zoom() const;
  
  void handlePan(int x, int y);
  
  void pan(const SGVec2d& delta);
  void zoomIn();
  void zoomOut();
  
  void paintAircraftLocation(const SGGeod& aircraftPos);
  void paintRoute();
  void paintRuler();
  
  void drawGPSData();
  void drawNavRadio(SGPropertyNode_ptr radio);
  void drawTunedLocalizer(SGPropertyNode_ptr radio);
  
  void drawLatLonGrid();
  SGVec2d gridPoint(int ix, int iy);
  bool drawLineClipped(const SGVec2d& a, const SGVec2d& b);
  
  void drawAirports();
  void drawAirport(FGAirport* apt);
  int scoreAirportRunways(FGAirport* apt);
  void drawRunwayPre(FGRunway* rwy);
  void drawRunway(FGRunway* rwy);
  void drawILS(bool tuned, FGRunway* rwy);
  
  void drawNavaids();
  void drawNDB(bool tuned, FGNavRecord* nav);
  void drawVOR(bool tuned, FGNavRecord* nav);
  void drawFix(FGFix* fix);
  
  void drawTraffic();
  void drawAIAircraft(const SGPropertyNode* model, const SGGeod& pos, double hdg);
  void drawAIShip(const SGPropertyNode* model, const SGGeod& pos, double hdg);
  
  void drawData();
  bool validDataForKey(void* key);
  MapData* getOrCreateDataForKey(void* key);
  MapData* createDataForKey(void* key);
  void setAnchorForKey(void* key, const SGVec2d& anchor);
  void clearData();
  
  SGVec2d project(const SGGeod& geod) const;
  SGGeod unproject(const SGVec2d& p) const;
  double currentScale() const;
  
  int displayHeading(double trueHeading) const;
  
  void circleAt(const SGVec2d& center, int nSides, double r);
  void circleAtAlt(const SGVec2d& center, int nSides, double r, double r2);
  void drawLine(const SGVec2d& p1, const SGVec2d& p2);
  void drawLegendBox(const SGVec2d& pos, const std::string& t);
  
  int _width, _height;
  int _cachedZoom;
  double _drawRangeNm;
  double _upHeading; // true heading corresponding to +ve y-axis
  bool _magneticHeadings;
  bool _hasPanned;
  
  SGGeod _projectionCenter;
  bool _orthoAzimuthProject;
  SGGeod _aircraft;
  SGGeod _clickGeod;
  SGVec2d _hitLocation;
  FGRouteMgr* _route;
  SGPropertyNode_ptr _root;
  SGPropertyNode_ptr _gps;
  
  typedef std::map<void*, MapData*> KeyDataMap;
  KeyDataMap _mapData;
  std::vector<MapData*> _dataQueue;
  
  SGMagVar* _magVar;
  
  typedef std::map<int, SGVec2d> GridPointCache;
  GridPointCache _gridCache;
  double _gridSpacing;
  SGGeod _gridCenter;
};

#endif // of GUI_MAPWIDGET_HXX
