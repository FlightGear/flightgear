#ifndef __MENUBAR_HXX
#define __MENUBAR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>	// for SG_USING_STD

#include <plib/pu.h>

#include <map>
SG_USING_STD(map);

#include <vector>
SG_USING_STD(vector);


class puMenuBar;
class puObject;
class FGBinding;


/**
 * XML-configured PUI menu bar.
 */
class FGMenuBar
{
public:

    /**
     * Constructor.
     */
    FGMenuBar ();


    /**
     * Destructor.
     */
    virtual ~FGMenuBar ();


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


    /**
     * IGNORE THIS METHOD!!!
     *
     * This is necessary only because plib does not provide any easy
     * way to attach user data to a menu item.  FlightGear should not
     * have to know about PUI internals, but this method allows the
     * callback to pass the menu item one-shot on to the current menu.
     */
    virtual void fireItem (puObject * item);


private:

    void make_menu (SGPropertyNode_ptr node);
    void make_menubar ();

    bool _visible;
    puMenuBar * _menuBar;
    map<string,vector<FGBinding *>*> _bindings;
};

#endif // __MENUBAR_HXX
