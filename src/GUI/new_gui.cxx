// new_gui.cxx: implementation of XML-configurable GUI support.

#include "new_gui.hxx"

#include <plib/pu.h>
#include <plib/ul.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>

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
    map<string,FGDialog *>::iterator iter = _active_dialogs.begin();
    for(/**/; iter != _active_dialogs.end(); iter++)
        iter->second->update();
}

bool
NewGUI::showDialog (const string &name)
{
    if (_dialog_props.find(name) == _dialog_props.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Dialog " << name << " not defined");
        return false;
    } else {
        if(!_active_dialogs[name])
            _active_dialogs[name] = new FGDialog(_dialog_props[name]);
        return true;
    }
}

bool
NewGUI::closeActiveDialog ()
{
    if (_active_dialog == 0)
        return false;

    // Kill any entries in _active_dialogs...  Is there an STL
    // algorithm to do (delete map entries by value, not key)?  I hate
    // the STL :) -Andy
    map<string,FGDialog *>::iterator iter = _active_dialogs.begin();
    for(/**/; iter != _active_dialogs.end(); iter++) {
        if(iter->second == _active_dialog) {
            _active_dialogs.erase(iter);
            // iter is no longer valid
            break;
        }
    }

    delete _active_dialog;
    _active_dialog = 0;
    return true;
}

bool
NewGUI::closeDialog (const string& name)
{
    if(_active_dialogs.find(name) != _active_dialogs.end()) {
        if(_active_dialog == _active_dialogs[name])
            _active_dialog = 0;
        delete _active_dialogs[name];
        _active_dialogs.erase(name);
        return true;
    }
    return false; // dialog wasn't open...
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
NewGUI::newDialog (SGPropertyNode* props)
{
    const char* cname = props->getStringValue("name");
    if(!cname) {
        SG_LOG(SG_GENERAL, SG_ALERT, "New dialog has no <name> property");
        return;
    }
    string name = props->getStringValue("name");
    _dialog_props[name] = props;
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
            } catch (const sg_exception &) {
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
            newDialog(props);
        }
    }
    ulCloseDir(dir);
}

// end of new_gui.cxx
