// new_gui.cxx: implementation of XML-configurable GUI support.

#include "new_gui.hxx"

#include <plib/pu.h>

#include <vector>
SG_USING_STD(vector);

#include <simgear/misc/exception.hxx>
#include <Main/fg_props.hxx>


NewGUI::NewGUI ()
    : _activeObject(0)
{
}

NewGUI::~NewGUI ()
{
}

void
NewGUI::init ()
{
    SGPropertyNode props;

    try {
        fgLoadProps("gui.xml", &props);
    } catch (const sg_exception &ex) {
        SG_LOG(SG_INPUT, SG_ALERT, "Error parsing gui.xml: "
               << ex.getMessage());
        return;
    }

    int nChildren = props.nChildren();
    for (int i = 0; i < nChildren; i++) {
        SGPropertyNode_ptr child = props.getChild(i);
        if (!child->hasValue("name")) {
            SG_LOG(SG_INPUT, SG_WARN, "GUI node " << child->getName()
                   << " has no name; skipping.");
        } else {
            string name = child->getStringValue("name");
            SG_LOG(SG_INPUT, SG_BULK, "Saving GUI node " << name);
            _objects[name] = child;
        }
    }
}

void
NewGUI::update (double delta_time_sec)
{
    // NO OP
}

static void
close_callback (puObject * object)
{
    ((NewGUI *)object->getUserData())->closeActiveObject();
}

void
NewGUI::closeActiveObject ()
{
    delete _activeObject;
    _activeObject = 0;
}

void
NewGUI::display (const string &name)
{
    if (_activeObject != 0) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Another GUI object is still active");
        return;
    }

    if (_objects.find(name) == _objects.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Dialog " << name << " not defined");
        return;
    }
    SGPropertyNode_ptr props = _objects[name];
    
    _activeObject = makeObject(props, 1024, 768);

    if (_activeObject != 0) {
        _activeObject->reveal();
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "Dialog " << name
               << " does not contain a proper GUI definition");
    }
}

puObject *
NewGUI::makeObject (SGPropertyNode * props, int parentWidth, int parentHeight)
{
    int width = props->getIntValue("width", parentWidth);
    int height = props->getIntValue("height", parentHeight);

    int x = props->getIntValue("x", (parentWidth - width) / 2);
    int y = props->getIntValue("y", (parentHeight - height) / 2);

    string type = props->getName();

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
        b->setCallback(close_callback);
        b->setUserData(this);
        return b;
    } else {
        return 0;
    }
}

void
NewGUI::setupObject (puObject * object, SGPropertyNode * props)
{
    if (props->hasValue("legend"))
        object->setLegend(props->getStringValue("legend"));

    if (props->hasValue("label"))
        object->setLabel(props->getStringValue("label"));

    if (props->hasValue("default-value-prop"))
        object->setValue(fgGetString(props->getStringValue("default-value-prop")));

    object->makeReturnDefault(props->getBoolValue("default"));
}

void
NewGUI::setupGroup (puGroup * group, SGPropertyNode * props,
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

// end of new_gui.cxx
