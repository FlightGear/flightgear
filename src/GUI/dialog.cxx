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
    for (int i = 0; i < info->bindings.size(); i++)
        info->bindings[i]->fire();
    gui->setCurrentWidget(0);
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
        if (_propertyObjects[i]->name == objectName)
            _propertyObjects[i]->object
                ->setValue(_propertyObjects[i]->node->getStringValue());
    }
}

void
FGDialog::applyValue (const char * objectName)
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        if (_propertyObjects[i]->name == objectName)
            _propertyObjects[i]->node
                ->setStringValue(_propertyObjects[i]
                                 ->object->getStringValue());
    }
}

void
FGDialog::updateValues ()
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        puObject * object = _propertyObjects[i]->object;
        SGPropertyNode_ptr node = _propertyObjects[i]->node;
        object->setValue(node->getStringValue());
    }
}

void
FGDialog::applyValues ()
{
    for (int i = 0; i < _propertyObjects.size(); i++) {
        puObject * object = _propertyObjects[i]->object;
        SGPropertyNode_ptr node = _propertyObjects[i]->node;
        node->setStringValue(object->getStringValue());
    }
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
