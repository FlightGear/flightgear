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


class GUIWidget
{
public:
    GUIWidget (SGPropertyNode_ptr props);
    virtual ~GUIWidget ();

    virtual void updateProperties ();
    virtual void applyProperties ();

private:

    void display (SGPropertyNode_ptr props);

    GUIWidget (const GUIWidget &); // just for safety

    puObject * makeObject (SGPropertyNode * props,
                           int parentWidth, int parentHeight);

    void setupObject (puObject * object, SGPropertyNode * props);

    void setupGroup (puGroup * group, SGPropertyNode * props,
                     int width, int height, bool makeFrame = false);

    puObject * _object;

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
