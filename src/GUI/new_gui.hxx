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

class FGMenuBar;
class FGDialog;
class FGBinding;


class NewGUI : public FGSubsystem
{
public:

    NewGUI ();
    virtual ~NewGUI ();
    virtual void init ();
    virtual void update (double delta_time_sec);
    virtual void display (const string &name);

    virtual void setCurrentWidget (FGDialog * widget);
    virtual FGDialog * getCurrentWidget ();

    virtual FGMenuBar * getMenuBar ();


private:

    void readDir (const char * path);

    FGMenuBar * _menubar;
    FGDialog * _current_widget;
    map<string,SGPropertyNode_ptr> _widgets;

};


#endif // __NEW_GUI_HXX

// end of new_gui.hxx
