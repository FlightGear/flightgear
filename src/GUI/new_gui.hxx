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


class NewGUI : public FGSubsystem
{
public:

    NewGUI ();
    virtual ~NewGUI ();
    virtual void init ();
    virtual void update (double delta_time_sec);

    virtual void display (const string &name);

    virtual void updateProperties ();
    virtual void applyProperties ();
    virtual void closeActiveObject ();

private:

    void readDir (const char * path);

    puObject * makeObject (SGPropertyNode * props,
                           int parentWidth, int parentHeight);

    void setupObject (puObject * object, SGPropertyNode * props);

    void setupGroup (puGroup * group, SGPropertyNode * props,
                     int width, int height, bool makeFrame = false);

    map<string,SGPropertyNode_ptr> _objects;
    puObject * _activeObject;

    struct PropertyObject {
        PropertyObject (puObject * object, SGPropertyNode_ptr node);
        puObject * object;
        SGPropertyNode_ptr node;
    };

    vector<PropertyObject> _propertyObjects;

};

#endif // __NEW_GUI_HXX
