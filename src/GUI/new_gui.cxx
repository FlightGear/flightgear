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

extern puFont FONT_HELVETICA_14;
extern puFont FONT_SANS_12B;




////////////////////////////////////////////////////////////////////////
// Implementation of NewGUI.
////////////////////////////////////////////////////////////////////////



NewGUI::NewGUI ()
    : _font(FONT_HELVETICA_14),
      _menubar(new FGMenuBar),
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
    setStyle();
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
    map<string,FGDialog *>::iterator iter;
    vector<string> dlg;
    // close all open dialogs and remember them ...
    for (iter = _active_dialogs.begin(); iter != _active_dialogs.end(); iter++) {
        dlg.push_back(iter->first);
        closeDialog(iter->first);
    }

    unbind();
    clear();
    setStyle();
    _menubar = new FGMenuBar;
    init();
    bind();

    // open remembered dialogs again (no nasal generated ones, unfortunately)
//    for (unsigned int i = 0; i < dlg.size(); i++)
//        showDialog(dlg[i]);
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
    string name = cname;
    if(!_active_dialogs[name])
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
            SGPropertyNode *nameprop = props->getNode("name");
            if (!nameprop) {
                SG_LOG(SG_INPUT, SG_WARN, "dialog " << subpath
                   << " has no name; skipping.");
                delete props;
                continue;
            }
            string name = nameprop->getStringValue();
            if (_dialog_props[name])
                delete (SGPropertyNode *)_dialog_props[name];

            _dialog_props[name] = props;
        }
    }
    ulCloseDir(dir);
}



////////////////////////////////////////////////////////////////////////
// Style handling.
////////////////////////////////////////////////////////////////////////

void
NewGUI::setStyle (void)
{
    _colors.clear();

    // set up the traditional colors as default
    _colors["background"] = FGColor(0.8f, 0.8f, 0.9f, 0.85f);
    _colors["foreground"] = FGColor(0.0f, 0.0f, 0.0f, 1.0f);
    _colors["highlight"]  = FGColor(0.7f, 0.7f, 0.7f, 1.0f);
    _colors["label"]      = FGColor(0.0f, 0.0f, 0.0f, 1.0f);
    _colors["legend"]     = FGColor(0.0f, 0.0f, 0.0f, 1.0f);
    _colors["misc"]       = FGColor(0.0f, 0.0f, 0.0f, 1.0f);

    //puSetDefaultStyle();

    int which = fgGetInt("/sim/current-gui", 0);
    SGPropertyNode *sim = globals->get_props()->getNode("sim");
    SGPropertyNode *n = sim->getChild("gui", which);
    if (!n)
        n = sim->getChild("gui", 0, true);

    setupFont(n->getNode("font", true));
    n = n->getNode("colors", true);

    for (int i = 0; i < n->nChildren(); i++) {
        SGPropertyNode *child = n->getChild(i);
        _colors[child->getName()] = FGColor(child);
    }

    FGColor c = _colors["background"];
    puSetDefaultColourScheme(c.red(), c.green(), c.blue(), c.alpha());
}




static const struct {
    char *name;
    puFont *font;
} guifonts[] = {
    "default",      &FONT_HELVETICA_14,
    "FIXED_8x13",   &PUFONT_8_BY_13,
    "FIXED_9x15",   &PUFONT_9_BY_15,
    "TIMES_10",     &PUFONT_TIMES_ROMAN_10,
    "TIMES_24",     &PUFONT_TIMES_ROMAN_24,
    "HELVETICA_10", &PUFONT_HELVETICA_10,
    "HELVETICA_12", &PUFONT_HELVETICA_12,
    "HELVETICA_14", &FONT_HELVETICA_14,
    "HELVETICA_18", &PUFONT_HELVETICA_18,
    "SANS_12B",     &FONT_SANS_12B,
    0, 0,
};

void
NewGUI::setupFont (SGPropertyNode *node)
{
    string fontname = node->getStringValue("name", "Helvetica.txf");
    float size = node->getFloatValue("size", 15.0);
    float slant = node->getFloatValue("slant", 0.0);

    int i;
    for (i = 0; guifonts[i].name; i++)
        if (fontname == guifonts[i].name)
            break;
    if (guifonts[i].name)
        _font = *guifonts[i].font;
    else {
        SGPath fontpath;
        char* envp = ::getenv("FG_FONTS");
        if (envp != NULL) {
            fontpath.set(envp);
        } else {
            fontpath.set(globals->get_fg_root());
            fontpath.append("Fonts");
        }

        SGPath path(fontpath);
        path.append(fontname);

        if (_tex_font.load((char *)path.c_str())) {
            _font.initialize((fntFont *)&_tex_font, size, slant);
        } else {
            _font = *guifonts[0].font;
            fontname = "default";
        }
    }
    puSetDefaultFonts(_font, _font);
    node->setStringValue("name", fontname.c_str());
}




////////////////////////////////////////////////////////////////////////
// FGColor class.
////////////////////////////////////////////////////////////////////////

bool
FGColor::merge(const SGPropertyNode *node)
{
    if (!node)
        return false;

    bool dirty = false;
    const SGPropertyNode * n;
    if ((n = node->getNode("red")))
        _red = n->getFloatValue(), dirty = true;
    if ((n = node->getNode("green")))
        _green = n->getFloatValue(), dirty = true;
    if ((n = node->getNode("blue")))
        _blue = n->getFloatValue(), dirty = true;
    if ((n = node->getNode("alpha")))
        _alpha = n->getFloatValue(), dirty = true;
    return dirty;
}

bool
FGColor::merge(const FGColor& color)
{
    bool dirty = false;
    if (color._red >= 0.0)
        _red = color._red, dirty = true;
    if (color._green >= 0.0)
        _green = color._green, dirty = true;
    if (color._blue >= 0.0)
        _blue = color._blue, dirty = true;
    if (color._alpha >= 0.0)
        _alpha = color._alpha, dirty = true;
    return dirty;
}

// end of new_gui.cxx
