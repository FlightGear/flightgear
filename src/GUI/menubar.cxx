#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <iostream>
#include <plib/pu.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGBinding.hxx>

#include <Main/globals.hxx>

#include "new_gui.hxx"
#include "menubar.hxx"



////////////////////////////////////////////////////////////////////////
// FIXME!!
//
// Deprecated wrappers for old menu commands.
//
// DO NOT ADD TO THESE.  THEY WILL BE DELETED SOON!
//
// These are defined in gui_funcs.cxx.  They should be replaced with
// user-configured dialogs and new commands where necessary.
////////////////////////////////////////////////////////////////////////

#if defined(TR_HIRES_SNAP)
extern void dumpHiResSnapShot ();
static bool
do_hires_snapshot_dialog (const SGPropertyNode * arg)
{
    dumpHiResSnapShot();
    return true;
}
#endif // TR_HIRES_SNAP

#if defined( _WIN32 ) && !defined(__MINGW32__)
extern void printScreen ();
static bool
do_print_dialog (const SGPropertyNode * arg)
{
    printScreen();
    return true;
}
#endif

extern void helpCb ();
static bool
do_help_dialog (const SGPropertyNode * arg)
{
    helpCb();
    return true;
}

static struct {
    const char * name;
    SGCommandMgr::command_t command;
} deprecated_dialogs [] = {
#if defined(TR_HIRES_SNAP)
    { "old-hires-snapshot-dialog", do_hires_snapshot_dialog },
#endif
#if defined( _WIN32 ) && !defined(__MINGW32__)
    { "old-print-dialog", do_print_dialog },
#endif
    { "old-help-dialog", do_help_dialog },
    { 0, 0 }
};

static void
add_deprecated_dialogs ()
{
  SG_LOG(SG_GENERAL, SG_INFO, "Initializing old dialog commands:");
  for (int i = 0; deprecated_dialogs[i].name != 0; i++) {
    SG_LOG(SG_GENERAL, SG_INFO, "  " << deprecated_dialogs[i].name);
    globals->get_commands()->addCommand(deprecated_dialogs[i].name,
					deprecated_dialogs[i].command);
  }
}



////////////////////////////////////////////////////////////////////////
// Static functions.
////////////////////////////////////////////////////////////////////////


static void
menu_callback (puObject * object)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    gui->getMenuBar()->fireItem(object);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGMenuBar.
////////////////////////////////////////////////////////////////////////


FGMenuBar::FGMenuBar ()
    : _visible(false),
      _menuBar(0)
{
}

FGMenuBar::~FGMenuBar ()
{
    destroy_menubar();
}

void
FGMenuBar::init ()
{
    delete _menuBar;            // FIXME: check if PUI owns the pointer
    make_menubar();
                                // FIXME: temporary commands to get at
                                // old, hard-coded dialogs.
    add_deprecated_dialogs();
}

void
FGMenuBar::show ()
{
    if (_menuBar != 0)
        _menuBar->reveal();
    _visible = true;
}

void
FGMenuBar::hide ()
{
    if (_menuBar != 0)
        _menuBar->hide();
    _visible = false;
}

bool
FGMenuBar::isVisible () const
{
    return _visible;
}

void
FGMenuBar::fireItem (puObject * item)
{
    const char * name = item->getLegend();
    vector<SGBinding *> &bindings = _bindings[name];
    int nBindings = bindings.size();

    for (int i = 0; i < nBindings; i++)
        bindings[i]->fire();
}

void
FGMenuBar::make_menu (SGPropertyNode * node)
{
    const char * name = strdup(node->getStringValue("label"));
    vector<SGPropertyNode_ptr> item_nodes = node->getChildren("item");

    int array_size = item_nodes.size();

    char ** items = make_char_array(array_size);
    puCallback * callbacks = make_callback_array(array_size);

    for (unsigned int i = 0, j = item_nodes.size() - 1;
         i < item_nodes.size();
         i++, j--) {

                                // Set up the PUI entries for this item
        items[j] = strdup((char *)item_nodes[i]->getStringValue("label"));
        callbacks[j] = menu_callback;

                                // Load all the bindings for this item
        vector<SGPropertyNode_ptr> bindings = item_nodes[i]->getChildren("binding");
        SGPropertyNode * dest = fgGetNode("/sim/bindings/menu", true);

        for (unsigned int k = 0; k < bindings.size(); k++) {
            unsigned int m = 0;
            SGPropertyNode_ptr binding;
            while (dest->getChild("binding", m))
                m++;

            binding = dest->getChild("binding", m, true);
            copyProperties(bindings[k], binding);
            _bindings[items[j]].push_back(new SGBinding(binding, globals->get_props()));
        }
    }

    _menuBar->add_submenu(name, items, callbacks);
}

void
FGMenuBar::make_menubar ()
{
    SGPropertyNode *targetpath;
   
    targetpath = fgGetNode("/sim/menubar/default",true);
    // fgLoadProps("gui/menubar.xml", targetpath);
    
    /* NOTE: there is no check to see whether there's any usable data at all
     *
     * This would also have the advantage of being able to create some kind of
     * 'fallback' menu - just in case that either menubar.xml is empty OR that
     * its XML data is not valid, that way we would avoid displaying an
     * unusable menubar without any functionality - if we decided to add another
     * char * element to the commands structure in
     *  $FG_SRC/src/Main/fgcommands.cxx 
     * we could additionally save each function's (short) description and use
     * this as label for the fallback PUI menubar item labels - as a workaround
     * one might simply use the internal fgcommands and put them into the 
     * fallback menu, so that the user is at least able to re-init the menu
     * loading - just in case there was some malformed XML in it
     * (it happend to me ...)
     */
    
    make_menubar(targetpath);
}

/* WARNING: We aren't yet doing any validation of what's found - but since
 * this isn't done with menubar.xml either, it should not really matter
 * right now. Although one should later on consider to validate the
 * contents, whether they are representing a 'legal' menubar structure.
 */
void
FGMenuBar::make_menubar(SGPropertyNode * props) 
{    
    // Just in case.
    destroy_menubar();
    _menuBar = new puMenuBar;

    vector<SGPropertyNode_ptr> menu_nodes = props->getChildren("menu");
    for (unsigned int i = 0; i < menu_nodes.size(); i++)
        make_menu(menu_nodes[i]);

    _menuBar->close();
    make_object_map(props);

    if (_visible)
        _menuBar->reveal();
    else
        _menuBar->hide();
}

void
FGMenuBar::destroy_menubar ()
{
    if ( _menuBar == 0 )
        return;

    hide();
    puDeleteObject(_menuBar);

    unsigned int i;

                                // Delete all the character arrays
                                // we were forced to keep around for
                                // plib.
    SG_LOG(SG_GENERAL, SG_BULK, "Deleting char arrays");
    for (i = 0; i < _char_arrays.size(); i++) {
        for (int j = 0; _char_arrays[i][j] != 0; j++)
            free(_char_arrays[i][j]); // added with strdup
        delete[] _char_arrays[i];
    }

                                // Delete all the callback arrays
                                // we were forced to keep around for
                                // plib.
    SG_LOG(SG_GENERAL, SG_BULK, "Deleting callback arrays");
    for (i = 0; i < _callback_arrays.size(); i++)
        delete[] _callback_arrays[i];

                                // Delete all those bindings
    SG_LOG(SG_GENERAL, SG_BULK, "Deleting bindings");
    map<string,vector<SGBinding *> >::iterator it;
    for (it = _bindings.begin(); it != _bindings.end(); it++) {
        SG_LOG(SG_GENERAL, SG_BULK, "Deleting bindings for " << it->first);
        for ( i = 0; i < it->second.size(); i++ )
            delete it->second[i];
    }

    SG_LOG(SG_GENERAL, SG_BULK, "Done.");
}

void
FGMenuBar::make_object_map(SGPropertyNode * node)
{
    unsigned int menu_index = 0;
    vector<SGPropertyNode_ptr> menus = node->getChildren("menu");
    for (puObject *obj = ((puGroup *)_menuBar)->getFirstChild();
            obj; obj = obj->getNextObject()) {

        // skip puPopupMenus. They are also children of _menuBar,
        // but we access them via getUserData()  (see below)
        if (!(obj->getType() & PUCLASS_ONESHOT))
            continue;

        if (menu_index >= menus.size()) {
            SG_LOG(SG_GENERAL, SG_WARN, "'menu' object without node: "
                    << node->getPath() << "/menu[" << menu_index << ']');
            return;
        }

        SGPropertyNode *menu = menus.at(menu_index);
        _objects[menu->getPath()] = obj;
        add_enabled_listener(menu);

        puGroup *popup = (puGroup *)obj->getUserData();
        if (!popup)
            continue;

        // the entries are for some reason reversed (last first), and we
        // don't know yet how many there will be; so we collect first
        vector<puObject *> e;
        for (puObject *me = popup->getFirstChild(); me; me = me->getNextObject())
            e.push_back(me);

        vector<SGPropertyNode_ptr> items = menu->getChildren("item");
        for (unsigned int i = 0; i < e.size(); i++) {
            if (i >= items.size()) {
                SG_LOG(SG_GENERAL, SG_WARN, "'item' object without node: "
                        << menu->getPath() << "/item[" << i << ']');
                break;
            }
            SGPropertyNode *item = items.at(e.size() - i - 1);
            _objects[item->getPath()] = e[i];
            add_enabled_listener(item);
        }
        menu_index++;
    }
}

struct EnabledListener : SGPropertyChangeListener {
    void valueChanged(SGPropertyNode *node) {
        NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
        if (!gui)
            return;
        FGMenuBar *menubar = gui->getMenuBar();
        if (menubar)
            menubar->enable_item(node->getParent(), node->getBoolValue());
    }
};

void
FGMenuBar::add_enabled_listener(SGPropertyNode * node)
{
    if (!node->hasValue("enabled"))
        node->setBoolValue("enabled", true);

    enable_item(node, node->getBoolValue("enabled"));
    node->getNode("enabled")->addChangeListener(new EnabledListener());
}

bool
FGMenuBar::enable_item(const SGPropertyNode * node, bool state)
{
    const char *path = node->getPath();
    if (_objects.find(path) == _objects.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Trying to enable/disable "
            "non-existent menu item for node `" << path << '\'');
        return false;
    }
    puObject *object = _objects[path];
    if (state)
        object->activate();
    else
        object->greyOut();

    return true;
}

char **
FGMenuBar::make_char_array (int size)
{
    char ** list = new char*[size+1];
    for (int i = 0; i <= size; i++)
        list[i] = 0;
    _char_arrays.push_back(list);
    return list;
}

puCallback *
FGMenuBar::make_callback_array (int size)
{
    puCallback * list = new puCallback[size+1];
    for (int i = 0; i <= size; i++)
        list[i] = 0;
    _callback_arrays.push_back(list);
    return list;
}

// end of menubar.cxx
