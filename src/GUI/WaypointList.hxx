/**
 * WaypointList.hxx - scrolling list of waypoints, with special formatting
 */

#ifndef GUI_WAYPOINT_LIST_HXX
#define GUI_WAYPOINT_LIST_HXX

#include <simgear/compiler.h>
#include <simgear/timing/timestamp.hxx>

#include <plib/pu.h>

#include "FGPUIDialog.hxx" // for GUI_ID

// forward decls
class puaScrollBar;
class SGCallback;

namespace flightgear {
  class Waypt;
}

class WaypointList : public puFrame, public GUI_ID
{
public:
  WaypointList(int x, int y, int width, int height);
  virtual ~WaypointList();

  virtual void setSize(int width, int height);
  virtual int checkHit ( int button, int updown, int x, int y);
  virtual void doHit( int button, int updown, int x, int y ) ;
  virtual void draw( int dx, int dy ) ;
  virtual int checkKey(int key, int updown);
  virtual void invokeDownCallback (void);
  
  void setSelected(int rowIndex);
  int getSelected();
  
  /**
   * Do we want a vertical scrollbar (or similar)
   */
  bool wantsVScroll() const;
  
  /**
   * Get scrollbar position as a percentage of total range.
   * returns negative number if scrolling is not possible
   */
  float getVScrollPercent() const;
  
  /**
   *
   */
  void setVScrollPercent(float perc);
  
  /**
   * Get required thumb size as percentage of total height
   */
  float getVScrollThumbPercent() const;
  
  int numVisibleRows() const;
  
  void ensureRowVisible(int row);
  
  void setUpdateCallback(SGCallback* cb);
  void setScrollCallback(SGCallback* cb);
  
  /**
   * Abstract interface for waypoint source
   */
  class Model
  {
  public:
    virtual ~Model() { }
    
    virtual unsigned int numWaypoints() const = 0;
    virtual int currentWaypoint() const = 0;
    virtual flightgear::Waypt* waypointAt(unsigned int index) const = 0;
  
  // update notifications
    virtual void setUpdateCallback(SGCallback* cb) = 0;
  
  // editing operations
    virtual void deleteAt(unsigned int index) = 0;
    virtual void moveWaypointToIndex(unsigned int srcIndex, unsigned int dstIndex) = 0;
  };
  
  void setModel(Model* model);
  
  unsigned int numWaypoints() const;
protected:

private:
  void drawRow(int dx, int dy, int rowIndex, int yOrigin);

  void handleDrag(int x, int y);
  void doDrop(int x, int y);
  void doDragScroll();
  
  /**
   * Pixel height of a row, including padding
   */
  int rowHeightPx() const;
  
  /**
   * model row corresponding to an on-screen y-value
   */
  int rowForY(int y) const;
  
  /**
   * reutrn rowheight * total number of rows, i.e the height we'd
   * need to be to show every row without scrolling
   */
  int totalHeightPx() const;
  
  /**
   * Pixel scroll range, based on widget height and number of rows
   */
  int scrollRangePx() const;
  
  int firstVisibleRow() const;
  int lastVisibleRow() const;

  int numFullyVisibleRows() const;
  int firstFullyVisibleRow() const;
  int lastFullyVisibleRow() const;
    
  void modelUpdateCallback();
  
  int _scrollPx; // scroll ammount (in pixels)
  int _heightPx;
  
  bool _dragging;
  int _dragSourceRow;
  int _dragTargetRow;
  int _mouseDownX, _mouseDownY;
  
  int _dragScroll;
  SGTimeStamp _dragScrollTime;

  bool _showLatLon;
  Model* _model;
  SGCallback* _updateCallback;
  SGCallback* _scrollCallback;
 
  SGTimeStamp _blinkTimer;
  bool _blink;
  int _arrowWidth;
};

class ScrolledWaypointList : public puGroup
{
public:
  ScrolledWaypointList(int x, int y, int width, int height);
  
  virtual void setSize(int width, int height);
  
  void setScrollPercent(float v);
  
  virtual void setValue(float v);
  virtual void setValue(int v);
private:  
  void init(int w, int h);
  
  void updateScroll();
  void updateWantsScroll(int w, int h);
  
  void modelUpdated();
  
  puaScrollBar* _scrollbar;
  WaypointList* _list;
  int _scrollWidth;
  bool _hasVScroll;
};

#endif // GUI_WAYPOINT_LIST_HXX
