// new_gui.cxx: implementation of XML-configurable GUI support.

#include "new_gui.hxx"

#include <plib/pu.h>
#include <plib/ul.h>

#include <simgear/compiler.h>
#include <simgear/misc/exception.hxx>
#include <Main/fg_props.hxx>

#include "menubar.hxx"
#include "dialog.hxx"

SG_USING_STD(map);


////////////////////////////////////////////////////////////////////////
// Implementation of NewGUI.
////////////////////////////////////////////////////////////////////////


NewGUI::NewGUI ()
    : _menubar(new FGMenuBar),
      _active_dialog(0)
{
}

NewGUI::~NewGUI ()
{
    clear();
}

void
NewGUI::init ()
{
    char path1[1024];
    char path2[1024];
    ulMakePath(path1, globals->get_fg_root().c_str(), "gui");
    ulMakePath(path2, path1, "dialogs");
    readDir(path2);
    _menubar->init();
}

void
NewGUI::reinit ()
{
    unbind();
    clear();
    _menubar = new FGMenuBar;
    init();
    bind();
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

bool
NewGUI::showDialog (const string &name)
{
    if (_dialog_props.find(name) == _dialog_props.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Dialog " << name << " not defined");
        return false;
    } else {
        new FGDialog(_dialog_props[name]); // it will be deleted by a callback
        return true;
    }
}

bool
NewGUI::closeActiveDialog ()
{
    if (_active_dialog == 0) {
        return false;
    } else {
        delete _active_dialog;
        _active_dialog = 0;
        return true;
    }
}

void
NewGUI::setActiveDialog (FGDialog * dialog)
{
    _active_dialog = dialog;
}

FGDialog *
NewGUI::getActiveDialog ()
{
    return _active_dialog;
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
NewGUI::clear ()
{
    delete _menubar;
    _menubar = 0;

    map<string,SGPropertyNode *>::iterator it;
    for (it = _dialog_props.begin(); it != _dialog_props.end(); it++)
        delete it->second;
    _dialog_props.clear();
}

static bool
test_extension (const char * path, const char * ext)
{
    int pathlen = strlen(path);
    int extlen = strlen(ext);
    
    for (int i = 1; i <= pathlen && i <= extlen; i++) {
        if (path[pathlen-i] != ext[extlen-i])
            return false;
    }
    return true;
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

    for (ulDirEnt * dirEnt = ulReadDir(dir);
         dirEnt != 0;
         dirEnt = ulReadDir(dir)) {

        char subpath[1024];

        ulMakePath(subpath, path, dirEnt->d_name);

        if (!dirEnt->d_isdir && test_extension(subpath, ".xml")) {
            SGPropertyNode * props = new SGPropertyNode;
            try {
                readProperties(subpath, props);
            } catch (const sg_exception &ex) {
                SG_LOG(SG_INPUT, SG_ALERT, "Error parsing dialog "
                       << subpath);
                delete props;
                continue;
            }
            if (!props->hasValue("name")) {
                SG_LOG(SG_INPUT, SG_WARN, "dialog " << subpath
                   << " has no name; skipping.");
                delete props;
                continue;
            }
            string name = props->getStringValue("name");
            SG_LOG(SG_INPUT, SG_BULK, "Saving dialog " << name);
            if (_dialog_props[name] != 0)
                delete _dialog_props[name];
            _dialog_props[name] = props;
        }
    }
    ulCloseDir(dir);
}

// end of new_gui.cxx
