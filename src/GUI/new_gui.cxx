// new_gui.cxx: implementation of XML-configurable GUI support.

#include "new_gui.hxx"

#include <plib/pu.h>
#include <plib/ul.h>

#include <vector>
SG_USING_STD(vector);

#include <simgear/misc/exception.hxx>
#include <Main/fg_props.hxx>



////////////////////////////////////////////////////////////////////////
// Callbacks.
////////////////////////////////////////////////////////////////////////

/**
 * Action callback.
 */
static void
action_callback (puObject * object)
{
    GUIInfo * info = (GUIInfo *)object->getUserData();
    NewGUI * gui =
        (NewGUI *)globals->get_subsystem_mgr()
          ->get_group(FGSubsystemMgr::INIT)->get_subsystem("gui");
    gui->setCurrentWidget(info->widget);
    for (int i = 0; i < info->bindings.size(); i++)
        info->bindings[i]->fire();
    gui->setCurrentWidget(0);
}



////////////////////////////////////////////////////////////////////////
// Implementation of GUIInfo.
////////////////////////////////////////////////////////////////////////

GUIInfo::GUIInfo (GUIWidget * w)
    : widget(w)
{
}

GUIInfo::~GUIInfo ()
{
    for (int i = 0; i < bindings.size(); i++) {
        delete bindings[i];
        bindings[i] = 0;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of GUIWidget.
////////////////////////////////////////////////////////////////////////

GUIWidget::GUIWidget (SGPropertyNode_ptr props)
    : _object(0)
{
    display(props);
}

GUIWidget::~GUIWidget ()
{
    delete _object;

    int i;
    for (i = 0; i < _info.size(); i++) {
        delete _info[i];
        _info[i] = 0;
    }

    for (i = 0; i < _propertyObjects.size(); i++) {
        delete _propertyObjects[i];
        _propertyObjects[i] = 0;
    }
}

void
GUIWidget::updateValue (const char * objectName)
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        if (_propertyObjects[i]->name == objectName)
            _propertyObjects[i]->object
                ->setValue(_propertyObjects[i]->node->getStringValue());
    }
}

void
GUIWidget::applyValue (const char * objectName)
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        if (_propertyObjects[i]->name == objectName)
            _propertyObjects[i]->node
                ->setStringValue(_propertyObjects[i]
                                 ->object->getStringValue());
    }
}

void
GUIWidget::updateValues ()
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        puObject * object = _propertyObjects[i]->object;
        SGPropertyNode_ptr node = _propertyObjects[i]->node;
        object->setValue(node->getStringValue());
    }
}

void
GUIWidget::applyValues ()
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        puObject * object = _propertyObjects[i]->object;
        SGPropertyNode_ptr node = _propertyObjects[i]->node;
        node->setStringValue(object->getStringValue());
    }
}

void
GUIWidget::display (SGPropertyNode_ptr props)
{
    if (_object != 0) {
        SG_LOG(SG_GENERAL, SG_ALERT, "This widget is already active");
        return;
    }

    _object = makeObject(props, 1024, 768);

    if (_object != 0) {
        _object->reveal();
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "Widget "
               << props->getStringValue("name", "[unnamed]")
               << " does not contain a proper GUI definition");
    }
}

puObject *
GUIWidget::makeObject (SGPropertyNode * props, int parentWidth, int parentHeight)
{
    int width = props->getIntValue("width", parentWidth);
    int height = props->getIntValue("height", parentHeight);

    int x = props->getIntValue("x", (parentWidth - width) / 2);
    int y = props->getIntValue("y", (parentHeight - height) / 2);

    string type = props->getName();
    if (type == "")
        type = props->getStringValue("type");
    if (type == "") {
        SG_LOG(SG_GENERAL, SG_ALERT, "No type specified for GUI object");
        return 0;
    }

    if (type == "dialog") {
        puPopup * dialog;
        if (props->getBoolValue("modal", false))
            dialog = new puDialogBox(x, y);
        else
            dialog = new puPopup(x, y);
        setupGroup(dialog, props, width, height, true);
        return dialog;
    } else if (type == "group") {
        puGroup * group = new puGroup(x, y);
        setupGroup(group, props, width, height, false);
        return group;
    } else if (type == "input") {
        puInput * input = new puInput(x, y, x + width, y + height);
        setupObject(input, props);
        return input;
    } else if (type == "text") {
        puText * text = new puText(x, y);
        setupObject(text, props);
        return text;
    } else if (type == "button") {
        puButton * b;
        const char * legend = props->getStringValue("legend", "[none]");
        if (props->getBoolValue("one-shot", true))
            b = new puOneShot(x, y, legend);
        else
            b = new puButton(x, y, legend);
        setupObject(b, props);
        return b;
    } else {
        return 0;
    }
}

void
GUIWidget::setupObject (puObject * object, SGPropertyNode * props)
{
    if (props->hasValue("legend"))
        object->setLegend(props->getStringValue("legend"));

    if (props->hasValue("label"))
        object->setLabel(props->getStringValue("label"));

    if (props->hasValue("property")) {
        const char * name = props->getStringValue("name");
        if (name == 0)
            name = "";
        const char * propname = props->getStringValue("property");
        SGPropertyNode_ptr node = fgGetNode(propname, true);
        object->setValue(node->getStringValue());
        if (name != 0)
            _propertyObjects.push_back(new PropertyObject(name, object, node));
    }

    vector<SGPropertyNode_ptr> nodes = props->getChildren("binding");
    if (nodes.size() > 0) {
        GUIInfo * info = new GUIInfo(this);

        for (int i = 0; i < nodes.size(); i++)
            info->bindings.push_back(new FGBinding(nodes[i]));
        object->setCallback(action_callback);
        object->setUserData(info);
        _info.push_back(info);
    }

    object->makeReturnDefault(props->getBoolValue("default"));
}

void
GUIWidget::setupGroup (puGroup * group, SGPropertyNode * props,
                    int width, int height, bool makeFrame)
{
    setupObject(group, props);

    if (makeFrame)
        new puFrame(0, 0, width, height);

    int nChildren = props->nChildren();
    for (int i = 0; i < nChildren; i++)
        makeObject(props->getChild(i), width, height);
    group->close();
}

GUIWidget::PropertyObject::PropertyObject (const char * n,
                                           puObject * o,
                                           SGPropertyNode_ptr p)
    : name(n),
      object(o),
      node(p)
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of NewGUI.
////////////////////////////////////////////////////////////////////////


NewGUI::NewGUI ()
    : _current_widget(0)
{
}

NewGUI::~NewGUI ()
{
}

void
NewGUI::init ()
{
    char path[1024];
    ulMakePath(path, getenv("FG_ROOT"), "gui");
    readDir(path);
}

void
NewGUI::update (double delta_time_sec)
{
    // NO OP
}

void
NewGUI::display (const string &name)
{
    if (_widgets.find(name) == _widgets.end())
        SG_LOG(SG_GENERAL, SG_ALERT, "Dialog " << name << " not defined");
    else
        new GUIWidget(_widgets[name]);
}

void
NewGUI::setCurrentWidget (GUIWidget * widget)
{
    _current_widget = widget;
}

GUIWidget *
NewGUI::getCurrentWidget ()
{
    return _current_widget;
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

    ulDirEnt * dirEnt = ulReadDir(dir);
    while (dirEnt != 0) {
        char subpath[1024];

        ulMakePath(subpath, path, dirEnt->d_name);

        if (dirEnt->d_isdir && dirEnt->d_name[0] != '.') {
            readDir(subpath);
        } else {
            SGPropertyNode_ptr props = new SGPropertyNode;
            try {
                readProperties(subpath, props);
            } catch (const sg_exception &ex) {
                SG_LOG(SG_INPUT, SG_ALERT, "Error parsing GUI file "
                       << subpath);
            }
            if (!props->hasValue("name")) {
                SG_LOG(SG_INPUT, SG_WARN, "GUI file " << subpath
                   << " has no name; skipping.");
            } else {
                string name = props->getStringValue("name");
                SG_LOG(SG_INPUT, SG_BULK, "Saving GUI node " << name);
                _widgets[name] = props;
            }
        }
        dirEnt = ulReadDir(dir);
    }
    ulCloseDir(dir);
}

// end of new_gui.cxx
