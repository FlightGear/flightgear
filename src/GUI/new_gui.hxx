// new_gui.hxx - XML-configurable GUI subsystem.

#ifndef __NEW_GUI_HXX
#define __NEW_GUI_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <plib/pu.h>

#include <simgear/compiler.h>	// for SG_USING_STD
#include <simgear/misc/props.hxx>

#include <vector>
SG_USING_STD(vector);

#include <map>
SG_USING_STD(map);

#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>
#include <Input/input.hxx>

class FGMenuBar;
class GUIWidget;


/**
 * Information about a GUI widget.
 */
struct GUIInfo
{
    GUIInfo (GUIWidget * w);
    virtual ~GUIInfo ();

    GUIWidget * widget;
    vector <FGBinding *> bindings;
};


/**
 * Top-level GUI widget.
 */
class GUIWidget
{
public:


    /**
     * Construct a new GUI widget configured by a property tree.
     */
    GUIWidget (SGPropertyNode_ptr props);


    /**
     * Destructor.
     */
    virtual ~GUIWidget ();


    /**
     * Update the values of all GUI objects with a specific name.
     *
     * This method copies from the property to the GUI object.
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all unnamed objects.
     */
    virtual void updateValue (const char * objectName);


    /**
     * Apply the values of all GUI objects with a specific name.
     *
     * This method copies from the GUI object to the property.
     *
     * @param objectName The name of the GUI object(s) to update.
     *        Use the empty name for all unnamed objects.
     */
    virtual void applyValue (const char * objectName);


    /**
     * Update the values of all GUI objects.
     *
     * This method copies from the properties to the GUI objects.
     */
    virtual void updateValues ();


    /**
     * Apply the values of all GUI objects.
     *
     * This method copies from the GUI objects to the properties.
     */
    virtual void applyValues ();


private:
    GUIWidget (const GUIWidget &); // just for safety

    void display (SGPropertyNode_ptr props);
    puObject * makeObject (SGPropertyNode * props,
                           int parentWidth, int parentHeight);
    void setupObject (puObject * object, SGPropertyNode * props);
    void setupGroup (puGroup * group, SGPropertyNode * props,
                     int width, int height, bool makeFrame = false);

    puObject * _object;
    vector<GUIInfo *> _info;
    struct PropertyObject {
        PropertyObject (const char * name,
                        puObject * object,
                        SGPropertyNode_ptr node);
        string name;
        puObject * object;
        SGPropertyNode_ptr node;
    };
    vector<PropertyObject *> _propertyObjects;
};


class NewGUI : public FGSubsystem
{
public:

    NewGUI ();
    virtual ~NewGUI ();
    virtual void init ();
    virtual void update (double delta_time_sec);
    virtual void display (const string &name);

    virtual void setCurrentWidget (GUIWidget * widget);
    virtual GUIWidget * getCurrentWidget ();

    virtual FGMenuBar * getMenuBar ();


private:

    void readDir (const char * path);

    FGMenuBar * _menubar;
    GUIWidget * _current_widget;
    map<string,SGPropertyNode_ptr> _widgets;

};


#endif // __NEW_GUI_HXX

// end of new_gui.hxx
