#ifndef GUI_MAPWIDGET_HXX
#define GUI_MAPWIDGET_HXX

#include <map>
#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/props/props.hxx>

#include <Navaids/positioned.hxx>
#include <plib/pu.h>

#include "FGPUIDialog.hxx"

// forward decls
class FGRouteMgr;
class FGRunway;
class FGHelipad;
class FGAirport;
class FGNavRecord;
class FGFix;
class MapData;
class SGMagVar;

typedef std::vector<SGGeod> SGGeodVec;

class MapWidget : public puObject, public FGPUIDialog::ActiveWidget
{
public:
  MapWidget(int x, int y, int width, int height);
  virtual ~MapWidget();
  
    // puObject over-rides
  virtual void setSize(int width, int height);
  virtual void doHit( int button, int updown, int x, int y ) ;
  virtual void draw( int dx, int dy ) ;
  virtual int checkKey(int key, int updown);
    
  void setProperty(SGPropertyNode_ptr prop);
    
    // PUIDialog::ActiveWidget over-rides
    virtual void update();
    
private:
    enum Projection {
        PROJECTION_SAMSON_FLAMSTEED,
        PROJECTION_AZIMUTHAL_EQUIDISTANT,
        PROJECTION_ORTHO_AZIMUTH,
        PROJECTION_SPHERICAL
    };
    
  int zoom() const;
  
  void handlePan(int x, int y);
  
  void pan(const SGVec2d& delta);
  void zoomIn();
  void zoomOut();
  
  void paintAircraftLocation(const SGGeod& aircraftPos);
  void paintRoute();
  void paintRuler();
  void drawFlightHistory();
  
  void drawGPSData();
  void drawNavRadio(SGPropertyNode_ptr radio);
  void drawTunedLocalizer(SGPropertyNode_ptr radio);
  
  void drawLatLonGrid();
  SGVec2d gridPoint(int ix, int iy);
  bool drawLineClipped(const SGVec2d& a, const SGVec2d& b);
  
  void drawAirport(FGAirport* apt);
  int scoreAirportRunways(FGAirport* apt);
  void drawRunwayPre(FGRunway* rwy);
  void drawRunway(FGRunway* rwy);
  void drawHelipad(FGHelipad* hp);
  void drawILS(bool tuned, FGRunway* rwy);
  
  void drawPositioned();
  void drawNDB(bool tuned, FGNavRecord* nav);
  void drawVOR(bool tuned, FGNavRecord* nav);
  void drawFix(FGFix* fix);

  void drawPOI(FGPositioned* rec);
    
  void drawTraffic();
    
    class DrawAIObject
    {
    public:
        DrawAIObject(SGPropertyNode* model, const SGGeod& g);
        
        SGPropertyNode* model;
        bool boat;
        SGGeod pos;
        double heading;
        int speedKts;
        std::string label;
        std::string legend;
    };
    
    typedef std::vector<DrawAIObject> AIDrawVec;
    AIDrawVec _aiDrawVec;
    
    void updateAIObjects();
    void drawAI(const DrawAIObject& dai);
  
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
  void squareAt(const SGVec2d& center, double r);
  void drawLine(const SGVec2d& p1, const SGVec2d& p2);
  void drawLegendBox(const SGVec2d& pos, const std::string& t);
  
  int _width, _height;
  int _cachedZoom;
  double _drawRangeNm;
  double _upHeading; // true heading corresponding to +ve y-axis
  bool _magneticHeadings;
  bool _hasPanned;
  bool _aircraftUp;
  int _displayHeading;
    
  SGGeod _projectionCenter;
  Projection _projection;
  SGGeod _aircraft;
  SGGeod _clickGeod;
  SGVec2d _hitLocation;
  FGRouteMgr* _route;
  SGPropertyNode_ptr _root;
  SGPropertyNode_ptr _gps;
  SGGeodVec _flightHistoryPath;
    
  FGPositionedList _itemsToDraw;
    
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
