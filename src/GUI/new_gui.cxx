// new_gui.cxx: implementation of XML-configurable GUI support.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "new_gui.hxx"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <sys/types.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_dir.hxx>

#include <Add-ons/AddonManager.hxx>
#include <Main/fg_props.hxx>

#if defined(SG_UNIX) && !defined(SG_MAC) 
#include "GL/glx.h"
#endif

#include "FGPUIMenuBar.hxx"

#if defined(SG_MAC)
#include "FGCocoaMenuBar.hxx"
#endif

#if defined(SG_WINDOWS)
#include "FGWindowsMenuBar.hxx"
#endif

#include "FGPUIDialog.hxx"
#include "FGFontCache.hxx"
#include "FGColor.hxx"

// ignore the word Navaid here, it's a DataCache
#include <Navaids/NavDataCache.hxx>

using std::map;
using std::string;

extern void puCleanUpJunk(void);

////////////////////////////////////////////////////////////////////////
// Implementation of NewGUI.
////////////////////////////////////////////////////////////////////////



NewGUI::NewGUI () :
  _active_dialog(0)
{
}

NewGUI::~NewGUI ()
{
    for (_itt_t it = _colors.begin(); it != _colors.end(); ++it)
        delete it->second;
}

void
NewGUI::init ()
{
    createMenuBarImplementation();
    fgTie("/sim/menubar/visibility", this,
          &NewGUI::getMenuBarVisible, &NewGUI::setMenuBarVisible);
    
    fgTie("/sim/menubar/overlap-hide", this,
          &NewGUI::getMenuBarOverlapHide, &NewGUI::setMenuBarOverlapHide);

    setStyle();
    SGPath p(globals->get_fg_root(), "gui/dialogs");
    readDir(p);
    
    SGPath aircraftDialogDir(string(fgGetString("/sim/aircraft-dir")), "gui/dialogs");
    if (aircraftDialogDir.exists()) {
        readDir(aircraftDialogDir);
    }

    // Read XML dialogs made available by registered add-ons
    const auto& addonManager = flightgear::addons::AddonManager::instance();
    for (const auto& addon: addonManager->registeredAddons()) {
        SGPath addonDialogDir = addon->getBasePath() / "gui/dialogs";

        if (addonDialogDir.exists()) {
            readDir(addonDialogDir);
        }
    }

    // Fix for http://code.google.com/p/flightgear-bugs/issues/detail?id=947
    fgGetNode("sim/menubar")->setAttribute(SGPropertyNode::PRESERVE, true);
    _menubar->init();
}

void
NewGUI::shutdown()
{
    DialogDict::iterator it = _active_dialogs.begin();
    for (; it != _active_dialogs.end(); ++it) {
        delete it->second;
    }
    _active_dialogs.clear();
    
    fgUntie("/sim/menubar/visibility");
    _menubar.reset();
    _dialog_props.clear();

    puCleanUpJunk();
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
NewGUI::createMenuBarImplementation()
{
#if defined(SG_MAC)
    if (fgGetBool("/sim/menubar/native", true)) {
        _menubar.reset(new FGCocoaMenuBar);
    }
#endif
#if defined(SG_WINDOWS)
	if (fgGetBool("/sim/menubar/native", true)) {
	// Windows-native menubar disabled for the moment, fall-through
	// to PUI version
   //     _menubar.reset(new FGWindowsMenuBar);
    }
#endif
    if (!_menubar.get()) {
        _menubar.reset(new FGPUIMenuBar);
    }
}

void
NewGUI::reset (bool reload)
{
    DialogDict::iterator iter;
    string_list openDialogs;
    // close all open dialogs and remember them ...
    for (iter = _active_dialogs.begin(); iter != _active_dialogs.end(); ++iter)
        openDialogs.push_back(iter->first);

    for (auto d : openDialogs)
        closeDialog(d);

    setStyle();

    unbind();

    if (reload) {
        _dialog_props.clear();
        _dialog_names.clear();
        init();
    } else {
        createMenuBarImplementation();
        _menubar->init();
    }

    bind();

    // open dialogs again
    for (auto d : openDialogs)
        showDialog(d);
}

void
NewGUI::bind ()
{
}

void
NewGUI::unbind ()
{
}

void
NewGUI::update (double delta_time_sec)
{
    SG_UNUSED(delta_time_sec);
    map<string,FGDialog *>::iterator iter = _active_dialogs.begin();
    for(/**/; iter != _active_dialogs.end(); iter++)
        iter->second->update();
}

bool
NewGUI::showDialog (const string &name)
{
    // first, check if it's already shown
    if (_active_dialogs.find(name) != _active_dialogs.end()){
        _active_dialogs[name]->bringToFront();
        return true;
    }
  
    // check we know about the dialog by name
    if (_dialog_names.find(name) == _dialog_names.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Dialog " << name << " not defined");
        return false;
    }
    _active_dialogs[name] = new FGPUIDialog(getDialogProperties(name));
    fgSetString("/sim/gui/dialogs/current-dialog", name);

//    setActiveDialog(new FGPUIDialog(getDialogProperties(name)));
    return true;
}

bool
NewGUI::toggleDialog (const string &name)
{
    if (_active_dialogs.find(name) == _active_dialogs.end()) {
        showDialog(name);
        return true;
    }
    else {
        closeDialog(name);
        return false;
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
    if (_active_dialogs.size())
    {
        fgSetString("/sim/gui/dialogs/current-dialog", _active_dialogs.begin()->second->getName());
    }
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
    if (_dialog_names.find(name) == _dialog_names.end()) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Dialog " << name << " not defined");
      return NULL;
    }
  
    NameDialogDict::iterator it = _dialog_props.find(name);
    if (it == _dialog_props.end()) {
      // load the XML
      SGPath path = _dialog_names[name];
      SGPropertyNode_ptr props = new SGPropertyNode;
      try {
        readProperties(path, props);
      } catch (const sg_exception &) {
        SG_LOG(SG_INPUT, SG_ALERT, "Error parsing dialog " << path);
        return NULL;
      }
      
      it = _dialog_props.insert(it, std::make_pair(name, props));
    }

    return it->second;
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
    if (dialog){
        fgSetString("/sim/gui/dialogs/current-dialog", dialog->getName());
    }
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

bool
NewGUI::getMenuBarOverlapHide() const
{
    return _menubar->getHideIfOverlapsWindow();
}

void
NewGUI::setMenuBarOverlapHide(bool hide)
{
    _menubar->setHideIfOverlapsWindow(hide);
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
  
    if(_active_dialogs.find(name) == _active_dialogs.end()) {
        _dialog_props[name] = props;
    // add a dummy path entry, so we believe the dialog exists
        _dialog_names[name] = SGPath();
    }
}

void
NewGUI::readDir (const SGPath& path)
{
    simgear::Dir dir(path);
    if( !dir.exists() )
    {
      SG_LOG(SG_INPUT, SG_INFO, "directory does not exist: " << path);
      return;
    }

    flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
    flightgear::NavDataCache::Transaction txn(cache);
    for (SGPath xmlPath : dir.children(simgear::Dir::TYPE_FILE, ".xml")) {
      if (!cache->isCachedFileModified(xmlPath)) {
        // cached, easy
        string name = cache->readStringProperty(xmlPath.utf8Str());
        _dialog_names[name] = xmlPath;
        continue;
      }
      
    // we need to parse the actual XML
      SGPropertyNode_ptr props = new SGPropertyNode;
      try {
        readProperties(xmlPath, props);
      } catch (const sg_exception &) {
        SG_LOG(SG_INPUT, SG_ALERT, "Error parsing dialog " << xmlPath);
        continue;
      }
      
      SGPropertyNode *nameprop = props->getNode("name");
      if (!nameprop) {
        SG_LOG(SG_INPUT, SG_WARN, "dialog " << xmlPath << " has no name; skipping.");
        continue;
      }
      
      string name = nameprop->getStringValue();
      _dialog_names[name] = xmlPath;
    // update cached values
        if (!cache->isReadOnly()) {
            cache->stampCacheFile(xmlPath);
            cache->writeStringProperty(xmlPath.utf8Str(), name);
        }
    } // of directory children iteration
  
    txn.commit();
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

    
    if (0) {
        // Re-read gui/style/*.xml files so that one can edit them and see the
        // results without restarting flightgear.
        SGPath  p(globals->get_fg_root(), "gui/styles");
        SGPropertyNode* sim_gui = globals->get_props()->getNode("sim/gui/", true);
        simgear::Dir dir(p);
        int i = 0;
        for (SGPath xml: dir.children(simgear::Dir::TYPE_FILE, ".xml")) {
            SGPropertyNode_ptr node = sim_gui->getChild("style", i, true);
            node->removeAllChildren();
            SG_LOG(SG_GENERAL, SG_WARN, "reading from " << xml << " into " << node->getPath());
            readProperties(xml, node);
            i += 1;
        }
    }
    
    int which = fgGetInt("/sim/gui/current-style", 0);
    SGPropertyNode *sim = globals->get_props()->getNode("sim/gui", true);
    SGPropertyNode *n = sim->getChild("style", which);
    if (!n)
        n = sim->getChild("style", 0, true);
    
    SGPropertyNode *selected_style = globals->get_props()->getNode("sim/gui/selected-style", true);

    // n->copy() doesn't delete existing nodes, so need to clear them all
    // first.
    selected_style->removeAllChildren();
    n->copy(selected_style);

    //if (selected_style && n)
    //    n->alias(selected_style);

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
    _font = FGFontCache::instance()->get(node);
    puSetDefaultFonts(*_font, *_font);
    return;
}


// Register the subsystem.
SGSubsystemMgr::Registrant<NewGUI> registrantNewGUI(
    SGSubsystemMgr::INIT);

// end of new_gui.cxx
