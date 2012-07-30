// new_gui.cxx: implementation of XML-configurable GUI support.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "new_gui.hxx"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <sys/types.h>

#include <plib/pu.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_dir.hxx>

#include <boost/algorithm/string/case_conv.hpp>

#include <Main/fg_props.hxx>

#if defined(SG_UNIX) && !defined(SG_MAC) 
#include "GL/glx.h"
#endif

#include "FGPUIMenuBar.hxx"

#if defined(SG_MAC)
#include "FGCocoaMenuBar.hxx"
#endif

#include "FGPUIDialog.hxx"
#include "FGFontCache.hxx"
#include "FGColor.hxx"

using std::map;
using std::string;

////////////////////////////////////////////////////////////////////////
// Implementation of NewGUI.
////////////////////////////////////////////////////////////////////////



NewGUI::NewGUI () :
  _active_dialog(0)
{
#if defined(SG_MAC)
    if (fgGetBool("/sim/menubar/native", true)) {
        _menubar.reset(new FGCocoaMenuBar);
        return;
    }
#endif
  _menubar.reset(new FGPUIMenuBar);
}

NewGUI::~NewGUI ()
{
    _dialog_props.clear();
    for (_itt_t it = _colors.begin(); it != _colors.end(); ++it)
        delete it->second;
}

void
NewGUI::init ()
{
    setStyle();
    SGPath p(globals->get_fg_root(), "gui/dialogs");
    readDir(p);
    const std::string aircraft_dir(fgGetString("/sim/aircraft-dir"));
    readDir( SGPath(aircraft_dir, "gui/dialogs") );
    _menubar->init();
}

void
NewGUI::reinit ()
{
    reset(true);
    fgSetBool("/sim/signals/reinit-gui", true);
}

void
NewGUI::redraw ()
{
    reset(false);
}

void
NewGUI::reset (bool reload)
{
    map<string,FGDialog *>::iterator iter;
    std::vector<string> dlg;
    // close all open dialogs and remember them ...
    for (iter = _active_dialogs.begin(); iter != _active_dialogs.end(); ++iter)
        dlg.push_back(iter->first);

    unsigned int i;
    for (i = 0; i < dlg.size(); i++)
        closeDialog(dlg[i]);

    setStyle();

    unbind();
#if !defined(SG_MAC)
    _menubar.reset(new FGPUIMenuBar);
#endif

    if (reload) {
        _dialog_props.clear();
        init();
    } else {
        _menubar->init();
    }

    bind();

    // open dialogs again
    for (i = 0; i < dlg.size(); i++)
        showDialog(dlg[i]);
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
            _active_dialogs[name] = new FGPUIDialog(_dialog_props[name]);
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

SGPropertyNode_ptr
NewGUI::getDialogProperties (const string &name)
{
    if(_dialog_props.find(name) != _dialog_props.end())
        return _dialog_props[name];

    SG_LOG(SG_GENERAL, SG_DEBUG, "dialog '" << name << "' missing");
    return 0;
}

FGDialog *
NewGUI::getDialog (const string &name)
{
    if(_active_dialogs.find(name) != _active_dialogs.end())
        return _active_dialogs[name];

    SG_LOG(SG_GENERAL, SG_DEBUG, "dialog '" << name << "' missing");
    return 0;
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
    return _menubar.get();
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
NewGUI::newDialog (SGPropertyNode* props)
{
    const char* cname = props->getStringValue("name");
    if(!cname) {
        SG_LOG(SG_GENERAL, SG_ALERT, "New dialog has no <name> property");
        return;
    }
    string name = cname;
    if(_active_dialogs.find(name) == _active_dialogs.end())
        _dialog_props[name] = props;
}

void
NewGUI::readDir (const SGPath& path)
{
    simgear::Dir dir(path);
    simgear::PathList xmls = dir.children(simgear::Dir::TYPE_FILE, ".xml");
    
    for (unsigned int i=0; i<xmls.size(); ++i) {
      SGPropertyNode * props = new SGPropertyNode;
      try {
          readProperties(xmls[i].str(), props);
      } catch (const sg_exception &) {
          SG_LOG(SG_INPUT, SG_ALERT, "Error parsing dialog "
                 << xmls[i].str());
          delete props;
          continue;
      }
      SGPropertyNode *nameprop = props->getNode("name");
      if (!nameprop) {
          SG_LOG(SG_INPUT, SG_WARN, "dialog " << xmls[i].str()
             << " has no name; skipping.");
          delete props;
          continue;
      }
      string name = nameprop->getStringValue();
      _dialog_props[name] = props;
    }
}



////////////////////////////////////////////////////////////////////////
// Style handling.
////////////////////////////////////////////////////////////////////////

void
NewGUI::setStyle (void)
{
    _itt_t it;
    for (it = _colors.begin(); it != _colors.end(); ++it)
      delete it->second;
    _colors.clear();

    // set up the traditional colors as default
    _colors["background"] = new FGColor(0.8f, 0.8f, 0.9f, 0.85f);
    _colors["foreground"] = new FGColor(0.0f, 0.0f, 0.0f, 1.0f);
    _colors["highlight"]  = new FGColor(0.7f, 0.7f, 0.7f, 1.0f);
    _colors["label"]      = new FGColor(0.0f, 0.0f, 0.0f, 1.0f);
    _colors["legend"]     = new FGColor(0.0f, 0.0f, 0.0f, 1.0f);
    _colors["misc"]       = new FGColor(0.0f, 0.0f, 0.0f, 1.0f);
    _colors["inputfield"] = new FGColor(0.8f, 0.7f, 0.7f, 1.0f);

    //puSetDefaultStyle();

    int which = fgGetInt("/sim/gui/current-style", 0);
    SGPropertyNode *sim = globals->get_props()->getNode("sim/gui", true);
    SGPropertyNode *n = sim->getChild("style", which);
    if (!n)
        n = sim->getChild("style", 0, true);

    setupFont(n->getNode("fonts/gui", true));
    n = n->getNode("colors", true);

    for (int i = 0; i < n->nChildren(); i++) {
        SGPropertyNode *child = n->getChild(i);
        _colors[child->getName()] = new FGColor(child);
    }

    FGColor *c = _colors["background"];
    puSetDefaultColourScheme(c->red(), c->green(), c->blue(), c->alpha());
}


void
NewGUI::setupFont (SGPropertyNode *node)
{
    _font = globals->get_fontcache()->get(node);
    puSetDefaultFonts(*_font, *_font);
    return;
}

// end of new_gui.cxx
