// new_gui.cxx: implementation of XML-configurable GUI support.

/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include "new_gui.hxx"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <sys/types.h>

#include <simgear/compiler.h>
#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include <Add-ons/AddonManager.hxx>
#include <Main/fg_props.hxx>
#include <Main/sentryIntegration.hxx>
#include <Scripting/NasalSys.hxx>

#if defined(SG_UNIX) && !defined(SG_MAC) 
#include "GL/glx.h"
#endif


#if defined(SG_MAC)
#include "FGCocoaMenuBar.hxx"
#endif

#if defined(SG_WINDOWS)
#include "FGWindowsMenuBar.hxx"
#endif

// ensure we include this before puAux.h, so that
// #define _PU_H_ 1 has been done, and hence we don't
// include the un-modified system pu.h
#include "FlightGear_pu.h"

#include <plib/puAux.h>


#include "FGPUIDialog.hxx"
#include "FGPUIMenuBar.hxx"

#include "FGFontCache.hxx"
#include "FGColor.hxx"

#include "Highlight.hxx"

// ignore the word Navaid here, it's a DataCache
#include <Navaids/NavDataCache.hxx>

using std::map;
using std::string;

extern void puCleanUpJunk(void);

////////////////////////////////////////////////////////////////////////
// Implementation of NewGUI.
////////////////////////////////////////////////////////////////////////


NewGUI::NewGUI()
{
}

NewGUI::~NewGUI ()
{
    for (_itt_t it = _colors.begin(); it != _colors.end(); ++it)
        delete it->second;
}

// Recursively finds all nodes called <leaf> and appends the string values of
// these nodes to <out>.
//
static void findAllLeafValues(SGPropertyNode* node, const std::string& leaf, std::vector<std::string>& out)
{
    string name = node->getNameString();
    if (name == leaf) {
        out.push_back(node->getStringValue());
    }
    for (int i=0; i<node->nChildren(); ++i) {
        findAllLeafValues(node->getChild(i), leaf, out);
    }
}


/* Registers menu-dialog associations with Highlight::add_menu_dialog(). */
static void scanMenus()
{
    /* We find all nodes called 'dialog-name' in
    sim/menubar/default/menu[]/item[]. */
    SGPropertyNode* menubar = globals->get_props()->getNode("sim/menubar/default");
    assert(menubar);
    auto highlight = globals->get_subsystem<Highlight>();
    if (!highlight) {
        return;
    }
    for (int menu_p=0; menu_p<menubar->nChildren(); ++menu_p) {
        SGPropertyNode* menu = menubar->getChild(menu_p);
        if (menu->getNameString() != "menu") continue;
        for (int item_p=0; item_p<menu->nChildren(); ++item_p) {
            SGPropertyNode* item = menu->getChild(item_p);
            if (item->getNameString() != "item") continue;
            std::vector<std::string> dialog_names;
            findAllLeafValues(item, "dialog-name", dialog_names);
            for (auto dialog_name: dialog_names) {
                highlight->addMenuDialog(HighlightMenu(menu->getIndex(), item->getIndex()), dialog_name);
            }
        }
    }
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
    
    if (fgGetBool("/sim/gui/startup") == false) {
        SGPath aircraftDialogDir(fgGetString("/sim/aircraft-dir"), "gui/dialogs");
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
    }

    // Fix for http://code.google.com/p/flightgear-bugs/issues/detail?id=947
    fgGetNode("sim/menubar")->setAttribute(SGPropertyNode::PRESERVE, true);
    _menubar->init();
    scanMenus();
}

void
NewGUI::shutdown()
{
    _active_dialogs.clear();
    _active_dialog.clear();

    fgUntie("/sim/menubar/visibility");
    fgUntie("/sim/menubar/overlap-hide");
    _menubar.reset();
    _dialog_props.clear();

#if defined(HAVE_PUI)
    puCleanUpJunk();
#endif
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
    if (!_menubar) {
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

void NewGUI::postinit()
{
    _menubar->postinit();
}

void
NewGUI::update (double delta_time_sec)
{
    SG_UNUSED(delta_time_sec);
    auto iter = _active_dialogs.begin();
    for(/**/; iter != _active_dialogs.end(); iter++)
        iter->second->update();
}

bool
NewGUI::showDialog (const string &name)
{
    if (name.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "showDialog: no dialog name provided");
        return false;
    }

    // first, check if it's already shown
    if (_active_dialogs.find(name) != _active_dialogs.end()){
        _active_dialogs[name]->bringToFront();
        return true;
    }
  
    // check we know about the dialog by name
    if (_dialog_names.find(name) == _dialog_names.end()) {
        simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::GUIDialog, "Dialog not found:" + name);
        return false;
    }

    flightgear::addSentryBreadcrumb("showing GUI dialog:" + name, "info");
    try {
        _active_dialogs[name] = new FGPUIDialog(getDialogProperties(name));

        fgSetString("/sim/gui/dialogs/current-dialog", name);

        //    setActiveDialog(new FGPUIDialog(getDialogProperties(name)));
        return true;
    } catch (sg_exception& e) {
        simgear::reportFailure(simgear::LoadFailure::Misconfigured, simgear::ErrorCode::GUIDialog, "Dialog failed to show:" + name + ":" + e.getFormattedMessage(), e.getLocation());
        return false;
    }
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

    // TODO support a request-close callback here, optionally
    // many places in code assume this code-path does an
    // immediate close, but for some UI paths it would be nice to
    // allow some dialogs the chance to inervene

    // Kill any entries in _active_dialogs...  Is there an STL
    // algorithm to do (delete map entries by value, not key)?  I hate
    // the STL :) -Andy
    auto iter = _active_dialogs.begin();
    for(/**/; iter != _active_dialogs.end(); iter++) {
        if(iter->second == _active_dialog) {
            _active_dialog->close();

            _active_dialogs.erase(iter);
            // iter is no longer valid
            break;
        }
    }

    _active_dialog = 0;
    if (!_active_dialogs.empty()) {
        fgSetString("/sim/gui/dialogs/current-dialog", _active_dialogs.begin()->second->getName());
    }
    return true;
}

bool
NewGUI::closeDialog (const string& name)
{
    if(_active_dialogs.find(name) != _active_dialogs.end()) {
        flightgear::addSentryBreadcrumb("closing GUI dialog:" + name, "info");

        if(_active_dialog == _active_dialogs[name])
            _active_dialog.clear();

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

NewGUI::FGDialogRef
NewGUI::getDialog(const string& name)
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

NewGUI::FGDialogRef
NewGUI::getActiveDialog()
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
    string name = props->getStringValue("name", "");
    if(name.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "New dialog has no <name> property");
        return;
    }
  
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
    auto highlight = globals->get_subsystem<Highlight>();
    for (SGPath xmlPath : dir.children(simgear::Dir::TYPE_FILE, ".xml")) {

      SGPropertyNode_ptr props = new SGPropertyNode;
      SGPropertyNode *nameprop = nullptr;
      std::string name;

      /* Always parse the dialog even if cache->isCachedFileModified() says
      it is cached, so that we can register property-dialog associations with
      Highlight::add_property_dialog(). */
      try {
        readProperties(xmlPath, props);
      } catch (const sg_exception &) {
        SG_LOG(SG_INPUT, SG_ALERT, "Error parsing dialog " << xmlPath);
        props.reset();
      }
      if (props) {
        nameprop = props->getNode("name");
        if (nameprop) {
          name = nameprop->getStringValue();
          // Look for 'property' nodes within the dialog. (could
          // also look for "dialog-name" to catch things like
          // dialog-show.)
          std::vector<std::string> property_paths;
          findAllLeafValues(props, "property", property_paths);
          for (auto property_path: property_paths) {
            // We could maybe hoist this test for hightlight to avoid reaching
            // here if it is null, but that's difficult to test, so we do it
            // here instead.
            if (highlight) {
              highlight->addPropertyDialog(property_path, name);
            }
          }
        }
      }
        
      if (!cache->isCachedFileModified(xmlPath)) {
        // cached, easy
        string name = cache->readStringProperty(xmlPath.utf8Str());
        _dialog_names[name] = xmlPath;
        continue;
      }
      
      // we need to parse the actual XML
      if (!props) {
        SG_LOG(SG_INPUT, SG_ALERT, "Error parsing dialog " << xmlPath);
        continue;
      }
      
      if (!nameprop) {
        SG_LOG(SG_INPUT, SG_WARN, "dialog " << xmlPath << " has no name; skipping.");
        continue;
      }
      
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
    copyProperties(n, selected_style);

    //if (selected_style && n)
    //    n->alias(selected_style);

    setupFont(n->getNode("fonts/gui", true));
    n = n->getNode("colors", true);

    for (int i = 0; i < n->nChildren(); i++) {
        SGPropertyNode *child = n->getChild(i);
        _colors[child->getNameString()] = new FGColor(child);
    }

    FGColor *c = _colors["background"];
#if defined(HAVE_PUI)
    puSetDefaultColourScheme(c->red(), c->green(), c->blue(), c->alpha());
#endif
}


void
NewGUI::setupFont (SGPropertyNode *node)
{
#if defined(HAVE_PUI)
    _font = FGFontCache::instance()->get(node);
    puSetDefaultFonts(*_font, *_font);
#endif
    return;
}


// Register the subsystem.
SGSubsystemMgr::Registrant<NewGUI> registrantNewGUI(
    SGSubsystemMgr::INIT);

// end of new_gui.cxx
