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


class GUIWidget;


/**
 * User data attached to a GUI object.
 */
struct GUIData
{
    GUIData (GUIWidget * w, const char * a);
    GUIWidget * widget;
    string command;
};


/**
 * Top-level GUI widget.
 */
class GUIWidget
{
public:
    GUIWidget (SGPropertyNode_ptr props);
    virtual ~GUIWidget ();

    virtual void action (const string &command);

private:
    GUIWidget (const GUIWidget &); // just for safety
    void display (SGPropertyNode_ptr props);
    virtual void updateProperties ();
    virtual void applyProperties ();
    puObject * makeObject (SGPropertyNode * props,
                           int parentWidth, int parentHeight);
    void setupObject (puObject * object, SGPropertyNode * props);
    void setupGroup (puGroup * group, SGPropertyNode * props,
                     int width, int height, bool makeFrame = false);

    puObject * _object;
    vector<GUIData> _actions;
    struct PropertyObject {
        PropertyObject (puObject * object, SGPropertyNode_ptr node);
        puObject * object;
        SGPropertyNode_ptr node;
    };
    vector<PropertyObject> _propertyObjects;
};


class NewGUI : public FGSubsystem
{
public:

    NewGUI ();
    virtual ~NewGUI ();
    virtual void init ();
    virtual void update (double delta_time_sec);
    virtual void display (const string &name);

private:

    void readDir (const char * path);

    map<string,SGPropertyNode_ptr> _widgets;

};

#endif // __NEW_GUI_HXX

// end of new_gui.hxx
