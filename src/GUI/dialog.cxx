// dialog.cxx: implementation of an XML-configurable dialog box.

#include <Input/input.hxx>

#include "dialog.hxx"
#include "new_gui.hxx"

#include "puList.hxx"
#include "AirportList.hxx"
#include "layout.hxx"

int fgPopup::checkHit(int button, int updown, int x, int y)
{
    int result = puPopup::checkHit(button, updown, x, y);

    // This is annoying.  We would really want a true result from the
    // superclass to indicate "handled by child object", but all it
    // tells us is that the pointer is inside the dialog.  So do the
    // intersection test (again) to make sure we don't start a drag
    // when inside controls.  A further weirdness: plib inserts a
    // "ghost" child which covers the whole control. (?)  Skip it.
    if(!result) return result;
    puObject* child = getFirstChild();
    if(child) child = child->getNextObject();
    while(child) {
        int cx, cy, cw, ch;
        child->getAbsolutePosition(&cx, &cy);
        child->getSize(&cw, &ch);
        if(x >= cx && x < cx + cw && y >= cy && y < cy + ch)
            return result;
        child = child->getNextObject();
    }

    // Finally, handle the mouse event
    if(updown == PU_DOWN) {
        int px, py;
        getPosition(&px, &py);
        _dragging = true;
        _dX = px - x;
        _dY = py - y;
    } else if(updown == PU_DRAG && _dragging) {
        setPosition(x + _dX, y + _dY);
    } else {
        _dragging = false;
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////
// Callbacks.
////////////////////////////////////////////////////////////////////////

/**
 * User data for a GUI object.
 */
struct GUIInfo
{
    GUIInfo (FGDialog * d);
    virtual ~GUIInfo ();

    FGDialog * dialog;
    vector <FGBinding *> bindings;
};


/**
 * Action callback.
 */
static void
action_callback (puObject * object)
{
    GUIInfo * info = (GUIInfo *)object->getUserData();
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    gui->setActiveDialog(info->dialog);
    int nBindings = info->bindings.size();
    for (int i = 0; i < nBindings; i++) {
        info->bindings[i]->fire();
        if (gui->getActiveDialog() == 0)
            break;
    }
    gui->setActiveDialog(0);
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

GUIInfo::GUIInfo (FGDialog * d)
    : dialog(d)
{
}

GUIInfo::~GUIInfo ()
{
    for (unsigned int i = 0; i < bindings.size(); i++) {
        delete bindings[i];
        bindings[i] = 0;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGDialog.
////////////////////////////////////////////////////////////////////////

FGDialog::FGDialog (SGPropertyNode * props)
    : _object(0)
{
    display(props);
}

FGDialog::~FGDialog ()
{
    puDeleteObject(_object);

    unsigned int i;

                                // Delete all the arrays we made
                                // and were forced to keep around
                                // because PUI won't do its own
                                // memory management.
    for (i = 0; i < _char_arrays.size(); i++) {
        for (int j = 0; _char_arrays[i][j] != 0; j++)
            free(_char_arrays[i][j]); // added with strdup
        delete[] _char_arrays[i];
    }

                                // Delete all the info objects we
                                // were forced to keep around because
                                // PUI cannot delete its own user data.
    for (i = 0; i < _info.size(); i++) {
        delete (GUIInfo *)_info[i];
        _info[i] = 0;
    }

                                // Finally, delete the property links.
    for (i = 0; i < _propertyObjects.size(); i++) {
        delete _propertyObjects[i];
        _propertyObjects[i] = 0;
    }
}

void
FGDialog::updateValue (const char * objectName)
{
    for (unsigned int i = 0; i < _propertyObjects.size(); i++) {
        const string &name = _propertyObjects[i]->name;
        if (name == objectName)
            copy_to_pui(_propertyObjects[i]->node,
                        _propertyObjects[i]->object);
    }
}

void
FGDialog::applyValue (const char * objectName)
{
    for (unsigned int i = 0; i < _propertyObjects.size(); i++) {
        if (_propertyObjects[i]->name == objectName)
            copy_from_pui(_propertyObjects[i]->object,
                          _propertyObjects[i]->node);
    }
}

void
FGDialog::updateValues ()
{
    for (unsigned int i = 0; i < _propertyObjects.size(); i++)
        copy_to_pui(_propertyObjects[i]->node, _propertyObjects[i]->object);
}

void
FGDialog::applyValues ()
{
    for (unsigned int i = 0; i < _propertyObjects.size(); i++)
        copy_from_pui(_propertyObjects[i]->object,
                      _propertyObjects[i]->node);
}

void
FGDialog::display (SGPropertyNode * props)
{
    if (_object != 0) {
        SG_LOG(SG_GENERAL, SG_ALERT, "This widget is already active");
        return;
    }

    int screenw = globals->get_props()->getIntValue("/sim/startup/xsize");
    int screenh = globals->get_props()->getIntValue("/sim/startup/ysize");

    LayoutWidget wid(props);
    int pw=0, ph=0;
    if(!props->hasValue("width") || !props->hasValue("height"))
        wid.calcPrefSize(&pw, &ph);
    pw = props->getIntValue("width", pw);
    ph = props->getIntValue("height", ph);
    int px = props->getIntValue("x", (screenw - pw) / 2);
    int py = props->getIntValue("y", (screenh - ph) / 2);
    wid.layout(px, py, pw, ph);

    _object = makeObject(props, screenw, screenh);

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
    bool presetSize = props->hasValue("width") && props->hasValue("height");
    int width = props->getIntValue("width", parentWidth);
    int height = props->getIntValue("height", parentHeight);
    int x = props->getIntValue("x", (parentWidth - width) / 2);
    int y = props->getIntValue("y", (parentHeight - height) / 2);

    string type = props->getName();
    if (type == "")
        type = "dialog";

    if (type == "dialog") {
        puPopup * dialog;
        if (props->getBoolValue("modal", false))
            dialog = new puDialogBox(x, y);
        else
            dialog = new fgPopup(x, y);
        setupGroup(dialog, props, width, height, true);
        return dialog;
    } else if (type == "group") {
        puGroup * group = new puGroup(x, y);
        setupGroup(group, props, width, height, false);
        return group;
    } else if (type == "list") {
        puList * list = new puList(x, y, x + width, y + height);
        setupObject(list, props);
        return list;
    } else if (type == "airport-list") {
        AirportList * list = new AirportList(x, y, x + width, y + height);
        setupObject(list, props);
        return list;
    } else if (type == "input") {
        puInput * input = new puInput(x, y, x + width, y + height);
        setupObject(input, props);
        return input;
    } else if (type == "text") {
        puText * text = new puText(x, y);
        setupObject(text, props);
        // Layed-out objects need their size set, and non-layout ones
        // get a different placement.
        if(presetSize) text->setSize(width, height);
        else text->setLabelPlace(PUPLACE_LABEL_DEFAULT);
        return text;
    } else if (type == "checkbox") {
        puButton * b;
        b = new puButton(x, y, x + width, y + height, PUBUTTON_XCHECK);
        b->setColourScheme(.8, .7, .7); // matches "PUI input pink"
        setupObject(b, props);
        return b;
    } else if (type == "radio") {
        puButton * b;
        b = new puButton(x, y, x + width, y + height, PUBUTTON_CIRCLE);
        b->setColourScheme(.8, .7, .7); // matches "PUI input pink"
        setupObject(b, props);
        return b;
    } else if (type == "button") {
        puButton * b;
        const char * legend = props->getStringValue("legend", "[none]");
        if (props->getBoolValue("one-shot", true))
            b = new puOneShot(x, y, legend);
        else
            b = new puButton(x, y, legend);
        if(presetSize)
            b->setSize(width, height);
        setupObject(b, props);
        return b;
    } else if (type == "combo") {
        vector<SGPropertyNode_ptr> value_nodes = props->getChildren("value");
        char ** entries = make_char_array(value_nodes.size());
        for (unsigned int i = 0, j = value_nodes.size() - 1;
             i < value_nodes.size();
             i++, j--)
            entries[i] = strdup((char *)value_nodes[i]->getStringValue());
        puComboBox * combo =
            new puComboBox(x, y, x + width, y + height, entries,
                           props->getBoolValue("editable", false));
        setupObject(combo, props);
        return combo;
    } else if (type == "slider") {
        bool vertical = props->getBoolValue("vertical", false);
        puSlider * slider = new puSlider(x, y, (vertical ? height : width));
        slider->setMinValue(props->getFloatValue("min", 0.0));
        slider->setMaxValue(props->getFloatValue("max", 1.0));
        setupObject(slider, props);
        return slider;
    } else if (type == "dial") {
        puDial * dial = new puDial(x, y, width);
        dial->setMinValue(props->getFloatValue("min", 0.0));
        dial->setMaxValue(props->getFloatValue("max", 1.0));
        dial->setWrap(props->getBoolValue("wrap", true));
        setupObject(dial, props);
        return dial;
    } else if (type == "select") {
        vector<SGPropertyNode_ptr> value_nodes;
        SGPropertyNode * selection_node =
                fgGetNode(props->getChild("selection")->getStringValue(), true);

        for (int q = 0; q < selection_node->nChildren(); q++)
            value_nodes.push_back(selection_node->getChild(q));

        char ** entries = make_char_array(value_nodes.size());
        for (unsigned int i = 0, j = value_nodes.size() - 1;
             i < value_nodes.size();
             i++, j--)
            entries[i] = strdup((char *)value_nodes[i]->getName());
        puSelectBox * select =
            new puSelectBox(x, y, x + width, y + height, entries);
        setupObject(select, props);
        return select;
    } else {
        return 0;
    }
}

void
FGDialog::setupObject (puObject * object, SGPropertyNode * props)
{
    object->setLabelPlace(PUPLACE_CENTERED_RIGHT);

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

        for (unsigned int i = 0; i < nodes.size(); i++)
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

    if (makeFrame) {
        puFrame* f = new puFrame(0, 0, width, height);
        f->setColorScheme(0.8, 0.8, 0.9, 0.85);
    }

    int nChildren = props->nChildren();
    for (int i = 0; i < nChildren; i++)
        makeObject(props->getChild(i), width, height);
    group->close();
}

char **
FGDialog::make_char_array (int size)
{
    char ** list = new char*[size+1];
    for (int i = 0; i <= size; i++)
        list[i] = 0;
    _char_arrays.push_back(list);
    return list;
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
