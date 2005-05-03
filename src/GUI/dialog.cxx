// dialog.cxx: implementation of an XML-configurable dialog box.

#include <stdlib.h>		// atof()

#include <Input/input.hxx>

#include "dialog.hxx"
#include "new_gui.hxx"

#include "puList.hxx"
#include "AirportList.hxx"
#include "layout.hxx"

int fgPopup::checkHit(int button, int updown, int x, int y)
{
    int result = puPopup::checkHit(button, updown, x, y);

    if ( !_draggable)
       return result;

    // This is annoying.  We would really want a true result from the
    // superclass to indicate "handled by child object", but all it
    // tells us is that the pointer is inside the dialog.  So do the
    // intersection test (again) to make sure we don't start a drag
    // when inside controls.

    if(updown == PU_DOWN && !_dragging) {
        if(!result)
            return 0;

        int hit = getHitObjects(this, x, y);
        if(hit & (PUCLASS_BUTTON|PUCLASS_ONESHOT|PUCLASS_INPUT))
            return result;

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
    return result;
}

int fgPopup::getHitObjects(puObject *object, int x, int y)
{
    int type = 0;
    if(object->getType() & PUCLASS_GROUP)
        for (puObject *obj = ((puGroup *)object)->getFirstChild();
                obj; obj = obj->getNextObject())
            type |= getHitObjects(obj, x, y);

    int cx, cy, cw, ch;
    object->getAbsolutePosition(&cx, &cy);
    object->getSize(&cw, &ch);
    if(x >= cx && x < cx + cw && y >= cy && y < cy + ch)
        type |= object->getType();
    return type;
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


static void
format_callback(puObject *obj, int dx, int dy, void *n)
{
    SGPropertyNode *node = (SGPropertyNode *)n;
    const char *format = node->getStringValue("format"), *f = format;
    bool number, l = false;
    // make sure the format matches '[ -+#]?\d*(\.\d*)?l?[fs]'
    for (; *f; f++) {
        if (*f == '%') {
            if (f[1] == '%')
                f++;
            else
                break;
        }
    }
    if (*f++ != '%')
        return;
    if (*f == ' ' || *f == '+' || *f == '-' || *f == '#')
        f++;
    while (*f && isdigit(*f))
        f++;
    if (*f == '.') {
        f++;
        while (*f && isdigit(*f))
            f++;
    }
    if (*f == 'l')
        l = true, f++;

    if (*f == 'f')
        number = true;
    else if (*f == 's') {
        if (l)
            return;
        number = false;
    } else
        return;

    for (++f; *f; f++) {
        if (*f == '%') {
            if (f[1] == '%')
                f++;
            else
                return;
        }
    }

    char buf[256];
    const char *src = obj->getLabel();

    if (number) {
        float value = atof(src);
        snprintf(buf, 256, format, value);
    } else {
        snprintf(buf, 256, format, src);
    }

    buf[255] = '\0';

    SGPropertyNode *result = node->getNode("formatted", true);
    result->setStringValue(buf);
    obj->setLabel(result->getStringValue());
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
    // Treat puText objects specially, so their "values" can be set
    // from properties.
    if(object->getType() & PUCLASS_TEXT) {
        object->setLabel(node->getStringValue());
        return;
    }

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
    // puText objects are immutable, so should not be copied out
    if(object->getType() & PUCLASS_TEXT)
        return;

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
        // Special case to handle lists, as getStringValue cannot be overridden
        if(object->getType() & PUCLASS_LIST)
        {
            node->setStringValue(((puList *) object)->getListStringValue());
        }
        else
        {
            node->setStringValue(object->getStringValue());
        }
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
FGDialog::update ()
{
    for (unsigned int i = 0; i < _liveObjects.size(); i++) {
        puObject *obj = _liveObjects[i]->object;
        if (obj->getType() & PUCLASS_INPUT && ((puInput *)obj)->isAcceptingInput())
            continue;

        copy_to_pui(_liveObjects[i]->node, obj);
    }
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

    bool userx = props->hasValue("x");
    bool usery = props->hasValue("y");
    bool userw = props->hasValue("width");
    bool userh = props->hasValue("height");

    LayoutWidget wid(props);
    int pw=0, ph=0;
    if(!userw || !userh)
        wid.calcPrefSize(&pw, &ph);
    pw = props->getIntValue("width", pw);
    ph = props->getIntValue("height", ph);
    int px = props->getIntValue("x", (screenw - pw) / 2);
    int py = props->getIntValue("y", (screenh - ph) / 2);
    wid.layout(px, py, pw, ph);

    _object = makeObject(props, screenw, screenh);

    // Remove automatically generated properties, so the layout looks
    // the same next time around.
    if(!userx) props->removeChild("x");
    if(!usery) props->removeChild("y");
    if(!userw) props->removeChild("width");
    if(!userh) props->removeChild("height");

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

    sgVec4 color = {0.8, 0.8, 0.9, 0.85};
    SGPropertyNode *ncs = props->getNode("color", false);
    if ( ncs ) {
       color[0] = ncs->getFloatValue("red", 0.8);
       color[1] = ncs->getFloatValue("green", 0.8);
       color[2] = ncs->getFloatValue("blue", 0.9);
       color[3] = ncs->getFloatValue("alpha", 0.85);
    }

    string type = props->getName();
    if (type == "")
        type = "dialog";

    if (type == "dialog") {
        puPopup * dialog;
        bool draggable = props->getBoolValue("draggable", true);
        if (props->getBoolValue("modal", false))
            dialog = new puDialogBox(x, y);
        else
            dialog = new fgPopup(x, y, draggable);
        setupGroup(dialog, props, width, height, color, true);
        return dialog;
    } else if (type == "group") {
        puGroup * group = new puGroup(x, y);
        setupGroup(group, props, width, height, color, false);
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

        if (props->getNode("format")) {
            SGPropertyNode *live = props->getNode("live");
            if (live && live->getBoolValue())
                text->setRenderCallback(format_callback, props);
            else
                format_callback(text, x, y, props);
        }
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
        if(presetSize)
            slider->setSize(width, height);
        return slider;
    } else if (type == "dial") {
        puDial * dial = new puDial(x, y, width);
        dial->setMinValue(props->getFloatValue("min", 0.0));
        dial->setMaxValue(props->getFloatValue("max", 1.0));
        dial->setWrap(props->getBoolValue("wrap", true));
        setupObject(dial, props);
        return dial;
    } else if (type == "textbox") {
       int slider_width = props->getIntValue("slider", parentHeight);
       int wrap = props->getBoolValue("wrap", true)==true;
       if (slider_width==0) slider_width=20;
       puLargeInput * puTextBox =
                     new puLargeInput(x, y, x+width, x+height, 2, slider_width, wrap);
       if  (props->hasValue("editable"))
       {
          if (props->getBoolValue("editable")==false)
             puTextBox->disableInput();
          else
             puTextBox->enableInput();
       }
       setupObject(puTextBox,props);
       return puTextBox;
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

    if ( SGPropertyNode *nft = props->getNode("font", false) ) {
       SGPath path( _font_path );
       string name = props->getStringValue("name");
       float size = props->getFloatValue("size", 13.0);
       float slant = props->getFloatValue("slant", 0.0);
       if ( name.empty() ) name = "typewriter";
       path.append( name );
       path.concat( ".txf" );

       fntFont *font = new fntTexFont;
       font->load( (char *)path.c_str() );

       puFont lfnt(font, size, slant);
       object->setLabelFont( lfnt );
    }

    if ( SGPropertyNode *ncs = props->getNode("color", false) ) {
       sgVec4 color;
       color[0] = ncs->getFloatValue("red", 0.0);
       color[1] = ncs->getFloatValue("green", 0.0);
       color[2] = ncs->getFloatValue("blue", 0.0);
       color[3] = ncs->getFloatValue("alpha", 1.0);
       object->setColor(PUCOL_LABEL, color[0], color[1], color[2], color[3]);
    }

    if (props->hasValue("property")) {
        const char * name = props->getStringValue("name");
        if (name == 0)
            name = "";
        const char * propname = props->getStringValue("property");
        SGPropertyNode_ptr node = fgGetNode(propname, true);
        copy_to_pui(node, object);
        PropertyObject* po = new PropertyObject(name, object, node);
        _propertyObjects.push_back(po);
        if(props->getBoolValue("live"))
            _liveObjects.push_back(po);
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
                    int width, int height, sgVec4 color, bool makeFrame)
{
    setupObject(group, props);

    if (makeFrame) {
        puFrame* f = new puFrame(0, 0, width, height);
        f->setColorScheme(color[0], color[1], color[2], color[3]);
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
