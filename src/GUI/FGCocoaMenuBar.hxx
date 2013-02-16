// menubar.hxx - XML-configured menu bar.

#ifndef FG_COCOA_MENUBAR_HXX
#define FG_COCOA_MENUBAR_HXX 1

#include <GUI/menubar.hxx>

#include <memory>

/**
 * XML-configured Cocoa menu bar.
 *
 * This class creates a menu bar from a tree of XML properties.  These
 * properties are not part of the main FlightGear property tree, but
 * are read from a separate file ($FG_ROOT/gui/menubar.xml).
 *
 * WARNING: because PUI provides no easy way to attach user data to a
 * menu item, all menu item strings must be unique; otherwise, this
 * class will always use the first binding with any given name.
 */
class FGCocoaMenuBar : public FGMenuBar
{
public:
  
  /**
   * Constructor.
   */
  FGCocoaMenuBar ();
  
  
  /**
   * Destructor.
   */
  virtual ~FGCocoaMenuBar ();
  
  
  /**
   * Initialize the menu bar from $FG_ROOT/gui/menubar.xml
   */
  virtual void init ();
  
  /**
   * Make the menu bar visible.
   */
  virtual void show ();
  
  
  /**
   * Make the menu bar invisible.
   */
  virtual void hide ();
  
  
  /**
   * Test whether the menu bar is visible.
   */
  virtual bool isVisible () const;
  
  class CocoaMenuBarPrivate;
private:
  std::auto_ptr<CocoaMenuBarPrivate> p;
  
};

/**
 * open a URL using the system's web-browser
 */
void cocoaOpenUrl(const std::string& url);

#endif // __MENUBAR_HXX
