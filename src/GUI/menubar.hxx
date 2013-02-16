// menubar.hxx - XML-configured menu bar.

#ifndef __MENUBAR_HXX
#define __MENUBAR_HXX 1

class SGPropertyNode;

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
    FGMenuBar();

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

    /**
     * Read a menu label from the menu's property tree.
     * Take care of mapping it to the appropriate translation, if available.
     * Returns an UTF-8 encoded string.
     */
    static const char* getLocalizedLabel(SGPropertyNode* node);

};

#endif // __MENUBAR_HXX
