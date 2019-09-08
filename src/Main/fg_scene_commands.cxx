// fg_scene_commands.cxx - internal FGFS commands.

#include "config.h"

#include <string.h>		// strcmp()

#include <simgear/compiler.h>

#include <string>
#include <fstream>

#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/sound/soundmgr.hxx>
#include <simgear/timing/sg_time.hxx>
#include <Network/RemoteXMLRequest.hxx>

#include <FDM/flight.hxx>
#include <GUI/gui.h>
#include <GUI/new_gui.hxx>
#include <GUI/dialog.hxx>
#include <Aircraft/replay.hxx>
#include <Scenery/scenery.hxx>
#include <Scripting/NasalSys.hxx>
#include <Sound/sample_queue.hxx>
#include <Airports/xmlloader.hxx>
#include <Network/HTTPClient.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/view.hxx>
#include <Environment/presets.hxx>
#include <Navaids/NavDataCache.hxx>

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_os.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "globals.hxx"
#include "logger.hxx"
#include "util.hxx"
#include "main.hxx"
#include "positioninit.hxx"

#if FG_HAVE_GPERFTOOLS
# include <google/profiler.h>
#endif

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
#endif


////////////////////////////////////////////////////////////////////////
// Command implementations.
////////////////////////////////////////////////////////////////////////

/**
 * Built-in command: exit FlightGear.
 *
 * status: the exit status to return to the operating system (defaults to 0)
 */
static bool
do_exit (const SGPropertyNode * arg, SGPropertyNode * root)
{
    SG_LOG(SG_INPUT, SG_INFO, "Program exit requested.");
    fgSetBool("/sim/signals/exit", true);
    globals->saveUserSettings();
    fgOSExit(arg->getIntValue("status", 0));
    return true;
}


/**
 * Reset FlightGear (Shift-Escape or Menu->File->Reset)
 */
static bool
do_reset (const SGPropertyNode * arg, SGPropertyNode * root)
{
    fgResetIdleState();
    return true;
}


/**
 * Change aircraft
 */
static bool
do_switch_aircraft (const SGPropertyNode * arg, SGPropertyNode * root)
{
    fgSetString("/sim/aircraft", arg->getStringValue("aircraft"));
    // start a reset
    fgResetIdleState();
    return true;
}

/**
 */
static bool
do_reposition (const SGPropertyNode * arg, SGPropertyNode * root)
{
  fgStartReposition();
  return true;
}

/**
 * Built-in command: (re)load the panel.
 *
 * path (optional): the file name to load the panel from
 * (relative to FG_ROOT).  Defaults to the value of /sim/panel/path,
 * and if that's unspecified, to "Panels/Default/default.xml".
 */
static bool
do_panel_load (const SGPropertyNode * arg, SGPropertyNode * root)
{
  string panel_path = arg->getStringValue("path");
  if (!panel_path.empty()) {
    // write to the standard property, which will force a load
    fgSetString("/sim/panel/path", panel_path.c_str());
  }

  return true;
}

/**
 * Built-in command: (re)load preferences.
 *
 * path (optional): the file name to load the panel from (relative
 * to FG_ROOT). Defaults to "preferences.xml".
 */
static bool
do_preferences_load (const SGPropertyNode * arg, SGPropertyNode * root)
{
    // disabling this command which was formerly used to reload 'preferences.xml'
    // reloading the defaults doesn't make sense (better to reset the simulator),
    // and loading arbitrary XML docs into the property-tree can be done via
    // the standard load-xml command.
    SG_LOG(SG_GENERAL, SG_ALERT, "preferences-load command is deprecated and non-functional");
    return false;
}

/**
 * An fgcommand to toggle fullscreen mode.
 * No parameters.
 */
static bool
do_toggle_fullscreen(const SGPropertyNode*, SGPropertyNode*)
{
    fgOSFullScreen();
    return true;
}

/**
 * Built-in command: capture screen.
 */
static bool
do_screen_capture(const SGPropertyNode*, SGPropertyNode*)
{
  return fgDumpSnapShot();
}

static bool
do_reload_shaders (const SGPropertyNode*, SGPropertyNode*)
{
    simgear::reload_shaders();
    return true;
}

static bool
do_dump_scene_graph (const SGPropertyNode*, SGPropertyNode*)
{
    fgDumpSceneGraph();
    return true;
}

static bool
do_dump_terrain_branch (const SGPropertyNode*, SGPropertyNode*)
{
    fgDumpTerrainBranch();

    double lon_deg = fgGetDouble("/position/longitude-deg");
    double lat_deg = fgGetDouble("/position/latitude-deg");
    SGGeod geodPos = SGGeod::fromDegFt(lon_deg, lat_deg, 0.0);
    SGVec3d zero = SGVec3d::fromGeod(geodPos);

    SG_LOG(SG_INPUT, SG_INFO, "Model parameters:");
    SG_LOG(SG_INPUT, SG_INFO, "Center: " << zero.x() << ", " << zero.y() << ", " << zero.z() );
    SG_LOG(SG_INPUT, SG_INFO, "Rotation: " << lat_deg << ", " << lon_deg );

    return true;
}

static bool
do_print_visible_scene_info(const SGPropertyNode*, SGPropertyNode*)
{
    fgPrintVisibleSceneInfoCommand();
    return true;
}

/**
 * Built-in command: hires capture screen.
 */
static bool
do_hires_screen_capture (const SGPropertyNode * arg, SGPropertyNode * root)
{
  fgHiResDump();
  return true;
}


/**
 * Reload the tile cache.
 */
static bool
do_tile_cache_reload (const SGPropertyNode * arg, SGPropertyNode * root)
{
    SGPropertyNode *master_freeze = fgGetNode("/sim/freeze/master");
    bool freeze = master_freeze->getBoolValue();
    SG_LOG(SG_INPUT, SG_INFO, "ReIniting TileCache");
    if ( !freeze ) {
        master_freeze->setBoolValue(true);
    }

    globals->get_subsystem("scenery")->reinit();

    if ( !freeze ) {
        master_freeze->setBoolValue(false);
    }
    return true;
}

/**
 * Reload the materials definition
 */
static bool
do_materials_reload (const SGPropertyNode * arg, SGPropertyNode * root)
{
    SG_LOG(SG_INPUT, SG_INFO, "Reloading Materials");
    SGMaterialLib* new_matlib =  new SGMaterialLib;
    SGPath mpath( globals->get_fg_root() );
    mpath.append( fgGetString("/sim/rendering/materials-file") );
    bool loaded = new_matlib->load(globals->get_fg_root(),
                                  mpath,
                                  globals->get_props());

    if ( ! loaded ) {
       SG_LOG( SG_GENERAL, SG_ALERT,
               "Error loading materials file " << mpath );
       return false;
    }

    globals->set_matlib(new_matlib);
    FGScenery* scenery = static_cast<FGScenery*>(globals->get_scenery());
    scenery->materialLibChanged();

    return true;
}

/**
 * Built-in command: Add a dialog to the GUI system.  Does *not*
 * display the dialog.  The property node should have the same format
 * as a dialog XML configuration.  It must include:
 *
 * name: the name of the GUI dialog for future reference.
 */
static bool
do_dialog_new (const SGPropertyNode * arg, SGPropertyNode * root)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    if (!gui) {
      return false;
    }

    // Note the casting away of const: this is *real*.  Doing a
    // "dialog-apply" command later on will mutate this property node.
    // I'm not convinced that this isn't the Right Thing though; it
    // allows client to create a node, pass it to dialog-new, and get
    // the values back from the dialog by reading the same node.
    // Perhaps command arguments are not as "const" as they would
    // seem?
    gui->newDialog((SGPropertyNode*)arg);
    return true;
}

/**
 * Built-in command: Show an XML-configured dialog.
 *
 * dialog-name: the name of the GUI dialog to display.
 */
static bool
do_dialog_show (const SGPropertyNode * arg, SGPropertyNode * root)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    gui->showDialog(arg->getStringValue("dialog-name"));
    return true;
}

/**
 * Built-in command: Show an XML-configured dialog.
 *
 * dialog-name: the name of the GUI dialog to display.
 */
static bool
do_dialog_toggle (const SGPropertyNode * arg, SGPropertyNode * root)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    gui->toggleDialog(arg->getStringValue("dialog-name"));
    return true;
}


/**
 * Built-in Command: Hide the active XML-configured dialog.
 */
static bool
do_dialog_close (const SGPropertyNode * arg, SGPropertyNode * root)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    if(arg->hasValue("dialog-name"))
        return gui->closeDialog(arg->getStringValue("dialog-name"));
    return gui->closeActiveDialog();
}


/**
 * Update a value in the active XML-configured dialog.
 *
 * object-name: The name of the GUI object(s) (all GUI objects if omitted).
 */
static bool
do_dialog_update (const SGPropertyNode * arg, SGPropertyNode * root)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    FGDialog * dialog;
    if (arg->hasValue("dialog-name"))
        dialog = gui->getDialog(arg->getStringValue("dialog-name"));
    else
        dialog = gui->getActiveDialog();

    if (dialog != 0) {
        dialog->updateValues(arg->getStringValue("object-name"));
        return true;
    } else {
        return false;
    }
}

static bool
do_open_browser (const SGPropertyNode * arg, SGPropertyNode * root)
{
    string path;
    if (arg->hasValue("path"))
        path = arg->getStringValue("path");
    else
    if (arg->hasValue("url"))
        path = arg->getStringValue("url");
    else
        return false;

    return openBrowser(path);
}

static bool
do_open_launcher(const SGPropertyNode*, SGPropertyNode*)
{
#if defined(HAVE_QT)
    bool ok = flightgear::runInAppLauncherDialog();
    if (ok) {
        // start a full reset
        fgResetIdleState();
    }
    return true; // don't report failure
#else
    return false;
#endif
}

/**
 * Apply a value in the active XML-configured dialog.
 *
 * object-name: The name of the GUI object(s) (all GUI objects if omitted).
 */
static bool
do_dialog_apply (const SGPropertyNode * arg, SGPropertyNode * root)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    FGDialog * dialog;
    if (arg->hasValue("dialog-name"))
        dialog = gui->getDialog(arg->getStringValue("dialog-name"));
    else
        dialog = gui->getActiveDialog();

    if (dialog != 0) {
        dialog->applyValues(arg->getStringValue("object-name"));
        return true;
    } else {
        return false;
    }
}


/**
 * Redraw GUI (applying new widget colors). Doesn't reload the dialogs,
 * unlike reinit().
 */
static bool
do_gui_redraw (const SGPropertyNode * arg, SGPropertyNode * root)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    gui->redraw();
    return true;
}


/**
 * Adds model to the scenery. The path to the added branch (/models/model[*])
 * is returned in property "property".
 */
static bool
do_add_model (const SGPropertyNode * arg, SGPropertyNode * root)
{
    SGPropertyNode * model = fgGetNode("models", true);
    int i;
    for (i = 0; model->hasChild("model",i); i++);
    model = model->getChild("model", i, true);
    copyProperties(arg, model);
    if (model->hasValue("elevation-m"))
        model->setDoubleValue("elevation-ft", model->getDoubleValue("elevation-m")
                * SG_METER_TO_FEET);
    model->getNode("load", true);
    model->removeChildren("load");
    const_cast<SGPropertyNode *>(arg)->setStringValue("property", model->getPath());
    return true;
}

/**
 * Built-in command: commit presets (read from in /sim/presets/)
 */
static bool
do_presets_commit (const SGPropertyNode * arg, SGPropertyNode * root)
{
    if (fgGetBool("/sim/initialized", false)) {
      fgResetIdleState();
    } else {
      // Nasal can trigger this during initial init, which confuses
      // the logic in ReInitSubsystems, since initial state has not been
      // saved at that time. Short-circuit everything here.
      flightgear::initPosition();
    }

    return true;
}

static bool
do_press_cockpit_button (const SGPropertyNode * arg, SGPropertyNode * root)
{
  const char *prefix = arg->getStringValue("prefix");

  if (arg->getBoolValue("guarded") && fgGetDouble((string(prefix) + "-guard").c_str()) < 1)
    return true;

  string prop = string(prefix) + "-button";
  double value;

  if (arg->getBoolValue("latching"))
    value = fgGetDouble(prop.c_str()) > 0 ? 0 : 1;
  else
    value = 1;

  fgSetDouble(prop.c_str(), value);
  fgSetBool(arg->getStringValue("discrete"), value > 0);

  return true;
}

static bool
do_release_cockpit_button (const SGPropertyNode * arg, SGPropertyNode * root)
{
  const char *prefix = arg->getStringValue("prefix");

  if (arg->getBoolValue("guarded")) {
    string prop = string(prefix) + "-guard";
    if (fgGetDouble(prop.c_str()) < 1) {
      fgSetDouble(prop.c_str(), 1);
      return true;
    }
  }

  if (! arg->getBoolValue("latching")) {
    fgSetDouble((string(prefix) + "-button").c_str(), 0);
    fgSetBool(arg->getStringValue("discrete"), false);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////
// Command setup.
////////////////////////////////////////////////////////////////////////


/**
 * Table of built-in commands.
 *
 * New commands do not have to be added here; any module in the application
 * can add a new command using globals->get_commands()->addCommand(...).
 */
static struct {
  const char * name;
  SGCommandMgr::command_t command;
} built_ins [] = {
    { "exit", do_exit },
    { "reset", do_reset },
    { "reposition", do_reposition },
    { "switch-aircraft", do_switch_aircraft },
    { "panel-load", do_panel_load },
    { "preferences-load", do_preferences_load },
    { "toggle-fullscreen", do_toggle_fullscreen },
    { "screen-capture", do_screen_capture },
    { "hires-screen-capture", do_hires_screen_capture },
    { "tile-cache-reload", do_tile_cache_reload },
    { "dialog-new", do_dialog_new },
    { "dialog-show", do_dialog_show },
    { "dialog-toggle", do_dialog_toggle },
    { "dialog-close", do_dialog_close },
    { "dialog-update", do_dialog_update },
    { "dialog-apply", do_dialog_apply },
    { "open-browser", do_open_browser },
    { "gui-redraw", do_gui_redraw },
    { "add-model", do_add_model },
    { "presets-commit", do_presets_commit },
    { "press-cockpit-button", do_press_cockpit_button },
    { "release-cockpit-button", do_release_cockpit_button },
    { "dump-scenegraph", do_dump_scene_graph },
    { "dump-terrainbranch", do_dump_terrain_branch },
    { "print-visible-scene", do_print_visible_scene_info },
    { "reload-shaders", do_reload_shaders },
    { "reload-materials", do_materials_reload },
    { "open-launcher", do_open_launcher },
    { 0, 0 }			// zero-terminated
};


/**
 * Initialize the default built-in commands.
 *
 * Other commands may be added by other parts of the application.
 */
void
fgInitSceneCommands ()
{
  SG_LOG(SG_GENERAL, SG_BULK, "Initializing scene built-in commands:");
  for (int i = 0; built_ins[i].name != 0; i++) {
    SG_LOG(SG_GENERAL, SG_BULK, "  " << built_ins[i].name);
    globals->get_commands()->addCommand(built_ins[i].name,
					built_ins[i].command);
  }

}
