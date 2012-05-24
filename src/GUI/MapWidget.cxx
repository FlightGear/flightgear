#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "MapWidget.hxx"

#include <sstream>
#include <algorithm> // for std::sort
#include <plib/puAux.h>

#include <simgear/sg_inlines.h>
#include <simgear/misc/strutils.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx> // for magVar julianDate
#include <simgear/structure/exception.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Navaids/positioned.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/fix.hxx>
#include <Airports/simple.hxx>
#include <Airports/runways.hxx>
#include <Main/fg_os.hxx>      // fgGetKeyModifiers()
#include <Navaids/routePath.hxx>

const char* RULER_LEGEND_KEY = "ruler-legend";

/* equatorial and polar earth radius */
const float rec  = 6378137;          // earth radius, equator (?)
const float rpol = 6356752.314f;      // earth radius, polar   (?)

/************************************************************************
  some trigonometric helper functions
  (translated more or less directly from Alexei Novikovs perl original)
*************************************************************************/

//Returns Earth radius at a given latitude (Ellipsoide equation with two equal axis)
static float earth_radius_lat( float lat )
{
  double a = cos(lat)/rec;
  double b = sin(lat)/rpol;
  return 1.0f / sqrt( a * a + b * b );
}

///////////////////////////////////////////////////////////////////////////

static puBox makePuBox(int x, int y, int w, int h)
{
  puBox r;
  r.min[0] = x;
  r.min[1] = y;
  r.max[0] =  x + w;
  r.max[1] = y + h;
  return r;
}

static bool puBoxIntersect(const puBox& a, const puBox& b)
{
  int x0 = SG_MAX2(a.min[0], b.min[0]);
  int y0 = SG_MAX2(a.min[1], b.min[1]);
  int x1 = SG_MIN2(a.max[0], b.max[0]);
  int y1 = SG_MIN2(a.max[1], b.max[1]);

  return (x0 <= x1) && (y0 <= y1);
}

class MapData;
typedef std::vector<MapData*> MapDataVec;

class MapData
{
public:
  static const int HALIGN_LEFT = 1;
  static const int HALIGN_CENTER = 2;
  static const int HALIGN_RIGHT = 3;

  static const int VALIGN_TOP = 1 << 4;
  static const int VALIGN_CENTER = 2 << 4;
  static const int VALIGN_BOTTOM = 3 << 4;

  MapData(int priority) :
    _dirtyText(true),
    _age(0),
    _priority(priority),
    _width(0),
    _height(0),
    _offsetDir(HALIGN_LEFT | VALIGN_CENTER),
    _offsetPx(10),
    _dataVisible(false)
  {
  }

  void setLabel(const std::string& label)
  {
    if (label == _label) {
      return; // common case, and saves invalidation
    }

    _label = label;
    _dirtyText = true;
  }

  void setText(const std::string &text)
  {
    if (_rawText == text) {
      return; // common case, and saves invalidation
    }

    _rawText = text;
    _dirtyText = true;
  }

  void setDataVisible(bool vis) {
    if (vis == _dataVisible) {
      return;
    }

    if (_rawText.empty()) {
      vis = false;
    }

    _dataVisible = vis;
    _dirtyText = true;
  }

  static void setFont(puFont f)
  {
    _font = f;
    _fontHeight = f.getStringHeight();
    _fontDescender = f.getStringDescender();
  }

  static void setPalette(puColor* pal)
  {
    _palette = pal;
  }

  void setPriority(int pri)
  {
    _priority = pri;
  }

  int priority() const
  { return _priority; }

  void setAnchor(const SGVec2d& anchor)
  {
    _anchor = anchor;
  }

  void setOffset(int direction, int px)
  {
    if ((_offsetPx == px) && (_offsetDir == direction)) {
      return;
    }

    _dirtyOffset = true;
    _offsetDir = direction;
    _offsetPx = px;
  }

  bool isClipped(const puBox& vis) const
  {
    validate();
    if ((_width < 1) || (_height < 1)) {
      return true;
    }

    return !puBoxIntersect(vis, box());
  }

  bool overlaps(const MapDataVec& l) const
  {
    validate();
    puBox b(box());

    MapDataVec::const_iterator it;
    for (it = l.begin(); it != l.end(); ++it) {
      if (puBoxIntersect(b, (*it)->box())) {
        return true;
      }
    } // of list iteration

    return false;
  }

  puBox box() const
  {
    validate();
    return makePuBox(
      _anchor.x() + _offset.x(),
      _anchor.y() + _offset.y(),
      _width, _height);
  }

  void draw()
  {
    validate();

    int xx = _anchor.x() + _offset.x();
    int yy = _anchor.y() + _offset.y();

    if (_dataVisible) {
      puBox box(makePuBox(0,0,_width, _height));
      int border = 1;
      box.draw(xx, yy, PUSTYLE_DROPSHADOW, _palette, FALSE, border);

      // draw lines
      int lineHeight = _fontHeight;
      int xPos = xx + MARGIN;
      int yPos = yy + _height - (lineHeight + MARGIN);
      glColor3f(0.8, 0.8, 0.8);

      for (unsigned int ln=0; ln<_lines.size(); ++ln) {
        _font.drawString(_lines[ln].c_str(), xPos, yPos);
        yPos -= lineHeight + LINE_LEADING;
      }
    } else {
      glColor3f(0.8, 0.8, 0.8);
      _font.drawString(_label.c_str(), xx, yy + _fontDescender);
    }
  }

  void age()
  {
    ++_age;
  }

  void resetAge()
  {
    _age = 0;
  }

  bool isExpired() const
  { return (_age > 100); }

  static bool order(MapData* a, MapData* b)
  {
    return a->_priority > b->_priority;
  }
private:
  void validate() const
  {
    if (!_dirtyText) {
      if (_dirtyOffset) {
        computeOffset();
      }

      return;
    }

    if (_dataVisible) {
      measureData();
    } else {
      measureLabel();
    }

    computeOffset();
    _dirtyText = false;
  }

  void measureData() const
  {
    _lines = simgear::strutils::split(_rawText, "\n");
  // measure text to find width and height
    _width = -1;
    _height = 0;

    for (unsigned int ln=0; ln<_lines.size(); ++ln) {
      _height += _fontHeight;
      if (ln > 0) {
        _height += LINE_LEADING;
      }

      int lw = _font.getStringWidth(_lines[ln].c_str());
      _width = std::max(_width, lw);
    } // of line measurement

    if ((_width < 1) || (_height < 1)) {
      // will be clipped
      return;
    }

    _height += MARGIN * 2;
    _width += MARGIN * 2;
  }

  void measureLabel() const
  {
    if (_label.empty()) {
      _width = _height = -1;
      return;
    }

    _height = _fontHeight;
    _width = _font.getStringWidth(_label.c_str());
  }

  void computeOffset() const
  {
    _dirtyOffset = false;
    if ((_width <= 0) || (_height <= 0)) {
      return;
    }

    int hOffset = 0;
    int vOffset = 0;

    switch (_offsetDir & 0x0f) {
    default:
    case HALIGN_LEFT:
      hOffset = _offsetPx;
      break;

    case HALIGN_CENTER:
      hOffset = -(_width>>1);
      break;

    case HALIGN_RIGHT:
      hOffset = -(_offsetPx + _width);
      break;
    }

    switch (_offsetDir & 0xf0) {
    default:
    case VALIGN_TOP:
      vOffset = -(_offsetPx + _height);
      break;

    case VALIGN_CENTER:
      vOffset = -(_height>>1);
      break;

    case VALIGN_BOTTOM:
      vOffset = _offsetPx;
      break;
    }

    _offset = SGVec2d(hOffset, vOffset);
  }

  static const int LINE_LEADING = 3;
	static const int MARGIN = 3;

  mutable bool _dirtyText;
  mutable bool _dirtyOffset;
  int _age;
  std::string _rawText;
  std::string _label;
  mutable std::vector<std::string> _lines;
  int _priority;
  mutable int _width, _height;
  SGVec2d _anchor;
  int _offsetDir;
  int _offsetPx;
  mutable SGVec2d _offset;
  bool _dataVisible;

  static puFont _font;
  static puColor* _palette;
  static int _fontHeight;
  static int _fontDescender;
};

puFont MapData::_font;
puColor* MapData::_palette;
int MapData::_fontHeight = 0;
int MapData::_fontDescender = 0;

///////////////////////////////////////////////////////////////////////////

const int MAX_ZOOM = 12;
const int SHOW_DETAIL_ZOOM = 8;
const int CURSOR_PAN_STEP = 32;

MapWidget::MapWidget(int x, int y, int maxX, int maxY) :
  puObject(x,y,maxX, maxY)
{
  _route = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
  _gps = fgGetNode("/instrumentation/gps");

  _width = maxX - x;
  _height = maxY - y;
  _hasPanned = false;
  _orthoAzimuthProject = false;
  
  MapData::setFont(legendFont);
  MapData::setPalette(colour);

  _magVar = new SGMagVar();
}

MapWidget::~MapWidget()
{
  delete _magVar;
  clearData();
}

void MapWidget::setProperty(SGPropertyNode_ptr prop)
{
  _root = prop;
  int zoom = _root->getIntValue("zoom", -1);
  if (zoom < 0) {
    _root->setIntValue("zoom", 6); // default zoom
  }
  
// expose MAX_ZOOM to the UI
  _root->setIntValue("max-zoom", MAX_ZOOM);
  _root->setBoolValue("centre-on-aircraft", true);
  _root->setBoolValue("draw-data", false);
  _root->setBoolValue("magnetic-headings", true);
}

void MapWidget::setSize(int w, int h)
{
  puObject::setSize(w, h);

  _width = w;
  _height = h;

}

void MapWidget::doHit( int button, int updown, int x, int y )
{
  puObject::doHit(button, updown, x, y);
  if (updown == PU_DRAG) {
    handlePan(x, y);
    return;
  }

  if (button == 3) { // mouse-wheel up
    zoomIn();
  } else if (button == 4) { // mouse-wheel down
    zoomOut();
  }

  if (button != active_mouse_button) {
    return;
  }

  _hitLocation = SGVec2d(x - abox.min[0], y - abox.min[1]);

  if (updown == PU_UP) {
    puDeactivateWidget();
  } else if (updown == PU_DOWN) {
    puSetActiveWidget(this, x, y);

    if (fgGetKeyModifiers() & KEYMOD_CTRL) {
      _clickGeod = unproject(_hitLocation - SGVec2d(_width>>1, _height>>1));
    }
  }
}

void MapWidget::handlePan(int x, int y)
{
  SGVec2d delta = SGVec2d(x, y) - _hitLocation;
  pan(delta);
  _hitLocation = SGVec2d(x,y);
}

int MapWidget::checkKey (int key, int updown )
{
  if ((updown == PU_UP) || !isVisible () || !isActive () || (window != puGetWindow())) {
    return FALSE ;
  }

  switch (key)
  {

  case PU_KEY_UP:
    pan(SGVec2d(0, -CURSOR_PAN_STEP));
    break;

  case PU_KEY_DOWN:
    pan(SGVec2d(0, CURSOR_PAN_STEP));
    break ;

  case PU_KEY_LEFT:
    pan(SGVec2d(CURSOR_PAN_STEP, 0));
    break;

  case PU_KEY_RIGHT:
    pan(SGVec2d(-CURSOR_PAN_STEP, 0));
    break;

  case '-':
    zoomOut();

    break;

  case '=':
    zoomIn();
    break;

  default :
    return FALSE;
  }

  return TRUE ;
}

void MapWidget::pan(const SGVec2d& delta)
{
  _hasPanned = true; 
  _projectionCenter = unproject(-delta);
}

int MapWidget::zoom() const
{
  int z = _root->getIntValue("zoom");
  SG_CLAMP_RANGE(z, 0, MAX_ZOOM);
  return z;
}

void MapWidget::zoomIn()
{
  if (zoom() >= MAX_ZOOM) {
    return;
  }

  _root->setIntValue("zoom", zoom() + 1);
}

void MapWidget::zoomOut()
{
  if (zoom() <= 0) {
    return;
  }

  _root->setIntValue("zoom", zoom() - 1);
}

void MapWidget::draw(int dx, int dy)
{
  _aircraft = SGGeod::fromDeg(fgGetDouble("/position/longitude-deg"),
    fgGetDouble("/position/latitude-deg"));
    
  bool mag = _root->getBoolValue("magnetic-headings");
  if (mag != _magneticHeadings) {
    clearData(); // flush cached data text, since it often includes heading
    _magneticHeadings =  mag;
  }
  
  if (_hasPanned) {
      _root->setBoolValue("centre-on-aircraft", false);
      _hasPanned = false;
  }
  else if (_root->getBoolValue("centre-on-aircraft")) {
    _projectionCenter = _aircraft;
  }

  double julianDate = globals->get_time_params()->getJD();
  _magVar->update(_projectionCenter, julianDate);

  bool aircraftUp = _root->getBoolValue("aircraft-heading-up");
  if (aircraftUp) {
    _upHeading = fgGetDouble("/orientation/heading-deg");
  } else {
    _upHeading = 0.0;
  }

  _cachedZoom = MAX_ZOOM - zoom();
  SGGeod topLeft = unproject(SGVec2d(_width/2, _height/2));
  // compute draw range, including a fudge factor for ILSs and other 'long'
  // symbols
  _drawRangeNm = SGGeodesy::distanceNm(_projectionCenter, topLeft) + 10.0;

// drawing operations
  GLint sx = (int) abox.min[0],
    sy = (int) abox.min[1];
  glScissor(dx + sx, dy + sy, _width, _height);
  glEnable(GL_SCISSOR_TEST);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  // cetere drawing about the widget center (which is also the
  // projection centre)
  glTranslated(dx + sx + (_width/2), dy + sy + (_height/2), 0.0);

  drawLatLonGrid();

  if (aircraftUp) {
    int textHeight = legendFont.getStringHeight() + 5;

    // draw heading line
    SGVec2d loc = project(_aircraft);
    glColor3f(1.0, 1.0, 1.0);
    drawLine(loc, SGVec2d(loc.x(), (_height / 2) - textHeight));

    int displayHdg;
    if (_magneticHeadings) {
      displayHdg = (int) fgGetDouble("/orientation/heading-magnetic-deg");
    } else {
      displayHdg = (int) _upHeading;
    }

    double y = (_height / 2) - textHeight;
    char buf[16];
    ::snprintf(buf, 16, "%d", displayHdg);
    int sw = legendFont.getStringWidth(buf);
    legendFont.drawString(buf, loc.x() - sw/2, y);
  }

  drawAirports();
  drawNavaids();
  drawTraffic();
  drawGPSData();
  drawNavRadio(fgGetNode("/instrumentation/nav[0]", false));
  drawNavRadio(fgGetNode("/instrumentation/nav[1]", false));
  paintAircraftLocation(_aircraft);
  paintRoute();
  paintRuler();

  drawData();

  glPopMatrix();
  glDisable(GL_SCISSOR_TEST);
}

void MapWidget::paintRuler()
{
  if (_clickGeod == SGGeod()) {
    return;
  }

  SGVec2d acftPos = project(_aircraft);
  SGVec2d clickPos = project(_clickGeod);

  glColor4f(0.0, 1.0, 1.0, 0.6);
  drawLine(acftPos, clickPos);

  circleAtAlt(clickPos, 8, 10, 5);

  double dist, az, az2;
  SGGeodesy::inverse(_aircraft, _clickGeod, az, az2, dist);
  char buffer[1024];
	::snprintf(buffer, 1024, "%03d/%.1fnm",
		displayHeading(az), dist * SG_METER_TO_NM);

  MapData* d = getOrCreateDataForKey((void*) RULER_LEGEND_KEY);
  d->setLabel(buffer);
  d->setAnchor(clickPos);
  d->setOffset(MapData::VALIGN_TOP | MapData::HALIGN_CENTER, 15);
  d->setPriority(20000);


}

void MapWidget::paintAircraftLocation(const SGGeod& aircraftPos)
{
  SGVec2d loc = project(aircraftPos);

  double hdg = fgGetDouble("/orientation/heading-deg");

  glLineWidth(2.0);
  glColor4f(1.0, 1.0, 0.0, 1.0);
  glPushMatrix();
  glTranslated(loc.x(), loc.y(), 0.0);
  glRotatef(hdg - _upHeading, 0.0, 0.0, -1.0);

  const SGVec2d wingspan(12, 0);
  const SGVec2d nose(0, 8);
  const SGVec2d tail(0, -14);
  const SGVec2d tailspan(4,0);

  drawLine(-wingspan, wingspan);
  drawLine(nose, tail);
  drawLine(tail - tailspan, tail + tailspan);

  glPopMatrix();
  glLineWidth(1.0);
}

void MapWidget::paintRoute()
{
  if (_route->numWaypts() < 2) {
    return;
  }

  RoutePath path(_route->flightPlan());

// first pass, draw the actual lines
  glLineWidth(2.0);

  for (int w=0; w<_route->numWaypts(); ++w) {
    SGGeodVec gv(path.pathForIndex(w));
    if (gv.empty()) {
      continue;
    }

    if (w < _route->currentIndex()) {
      glColor4f(0.5, 0.5, 0.5, 0.7);
    } else {
      glColor4f(1.0, 0.0, 1.0, 1.0);
    }

    flightgear::WayptRef wpt(_route->wayptAtIndex(w));
    if (wpt->flag(flightgear::WPT_MISS)) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(1, 0x00FF);
    }

    glBegin(GL_LINE_STRIP);
    for (unsigned int i=0; i<gv.size(); ++i) {
      SGVec2d p = project(gv[i]);
      glVertex2d(p.x(), p.y());
    }

    glEnd();
    glDisable(GL_LINE_STIPPLE);
  }

  glLineWidth(1.0);
// second pass, draw waypoint symbols and data
  for (int w=0; w < _route->numWaypts(); ++w) {
    flightgear::WayptRef wpt(_route->wayptAtIndex(w));
    SGGeod g = path.positionForIndex(w);
    if (g == SGGeod()) {
      continue; // Vectors or similar
    }

    SGVec2d p = project(g);
    glColor4f(1.0, 0.0, 1.0, 1.0);
    circleAtAlt(p, 8, 12, 5);

    std::ostringstream legend;
    legend << wpt->ident();
    if (wpt->altitudeRestriction() != flightgear::RESTRICT_NONE) {
      legend << '\n' << SGMiscd::roundToInt(wpt->altitudeFt()) << '\'';
    }

    if (wpt->speedRestriction() == flightgear::SPEED_RESTRICT_MACH) {
      legend << '\n' << wpt->speedMach() << "M";
    } else if (wpt->speedRestriction() != flightgear::RESTRICT_NONE) {
      legend << '\n' << SGMiscd::roundToInt(wpt->speedKts()) << "Kts";
    }

    MapData* d = getOrCreateDataForKey(reinterpret_cast<void*>(w * 2));
    d->setText(legend.str());
    d->setLabel(wpt->ident());
    d->setAnchor(p);
    d->setOffset(MapData::VALIGN_TOP | MapData::HALIGN_CENTER, 15);
    d->setPriority(w < _route->currentIndex() ? 9000 : 12000);

  } // of second waypoint iteration
}

/**
 * Round a SGGeod to an arbitrary precision.
 * For example, passing precision of 0.5 will round to the nearest 0.5 of
 * a degree in both lat and lon - passing in 3.0 rounds to the nearest 3 degree
 * multiple, and so on.
 */
static SGGeod roundGeod(double precision, const SGGeod& g)
{
  double lon = SGMiscd::round(g.getLongitudeDeg() / precision);
  double lat = SGMiscd::round(g.getLatitudeDeg() / precision);

  return SGGeod::fromDeg(lon * precision, lat * precision);
}

bool MapWidget::drawLineClipped(const SGVec2d& a, const SGVec2d& b)
{
  double minX = SGMiscd::min(a.x(), b.x()),
    minY = SGMiscd::min(a.y(), b.y()),
    maxX = SGMiscd::max(a.x(), b.x()),
    maxY = SGMiscd::max(a.y(), b.y());

  int hh = _height >> 1, hw = _width >> 1;

  if ((maxX < -hw) || (minX > hw) || (minY > hh) || (maxY < -hh)) {
    return false;
  }

  glVertex2dv(a.data());
  glVertex2dv(b.data());
  return true;
}

SGVec2d MapWidget::gridPoint(int ix, int iy)
{
	int key = (ix + 0x7fff) | ((iy + 0x7fff) << 16);
	GridPointCache::iterator it = _gridCache.find(key);
	if (it != _gridCache.end()) {
		return it->second;
	}

	SGGeod gp = SGGeod::fromDeg(
    _gridCenter.getLongitudeDeg() + ix * _gridSpacing,
		_gridCenter.getLatitudeDeg() + iy * _gridSpacing);

	SGVec2d proj = project(gp);
	_gridCache[key] = proj;
	return proj;
}

void MapWidget::drawLatLonGrid()
{
  _gridSpacing = 1.0;
  _gridCenter = roundGeod(_gridSpacing, _projectionCenter);
  _gridCache.clear();

  int ix = 0;
  int iy = 0;

  glColor4f(0.8, 0.8, 0.8, 0.4);
  glBegin(GL_LINES);
  bool didDraw;
  do {
    didDraw = false;
    ++ix;
    ++iy;

    for (int x = -ix; x < ix; ++x) {
      didDraw |= drawLineClipped(gridPoint(x, -iy), gridPoint(x+1, -iy));
      didDraw |= drawLineClipped(gridPoint(x, iy), gridPoint(x+1, iy));
      didDraw |= drawLineClipped(gridPoint(x, -iy), gridPoint(x, -iy + 1));
      didDraw |= drawLineClipped(gridPoint(x, iy), gridPoint(x, iy - 1));

    }

    for (int y = -iy; y < iy; ++y) {
      didDraw |= drawLineClipped(gridPoint(-ix, y), gridPoint(-ix, y+1));
      didDraw |= drawLineClipped(gridPoint(-ix, y), gridPoint(-ix + 1, y));
      didDraw |= drawLineClipped(gridPoint(ix, y), gridPoint(ix, y+1));
      didDraw |= drawLineClipped(gridPoint(ix, y), gridPoint(ix - 1, y));
    }

    if (ix > 30) {
      break;
    }
  } while (didDraw);

  glEnd();
}

void MapWidget::drawGPSData()
{
  std::string gpsMode = _gps->getStringValue("mode");

  SGGeod wp0Geod = SGGeod::fromDeg(
        _gps->getDoubleValue("wp/wp[0]/longitude-deg"),
        _gps->getDoubleValue("wp/wp[0]/latitude-deg"));

  SGGeod wp1Geod = SGGeod::fromDeg(
        _gps->getDoubleValue("wp/wp[1]/longitude-deg"),
        _gps->getDoubleValue("wp/wp[1]/latitude-deg"));

// draw track line
  double gpsTrackDeg = _gps->getDoubleValue("indicated-track-true-deg");
  double gpsSpeed = _gps->getDoubleValue("indicated-ground-speed-kt");
  double az2;

  if (gpsSpeed > 3.0) { // only draw track line if valid
    SGGeod trackRadial;
    SGGeodesy::direct(_aircraft, gpsTrackDeg, _drawRangeNm * SG_NM_TO_METER, trackRadial, az2);

    glColor4f(1.0, 1.0, 0.0, 1.0);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x00FF);
    drawLine(project(_aircraft), project(trackRadial));
    glDisable(GL_LINE_STIPPLE);
  }

  if (gpsMode == "dto") {
    SGVec2d wp0Pos = project(wp0Geod);
    SGVec2d wp1Pos = project(wp1Geod);

    glColor4f(1.0, 0.0, 1.0, 1.0);
    drawLine(wp0Pos, wp1Pos);

  }

  if (_gps->getBoolValue("scratch/valid")) {
    // draw scratch data

  }
}

class MapAirportFilter : public FGAirport::AirportFilter
{
public:
  MapAirportFilter(SGPropertyNode_ptr nd)
  {
    _heliports = nd->getBoolValue("show-heliports", false);
    _hardRunwaysOnly = nd->getBoolValue("hard-surfaced-airports", true);
    _minLengthFt = fgGetDouble("/sim/navdb/min-runway-length-ft", 2000);
  }

  virtual FGPositioned::Type maxType() const {
    return _heliports ? FGPositioned::HELIPORT : FGPositioned::AIRPORT;
  }

  virtual bool passAirport(FGAirport* aApt) const {
    if (_hardRunwaysOnly) {
      return aApt->hasHardRunwayOfLengthFt(_minLengthFt);
    }

    return true;
  }

private:
  bool _heliports;
  bool _hardRunwaysOnly;
  double _minLengthFt;
};

void MapWidget::drawAirports()
{
  MapAirportFilter af(_root);
  FGPositioned::List apts = FGPositioned::findWithinRange(_projectionCenter, _drawRangeNm, &af);
  for (unsigned int i=0; i<apts.size(); ++i) {
    drawAirport((FGAirport*) apts[i].get());
  }
}

class NavaidFilter : public FGPositioned::Filter
{
public:
  NavaidFilter(bool fixesEnabled, bool navaidsEnabled) :
    _fixes(fixesEnabled),
    _navaids(navaidsEnabled)
  {}

  virtual bool pass(FGPositioned* aPos) const {
    if (_fixes && (aPos->type() == FGPositioned::FIX)) {
      // ignore fixes which end in digits - expirmental
      if (aPos->ident().length() > 4 && isdigit(aPos->ident()[3]) && isdigit(aPos->ident()[4])) {
        return false;
      }
    }

    return true;
  }

  virtual FGPositioned::Type minType() const {
    return _fixes ? FGPositioned::FIX : FGPositioned::VOR;
  }

  virtual FGPositioned::Type maxType() const {
    return _navaids ? FGPositioned::NDB : FGPositioned::FIX;
  }

private:
  bool _fixes, _navaids;
};

void MapWidget::drawNavaids()
{
  bool fixes = _root->getBoolValue("draw-fixes");
  NavaidFilter f(fixes, _root->getBoolValue("draw-navaids"));

  if (f.minType() <= f.maxType()) {
    FGPositioned::List navs = FGPositioned::findWithinRange(_projectionCenter, _drawRangeNm, &f);

    glLineWidth(1.0);
    for (unsigned int i=0; i<navs.size(); ++i) {
      FGPositioned::Type ty = navs[i]->type();
      if (ty == FGPositioned::NDB) {
        drawNDB(false, (FGNavRecord*) navs[i].get());
      } else if (ty == FGPositioned::VOR) {
        drawVOR(false, (FGNavRecord*) navs[i].get());
      } else if (ty == FGPositioned::FIX) {
        drawFix((FGFix*) navs[i].get());
      }
    } // of navaid iteration
  } // of navaids || fixes are drawn test
}

void MapWidget::drawNDB(bool tuned, FGNavRecord* ndb)
{
  SGVec2d pos = project(ndb->geod());

  if (tuned) {
    glColor3f(0.0, 1.0, 1.0);
  } else {
    glColor3f(0.0, 0.0, 0.0);
  }

  glEnable(GL_LINE_STIPPLE);
  glLineStipple(1, 0x00FF);
  circleAt(pos, 20, 6);
  circleAt(pos, 20, 10);
  glDisable(GL_LINE_STIPPLE);

  if (validDataForKey(ndb)) {
    setAnchorForKey(ndb, pos);
    return;
  }

  char buffer[1024];
	::snprintf(buffer, 1024, "%s\n%s %3.0fKhz",
		ndb->name().c_str(), ndb->ident().c_str(),ndb->get_freq()/100.0);

  MapData* d = createDataForKey(ndb);
  d->setPriority(40);
  d->setLabel(ndb->ident());
  d->setText(buffer);
  d->setOffset(MapData::HALIGN_CENTER | MapData::VALIGN_BOTTOM, 10);
  d->setAnchor(pos);

}

void MapWidget::drawVOR(bool tuned, FGNavRecord* vor)
{
  SGVec2d pos = project(vor->geod());
  if (tuned) {
    glColor3f(0.0, 1.0, 1.0);
  } else {
    glColor3f(0.0, 0.0, 1.0);
  }

  circleAt(pos, 6, 8);

  if (validDataForKey(vor)) {
    setAnchorForKey(vor, pos);
    return;
  }

  char buffer[1024];
	::snprintf(buffer, 1024, "%s\n%s %6.3fMhz",
		vor->name().c_str(), vor->ident().c_str(),
    vor->get_freq() / 100.0);

  MapData* d = createDataForKey(vor);
  d->setText(buffer);
  d->setLabel(vor->ident());
  d->setPriority(tuned ? 10000 : 100);
  d->setOffset(MapData::HALIGN_CENTER | MapData::VALIGN_BOTTOM, 12);
  d->setAnchor(pos);
}

void MapWidget::drawFix(FGFix* fix)
{
  SGVec2d pos = project(fix->geod());
  glColor3f(0.0, 0.0, 0.0);
  circleAt(pos, 3, 6);

  if (_cachedZoom > SHOW_DETAIL_ZOOM) {
    return; // hide fix labels beyond a certain zoom level
  }

  if (validDataForKey(fix)) {
    setAnchorForKey(fix, pos);
    return;
  }

  MapData* d = createDataForKey(fix);
  d->setLabel(fix->ident());
  d->setPriority(20);
  d->setOffset(MapData::VALIGN_CENTER | MapData::HALIGN_LEFT, 10);
  d->setAnchor(pos);
}

void MapWidget::drawNavRadio(SGPropertyNode_ptr radio)
{
  if (!radio || radio->getBoolValue("slaved-to-gps", false)
        || !radio->getBoolValue("in-range", false)) {
    return;
  }

  if (radio->getBoolValue("nav-loc", false)) {
    drawTunedLocalizer(radio);
  }

  // identify the tuned station - unfortunately we don't get lat/lon directly,
  // need to do the frequency search again
  double mhz = radio->getDoubleValue("frequencies/selected-mhz", 0.0);
  FGNavRecord* nav = globals->get_navlist()->findByFreq(mhz, _aircraft);
  if (!nav || (nav->ident() != radio->getStringValue("nav-id"))) {
    // mismatch between navradio selection logic and ours!
    return;
  }

  glLineWidth(1.0);
  drawVOR(true, nav);

  SGVec2d pos = project(nav->geod());
  SGGeod range;
  double az2;
  double trueRadial = radio->getDoubleValue("radials/target-radial-deg");
  SGGeodesy::direct(nav->geod(), trueRadial, nav->get_range() * SG_NM_TO_METER, range, az2);
  SGVec2d prange = project(range);

  SGVec2d norm = normalize(prange - pos);
  SGVec2d perp(norm.y(), -norm.x());

  circleAt(pos, 64, length(prange - pos));
  drawLine(pos, prange);

// draw to/from arrows
  SGVec2d midPoint = (pos + prange) * 0.5;
  if (radio->getBoolValue("from-flag")) {
    norm = -norm;
    perp = -perp;
  }

  int sz = 10;
  SGVec2d arrowB = midPoint - (norm * sz) + (perp * sz);
  SGVec2d arrowC = midPoint - (norm * sz) - (perp * sz);
  drawLine(midPoint, arrowB);
  drawLine(arrowB, arrowC);
  drawLine(arrowC, midPoint);

  drawLine(pos, (2 * pos) - prange); // reciprocal radial
}

void MapWidget::drawTunedLocalizer(SGPropertyNode_ptr radio)
{
  double mhz = radio->getDoubleValue("frequencies/selected-mhz", 0.0);
  FGNavRecord* loc = globals->get_loclist()->findByFreq(mhz, _aircraft);
  if (!loc || (loc->ident() != radio->getStringValue("nav-id"))) {
    // mismatch between navradio selection logic and ours!
    return;
  }

  if (loc->runway()) {
    drawILS(true, loc->runway());
  }
}

/*
void MapWidget::drawObstacle(FGPositioned* obs)
{
  SGVec2d pos = project(obs->geod());
  glColor3f(0.0, 0.0, 0.0);
  glLineWidth(2.0);
  drawLine(pos, pos + SGVec2d());
}
*/

void MapWidget::drawAirport(FGAirport* apt)
{
	// draw tower location
	SGVec2d towerPos = project(apt->getTowerLocation());

  if (_cachedZoom <= SHOW_DETAIL_ZOOM) {
    glColor3f(1.0, 1.0, 1.0);
    glLineWidth(1.0);

    drawLine(towerPos + SGVec2d(3, 0), towerPos + SGVec2d(3, 10));
    drawLine(towerPos + SGVec2d(-3, 0), towerPos + SGVec2d(-3, 10));
    drawLine(towerPos + SGVec2d(-6, 20), towerPos + SGVec2d(-3, 10));
    drawLine(towerPos + SGVec2d(6, 20), towerPos + SGVec2d(3, 10));
    drawLine(towerPos + SGVec2d(-6, 20), towerPos + SGVec2d(6, 20));
  }

  if (validDataForKey(apt)) {
    setAnchorForKey(apt, towerPos);
  } else {
    char buffer[1024];
    ::snprintf(buffer, 1024, "%s\n%s",
      apt->ident().c_str(), apt->name().c_str());

    MapData* d = createDataForKey(apt);
    d->setText(buffer);
    d->setLabel(apt->ident());
    d->setPriority(100 + scoreAirportRunways(apt));
    d->setOffset(MapData::VALIGN_TOP | MapData::HALIGN_CENTER, 6);
    d->setAnchor(towerPos);
  }

  if (_cachedZoom > SHOW_DETAIL_ZOOM) {
    return;
  }

  for (unsigned int r=0; r<apt->numRunways(); ++r) {
    FGRunway* rwy = apt->getRunwayByIndex(r);
		if (!rwy->isReciprocal()) {
			drawRunwayPre(rwy);
		}
  }

	for (unsigned int r=0; r<apt->numRunways(); ++r) {
		FGRunway* rwy = apt->getRunwayByIndex(r);
		if (!rwy->isReciprocal()) {
			drawRunway(rwy);
		}

		if (rwy->ILS()) {
			drawILS(false, rwy);
		}
	} // of runway iteration

}

int MapWidget::scoreAirportRunways(FGAirport* apt)
{
  bool needHardSurface = _root->getBoolValue("hard-surfaced-airports", true);
  double minLength = _root->getDoubleValue("min-runway-length-ft", 2000.0);

  int score = 0;
  unsigned int numRunways(apt->numRunways());
  for (unsigned int r=0; r<numRunways; ++r) {
    FGRunway* rwy = apt->getRunwayByIndex(r);
    if (rwy->isReciprocal()) {
      continue;
    }

    if (needHardSurface && !rwy->isHardSurface()) {
      continue;
    }

    if (rwy->lengthFt() < minLength) {
      continue;
    }

    int scoreLength = SGMiscd::roundToInt(rwy->lengthFt() / 200.0);
    score += scoreLength;
  } // of runways iteration

  return score;
}

void MapWidget::drawRunwayPre(FGRunway* rwy)
{
  SGVec2d p1 = project(rwy->begin());
	SGVec2d p2 = project(rwy->end());

  glLineWidth(4.0);
  glColor3f(1.0, 0.0, 1.0);
	drawLine(p1, p2);
}

void MapWidget::drawRunway(FGRunway* rwy)
{
	// line for runway
	// optionally show active, stopway, etc
	// in legend, show published heading and length
	// and threshold elevation

  SGVec2d p1 = project(rwy->begin());
	SGVec2d p2 = project(rwy->end());
  glLineWidth(2.0);
  glColor3f(1.0, 1.0, 1.0);
  SGVec2d inset = normalize(p2 - p1) * 2;

	drawLine(p1 + inset, p2 - inset);

  if (validDataForKey(rwy)) {
    setAnchorForKey(rwy, (p1 + p2) * 0.5);
    return;
  }
  
	char buffer[1024];
	::snprintf(buffer, 1024, "%s/%s\n%03d/%03d\n%.0f'",
		rwy->ident().c_str(),
		rwy->reciprocalRunway()->ident().c_str(),
		displayHeading(rwy->headingDeg()),
		displayHeading(rwy->reciprocalRunway()->headingDeg()),
		rwy->lengthFt());

  MapData* d = createDataForKey(rwy);
  d->setText(buffer);
  d->setLabel(rwy->ident() + "/" + rwy->reciprocalRunway()->ident());
  d->setPriority(50);
  d->setOffset(MapData::HALIGN_CENTER | MapData::VALIGN_BOTTOM, 12);
  d->setAnchor((p1 + p2) * 0.5);
}

void MapWidget::drawILS(bool tuned, FGRunway* rwy)
{
	// arrow, tip centered on the landing threshold
  // using LOC transmitter position would be more accurate, but
  // is visually cluttered
	// arrow width is based upon the computed localizer width

	FGNavRecord* loc = rwy->ILS();
	double halfBeamWidth = loc->localizerWidth() * 0.5;
	SGVec2d t = project(rwy->threshold());
	SGGeod locEnd;
	double rangeM = loc->get_range() * SG_NM_TO_METER;
	double radial = loc->get_multiuse();
  SG_NORMALIZE_RANGE(radial, 0.0, 360.0);
	double az2;

// compute the three end points at the widge end of the arrow
	SGGeodesy::direct(loc->geod(), radial, -rangeM, locEnd, az2);
	SGVec2d endCentre = project(locEnd);

	SGGeodesy::direct(loc->geod(), radial + halfBeamWidth, -rangeM * 1.1, locEnd, az2);
	SGVec2d endR = project(locEnd);

	SGGeodesy::direct(loc->geod(), radial - halfBeamWidth, -rangeM * 1.1, locEnd, az2);
	SGVec2d endL = project(locEnd);

// outline two triangles
  glLineWidth(1.0);
  if (tuned) {
    glColor3f(0.0, 1.0, 1.0);
  } else {
    glColor3f(0.0, 0.0, 1.0);
	}

  glBegin(GL_LINE_LOOP);
		glVertex2dv(t.data());
		glVertex2dv(endCentre.data());
		glVertex2dv(endL.data());
	glEnd();
	glBegin(GL_LINE_LOOP);
		glVertex2dv(t.data());
		glVertex2dv(endCentre.data());
		glVertex2dv(endR.data());
	glEnd();

	if (validDataForKey(loc)) {
    setAnchorForKey(loc, endR);
    return;
  }

	char buffer[1024];
	::snprintf(buffer, 1024, "%s\n%s\n%03d - %3.2fMHz",
		loc->ident().c_str(), loc->name().c_str(),
    displayHeading(radial),
    loc->get_freq()/100.0);

  MapData* d = createDataForKey(loc);
  d->setPriority(40);
  d->setLabel(loc->ident());
  d->setText(buffer);
  d->setOffset(MapData::HALIGN_CENTER | MapData::VALIGN_BOTTOM, 10);
  d->setAnchor(endR);
}

void MapWidget::drawTraffic()
{
  if (!_root->getBoolValue("draw-traffic")) {
    return;
  }

  if (_cachedZoom > SHOW_DETAIL_ZOOM) {
    return;
  }

  const SGPropertyNode* ai = fgGetNode("/ai/models", true);

  for (int i = 0; i < ai->nChildren(); ++i) {
    const SGPropertyNode *model = ai->getChild(i);
    // skip bad or dead entries
    if (!model || model->getIntValue("id", -1) == -1) {
      continue;
    }

    const std::string& name(model->getName());
    SGGeod pos = SGGeod::fromDegFt(
      model->getDoubleValue("position/longitude-deg"),
      model->getDoubleValue("position/latitude-deg"),
      model->getDoubleValue("position/altitude-ft"));

    double dist = SGGeodesy::distanceNm(_projectionCenter, pos);
    if (dist > _drawRangeNm) {
      continue;
    }

    double heading = model->getDoubleValue("orientation/true-heading-deg");
    if ((name == "aircraft") || (name == "multiplayer") ||
        (name == "wingman") || (name == "tanker")) {
      drawAIAircraft(model, pos, heading);
    } else if ((name == "ship") || (name == "carrier") || (name == "escort")) {
      drawAIShip(model, pos, heading);
    }
  } // of ai/models iteration
}

void MapWidget::drawAIAircraft(const SGPropertyNode* model, const SGGeod& pos, double hdg)
{

  SGVec2d p = project(pos);

  glColor3f(0.0, 0.0, 0.0);
  glLineWidth(2.0);
  circleAt(p, 4, 6.0); // black diamond

// draw heading vector
  int speedKts = static_cast<int>(model->getDoubleValue("velocities/true-airspeed-kt"));
  if (speedKts > 1) {
    glLineWidth(1.0);

    const double dt = 15.0 / (3600.0); // 15 seconds look-ahead
    double distanceM = speedKts * SG_NM_TO_METER * dt;

    SGGeod advance;
    double az2;
    SGGeodesy::direct(pos, hdg, distanceM, advance, az2);

    drawLine(p, project(advance));
  }


  // draw callsign / altitude / speed
  char buffer[1024];
	::snprintf(buffer, 1024, "%s\n%d'\n%dkts",
		model->getStringValue("callsign", "<>"),
		static_cast<int>(pos.getElevationFt() / 50.0) * 50,
    speedKts);

  MapData* d = getOrCreateDataForKey((void*) model);
  d->setText(buffer);
  d->setLabel(model->getStringValue("callsign", "<>"));
  d->setPriority(speedKts > 5 ? 60 : 10); // low priority for parked aircraft
  d->setOffset(MapData::VALIGN_CENTER | MapData::HALIGN_LEFT, 10);
  d->setAnchor(p);

}

void MapWidget::drawAIShip(const SGPropertyNode* model, const SGGeod& pos, double hdg)
{
  SGVec2d p = project(pos);

  glColor3f(0.0, 0.0, 0.5);
  glLineWidth(2.0);
  circleAt(p, 4, 6.0); // blue diamond (to differentiate from aircraft.

// draw heading vector
  int speedKts = static_cast<int>(model->getDoubleValue("velocities/speed-kts"));
  if (speedKts > 1) {
    glLineWidth(1.0);

    const double dt = 15.0 / (3600.0); // 15 seconds look-ahead
    double distanceM = speedKts * SG_NM_TO_METER * dt;

    SGGeod advance;
    double az2;
    SGGeodesy::direct(pos, hdg, distanceM, advance, az2);

    drawLine(p, project(advance));
  }

  // draw callsign / speed
  char buffer[1024];
	::snprintf(buffer, 1024, "%s\n%dkts",
		model->getStringValue("name", "<>"),
    speedKts);

  MapData* d = getOrCreateDataForKey((void*) model);
  d->setText(buffer);
  d->setLabel(model->getStringValue("name", "<>"));
  d->setPriority(speedKts > 2 ? 30 : 10); // low priority for slow moving ships
  d->setOffset(MapData::VALIGN_CENTER | MapData::HALIGN_LEFT, 10);
  d->setAnchor(p);
}

SGVec2d MapWidget::project(const SGGeod& geod) const
{
  SGVec2d p;
  double r = earth_radius_lat(geod.getLatitudeRad());
  
  if (_orthoAzimuthProject) {
    // http://mathworld.wolfram.com/OrthographicProjection.html
    double cosTheta = cos(geod.getLatitudeRad());
    double sinDLambda = sin(geod.getLongitudeRad() - _projectionCenter.getLongitudeRad());
    double cosDLambda = cos(geod.getLongitudeRad() - _projectionCenter.getLongitudeRad());
    double sinTheta1 = sin(_projectionCenter.getLatitudeRad());
    double sinTheta = sin(geod.getLatitudeRad());
    double cosTheta1 = cos(_projectionCenter.getLatitudeRad());
    
    p = SGVec2d(cosTheta * sinDLambda,
                (cosTheta1 * sinTheta) - (sinTheta1 * cosTheta * cosDLambda)) * r * currentScale();
    
  } else {
    // Sanson-Flamsteed projection, relative to the projection center
    double lonDiff = geod.getLongitudeRad() - _projectionCenter.getLongitudeRad(),
      latDiff = geod.getLatitudeRad() - _projectionCenter.getLatitudeRad();

    p = SGVec2d(cos(geod.getLatitudeRad()) * lonDiff, latDiff) * r * currentScale();
      
  }
  
// rotate as necessary
  double cost = cos(_upHeading * SG_DEGREES_TO_RADIANS),
    sint = sin(_upHeading * SG_DEGREES_TO_RADIANS);
  double rx = cost * p.x() - sint * p.y();
  double ry = sint * p.x() + cost * p.y();
  return SGVec2d(rx, ry);
}

SGGeod MapWidget::unproject(const SGVec2d& p) const
{
  // unrotate, if necessary
  double cost = cos(-_upHeading * SG_DEGREES_TO_RADIANS),
    sint = sin(-_upHeading * SG_DEGREES_TO_RADIANS);
  SGVec2d ur(cost * p.x() - sint * p.y(),
             sint * p.x() + cost * p.y());

  double r = earth_radius_lat(_projectionCenter.getLatitudeRad());
  SGVec2d unscaled = ur * (1.0 / (currentScale() * r));

  if (_orthoAzimuthProject) {
      double phi = length(p);
      double c = asin(phi);
      double sinTheta1 = sin(_projectionCenter.getLatitudeRad());
      double cosTheta1 = cos(_projectionCenter.getLatitudeRad());
      
      double lat = asin(cos(c) * sinTheta1 + ((unscaled.y() * sin(c) * cosTheta1) / phi));
      double lon = _projectionCenter.getLongitudeRad() + 
        atan((unscaled.x()* sin(c)) / (phi  * cosTheta1 * cos(c) - unscaled.y() * sinTheta1 * sin(c)));
      return SGGeod::fromRad(lon, lat);
  } else {
      double lat = unscaled.y() + _projectionCenter.getLatitudeRad();
      double lon = (unscaled.x() / cos(lat)) + _projectionCenter.getLongitudeRad();
      return SGGeod::fromRad(lon, lat);
  }
}

double MapWidget::currentScale() const
{
  return 1.0 / pow(2.0, _cachedZoom);
}

void MapWidget::circleAt(const SGVec2d& center, int nSides, double r)
{
  glBegin(GL_LINE_LOOP);
  double advance = (SGD_PI * 2) / nSides;
  glVertex2d(center.x(), center.y() + r);
  double t=advance;
  for (int i=1; i<nSides; ++i) {
    glVertex2d(center.x() + (sin(t) * r), center.y() + (cos(t) * r));
    t += advance;
  }
  glEnd();
}

void MapWidget::circleAtAlt(const SGVec2d& center, int nSides, double r, double r2)
{
  glBegin(GL_LINE_LOOP);
  double advance = (SGD_PI * 2) / nSides;
  glVertex2d(center.x(), center.y() + r);
  double t=advance;
  for (int i=1; i<nSides; ++i) {
    double rr = (i%2 == 0) ? r : r2;
    glVertex2d(center.x() + (sin(t) * rr), center.y() + (cos(t) * rr));
    t += advance;
  }
  glEnd();
}

void MapWidget::drawLine(const SGVec2d& p1, const SGVec2d& p2)
{
  glBegin(GL_LINES);
    glVertex2dv(p1.data());
    glVertex2dv(p2.data());
  glEnd();
}

void MapWidget::drawLegendBox(const SGVec2d& pos, const std::string& t)
{
	std::vector<std::string> lines(simgear::strutils::split(t, "\n"));
	const int LINE_LEADING = 4;
	const int MARGIN = 4;

// measure
	int maxWidth = -1, totalHeight = 0;
	int lineHeight = legendFont.getStringHeight();

	for (unsigned int ln=0; ln<lines.size(); ++ln) {
		totalHeight += lineHeight;
		if (ln > 0) {
			totalHeight += LINE_LEADING;
		}

		int lw = legendFont.getStringWidth(lines[ln].c_str());
		maxWidth = std::max(maxWidth, lw);
	} // of line measurement

	if (maxWidth < 0) {
		return; // all lines are empty, don't draw
	}

	totalHeight += MARGIN * 2;

// draw box
	puBox box;
	box.min[0] = 0;
	box.min[1] = -totalHeight;
	box.max[0] = maxWidth + (MARGIN * 2);
	box.max[1] = 0;
	int border = 1;
	box.draw (pos.x(), pos.y(), PUSTYLE_DROPSHADOW, colour, FALSE, border);

// draw lines
	int xPos = pos.x() + MARGIN;
	int yPos = pos.y() - (lineHeight + MARGIN);
	glColor3f(0.8, 0.8, 0.8);

	for (unsigned int ln=0; ln<lines.size(); ++ln) {
		legendFont.drawString(lines[ln].c_str(), xPos, yPos);
		yPos -= lineHeight + LINE_LEADING;
	}
}

void MapWidget::drawData()
{
  std::sort(_dataQueue.begin(), _dataQueue.end(), MapData::order);

  int hw = _width >> 1,
    hh = _height >> 1;
  puBox visBox(makePuBox(-hw, -hh, _width, _height));

  unsigned int d = 0;
  int drawn = 0;
  std::vector<MapData*> drawQueue;

  bool drawData = _root->getBoolValue("draw-data");
  const int MAX_DRAW_DATA = 25;
  const int MAX_DRAW = 50;

  for (; (d < _dataQueue.size()) && (drawn < MAX_DRAW); ++d) {
    MapData* md = _dataQueue[d];
    md->setDataVisible(drawData);

    if (md->isClipped(visBox)) {
      continue;
    }

    if (md->overlaps(drawQueue)) {
      if (drawData) { // overlapped with data, let's try just the label
        md->setDataVisible(false);
        if (md->overlaps(drawQueue)) {
          continue;
        }
      } else {
        continue;
      }
    } // of overlaps case

    drawQueue.push_back(md);
    ++drawn;
    if (drawData && (drawn >= MAX_DRAW_DATA)) {
      drawData = false;
    }
  }

  // draw lowest-priority first, so higher-priorty items appear on top
  std::vector<MapData*>::reverse_iterator r;
  for (r = drawQueue.rbegin(); r!= drawQueue.rend(); ++r) {
    (*r)->draw();
  }

  _dataQueue.clear();
  KeyDataMap::iterator it = _mapData.begin();
  for (; it != _mapData.end(); ) {
    it->second->age();
    if (it->second->isExpired()) {
      delete it->second;
      KeyDataMap::iterator cur = it++;
      _mapData.erase(cur);
    } else {
      ++it;
    }
  } // of expiry iteration
}

bool MapWidget::validDataForKey(void* key)
{
  KeyDataMap::iterator it = _mapData.find(key);
  if (it == _mapData.end()) {
    return false; // no valid data for the key!
  }

  it->second->resetAge(); // mark data as valid this frame
  _dataQueue.push_back(it->second);
  return true;
}

void MapWidget::setAnchorForKey(void* key, const SGVec2d& anchor)
{
  KeyDataMap::iterator it = _mapData.find(key);
  if (it == _mapData.end()) {
    throw sg_exception("no valid data for key!");
  }

  it->second->setAnchor(anchor);
}

MapData* MapWidget::getOrCreateDataForKey(void* key)
{
  KeyDataMap::iterator it = _mapData.find(key);
  if (it == _mapData.end()) {
    return createDataForKey(key);
  }

  it->second->resetAge(); // mark data as valid this frame
  _dataQueue.push_back(it->second);
  return it->second;
}

MapData* MapWidget::createDataForKey(void* key)
{
  KeyDataMap::iterator it = _mapData.find(key);
  if (it != _mapData.end()) {
    throw sg_exception("duplicate data requested for key!");
  }

  MapData* d =  new MapData(0);
  _mapData[key] = d;
  _dataQueue.push_back(d);
  d->resetAge();
  return d;
}

void MapWidget::clearData()
{
  KeyDataMap::iterator it = _mapData.begin();
  for (; it != _mapData.end(); ++it) {
    delete it->second;
  }
  
  _mapData.clear();
}

int MapWidget::displayHeading(double h) const
{
  if (_magneticHeadings) {
    h -= _magVar->get_magvar() * SG_RADIANS_TO_DEGREES;
  }
  
  SG_NORMALIZE_RANGE(h, 0.0, 360.0);
  return SGMiscd::roundToInt(h);
}
