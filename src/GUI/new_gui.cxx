// new_gui.cxx: implementation of XML-configurable GUI support.

#include "new_gui.hxx"

#include <plib/pu.h>
#include <plib/ul.h>

#include <simgear/misc/exception.hxx>
#include <Main/fg_props.hxx>

#include "menubar.hxx"
#include "dialog.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of NewGUI.
////////////////////////////////////////////////////////////////////////


NewGUI::NewGUI ()
    : _menubar(new FGMenuBar),
      _current_widget(0)
{
}

NewGUI::~NewGUI ()
{
    delete _menubar;
}

void
NewGUI::init ()
{
    char path1[1024];
    char path2[1024];
    ulMakePath(path1, globals->get_fg_root().c_str(), "gui");
    ulMakePath(path2, path1, "dialogs");
    readDir(path2);
#if !defined(FG_OLD_MENUBAR)
    _menubar->init();
#endif
}

void
NewGUI::bind ()
{
    fgTie("/sim/menubar/visibility", this,
          &NewGUI::getMenuBarVisible, &NewGUI::setMenuBarVisible);
}

void
NewGUI::unbind ()
{
    fgUntie("/sim/menubar/visibility");
}

void
NewGUI::update (double delta_time_sec)
{
    // NO OP
}

void
NewGUI::display (const string &name)
{
    if (_widgets.find(name) == _widgets.end())
        SG_LOG(SG_GENERAL, SG_ALERT, "Dialog " << name << " not defined");
    else
        new FGDialog(_widgets[name]);
}

void
NewGUI::setCurrentWidget (FGDialog * widget)
{
    _current_widget = widget;
}

FGDialog *
NewGUI::getCurrentWidget ()
{
    return _current_widget;
}

FGMenuBar *
NewGUI::getMenuBar ()
{
    return _menubar;
}

bool
NewGUI::getMenuBarVisible () const
{
    return _menubar->isVisible();
}

void
NewGUI::setMenuBarVisible (bool visible)
{
    if (visible)
        _menubar->show();
    else
        _menubar->hide();
}

void
NewGUI::readDir (const char * path)
{
    ulDir * dir = ulOpenDir(path);

    if (dir == 0) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Failed to read GUI files from "
               << path);
        return;
    }

    ulDirEnt * dirEnt = ulReadDir(dir);
    while (dirEnt != 0) {
        char subpath[1024];

        ulMakePath(subpath, path, dirEnt->d_name);

        if (dirEnt->d_isdir && dirEnt->d_name[0] != '.') {
            readDir(subpath);
        } else {
            SGPropertyNode_ptr props = new SGPropertyNode;
            try {
                readProperties(subpath, props);
            } catch (const sg_exception &ex) {
                SG_LOG(SG_INPUT, SG_ALERT, "Error parsing GUI file "
                       << subpath);
            }
            if (!props->hasValue("name")) {
                SG_LOG(SG_INPUT, SG_WARN, "GUI file " << subpath
                   << " has no name; skipping.");
            } else {
                string name = props->getStringValue("name");
                SG_LOG(SG_INPUT, SG_BULK, "Saving GUI node " << name);
                _widgets[name] = props;
            }
        }
        dirEnt = ulReadDir(dir);
    }
    ulCloseDir(dir);
}

// end of new_gui.cxx
