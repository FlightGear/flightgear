#include <string.h>
#include <iostream>
#include <plib/pu.h>
#include <simgear/debug/logstream.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "new_gui.hxx"
#include "menubar.hxx"



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
    hide();
    delete _menuBar;            // FIXME: check if PUI owns the pointer

                                // Delete all those bindings
    map<string,vector<FGBinding *> >::iterator it;
    it = _bindings.begin();
    while (it != _bindings.end()) {
        for (int i = 0; i < it->second.size(); i++)
            delete it->second[i];
    }
}

void
FGMenuBar::init ()
{
    if (_menuBar != 0)          // FIXME: check if PUI owns the pointer
        delete _menuBar;
    make_menubar();
}

void
FGMenuBar::show ()
{
    if (_menuBar != 0) {
        _menuBar->reveal();
        _visible = true;
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "No menu bar to show");
        _visible = false;
    }
}

void
FGMenuBar::hide ()
{
    if (_menuBar != 0) {
        _menuBar->hide();
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "No menu bar to show");
    }
    _visible = false;
}

bool
FGMenuBar::isVisible () const
{
    return _visible;
}

bool
FGMenuBar::fireItem (puObject * item)
{
    const char * name = item->getLegend();
    vector<FGBinding *> &bindings = _bindings[name];

    for (int i = 0; i < bindings.size(); i++)
        bindings[i]->fire();
}

void
FGMenuBar::make_menu (SGPropertyNode_ptr node)
{
    const char * name = strdup(node->getStringValue("label"));
    vector<SGPropertyNode_ptr> item_nodes = node->getChildren("item");

    int array_size = item_nodes.size() + 1;

    char ** items = new char*[array_size];
    puCallback * callbacks = new puCallback[array_size];

    for (int i = 0, j = item_nodes.size() - 1;
         i < item_nodes.size();
         i++, j--) {
        
                                // Set up the PUI entries for this item
        items[j] = strdup((char *)item_nodes[i]->getStringValue("label"));
        callbacks[j] = menu_callback;

                                // Load all the bindings for this item
        vector<SGPropertyNode_ptr> binding_nodes =
            item_nodes[i]->getChildren("binding");
        for (int k = 0; k < binding_nodes.size(); k++)
            _bindings[items[j]].push_back(new FGBinding(binding_nodes[k]));
    }

    items[item_nodes.size()] = 0;
    callbacks[item_nodes.size()] = 0;

    _menuBar->add_submenu(name, items, callbacks);
}

void
FGMenuBar::make_menubar ()
{
    _menuBar = new puMenuBar;
    SGPropertyNode props;

    fgLoadProps("gui/menubar.xml", &props);
    vector<SGPropertyNode_ptr> menu_nodes = props.getChildren("menu");
    for (int i = 0; i < menu_nodes.size(); i++)
        make_menu(menu_nodes[i]);

    _menuBar->close();
}

// end of menubar.cxx
