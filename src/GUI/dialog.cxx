// dialog.cxx: implementation of an XML-configurable dialog box.

#include <Input/input.hxx>

#include "dialog.hxx"
#include "new_gui.hxx"



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
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    gui->setCurrentWidget(info->widget);
    int nBindings = info->bindings.size();
    for (int i = 0; i < nBindings; i++) {
        info->bindings[i]->fire();
        if (gui->getCurrentWidget() == 0)
            break;
    }
    gui->setCurrentWidget(0);
}



////////////////////////////////////////////////////////////////////////
// Static helper functions.
////////////////////////////////////////////////////////////////////////

/**
 * Copy a property value to a PUI object.
 */
static void
copy_to_pui (SGPropertyNode * node, puObject * object)
{
    switch (node->getType()) {
    case SGPropertyNode::BOOL:
    case SGPropertyNode::INT:
    case SGPropertyNode::LONG:
        object->setValue(node->getIntValue());
        break;
    case SGPropertyNode::FLOAT:
    case SGPropertyNode::DOUBLE:
        object->setValue(node->getFloatValue());
        break;
    default:
        object->setValue(node->getStringValue());
        break;
    }
}


static void
copy_from_pui (puObject * object, SGPropertyNode * node)
{
    switch (node->getType()) {
    case SGPropertyNode::BOOL:
    case SGPropertyNode::INT:
    case SGPropertyNode::LONG:
        node->setIntValue(object->getIntegerValue());
        break;
    case SGPropertyNode::FLOAT:
    case SGPropertyNode::DOUBLE:
        node->setFloatValue(object->getFloatValue());
        break;
    default:
        node->setStringValue(object->getStringValue());
        break;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of GUIInfo.
////////////////////////////////////////////////////////////////////////

GUIInfo::GUIInfo (FGDialog * w)
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
// Implementation of FGDialog.
////////////////////////////////////////////////////////////////////////

FGDialog::FGDialog (SGPropertyNode_ptr props)
    : _object(0)
{
    display(props);
}

FGDialog::~FGDialog ()
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
FGDialog::updateValue (const char * objectName)
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        const string &name = _propertyObjects[i]->name;
        if (name == objectName)
            copy_to_pui(_propertyObjects[i]->node,
                        _propertyObjects[i]->object);
    }
}

void
FGDialog::applyValue (const char * objectName)
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        if (_propertyObjects[i]->name == objectName)
            copy_from_pui(_propertyObjects[i]->object,
                          _propertyObjects[i]->node);
    }
}

void
FGDialog::updateValues ()
{
    for (int i = 0; i < _propertyObjects.size(); i++)
        copy_to_pui(_propertyObjects[i]->node, _propertyObjects[i]->object);
}

void
FGDialog::applyValues ()
{
    for (int i = 0; i < _propertyObjects.size(); i++)
        copy_from_pui(_propertyObjects[i]->object,
                      _propertyObjects[i]->node);
}

void
FGDialog::display (SGPropertyNode_ptr props)
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
FGDialog::makeObject (SGPropertyNode * props, int parentWidth, int parentHeight)
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
    } else if (type == "checkbox") {
        puButton * b;
        b = new puButton(x, y, x + width, y + height, PUBUTTON_CIRCLE);
        setupObject(b, props);
        return b;
    } else if (type == "button") {
        puButton * b;
        const char * legend = props->getStringValue("legend", "[none]");
        if (props->getBoolValue("one-shot", true))
            b = new puOneShot(x, y, legend);
        else
            b = new puButton(x, y, legend);
        setupObject(b, props);
        return b;
    } else if (type == "combo") {
        vector<SGPropertyNode_ptr> value_nodes = props->getChildren("value");
        char ** entries = new char*[value_nodes.size()+1];
        for (int i = 0, j = value_nodes.size() - 1;
             i < value_nodes.size();
             i++, j--)
            entries[i] = (char *)value_nodes[i]->getStringValue();
        entries[value_nodes.size()] = 0;
        puComboBox * combo =
            new puComboBox(x, y, x + width, y + height, entries,
                           props->getBoolValue("editable", false));
//         delete entries;
        setupObject(combo, props);
        return combo;
    } else {
        return 0;
    }
}

void
FGDialog::setupObject (puObject * object, SGPropertyNode * props)
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
        copy_to_pui(node, object);
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
FGDialog::setupGroup (puGroup * group, SGPropertyNode * props,
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



////////////////////////////////////////////////////////////////////////
// Implementation of FGDialog::PropertyObject.
////////////////////////////////////////////////////////////////////////

FGDialog::PropertyObject::PropertyObject (const char * n,
                                           puObject * o,
                                           SGPropertyNode_ptr p)
    : name(n),
      object(o),
      node(p)
{
}


// end of dialog.cxx
