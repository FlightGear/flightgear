// menubar.hxx - XML-configured menu bar.

#ifndef __MENUBAR_HXX
#define __MENUBAR_HXX 1


/**
 * XML-configured menu bar interface
 *
 * This class creates a menu bar from a tree of XML properties.  These
 * properties are not part of the main FlightGear property tree, but
 * are read from a separate file ($FG_ROOT/gui/menubar.xml).

 */
class FGMenuBar
{
public:


    /**
     * Destructor.
     */
    virtual ~FGMenuBar ();


    /**
     * Initialize the menu bar from $FG_ROOT/gui/menubar.xml
     */
    virtual void init () = 0;
    
    /**
     * Make the menu bar visible.
     */
    virtual void show () = 0;


    /**
     * Make the menu bar invisible.
     */
    virtual void hide () = 0;


    /**
     * Test whether the menu bar is visible.
     */
    virtual bool isVisible () const = 0;

};

#endif // __MENUBAR_HXX
