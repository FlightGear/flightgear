// PUICompatObject.cxx - XML dialog object without using PUI
// Copyright (C) 2022 James Turner
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.h"

#include "PUICompatObject.hxx"

#include <algorithm>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx> // for copyProperties

#include <GUI/FGPUICompatDialog.hxx>
#include <GUI/new_gui.hxx>
#include <Main/fg_props.hxx>
#include <Scripting/NasalSys.hxx>

extern naRef propNodeGhostCreate(naContext c, SGPropertyNode* n);

PUICompatObject::PUICompatObject(naRef impl, const std::string& type) : nasal::Object(impl),
                                                                        _type(type)
{
}

PUICompatObject::~PUICompatObject()
{
    auto labelNode = _config->getChild("label");
    if (labelNode) {
        labelNode->removeChangeListener(this);
    }

    if (_value) {
        _value->removeChangeListener(this);
    }
}

naRef f_makeCompatObjectPeer(const nasal::CallContext& ctx)
{
    return ctx.to_nasal(SGSharedPtr<PUICompatObject>(
        new PUICompatObject(ctx.requireArg<naRef>(0), ctx.requireArg<std::string>(1))));
}

void PUICompatObject::setupGhost(nasal::Hash& compatModule)
{
    using NasalGUIObject = nasal::Ghost<SGSharedPtr<PUICompatObject>>;
    NasalGUIObject::init("gui.xml.CompatObject")
        .bases<nasal::ObjectRef>()
        .member("config", &PUICompatObject::config)
        .method("configValue", &PUICompatObject::nasalGetConfigValue)
        .member("value", &PUICompatObject::propertyValue)
        .member("property", &PUICompatObject::property)
        .member("geometry", &PUICompatObject::geometry)
        .member("x", &PUICompatObject::getX)
        .member("y", &PUICompatObject::getY)
        .member("width", &PUICompatObject::width)
        .member("height", &PUICompatObject::height)
        .member("children", &PUICompatObject::children)
        .member("dialog", &PUICompatObject::dialog)
        .member("parent", &PUICompatObject::parent)
        .method("show", &PUICompatObject::show)
        .method("activateBindings", &PUICompatObject::activateBindings)
        .method("gridLocation", &PUICompatObject::gridLocation);

    nasal::Hash objectHash = compatModule.createHash("Object");
    objectHash.set("new", &f_makeCompatObjectPeer);
}

PUICompatObjectRef PUICompatObject::createForType(const std::string& type, SGPropertyNode_ptr config)
{
    auto nas = globals->get_subsystem<FGNasalSys>();
    nasal::Context ctx;

    nasal::Hash guiModule{nas->getModule("gui"), ctx};
    if (guiModule.isNil()) {
        throw sg_exception("Can't initialize PUICompat Nasal");
    }

    auto f = guiModule.get<std::function<PUICompatObjectRef(std::string)>>("_createCompatObject");
    PUICompatObjectRef object = f(type);
    // set config
    object->_config = config;
    return object;
}

void PUICompatObject::init()
{
    
    // read conditions, bindings, etc

    _name = _config->getStringValue("name");
    _label = _config->getStringValue("label");
    _live = _config->getBoolValue("live");

    int parentWidth = 800;
    int parentHeight = 600;

    //bool presetSize = _config->hasValue("width") && _config->hasValue("height");
    int width = _config->getIntValue("width", parentWidth);
    int height = _config->getIntValue("height", parentHeight);
    int x = _config->getIntValue("x", (parentWidth - width) / 2);
    int y = _config->getIntValue("y", (parentHeight - height) / 2);

    setGeometry(SGRectd{static_cast<double>(x), static_cast<double>(y),
                        static_cast<double>(width), static_cast<double>(height)});

    if (_config->hasChild("visible")) {
        _visibleCondition = sgReadCondition(globals->get_props(), _config->getChild("visible"));
    }

    if (_config->hasChild("enable")) {
        _enableCondition = sgReadCondition(globals->get_props(), _config->getChild("enable"));
    }

    if (_config->hasChild("label")) {
        _config->getChild("label")->addChangeListener(this);
    }

    if (_config->hasValue("property")) {
        _value = fgGetNode(_config->getStringValue("property"), true);
        _value->addChangeListener(this);
    }

    const auto bindings = _config->getChildren("binding");
    if (!bindings.empty()) {
        for (auto bindingNode : bindings) {
            const auto cmd = bindingNode->getStringValue("command");
            if (cmd == "nasal") {
                // we need to clone the binding node, so we can unique the
                // Nasal module. Otherwise we always modify the global dialog
                // definition, and cloned dialogs use the same Nasal module for
                // <nasal> bindings, which goes wrong. (Especially, the property
                // inspector)

                // memory ownership works because SGBinding has a ref to its
                // argument node and holds onto it.
                SGPropertyNode_ptr copiedBinding = new SGPropertyNode;
                copyProperties(bindingNode, copiedBinding);
                copiedBinding->setStringValue("module", dialog()->nasalModule());

                bindingNode = copiedBinding;
            }

            _bindings.push_back(new SGBinding(bindingNode, globals->get_props()));
        }
    }

    // children
    int nChildren = _config->nChildren();
    for (int i = 0; i < nChildren; i++) {
        auto childNode = _config->getChild(i);

        const auto nodeName = childNode->getNameString();
        if (!isNodeAChildObject(nodeName)) {
            continue;
        }

        SGSharedPtr<PUICompatObject> childObject = createForType(nodeName, childNode);
        childObject->_parent = this;
        _children.push_back(childObject);
    }

    auto nas = globals->get_subsystem<FGNasalSys>();
    callMethod<void>("init", nas->wrappedPropsNode(_config));

    // recusively init children
    for (auto c : _children) {
        c->init();
    }

    callMethod<void>("postinit");
}

naRef PUICompatObject::show(naRef viewParent)
{
    nasal::Context ctx;
    return callMethod<naRef>("show", viewParent);
}

bool PUICompatObject::isNodeAChildObject(const std::string& nm)
{
    const string_list typeNames = {
        "button", "one-shot", "slider", "dial",
        "text", "input",
        "combo", "textbox", "select",
        "hrule", "vrule", "group", "frame",
        "checkbox"};

    auto it = std::find(typeNames.begin(), typeNames.end(), nm);
    return it != typeNames.end();
}

void PUICompatObject::update()
{
    if (_enableCondition) {
        const bool e = _enableCondition->test();
        if (e != _enabled) {
            _enabled = e;
            callMethod<void, bool>("enabledChanged", e);
        }
    }

    if (_visibleCondition) {
        const bool e = _visibleCondition->test();
        if (e != _visible) {
            _visible = e;
            callMethod<void, bool>("visibleChanged", e);
        }
    }

    if (_labelChanged) {
        _labelChanged = false;
        callMethod<void, std::string>("labelChanged", _label);
    }

    // read property value if live
    // check if it's changed
    if (_valueChanged) {
        _valueChanged = false;
        callMethod<void>("valueChanged");
    }
}

void PUICompatObject::apply()
{
    callMethod<void>("apply");
    _valueChanged = false;
}

naRef PUICompatObject::property() const
{
    if (!_value)
        return naNil();
    
    auto nas = globals->get_subsystem<FGNasalSys>();
    return nas->wrappedPropsNode(_value.get());
}

naRef PUICompatObject::propertyValue(naContext ctx) const
{
    return FGNasalSys::getPropertyValue(ctx, _value);
}


naRef PUICompatObject::config() const
{
    auto nas = globals->get_subsystem<FGNasalSys>();
    return nas->wrappedPropsNode(_config.get());
}

naRef PUICompatObject::nasalGetConfigValue(const nasal::CallContext ctx) const
{
    auto name = ctx.requireArg<std::string>(0);
    naRef defaultVal = ctx.getArg(1, naNil());
    SGPropertyNode_ptr nd = _config->getChild(name);
    if (!nd || !nd->hasValue())
        return defaultVal;
    
    return FGNasalSys::getPropertyValue(ctx.c_ctx(), nd.get());
}

void PUICompatObject::valueChanged(SGPropertyNode* node)
{
    if (node->getNameString() == "label") {
        _labelChanged = true;
        _label = node->getStringValue();
        return;
    }

    if (!_live)
        return;

    // don't fire Nasal callback now, might cause recursion
    _valueChanged = true;
}

void PUICompatObject::activateBindings()
{
    assert(_enabled);
    auto guiSub = globals->get_subsystem<NewGUI>();
    assert(guiSub);

    guiSub->setActiveDialog(dialog());
    fireBindingList(_bindings);
    guiSub->setActiveDialog(nullptr);
}

void PUICompatObject::setGeometry(const SGRectd& g)
{
    updateGeometry(g);
}

void PUICompatObject::updateGeometry(const SGRectd& newGeom)
{
    if (newGeom == _geometry) {
        return;
    }

    _geometry = newGeom;
    callMethod<void>("geometryChanged");

    // cascade to children?
}

double PUICompatObject::getX() const
{
    return _geometry.pos().x();
}

double PUICompatObject::getY() const
{
    return _geometry.pos().y();
}

double PUICompatObject::width() const
{
    return _geometry.width();
}

double PUICompatObject::height() const
{
    return _geometry.height();
}

SGRectd PUICompatObject::geometry() const
{
    return _geometry;
}

PUICompatObjectRef PUICompatObject::parent() const
{
    return _parent.lock();
}

PUICompatObjectVec PUICompatObject::children() const
{
    return _children;
}


void PUICompatObject::recursiveUpdate(const std::string& objectName)
{
    if (objectName.empty() || (objectName == _name)) {
        update();
    }

    for (auto child : _children) {
        child->recursiveUpdate(objectName);
    }
}

void PUICompatObject::recursiveApply(const std::string& objectName)
{
    if (objectName.empty() || (objectName == _name)) {
        apply();
    }

    for (auto child : _children) {
        child->recursiveApply(objectName);
    }
}

void PUICompatObject::recursiveOnDelete()
{
    // bottom up call of del()
    for (auto child : _children) {
        child->recursiveOnDelete();
    }

    callMethod<void>("del");
}

void PUICompatObject::setDialog(PUICompatDialogRef dialog)
{
    _dialog = dialog;
}

PUICompatDialogRef PUICompatObject::dialog() const
{
    PUICompatObjectRef pr = _parent.lock();
    if (pr) {
        return pr->dialog();
    }

    return _dialog.lock();
}

nasal::Hash PUICompatObject::gridLocation(const nasal::CallContext& ctx) const
{
    nasal::Hash result{ctx.c_ctx()};
    result.set("column", _config->getIntValue("col"));
    result.set("row", _config->getIntValue("row"));
    result.set("columnSpan", _config->getIntValue("colspan", 1));
    result.set("rowSpan", _config->getIntValue("rowpsan", 1));
    return result;
}
