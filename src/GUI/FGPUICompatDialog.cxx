// SPDX-FileName: FGPUICompatDialog.cxx
// SPDX-FileComment: XML dialog class without using PUI
// SPDX-FileCopyrightText: Copyright (C) 2022 James Turner
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.h"

#include "FGPUICompatDialog.hxx"

#include <simgear/debug/BufferedLogCallback.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/tsync/terrasync.hxx>
#include <simgear/structure/SGBinding.hxx>

#include <Main/fg_os.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>

#include "FGColor.hxx"
#include "FGFontCache.hxx"
#include "PUICompatObject.hxx"
#include "layout.hxx"
#include "new_gui.hxx"
#include "property_list.hxx"

#include <simgear/nasal/cppbind/NasalObject.hxx>
////////////////////////////////////////////////////////////

// should really be exposed properly
extern naRef propNodeGhostCreate(naContext c, SGPropertyNode* n);

class FGPUICompatDialog::DialogPeer : public nasal::Object
{
public:
    DialogPeer(naRef impl) : nasal::Object(impl)
    {
    }

    void setDialog(FGPUICompatDialog* dlg)
    {
        _dialog = dlg;
    }

    SGSharedPtr<FGPUICompatDialog> dialog() const
    {
        return _dialog.lock();
    }

private:
    // the Nasal peer does not hold an owning reference to the
    // main dialog object (dialogs are owned by the NewGUI subsystem)
    SGWeakPtr<FGPUICompatDialog> _dialog;
};

static naRef f_dialogModuleHash(FGPUICompatDialog& dialog, naContext c)
{
    auto nas = globals->get_subsystem<FGNasalSys>();
    if (!nas) {
        return naNil();
    }

    return nas->getModule(dialog.nasalModule());
}

naRef f_dialogRootObject(FGPUICompatDialog& dialog, naContext c)
{
    return nasal::to_nasal(c, dialog._root);
}

//----------------------------------------------------------------------------

naRef f_makeDialogPeer(const nasal::CallContext& ctx)
{
    return ctx.to_nasal(SGSharedPtr<FGPUICompatDialog::DialogPeer>(
        new FGPUICompatDialog::DialogPeer(ctx.requireArg<naRef>(0))));
}


void FGPUICompatDialog::setupGhost(nasal::Hash& compatModule)
{
    using NasalGUIDialog = nasal::Ghost<SGSharedPtr<FGPUICompatDialog>>;
    NasalGUIDialog::init("gui.xml.CompatDialog")
        .member("name", &FGPUICompatDialog::nameString)
        .member("module", &f_dialogModuleHash)
        .member("geometry", &FGPUICompatDialog::geometry)
        .member("x", &FGPUICompatDialog::getX)
        .member("y", &FGPUICompatDialog::getY)
        .member("width", &FGPUICompatDialog::width)
        .member("height", &FGPUICompatDialog::height)
        .member("root", f_dialogRootObject)
        .method("close", &FGPUICompatDialog::requestClose);


    using NasalDialogPeer = nasal::Ghost<SGSharedPtr<DialogPeer>>;
    NasalDialogPeer::init("CompatDialogPeer")
        .bases<nasal::ObjectRef>()
        .method("dialog", &DialogPeer::dialog);

    nasal::Hash dialogHash = compatModule.createHash("Dialog");
    dialogHash.set("new", &f_makeDialogPeer);
}

FGPUICompatDialog::FGPUICompatDialog(SGPropertyNode* props) : FGDialog(props),
                                                              _props(props),
                                                              _needsRelayout(false)
{
    _module = string("__dlg:") + props->getStringValue("name", "[unnamed]");
    _name = props->getStringValue("name", "[unnamed]");
}

FGPUICompatDialog::~FGPUICompatDialog()
{
    // nothing to do, all work was done in close()
}

void FGPUICompatDialog::close()
{
    if (_peer) {
        _peer->callMethod<void>("onClose");
    }
    
    _props->setIntValue("lastx", getX());
    _props->setIntValue("lasty", getY());
    // FIXME: save width/height as well?

    auto nas = globals->get_subsystem<FGNasalSys>();
    if (nas) {
        if (_nasal_close) {
            const auto s = _nasal_close->getStringValue();
            nas->createModule(_module.c_str(), _module.c_str(), s.c_str(), s.length(), _props);
        }
        nas->deleteModule(_module.c_str());
    }

    _root->recursiveOnDelete();

    _peer.clear();
}

bool FGPUICompatDialog::init()
{
    try {
        auto nas = globals->get_subsystem<FGNasalSys>();

        nasal::Context ctx;
        nasal::Hash guiModule{nas->getModule("gui"), ctx};
        if (guiModule.isNil()) {
            throw sg_exception("Can't initialize PUICompat Nasal");
        }

        using SelfRef = SGSharedPtr<FGPUICompatDialog>;
        using PeerRef = SGSharedPtr<DialogPeer>;

        const std::string type = _props->getStringValue("type", "dialog");
        auto f = guiModule.get<std::function<PeerRef(std::string, SelfRef)>>("_createDialogPeer");
        if (!f) {
            SG_LOG(SG_GUI, SG_DEV_ALERT, "PUICompat module loaded incorrectly");
            return false;
        }

        _peer = f(type, SelfRef{this});
        _peer->setDialog(this);
        _peer->callMethod<void>("init", nas->wrappedPropsNode(_props));

        SGPropertyNode* nasal = _props->getNode("nasal");
        if (nasal && nas) {
            _nasal_close = nasal->getNode("close");
            SGPropertyNode* open = nasal->getNode("open");
            if (open) {
                const auto s = open->getStringValue();
                nas->createModule(_module.c_str(), _module.c_str(), s.c_str(), s.length(), _props);
            }
        }
        display(_props);
        _peer->callMethod<void>("didBuild");
    } catch (std::exception& e) {
        SG_LOG(SG_GUI, SG_ALERT, "Failed to build dialog:" << e.what());

        return false;
    }

    return true;
}

void FGPUICompatDialog::bringToFront()
{
    _peer->callMethod<void>("bringToFront");
}

const char* FGPUICompatDialog::getName()
{
    return _name.c_str();
}

void FGPUICompatDialog::updateValues(const std::string& objectName)
{
    _root->recursiveUpdate(objectName);
}

void FGPUICompatDialog::applyValues(const std::string& objectName)
{
    _root->recursiveApply(objectName);
}

void FGPUICompatDialog::update()
{
    _root->recursiveUpdate();

    if (_needsRelayout) {
        relayout();
    }
}

void FGPUICompatDialog::display(SGPropertyNode* props)
{
    // map from physical to logical units for PUI
    const double ratio = fgGetDouble("/sim/rendering/gui-pixel-ratio", 1.0);
    const int physicalWidth = fgGetInt("/sim/startup/xsize"),
              physicalHeight = fgGetInt("/sim/startup/ysize");
    const int screenw = static_cast<int>(physicalWidth / ratio),
              screenh = static_cast<int>(physicalHeight / ratio);

    bool userx = props->hasValue("x");
    bool usery = props->hasValue("y");
    bool userw = props->hasValue("width");
    bool userh = props->hasValue("height");

    // Let the layout widget work in the same property subtree.
    LayoutWidget wid(props);

    SGPropertyNode* fontnode = props->getNode("font");
    if (fontnode) {
        SGPropertyNode* property_node = fontnode->getChild("property");
        if (property_node)
            fontnode = globals->get_props()->getNode(property_node->getStringValue());
        // _font = FGFontCache::instance()->get(fontnode);
    } else {
        // _font = _gui->getDefaultFont();
    }
    // wid.setDefaultFont(_font, int(_font->getPointSize()));

    int pw = 0, ph = 0;
    int px, py, savex, savey;
    if (!userw || !userh)
        wid.calcPrefSize(&pw, &ph);
    pw = props->getIntValue("width", pw);
    ph = props->getIntValue("height", ph);
    px = savex = props->getIntValue("x", (screenw - pw) / 2);
    py = savey = props->getIntValue("y", (screenh - ph) / 2);

    // Negative x/y coordinates are interpreted as distance from the top/right
    // corner rather than bottom/left.
    if (userx && px < 0)
        px = screenw - pw + px;
    if (usery && py < 0)
        py = screenh - ph + py;

    // Define "x", "y", "width" and/or "height" in the property tree if they
    // are not specified in the configuration file.
    wid.layout(px, py, pw, ph);

    _root = PUICompatObject::createForType("group", _props);
    _root->setDialog(this);
    _root->setGeometry(SGRectd{static_cast<double>(px), static_cast<double>(py),
                               static_cast<double>(pw), static_cast<double>(ph)});
    _root->init();

    // Remove automatically generated properties, so the layout looks
    // the same next time around, or restore x and y to preserve negative coords.
    if (userx)
        props->setIntValue("x", savex);
    else
        props->removeChild("x");

    if (usery)
        props->setIntValue("y", savey);
    else
        props->removeChild("y");

    if (!userw) props->removeChild("width");
    if (!userh) props->removeChild("height");
}

#if 0
puObject*
FGPUIDialog::makeObject(SGPropertyNode* props, int parentWidth, int parentHeight)
{
    if (!props->getBoolValue("enabled", true))
        return 0;

    bool presetSize = props->hasValue("width") && props->hasValue("height");
    int width = props->getIntValue("width", parentWidth);
    int height = props->getIntValue("height", parentHeight);
    int x = props->getIntValue("x", (parentWidth - width) / 2);
    int y = props->getIntValue("y", (parentHeight - height) / 2);
    string type = props->getName();

    if (type.empty())
        type = "dialog";

    if (type == "dialog") {
        puPopup* obj;
        bool draggable = props->getBoolValue("draggable", true);
        bool resizable = props->getBoolValue("resizable", false);
        if (props->getBoolValue("modal", false))
            obj = new puDialogBox(x, y);
        else
            obj = new fgPopup(this, x, y, resizable, draggable);
        setupGroup(obj, props, width, height, true);
        setColor(obj, props);
        return obj;

    } else if (type == "group") {
        puGroup* obj = new puGroup(x, y);
        setupGroup(obj, props, width, height, false);
        setColor(obj, props);
        return obj;

    } else if (type == "frame") {
        puGroup* obj = new puGroup(x, y);
        setupGroup(obj, props, width, height, true);
        setColor(obj, props);
        return obj;

    } else if (type == "hrule" || type == "vrule") {
        puFrame* obj = new puFrame(x, y, x + width, y + height);
        obj->setBorderThickness(0);
        setupObject(obj, props);
        setColor(obj, props, BACKGROUND | FOREGROUND | HIGHLIGHT);
        return obj;

    } else if (type == "list") {
        int slider_width = props->getIntValue("slider", 20);
        fgList* obj = new fgList(x, y, x + width, y + height, props, slider_width);
        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props);
        return obj;

    } else if (type == "airport-list") {
        AirportList* obj = new AirportList(x, y, x + width, y + height);
        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props);
        return obj;

    } else if (type == "property-list") {
        PropertyList* obj = new PropertyList(x, y, x + width, y + height, globals->get_props());
        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props);
        return obj;

    } else if (type == "input") {
        puInput* obj = new puInput(x, y, x + width, y + height);
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND | LABEL);
        return obj;

    } else if (type == "text") {
        puText* obj = new puText(x, y);
        setupObject(obj, props);

        // Layed-out objects need their size set, and non-layout ones
        // get a different placement.
        if (presetSize)
            obj->setSize(width, height);
        else
            obj->setLabelPlace(PUPLACE_LABEL_DEFAULT);
        setColor(obj, props, LABEL);
        return obj;

    } else if (type == "checkbox") {
        puButton* obj;
        obj = new puButton(x, y, x + width, y + height, PUBUTTON_XCHECK);
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND | LABEL);
        return obj;

    } else if (type == "radio") {
        puButton* obj;
        obj = new puButton(x, y, x + width, y + height, PUBUTTON_CIRCLE);
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND | LABEL);
        return obj;

    } else if (type == "button") {
        puButton* obj;
        const char* legend = props->getStringValue("legend", "[none]");
        if (props->getBoolValue("one-shot", true))
            obj = new puOneShot(x, y, legend);
        else
            obj = new puButton(x, y, legend);
        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props);
        return obj;
    } else if (type == "map") {
        MapWidget* mapWidget = new MapWidget(x, y, x + width, y + height);
        setupObject(mapWidget, props);
        _activeWidgets.push_back(mapWidget);
        return mapWidget;
    } else if (type == "canvas") {
        CanvasWidget* canvasWidget = new CanvasWidget(x, y,
                                                      x + width, y + height,
                                                      props,
                                                      _module);
        setupObject(canvasWidget, props);
        return canvasWidget;
    } else if (type == "combo") {
        fgComboBox* obj = new fgComboBox(x, y, x + width, y + height, props,
                                         props->getBoolValue("editable", false));
        setupObject(obj, props);
        setColor(obj, props, EDITFIELD);
        return obj;

    } else if (type == "slider") {
        bool vertical = props->getBoolValue("vertical", false);
        puSlider* obj = new puSlider(x, y, (vertical ? height : width), vertical);
        obj->setMinValue(props->getFloatValue("min", 0.0));
        obj->setMaxValue(props->getFloatValue("max", 1.0));
        obj->setStepSize(props->getFloatValue("step"));
        obj->setSliderFraction(props->getFloatValue("fraction"));
#if PLIB_VERSION > 185
        obj->setPageStepSize(props->getFloatValue("pagestep"));
#endif
        setupObject(obj, props);
        if (presetSize)
            obj->setSize(width, height);
        setColor(obj, props, FOREGROUND | LABEL);
        return obj;

    } else if (type == "dial") {
        puDial* obj = new puDial(x, y, width);
        obj->setMinValue(props->getFloatValue("min", 0.0));
        obj->setMaxValue(props->getFloatValue("max", 1.0));
        obj->setWrap(props->getBoolValue("wrap", true));
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND | LABEL);
        return obj;

    } else if (type == "textbox") {
        int slider_width = props->getIntValue("slider", 20);
        int wrap = props->getBoolValue("wrap", true);
#if PLIB_VERSION > 185
        puaLargeInput* obj = new puaLargeInput(x, y,
                                               x + width, x + height, 11, slider_width, wrap);
#else
        puaLargeInput* obj = new puaLargeInput(x, y,
                                               x + width, x + height, 2, slider_width, wrap);
#endif

        if (props->getBoolValue("editable"))
            obj->enableInput();
        else
            obj->disableInput();

        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND | LABEL);

        int top = props->getIntValue("top-line", 0);
        obj->setTopLineInWindow(top < 0 ? unsigned(-1) >> 1 : top);
        return obj;

    } else if (type == "select") {
        fgSelectBox* obj = new fgSelectBox(x, y, x + width, y + height, props);
        setupObject(obj, props);
        setColor(obj, props, EDITFIELD);
        return obj;
    } else if (type == "waypointlist") {
        ScrolledWaypointList* obj = new ScrolledWaypointList(x, y, width, height);
        setupObject(obj, props);
        return obj;

    } else if (type == "loglist") {
        LogList* obj = new LogList(x, y, width, height, 20);
        string logClass = props->getStringValue("logclass");
        if (logClass == "terrasync") {
            auto tsync = globals->get_subsystem<simgear::SGTerraSync>();
            if (tsync) {
                obj->setBuffer(tsync->log());
            }
        } else {
            auto nasal = globals->get_subsystem<FGNasalSys>();
            obj->setBuffer(nasal->log());
        }

        setupObject(obj, props);
        _activeWidgets.push_back(obj);
        setColor(obj, props, FOREGROUND | LABEL);
        return obj;
    } else {
        return 0;
    }
}
#endif

#if 0
void FGPUIDialog::setupObject(puObject* object, SGPropertyNode* props)
{
    GUIInfo* info = new GUIInfo(this);
    object->setUserData(info);
    _info.push_back(info);
    object->setLabelPlace(PUPLACE_CENTERED_RIGHT);
    object->makeReturnDefault(props->getBoolValue("default"));
    info->node = props;
    if (props->hasValue("legend")) {
        info->legend = props->getStringValue("legend");
        object->setLegend(info->legend.c_str());
    }

    if (props->hasValue("label")) {
        info->label = props->getStringValue("label");
        object->setLabel(info->label.c_str());
    }

    if (props->hasValue("border"))
        object->setBorderThickness(props->getIntValue("border", 2));

    if (SGPropertyNode* nft = props->getNode("font", false)) {
        if (nft) {
            SGPropertyNode* property_node = nft->getChild("property");
            if (property_node)
                nft = globals->get_props()->getNode(property_node->getStringValue());
            _font = FGFontCache::instance()->get(nft);
        } else {
            _font = _gui->getDefaultFont();
        }
        puFont* lfnt = FGFontCache::instance()->get(nft);
        object->setLabelFont(*lfnt);
        object->setLegendFont(*lfnt);
    } else {
        object->setLabelFont(*_font);
    }

    if (props->hasChild("visible")) {
        ConditionalObject* cnd = new ConditionalObject("visible", object);
        cnd->setCondition(sgReadCondition(globals->get_props(), props->getChild("visible")));
        _conditionalObjects.push_back(cnd);
    }

    if (props->hasChild("enable")) {
        ConditionalObject* cnd = new ConditionalObject("enable", object);
        cnd->setCondition(sgReadCondition(globals->get_props(), props->getChild("enable")));
        _conditionalObjects.push_back(cnd);
    }

    string type = props->getName();
    if (type == "input" && props->getBoolValue("live"))
        object->setDownCallback(action_callback);

    if (type == "text") {
        const char* format = props->getStringValue("format", 0);
        if (format) {
            info->fmt_type = validate_format(format);
            if (info->fmt_type != f_INVALID)
                info->format = format;
            else
                SG_LOG(SG_GENERAL, SG_ALERT, "DIALOG: invalid <format> '" << format << '\'');
        }
    }

    if (props->hasValue("property")) {
        const char* name = props->getStringValue("name");
        if (name == 0)
            name = "";
        const char* propname = props->getStringValue("property");
        SGPropertyNode_ptr node = fgGetNode(propname, true);
        if (type == "map") {
            // mapWidget binds to a sub-tree of properties, and
            // ignores the puValue mechanism, so special case things here
            MapWidget* mw = static_cast<MapWidget*>(object);
            mw->setProperty(node);
        } else {
            // normal widget, creating PropertyObject
            copy_to_pui(node, object);
            PropertyObject* po = new PropertyObject(name, object, node);
            _propertyObjects.push_back(po);
            if (props->getBoolValue("live"))
                _liveObjects.push_back(po);
        }
    }

    const auto bindings = props->getChildren("binding");
    if (!bindings.empty()) {
        info->key = props->getIntValue("keynum", -1);
        if (props->hasValue("key"))
            info->key = getKeyCode(props->getStringValue("key", ""));


        for (auto bindingNode : bindings) {
            const char* cmd = bindingNode->getStringValue("command");
            if (!strcmp(cmd, "nasal")) {
                // we need to clone the binding node, so we can unique the
                // Nasal module. Otherwise we always modify the global dialog
                // definition, and cloned dialogs use the same Nasal module for
                // <nasal> bindings, which goes wrong. (Especially, the property
                // inspector)

                // memory ownership works because SGBinding has a ref to its
                // argument node and holds onto it.
                SGPropertyNode_ptr copiedBinding = new SGPropertyNode;
                copyProperties(bindingNode, copiedBinding);
                copiedBinding->setStringValue("module", _module.c_str());

                bindingNode = copiedBinding;
            }

            info->bindings.push_back(new SGBinding(bindingNode, globals->get_props()));
        }
        object->setCallback(action_callback);
    }
}
#endif

#if 0
void FGPUIDialog::setupGroup(puGroup* group, SGPropertyNode* props,
                             int width, int height, bool makeFrame)
{
    setupObject(group, props);

    if (makeFrame) {
        puFrame* f = new puFrame(0, 0, width, height);
        setColor(f, props);
    }

    int nChildren = props->nChildren();
    for (int i = 0; i < nChildren; i++)
        makeObject(props->getChild(i), width, height);

    group->close();
}
#endif


void FGPUICompatDialog::relayout()
{
    _needsRelayout = false;

    int screenw = globals->get_props()->getIntValue("/sim/startup/xsize");
    int screenh = globals->get_props()->getIntValue("/sim/startup/ysize");

    bool userx = _props->hasValue("x");
    bool usery = _props->hasValue("y");
    bool userw = _props->hasValue("width");
    bool userh = _props->hasValue("height");

    // Let the layout widget work in the same property subtree.
    LayoutWidget wid(_props);
    // wid.setDefaultFont(_font, int(_font->getPointSize()));

    int pw = 0, ph = 0;
    int px, py, savex, savey;
    if (!userw || !userh) {
        wid.calcPrefSize(&pw, &ph);
    }

    pw = _props->getIntValue("width", pw);
    ph = _props->getIntValue("height", ph);
    px = savex = _props->getIntValue("x", (screenw - pw) / 2);
    py = savey = _props->getIntValue("y", (screenh - ph) / 2);

    // Negative x/y coordinates are interpreted as distance from the top/right
    // corner rather than bottom/left.
    if (userx && px < 0)
        px = screenw - pw + px;
    if (usery && py < 0)
        py = screenh - ph + py;

    // Define "x", "y", "width" and/or "height" in the property tree if they
    // are not specified in the configuration file.
    wid.layout(px, py, pw, ph);


    _root->setGeometry(SGRectd{static_cast<double>(px), static_cast<double>(py),
                               static_cast<double>(pw), static_cast<double>(ph)});

    _peer->callMethod<void>("geometryChanged");

    // Remove automatically generated properties, so the layout looks
    // the same next time around, or restore x and y to preserve negative coords.
    if (userx)
        _props->setIntValue("x", savex);
    else
        _props->removeChild("x");

    if (usery)
        _props->setIntValue("y", savey);
    else
        _props->removeChild("y");

    if (!userw) _props->removeChild("width");
    if (!userh) _props->removeChild("height");
}

#if 0
void FGPUIDialog::applySize(puObject* object)
{
    // compound plib widgets use setUserData() for internal purposes, so refuse
    // to descend into anything that has other bits set than the following
    const int validUserData = PUCLASS_VALUE | PUCLASS_OBJECT | PUCLASS_GROUP | PUCLASS_INTERFACE | PUCLASS_FRAME | PUCLASS_TEXT | PUCLASS_BUTTON | PUCLASS_ONESHOT | PUCLASS_INPUT | PUCLASS_ARROW | PUCLASS_DIAL | PUCLASS_POPUP;

    int type = object->getType();
    if ((type & PUCLASS_GROUP) && !(type & ~validUserData)) {
        puObject* c = ((puGroup*)object)->getFirstChild();
        for (; c != NULL; c = c->getNextObject()) {
            applySize(c);
        } // of child iteration
    }     // of group object case

    GUIInfo* info = (GUIInfo*)object->getUserData();
    if (!info)
        return;

    SGPropertyNode* n = info->node;
    if (!n) {
        SG_LOG(SG_GENERAL, SG_ALERT, "FGDialog::applySize: no props");
        return;
    }

    int x = n->getIntValue("x");
    int y = n->getIntValue("y");
    int w = n->getIntValue("width", 4);
    int h = n->getIntValue("height", 4);
    object->setPosition(x, y);
    object->setSize(w, h);
}
#endif

double FGPUICompatDialog::getX() const
{
    if (!_root)
        return 0.0;
    
    return _root->getX();
}

double FGPUICompatDialog::getY() const
{
    if (!_root)
        return 0.0;
    
    return _root->getY();
}

double FGPUICompatDialog::width() const
{
    if (!_root)
        return 800.0;
    
    return _root->width();
}

double FGPUICompatDialog::height() const
{
    if (!_root)
        return 600.0;
    
    return _root->height();
}

SGRectd FGPUICompatDialog::geometry() const
{
    return _root->geometry();
}

std::string FGPUICompatDialog::nameString() const
{
    return _name;
}

std::string FGPUICompatDialog::nasalModule() const
{
    return _module;
}

void FGPUICompatDialog::requestClose()
{
    auto gui = globals->get_subsystem<NewGUI>();
    gui->closeDialog(_name);
}
