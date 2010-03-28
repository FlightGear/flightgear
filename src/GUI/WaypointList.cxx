

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "WaypointList.hxx"

#include <algorithm>
#include <plib/puAux.h>

#include <simgear/route/waypoint.hxx>
#include <simgear/structure/callback.hxx>
#include <simgear/sg_inlines.h>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <Autopilot/route_mgr.hxx>

enum {
  SCROLL_NO = 0,
  SCROLL_UP,
  SCROLL_DOWN
};
  
static const int DRAG_START_DISTANCE_PX = 5;
  
class RouteManagerWaypointModel : 
  public WaypointList::Model, 
  public SGPropertyChangeListener
{
public:
  RouteManagerWaypointModel()
  {
    _rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
    
    SGPropertyNode* routeEdited = fgGetNode("/autopilot/route-manager/signals/edited", true);
    routeEdited->addChangeListener(this);
  }
  
  virtual ~RouteManagerWaypointModel()
  {
    SGPropertyNode* routeEdited = fgGetNode("/autopilot/route-manager/signals/edited", true);
    routeEdited->removeChangeListener(this);
  }
  
// implement WaypointList::Model
  virtual unsigned int numWaypoints() const
  {
    return _rm->size();
  }
  
  virtual int currentWaypoint() const
  {
    return _rm->currentWaypoint();
  }
  
  virtual SGWayPoint waypointAt(unsigned int index) const
  {
    return _rm->get_waypoint(index);
  }

  virtual void deleteAt(unsigned int index)
  {
    _rm->pop_waypoint(index);
  }
  
  virtual void setWaypointTargetAltitudeFt(unsigned int index, int altFt)
  {
    _rm->setWaypointTargetAltitudeFt(index, altFt);
  }
  
  virtual void moveWaypointToIndex(unsigned int srcIndex, unsigned int destIndex)
  {
    if (destIndex > srcIndex) {
      --destIndex;
    }
    
    SGWayPoint wp = _rm->pop_waypoint(srcIndex);
    _rm->add_waypoint(wp, destIndex);
  }
  
  virtual void setUpdateCallback(SGCallback* cb)
  {
    _cb = cb;
  }
    
// implement SGPropertyChangeListener
  void valueChanged(SGPropertyNode *prop)
  {
    if (_cb) {
      (*_cb)();
    }
  }
private:
  FGRouteMgr* _rm;
  SGCallback* _cb;
};

//////////////////////////////////////////////////////////////////////////////

static void drawClippedString(puFont& font, const char* s, int x, int y, int maxWidth)
{
  int fullWidth = font.getStringWidth(s);
  if (fullWidth <= maxWidth) { // common case, easy and efficent
    font.drawString(s, x, y);
    return;
  }
  
  std::string buf(s);
  int len = buf.size();
  do {
    buf.resize(--len);
    fullWidth = font.getStringWidth(buf.c_str());
  } while (fullWidth > maxWidth);
  
  font.drawString(buf.c_str(), x, y);
}

//////////////////////////////////////////////////////////////////////////////

WaypointList::WaypointList(int x, int y, int width, int height) :
  puFrame(x, y, width, height),
  GUI_ID(FGCLASS_WAYPOINTLIST),
  _scrollPx(0),
  _dragging(false),
  _dragScroll(SCROLL_NO),
  _showLatLon(false),
  _model(NULL),
  _updateCallback(NULL),
  _scrollCallback(NULL)
{
  // pretend to be a list, so fgPopup doesn't mess with our mouse events
  type |= PUCLASS_LIST;  
  setModel(new RouteManagerWaypointModel());
  setSize(width, height);
  setValue(-1);
}

WaypointList::~WaypointList()
{
  delete _model;
  delete _updateCallback;
  delete _scrollCallback;
}

void WaypointList::setUpdateCallback(SGCallback* cb)
{
  _updateCallback = cb;
}

void WaypointList::setScrollCallback(SGCallback* cb)
{
  _scrollCallback = cb;
}

void WaypointList::setSize(int width, int height)
{
  double scrollP = getVScrollPercent();
  _heightPx = height;
  puFrame::setSize(width, height);
  
  if (wantsVScroll()) {
    setVScrollPercent(scrollP);
  } else {
    _scrollPx = 0;
  }
}

int WaypointList::checkHit ( int button, int updown, int x, int y )
{
  if ( isHit( x, y ) || ( puActiveWidget () == this ) )
  {
    doHit ( button, updown, x, y ) ;
    return TRUE ;
  }

  return FALSE ;
}


void WaypointList::doHit( int button, int updown, int x, int y )
{
  puFrame::doHit(button, updown, x, y);  
  if (updown == PU_DRAG) {
    handleDrag(x, y);
    return;
  }
  
  if (button != active_mouse_button) {
    return;
  }
      
  if (updown == PU_UP) {
    puDeactivateWidget();
    if (_dragging) {
      doDrop(x, y);
      return;
    }
  } else if (updown == PU_DOWN) {
    puSetActiveWidget(this, x, y);
    _mouseDownX = x;
    _mouseDownY = y;
    return;
  }
  
// update selection
  int row = rowForY(y - abox.min[1]);
  if (row >= (int) _model->numWaypoints()) {
    row = -1; // 'no selection'
  }

  if (row == getSelected()) {
    _showLatLon = !_showLatLon;
    puPostRefresh();
    return;
  }
  
  setSelected(row);
}

void WaypointList::handleDrag(int x, int y)
{
  if (!_dragging) {
    // don't start drags immediately, require a certain mouse movement first
    int manhattanLength = abs(x - _mouseDownX) + abs(y - _mouseDownY);
    if (manhattanLength < DRAG_START_DISTANCE_PX) {
      return;
    }
    
    _dragSourceRow = rowForY(y - abox.min[1]);
    _dragging = true;
    _dragScroll = SCROLL_NO;
  }
  
  if (y < abox.min[1]) {
    if (_dragScroll != SCROLL_DOWN) {
      _dragScroll = SCROLL_DOWN;
      _dragScrollTime.stamp();
    }
  } else if (y > abox.max[1]) {
    if (_dragScroll != SCROLL_UP) {
      _dragScroll = SCROLL_UP;
      _dragScrollTime.stamp();
    }
  } else {
    _dragScroll = SCROLL_NO;
    _dragTargetRow = rowForY(y - abox.min[1] - (rowHeightPx() / 2));
  }
}

void WaypointList::doDrop(int x, int y)
{
  _dragging = false;
  puDeactivateWidget();
  
  if ((y < abox.min[1]) || (y >= abox.max[1])) {
    return;
  }
  
  if (_dragSourceRow != _dragTargetRow) {
    _model->moveWaypointToIndex(_dragSourceRow, _dragTargetRow);
    
    // keep row indexes linged up when moving an item down the list
    if (_dragSourceRow < _dragTargetRow) {
      --_dragTargetRow;
    }
    
    setSelected(_dragTargetRow);
  }
}

void WaypointList::invokeDownCallback(void)
{
  _dragging = false;
  _dragScroll = SCROLL_NO;
  SG_LOG(SG_GENERAL, SG_INFO, "cancel drag");
}

int WaypointList::rowForY(int y) const
{
  if (!_model) {
    return -1;
  }
  
  // flip y to increase down, not up (as rows do)
  int flipY = _heightPx - y;
  int row = (flipY + _scrollPx) / rowHeightPx();
  return row;
}

void WaypointList::draw( int dx, int dy )
{
  puFrame::draw(dx, dy);

  if (!_model) {
    return;
  }

  if (_dragScroll != SCROLL_NO) {
    doDragScroll();
  }
  
  glEnable(GL_SCISSOR_TEST);
  GLint sx = (int) abox.min[0],
    sy = abox.min[1];
  GLsizei w = (GLsizei) abox.max[0] - abox.min[0],
    h = _heightPx;
    
  sx += border_thickness;
  sy += border_thickness;
  w -= 2 * border_thickness;
  h -= 2 * border_thickness;
    
  glScissor(sx + dx, sy + dy, w, h);
  int row = firstVisibleRow(), 
    final = lastVisibleRow(),
    rowHeight = rowHeightPx(),
    y = rowHeight;
  
  y -= (_scrollPx % rowHeight); // partially draw the first row
  
  for ( ; row <= final; ++row, y += rowHeight) {
    drawRow(dx, dy, row, y);
  } // of row drawing iteration
  
  glDisable(GL_SCISSOR_TEST);
    
  if (_dragging) {
    // draw the insert marker after the rows
    int insertY = (_dragTargetRow * rowHeight) - _scrollPx;
    SG_CLAMP_RANGE(insertY, 0, std::min(_heightPx, totalHeightPx()));
    
    glColor4f(1.0f, 0.5f, 0.0f, 0.8f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
      glVertex2f(dx + abox.min[0], dy + abox.max[1] - insertY);
      glVertex2f(dx + abox.max[0], dy + abox.max[1] - insertY);
    glEnd();
  }
}

void WaypointList::drawRow(int dx, int dy, int rowIndex, int y)
{
  bool isSelected = (rowIndex == getSelected());
  bool isCurrent = (rowIndex == _model->currentWaypoint());
  bool isDragSource = (_dragging && (rowIndex == _dragSourceRow));
  
  puBox bkgBox = abox;
  bkgBox.min[1] = abox.max[1] - y;
  bkgBox.max[1] = bkgBox.min[1] + rowHeightPx();
  
  puColour currentColor;
  puSetColor(currentColor, 1.0, 1.0, 0.0, 0.5);
  
  if (isDragSource) {
    // draw later, on *top* of text string
  } else  if (isCurrent) {
    bkgBox.draw(dx, dy, PUSTYLE_PLAIN, &currentColor, false, 0);
  } else if (isSelected) { // -PLAIN means selected, apparently
    bkgBox.draw(dx, dy, -PUSTYLE_PLAIN, colour, false, 0);
  }
  
  int xx = dx + abox.min[0] + PUSTR_LGAP;
  int yy = dy + abox.max[1] - y ;
  yy += 4; // center text in row height
  
  // row textual data
  const SGWayPoint wp(_model->waypointAt(rowIndex));
  char buffer[128];
  int count = ::snprintf(buffer, 128, "%03d   %-5s", rowIndex, wp.get_id().c_str());
  
  if (wp.get_name().size() > 0 && (wp.get_name() != wp.get_id())) { 
    // append name if present, and different to id
    ::snprintf(buffer + count, 128 - count, " (%s)", wp.get_name().c_str());
  }
  
  glColor4fv ( colour [ PUCOL_LEGEND ] ) ;
  drawClippedString(legendFont, buffer, xx, yy, 300);
  
  if (_showLatLon) {
    char ns = (wp.get_target_lat() > 0.0) ? 'N' : 'S';
    char ew = (wp.get_target_lon() > 0.0) ? 'E' : 'W';
    
    ::snprintf(buffer, 128 - count, "%4.2f%c %4.2f%c",
      fabs(wp.get_target_lon()), ew, fabs(wp.get_target_lat()), ns);
  } else {
    ::snprintf(buffer, 128 - count, "%03.0f %5.1fnm",
      wp.get_track(), wp.get_distance() * SG_METER_TO_NM);
  }

  legendFont.drawString(buffer, xx + 300 + PUSTR_LGAP, yy);
  
  int altFt = (int) wp.get_target_alt() * SG_METER_TO_FEET;
  if (altFt > -9990) {
    int altHundredFt = (altFt + 50) / 100; // round to nearest 100ft
    if (altHundredFt < 100) {
      count = ::snprintf(buffer, 128, "%d'", altHundredFt * 100);
    } else { // display as a flight-level
      count = ::snprintf(buffer, 128, "FL%d", altHundredFt);
    }
    
    legendFont.drawString(buffer, xx + 400 + PUSTR_LGAP, yy);
  } // of valid wp altitude
  
  if (isDragSource) {
    puSetColor(currentColor, 1.0, 0.5, 0.0, 0.5);
    bkgBox.draw(dx, dy, PUSTYLE_PLAIN, &currentColor, false, 0);
  }
}

const double SCROLL_PX_SEC = 200.0;

void WaypointList::doDragScroll()
{
  double dt = (SGTimeStamp::now() - _dragScrollTime).toSecs();
  _dragScrollTime.stamp();
  int deltaPx = (int)(dt * SCROLL_PX_SEC);
  
  if (_dragScroll == SCROLL_UP) {
    _scrollPx = _scrollPx - deltaPx;
    SG_CLAMP_RANGE(_scrollPx, 0, scrollRangePx());
    _dragTargetRow = firstVisibleRow();
  } else {
    _scrollPx = _scrollPx + deltaPx;
    SG_CLAMP_RANGE(_scrollPx, 0, scrollRangePx());
    _dragTargetRow = lastFullyVisibleRow() + 1;
  }
  
  if (_scrollCallback) {
    (*_scrollCallback)();
  }
}

int WaypointList::getSelected()
{
  return getIntegerValue();
}

void WaypointList::setSelected(int rowIndex)
{
  if (rowIndex == getSelected()) {
    return;
  }
  
  setValue(rowIndex);
  invokeCallback();
  if (rowIndex == -1) {
    return;
  }

  ensureRowVisible(rowIndex);
}

void WaypointList::ensureRowVisible(int rowIndex)
{
  if ((rowIndex >= firstFullyVisibleRow()) && (rowIndex <= lastFullyVisibleRow())) {
    return; // already visible, fine
  }
  
  // ideal position would place the desired row in the middle of the
  // visible section - hence subtract half the visible height.
  int targetScrollPx = (rowIndex * rowHeightPx()) - (_heightPx / 2);
  
  // clamp the scroll value to something valid
  SG_CLAMP_RANGE(targetScrollPx, 0, scrollRangePx());
  _scrollPx = targetScrollPx;
  
  puPostRefresh();
  if (_scrollCallback) { // keep scroll observers in sync
    (*_scrollCallback)();
  }
}

unsigned int WaypointList::numWaypoints() const
{
  if (!_model) {
    return 0;
  }
  
  return _model->numWaypoints();
}

bool WaypointList::wantsVScroll() const
{
  return totalHeightPx() > _heightPx;
}

float WaypointList::getVScrollPercent() const
{
  float scrollRange = scrollRangePx();
  if (scrollRange < 1.0f) {
    return 0.0;
  }
  
  return _scrollPx / scrollRange;
}

float WaypointList::getVScrollThumbPercent() const
{
  return _heightPx / (float) totalHeightPx();
}

void WaypointList::setVScrollPercent(float perc)
{
  float scrollRange = scrollRangePx();
  _scrollPx = (int)(scrollRange * perc);
}

int WaypointList::firstVisibleRow() const
{
  return _scrollPx / rowHeightPx();
}

int WaypointList::firstFullyVisibleRow() const
{
  int rh = rowHeightPx();
  return (_scrollPx + rh - 1) / rh;
}
  
int WaypointList::numVisibleRows() const
{
  int rh = rowHeightPx();
  int topOffset = _scrollPx % rh; // pixels of first visible row
  return (_heightPx - topOffset + rh - 1) / rh;

}

int WaypointList::numFullyVisibleRows() const
{
  int rh = rowHeightPx();
  int topOffset = _scrollPx % rh; // pixels of first visible row
  return (_heightPx - topOffset) / rh;
}

int WaypointList::rowHeightPx() const
{
  return legendFont.getStringHeight() + PUSTR_BGAP;
}

int WaypointList::scrollRangePx() const
{
  return std::max(0, totalHeightPx() - _heightPx);
}

int WaypointList::totalHeightPx() const
{
  if (!_model) {
    return 0;
  }
  
  return (int) _model->numWaypoints() * rowHeightPx();
}

int WaypointList::lastFullyVisibleRow() const
{
  int row = firstFullyVisibleRow() + numFullyVisibleRows();
  return std::min(row, (int) _model->numWaypoints() - 1);
}

int WaypointList::lastVisibleRow() const
{
  int row = firstVisibleRow() + numVisibleRows();
  return std::min(row, (int) _model->numWaypoints() - 1);
}

void WaypointList::setModel(Model* model)
{
  if (_model) {
    delete _model;
  }
  
  _model = model;
  _model->setUpdateCallback(make_callback(this, &WaypointList::modelUpdateCallback));
  
  puPostRefresh();
}

int WaypointList::checkKey (int key, int updown )
{
  if ((updown == PU_UP) || !isVisible () || !isActive () || (window != puGetWindow())) {
    return FALSE ;
  }
  
  switch (key)
  {
  case PU_KEY_HOME:
    setSelected(0);
    break;

  case PU_KEY_END:
    setSelected(_model->numWaypoints() - 1);
    break ;

  case PU_KEY_UP        :
  case PU_KEY_PAGE_UP   :
    if (getSelected() >= 0) {
      setSelected(getSelected() - 1);
    }
    break ;

  case PU_KEY_DOWN      :
  case PU_KEY_PAGE_DOWN : {
    int newSel = getSelected() + 1;
    if (newSel >= (int) _model->numWaypoints()) {
      setSelected(-1);
    } else {
      setSelected(newSel);
    }
    break ;
  }
  
  case '-':
    if (getSelected() >= 0) {
      int newAlt = wayptAltFtHundreds(getSelected()) - 10;
      if (newAlt < 0) {
        _model->setWaypointTargetAltitudeFt(getSelected(), -9999);
      } else {
        _model->setWaypointTargetAltitudeFt(getSelected(), newAlt * 100);
      }
    }
    break;
    
  case '=':
    if (getSelected() >= 0) {
      int newAlt = wayptAltFtHundreds(getSelected()) + 10;
      if (newAlt < 0) {
        _model->setWaypointTargetAltitudeFt(getSelected(), 0);
      } else {
        _model->setWaypointTargetAltitudeFt(getSelected(), newAlt * 100);
      }
    }
    break;
  
  case 0x7f: // delete
    if (getSelected() >= 0) {
      int index = getSelected();
      _model->deleteAt(index);
      setSelected(index - 1);
    }
    break;

  default :
    return FALSE;
  }

  return TRUE ;
}

int WaypointList::wayptAltFtHundreds(int index) const
{
  double alt = _model->waypointAt(index).get_target_alt();
  if (alt < -9990.0) {
    return -9999;
  }
  
  int altFt = (int) alt * SG_METER_TO_FEET;
  return (altFt + 50) / 100; // round to nearest 100ft
}

void WaypointList::modelUpdateCallback()
{
  // local stuff
  
  if (_updateCallback) {
    (*_updateCallback)();
  }
}

//////////////////////////////////////////////////////////////////////////////


static void handle_scrollbar(puObject* scrollbar)
{
  ScrolledWaypointList* self = (ScrolledWaypointList*)scrollbar->getUserData();
  self->setScrollPercent(scrollbar->getFloatValue());
}

static void waypointListCb(puObject* wpl)
{
  ScrolledWaypointList* self = (ScrolledWaypointList*)wpl->getUserData();
  self->setValue(wpl->getIntegerValue());
  self->invokeCallback();
}

ScrolledWaypointList::ScrolledWaypointList(int x, int y, int width, int height) :
  puGroup(x,y),
  _scrollWidth(16)
{
  // ensure our type is compound, so fgPopup::applySize doesn't descend into
  // us, and try to cast our children's user-data to GUIInfo*.
  type |= PUCLASS_LIST;
  
  init(width, height);
}

void ScrolledWaypointList::setValue(float v)
{
  puGroup::setValue(v);
  _list->setValue(v);
}

void ScrolledWaypointList::setValue(int v)
{
  puGroup::setValue(v);
  _list->setValue(v);
}

void ScrolledWaypointList::init(int w, int h)
{
  _list = new WaypointList(0, 0, w, h);
  _list->setUpdateCallback(make_callback(this, &ScrolledWaypointList::modelUpdated));
  _hasVScroll = _list->wantsVScroll();
  _list->setUserData(this);
  _list->setCallback(waypointListCb);
  
  _list->setScrollCallback(make_callback(this, &ScrolledWaypointList::updateScroll));
  
  _scrollbar = new puaScrollBar(w - _scrollWidth, 0, h, 
    1 /*arrow*/, 1 /* vertical */, _scrollWidth);
  _scrollbar->setMinValue(0.0);
  _scrollbar->setMaxValue(1.0);
  _scrollbar->setUserData(this);
  _scrollbar->setCallback(handle_scrollbar);
  close(); // close the group
  
  setSize(w, h);
}

void ScrolledWaypointList::modelUpdated()
{  
  int w, h;
  getSize(&w, &h);
  updateWantsScroll(w, h);
}

void ScrolledWaypointList::setScrollPercent(float v)
{
  // slider's min is the bottom, so invert the value
  _list->setVScrollPercent(1.0f - v); 
}

void ScrolledWaypointList::setSize(int w, int h)
{
  updateWantsScroll(w, h);
}

void ScrolledWaypointList::updateWantsScroll(int w, int h)
{
  _hasVScroll = _list->wantsVScroll();
  
  if (_hasVScroll) {
    _scrollbar->reveal();
    _scrollbar->setPosition(w - _scrollWidth, 0);
    _scrollbar->setSize(_scrollWidth, h);
    _list->setSize(w - _scrollWidth, h);
    updateScroll();
  } else {
    _scrollbar->hide();
    _list->setSize(w, h);
  }
}

void ScrolledWaypointList::updateScroll()
{
 // _scrollbar->setMaxValue(_list->numWaypoints());
  _scrollbar->setValue(1.0f - _list->getVScrollPercent());
  _scrollbar->setSliderFraction(_list->getVScrollThumbPercent());
}

