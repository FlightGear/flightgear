/*
 * SPDX-FileName: NasalMenuBar.hxx
 * SPDX-FileComment: XML-configured menu bar
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: Copyright (C) 2023 James Turner
 */

#include <GUI/FGNasalMenuBar.hxx>

// std
#include <vector>

// SimGear
#include <simgear/misc/strutils.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/SGBinding.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>

static bool nameIsSeparator(const std::string& n)
{
    return simgear::strutils::starts_with(simgear::strutils::strip(n), "----");
}

class NasalMenu;
class NasalMenuItem;

using NasalMenuPtr = SGSharedPtr<NasalMenu>;
using NasalMenuItemPtr = SGSharedPtr<NasalMenuItem>;

enum class VisibilityMode {
    Visible,
    Hidden,
    AutoHide,
    HideIfOverlapsWindow
};

///////////////////////////////////////////////////////////////////////////////

class NasalMenuItem : public SGReferenced,
                      public SGPropertyChangeListener
{
public:
    std::string name() const
    {
        return _name;
    }

    std::string shortcut() const
    {
        return _shortcut;
    }

    bool isEnabled() const
    {
        return _enabled;
    }

    bool isChecked() const
    {
        return _checked;
    }

    bool isSeparator() const
    {
        return _isSeparator;
    }

    bool isCheckable() const
    {
        return _isCheckable;
    }

    std::string label() const
    {
        return _label;
    }

    void fire();

    void aboutToShow();

    NasalMenuPtr submenu() const
    {
        return _submenu;
    }

    void initFromNode(SGPropertyNode_ptr config);

    using NasalCallback = std::function<void()>;

    void addCallback(NasalCallback cb)
    {
        _callbacks.push_back(cb);
    }

protected:
    void valueChanged(SGPropertyNode* prop) override;

private:
    void runCallbacks()
    {
        std::for_each(_callbacks.begin(), _callbacks.end(), [](NasalCallback& cb) {
            cb();
        });
    }

    std::string _name;
    std::string _label;
    std::string _shortcut;
    bool _isSeparator = false;
    bool _isCheckable = false;
    bool _enabled = true;
    bool _checked = false;

    SGPropertyNode_ptr _enabledNode,
        _checkedNode, _labelNode;
    NasalMenuPtr _submenu;
    SGBindingList _bindings;
    std::vector<NasalCallback> _callbacks;
};

class NasalMenu : public SGReferenced,
                  public SGPropertyChangeListener
{
public:
    using ItemsVec = std::vector<NasalMenuItemPtr>;

    std::string label() const
    {
        return _label;
    }

    std::string name() const
    {
        return _name;
    }

    bool isEnabled() const
    {
        return _enabled;
    }

    const ItemsVec& items() const
    {
        return _items;
    }

    void aboutToShow();

    void initFromNode(SGPropertyNode_ptr config);

protected:
    void valueChanged(SGPropertyNode* prop) override;

private:
    std::string _name;
    std::string _label;
    bool _enabled = true;
    SGPropertyNode_ptr _enabledNode, _labelNode;
    ItemsVec _items;
};


///////////////////////////////////////////////////////////////////////////////

void NasalMenuItem::initFromNode(SGPropertyNode_ptr config)
{
    auto n = config->getChild("name");
    if (!n) {
        SG_LOG(SG_GUI, SG_DEV_WARN, "menu item without <name> element:" << config->getLocation());
    } else {
        _name = n->getStringValue();
        n->addChangeListener(this);

        if (n->getBoolValue("separator") || nameIsSeparator(_name)) {
            _isSeparator = true;
        }
    }

    auto _labelNode = config->getChild("label");
    if (_labelNode) {
        _labelNode->addChangeListener(this);
    }
    _label = FGMenuBar::getLocalizedLabel(config);

    _checkedNode = config->getChild("checked");
    if (_checkedNode) {
        _isCheckable = true;
        _checked = _checkedNode->getBoolValue();
        _checkedNode->addChangeListener(this);
    }
    // todo support checked-condition

    // we always create an enabled node, so we can decide later
    // to dynamically disable the menu. Otherwise our listener
    // wouldn't work
    _enabledNode = config->getChild("enabled");
    if (_enabledNode) {
        _enabled = _enabledNode->getBoolValue();
    } else {
        _enabledNode = config->addChild("enabled");
        _enabledNode->setBoolValue(true); // default to enabled
    }
    _enabledNode->addChangeListener(this);

    // TODO support enabled-condition

    n = config->getChild("key");
    if (n) {
        _shortcut = n->getStringValue();
    }

    auto bindingNodes = config->getChildren("binding");
    _bindings = readBindingList(bindingNodes, globals->get_props());

    n = config->getChild("menu");
    if (n) {
        _submenu = new NasalMenu();
        _submenu->initFromNode(n);
    }
}

void NasalMenuItem::valueChanged(SGPropertyNode* n)
{
    // sort this by likelyhood these changing, to avoid
    // unnecessary string comaprisons
    if (n == _enabledNode) {
        _enabled = _enabledNode->getBoolValue();
    } else if (n == _checkedNode) {
        _checked = _checkedNode->getBoolValue();
    } else if (n == _labelNode) {
        _label = FGMenuBar::getLocalizedLabel(_labelNode->getParent());
    } else if (n->getNameString() == "name") {
        _name = n->getStringValue();
    }

    // allow Nasal to response to changes
    runCallbacks();
}

void NasalMenuItem::fire()
{
    if (!_enabled) {
        return;
    }

    SGPropertyNode_ptr args{new SGPropertyNode};
    args->setBoolValue("checked", _checked);
    fireBindingList(_bindings, args);
}

void NasalMenuItem::aboutToShow()
{
    if (_submenu) {
        // update submenu title / enabled-ness
    }
}

///////////////////////////////////////////////////////////////////////////////


void NasalMenu::initFromNode(SGPropertyNode_ptr config)
{
    const auto name = config->getStringValue("name");
    _name = name;

    _enabledNode = config->getChild("enabled");
    if (_enabledNode) {
        _enabled = _enabledNode->getBoolValue();
    } else {
        _enabledNode = config->addChild("enabled");
        _enabledNode->setBoolValue(true); // default to enabled
    }
    _enabledNode->addChangeListener(this);

    auto _labelNode = config->getChild("label");
    if (_labelNode) {
        _labelNode->addChangeListener(this);
    }
    _label = FGMenuBar::getLocalizedLabel(config);

    for (auto i : config->getChildren("item")) {
        auto it = new NasalMenuItem;
        it->initFromNode(i);
        _items.push_back(it);
    }
}

void NasalMenu::aboutToShow()
{
    for (auto it : _items) {
        it->aboutToShow();
    }
}

void NasalMenu::valueChanged(SGPropertyNode* n)
{
    if (n == _enabledNode) {
        _enabled = n->getBoolValue();
    } else if (n == _labelNode) {
        _label = FGMenuBar::getLocalizedLabel(n->getParent());
    }
}

///////////////////////////////////////////////////////////////////////////////

class FGNasalMenuBar::NasalMenuBarPrivate
{
public:
    std::vector<NasalMenuPtr> getMenus() const
    {
        return menus;
    }

    VisibilityMode visibilityMode = VisibilityMode::Visible;
    bool computedVisibility = true;
    std::vector<NasalMenuPtr> menus;
};

///////////////////////////////////////////////////////////////////////////////

FGNasalMenuBar::FGNasalMenuBar() : _d(new NasalMenuBarPrivate)
{
}

void FGNasalMenuBar::init()
{
    SGPropertyNode_ptr props = fgGetNode("/sim/menubar/default", true);
    configure(props);
}

void FGNasalMenuBar::postinit()
{
    auto nas = globals->get_subsystem<FGNasalSys>();
    nasal::Context ctx;
    nasal::Hash guiModule{nas->getModule("gui"), ctx};

    using MenuBarRef = std::shared_ptr<NasalMenuBarPrivate>;
    auto f = guiModule.get<std::function<void(MenuBarRef)>>("_createMenuBar");
    if (!f) {
        SG_LOG(SG_GUI, SG_DEV_ALERT, "GUI: _createMenuBar implementation not found");
        return;
    }

    // invoke nasal callback to build up the menubar
    f(_d);
}

void FGNasalMenuBar::show()
{
    _d->visibilityMode = VisibilityMode::Visible;
    recomputeVisibility();
}

void FGNasalMenuBar::hide()
{
    _d->visibilityMode = VisibilityMode::Hidden;
    recomputeVisibility();
}

void FGNasalMenuBar::configure(SGPropertyNode_ptr config)
{
    _d->menus.clear();
    for (auto i : config->getChildren("menu")) {
        auto m = new NasalMenu;
        m->initFromNode(i);
        _d->menus.push_back(m);
    }
}

void FGNasalMenuBar::recomputeVisibility()
{
}

bool FGNasalMenuBar::isVisible() const
{
    return _d->computedVisibility;
}

void FGNasalMenuBar::setHideIfOverlapsWindow(bool hide)
{
    _d->visibilityMode = VisibilityMode::HideIfOverlapsWindow;
    recomputeVisibility();
}

bool FGNasalMenuBar::getHideIfOverlapsWindow() const
{
    return _d->visibilityMode == VisibilityMode::HideIfOverlapsWindow;
}

static naRef f_itemAddCallback(NasalMenuItem& item, const nasal::CallContext& ctx)
{
    auto cb = ctx.requireArg<NasalMenuItem::NasalCallback>(0);
    item.addCallback(cb);
    return naNil();
}

void FGNasalMenuBar::setupGhosts(nasal::Hash& compatModule)
{
    using MenuItemGhost = nasal::Ghost<NasalMenuItemPtr>;
    MenuItemGhost::init("gui.xml.MenuItem")
        .member("name", &NasalMenuItem::name)
        .member("enabled", &NasalMenuItem::isEnabled)
        .member("checked", &NasalMenuItem::isChecked)
        .member("checkable", &NasalMenuItem::isCheckable)
        .member("separator", &NasalMenuItem::isSeparator)
        .member("shortcut", &NasalMenuItem::shortcut)
        .member("submenu", &NasalMenuItem::submenu)
        .member("label", &NasalMenuItem::label)
        .method("fire", &NasalMenuItem::fire)
        .method("addChangedCallback", &f_itemAddCallback);

    using MenuGhost = nasal::Ghost<NasalMenuPtr>;
    MenuGhost::init("gui.xml.Menu")
        .member("label", &NasalMenu::label)
        .member("name", &NasalMenu::name)
        .member("enabled", &NasalMenu::isEnabled)
        .member("items", &NasalMenu::items);


    using MenuBarGhost = nasal::Ghost<std::shared_ptr<NasalMenuBarPrivate>>;
    MenuBarGhost::init("gui.xml.MenuBar")
        .member("menus", &NasalMenuBarPrivate::getMenus);
}
