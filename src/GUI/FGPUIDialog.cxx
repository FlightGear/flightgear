// dialog.cxx: implementation of an XML-configurable dialog box.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/structure/SGBinding.hxx>
#include <simgear/props/props_io.hxx>

#include <Scripting/NasalSys.hxx>
#include <Main/fg_os.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "FGPUIDialog.hxx"
#include "new_gui.hxx"
#include "AirportList.hxx"
#include "property_list.hxx"
#include "layout.hxx"
#include "WaypointList.hxx"
#include "MapWidget.hxx"
#include "FGFontCache.hxx"
#include "FGColor.hxx"

enum format_type { f_INVALID, f_INT, f_LONG, f_FLOAT, f_DOUBLE, f_STRING };
static const int FORMAT_BUFSIZE = 255;
static const int RESIZE_MARGIN = 7;


/**
 * Makes sure the format matches '%[ -+#]?\d*(\.\d*)?(l?[df]|s)', with
 * only one number or string placeholder and otherwise arbitrary prefix
 * and postfix containing only quoted percent signs (%%).
 */
static format_type
validate_format(const char *f)
{
    bool l = false;
    format_type type;
    for (; *f; f++) {
        if (*f == '%') {
            if (f[1] == '%')
                f++;
            else
                break;
        }
    }
    if (*f++ != '%')
        return f_INVALID;
    while (*f == ' ' || *f == '+' || *f == '-' || *f == '#' || *f == '0')
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

    if (*f == 'd') {
        type = l ? f_LONG : f_INT;
    } else if (*f == 'f')
        type = l ? f_DOUBLE : f_FLOAT;
    else if (*f == 's') {
        if (l)
            return f_INVALID;
        type = f_STRING;
    } else
        return f_INVALID;

    for (++f; *f; f++) {
        if (*f == '%') {
            if (f[1] == '%')
                f++;
            else
                return f_INVALID;
        }
    }
    return type;
}


////////////////////////////////////////////////////////////////////////
// Implementation of GUIInfo.
////////////////////////////////////////////////////////////////////////

/**
 * User data for a GUI object.
 */
struct GUIInfo
{
    GUIInfo(FGPUIDialog *d);
    virtual ~GUIInfo();
    void apply_format(SGPropertyNode *);

    FGPUIDialog *dialog;
    SGPropertyNode_ptr node;
    vector <SGBinding *> bindings;
    int key;
    string label, legend, text, format;
    format_type fmt_type;
};

GUIInfo::GUIInfo (FGPUIDialog *d) :
    dialog(d),
    key(-1),
    fmt_type(f_INVALID)
{
}

GUIInfo::~GUIInfo ()
{
    for (unsigned int i = 0; i < bindings.size(); i++) {
        delete bindings[i];
        bindings[i] = 0;
    }
}

void GUIInfo::apply_format(SGPropertyNode *n)
{
    char buf[FORMAT_BUFSIZE + 1];
    if (fmt_type == f_INT)
        snprintf(buf, FORMAT_BUFSIZE, format.c_str(), n->getIntValue());
    else if (fmt_type == f_LONG)
        snprintf(buf, FORMAT_BUFSIZE, format.c_str(), n->getLongValue());
    else if (fmt_type == f_FLOAT)
        snprintf(buf, FORMAT_BUFSIZE, format.c_str(), n->getFloatValue());
    else if (fmt_type == f_DOUBLE)
        snprintf(buf, FORMAT_BUFSIZE, format.c_str(), n->getDoubleValue());
    else
        snprintf(buf, FORMAT_BUFSIZE, format.c_str(), n->getStringValue());

    buf[FORMAT_BUFSIZE] = '\0';
    text = buf;
}



/**
 * Key handler.
 */
int fgPopup::checkKey(int key, int updown)
{
    if (updown == PU_UP || !isVisible() || !isActive() || window != puGetWindow())
        return false;

    puObject *input = getActiveInputField(this);
    if (input)
        return input->checkKey(key, updown);

    puObject *object = getKeyObject(this, key);
    if (!object)
        return puPopup::checkKey(key, updown);

    // invokeCallback() isn't enough; we need to simulate a mouse button press
    object->checkHit(PU_LEFT_BUTTON, PU_DOWN,
            (object->getABox()->min[0] + object->getABox()->max[0]) / 2,
            (object->getABox()->min[1] + object->getABox()->max[1]) / 2);
    object->checkHit(PU_LEFT_BUTTON, PU_UP,
            (object->getABox()->min[0] + object->getABox()->max[0]) / 2,
            (object->getABox()->min[1] + object->getABox()->max[1]) / 2);
    return true;
}

puObject *fgPopup::getKeyObject(puObject *object, int key)
{
    puObject *ret;
    if (object->getType() & PUCLASS_GROUP)
        for (puObject *obj = ((puGroup *)object)->getFirstChild();
                obj; obj = obj->getNextObject())
            if ((ret = getKeyObject(obj, key)))
                return ret;

    GUIInfo *info = (GUIInfo *)object->getUserData();
    if (info && info->key == key)
        return object;

    return 0;
}

puObject *fgPopup::getActiveInputField(puObject *object)
{
    puObject *ret;
    if (object->getType() & PUCLASS_GROUP)
        for (puObject *obj = ((puGroup *)object)->getFirstChild();
                obj; obj = obj->getNextObject())
            if ((ret = getActiveInputField(obj)))
                return ret;

    if (object->getType() & (PUCLASS_INPUT|PUCLASS_LARGEINPUT)
            && ((puInput *)object)->isAcceptingInput())
        return object;

    return 0;
}

/**
 * Mouse handler.
 */
int fgPopup::checkHit(int button, int updown, int x, int y)
{
    int result = 0;
    if (updown != PU_DRAG && !_dragging)
        result = puPopup::checkHit(button, updown, x, y);

    if (!_draggable)
       return result;

    // This is annoying.  We would really want a true result from the
    // superclass to indicate "handled by child object", but all it
    // tells us is that the pointer is inside the dialog.  So do the
    // intersection test (again) to make sure we don't start a drag
    // when inside controls.

    if (updown == PU_DOWN && !_dragging) {
        if (!result)
            return 0;
        int global_drag = fgGetKeyModifiers() & KEYMOD_SHIFT;
        int global_resize = fgGetKeyModifiers() & KEYMOD_CTRL;

        int hit = getHitObjects(this, x, y);
        if (hit & PUCLASS_LIST)  // ctrl-click in property browser (toggle bool)
            return result;
        if (!global_resize && hit & (PUCLASS_BUTTON|PUCLASS_ONESHOT|PUCLASS_INPUT
                |PUCLASS_LARGEINPUT|PUCLASS_SCROLLBAR))
            return result;

        getPosition(&_dlgX, &_dlgY);
        getSize(&_dlgW, &_dlgH);
        _start_cursor = fgGetMouseCursor();
        _dragging = true;
        _startX = x;
        _startY = y;

        // check and prepare for resizing
        static const int cursor[] = {
            MOUSE_CURSOR_POINTER, MOUSE_CURSOR_LEFTSIDE, MOUSE_CURSOR_RIGHTSIDE, 0,
            MOUSE_CURSOR_TOPSIDE, MOUSE_CURSOR_TOPLEFT, MOUSE_CURSOR_TOPRIGHT, 0,
            MOUSE_CURSOR_BOTTOMSIDE, MOUSE_CURSOR_BOTTOMLEFT, MOUSE_CURSOR_BOTTOMRIGHT, 0,
        };

        _resizing = 0;
        if (!global_drag && _resizable) {
            int hmargin = global_resize ? _dlgW / 3 : RESIZE_MARGIN;
            int vmargin = global_resize ? _dlgH / 3 : RESIZE_MARGIN;

            if (y - _dlgY < vmargin)
                _resizing |= BOTTOM;
            else if (_dlgY + _dlgH - y < vmargin)
                _resizing |= TOP;

            if (x - _dlgX < hmargin)
                _resizing |= LEFT;
            else if (_dlgX + _dlgW - x < hmargin)
                _resizing |= RIGHT;

            if (!_resizing && global_resize)
                _resizing = BOTTOM|RIGHT;

            _cursor = cursor[_resizing];
           if (_resizing && _resizable)
                fgSetMouseCursor(_cursor);
       }

    } else if (updown == PU_DRAG && _dragging) {
        if (_resizing) {
            GUIInfo *info = (GUIInfo *)getUserData();
            if (_resizable && info && info->node) {
                int w = _dlgW;
                int h = _dlgH;
                if (_resizing & LEFT)
                    w += _startX - x;
                if (_resizing & RIGHT)
                    w += x - _startX;
                if (_resizing & TOP)
                    h += y - _startY;
                if (_resizing & BOTTOM)
                    h += _startY - y;

                int prefw, prefh;
                LayoutWidget wid(info->node);
                wid.calcPrefSize(&prefw, &prefh);
                if (w < prefw)
                    w = prefw;
                if (h < prefh)
                    h = prefh;

                int x = _dlgX;
                int y = _dlgY;
                if (_resizing & LEFT)
                    x += _dlgW - w;
                if (_resizing & BOTTOM)
                    y += _dlgH - h;

                wid.layout(x, y, w, h);
                setSize(w, h);
                setPosition(x, y);
                applySize(static_cast<puObject *>(this));
                getFirstChild()->setSize(w, h); // dialog background puFrame
            }
        } else {
            int posX = x + _dlgX - _startX,
              posY = y + _dlgY - _startY;
            setPosition(posX, posY);
            
            GUIInfo *info = (GUIInfo *)getUserData();
            if (info && info->node) {
                info->node->setIntValue("x", posX);
                info->node->setIntValue("y", posY);
            }
        } // re-positioning

    } else if (_dragging) {
        fgSetMouseCursor(_start_cursor);
        _dragging = false;
    }
    return result;
}

int fgPopup::getHitObjects(puObject *object, int x, int y)
{
    if (!object->isVisible())
        return 0;

    int type = 0;
    if (object->getType() & PUCLASS_GROUP)
        for (puObject *obj = ((puGroup *)object)->getFirstChild();
                obj; obj = obj->getNextObject())
            type |= getHitObjects(obj, x, y);

    int cx, cy, cw, ch;
    object->getAbsolutePosition(&cx, &cy);
    object->getSize(&cw, &ch);
    if (x >= cx && x < cx + cw && y >= cy && y < cy + ch)
        type |= object->getType();
    return type;
}

void fgPopup::applySize(puObject *object)
{
    // compound plib widgets use setUserData() for internal purposes, so refuse
    // to descend into anything that has other bits set than the following
    const int validUserData = PUCLASS_VALUE|PUCLASS_OBJECT|PUCLASS_GROUP|PUCLASS_INTERFACE
            |PUCLASS_FRAME|PUCLASS_TEXT|PUCLASS_BUTTON|PUCLASS_ONESHOT|PUCLASS_INPUT
            |PUCLASS_ARROW|PUCLASS_DIAL|PUCLASS_POPUP;

    int type = object->getType();
    if (type & PUCLASS_GROUP && !(type & ~validUserData))
        for (puObject *obj = ((puGroup *)object)->getFirstChild();
                obj; obj = obj->getNextObject())
            applySize(obj);

    GUIInfo *info = (GUIInfo *)object->getUserData();
    if (!info)
        return;

    SGPropertyNode *n = info->node;
    if (!n) {
        SG_LOG(SG_GENERAL, SG_ALERT, "fgPopup::applySize: no props");
        return;
    }
    int x = n->getIntValue("x");
    int y = n->getIntValue("y");
    int w = n->getIntValue("width", 4);
    int h = n->getIntValue("height", 4);
    object->setPosition(x, y);
    object->setSize(w, h);
}

////////////////////////////////////////////////////////////////////////

void FGPUIDialog::ConditionalObject::update(FGPUIDialog* aDlg)
{
  if (_name == "visible") {
    bool wasVis = _pu->isVisible() != 0;
    bool newVis = test();
    
    if (newVis == wasVis) {
      return;
    }
    
    if (newVis) { // puObject needs a setVisible. Oh well.
    _pu->reveal();
    } else {
      _pu->hide();
    }
  } else if (_name == "enable") {
    bool wasEnabled = _pu->isActive() != 0;
    bool newEnable = test();
    
    if (wasEnabled == newEnable) {
      return;
    }
    
    if (newEnable) {
      _pu->activate();
    } else {
      _pu->greyOut();
    }
  }
  
  aDlg->setNeedsLayout();
}
////////////////////////////////////////////////////////////////////////
// Callbacks.
////////////////////////////////////////////////////////////////////////

/**
 * Action callback.
 */
static void
action_callback (puObject *object)
{
    GUIInfo *info = (GUIInfo *)object->getUserData();
    NewGUI *gui = (NewGUI *)globals->get_subsystem("gui");
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
copy_to_pui (SGPropertyNode *node, puObject *object)
{
    using namespace simgear;
    GUIInfo *info = (GUIInfo *)object->getUserData();
    if (!info) {
        SG_LOG(SG_GENERAL, SG_ALERT, "dialog: widget without GUIInfo!");
        return;   // this can't really happen
    }

    // Treat puText objects specially, so their "values" can be set
    // from properties.
    if (object->getType() & PUCLASS_TEXT) {
        if (info->fmt_type != f_INVALID)
            info->apply_format(node);
        else
            info->text = node->getStringValue();

        object->setLabel(info->text.c_str());
        return;
    }

    switch (node->getType()) {
    case props::BOOL:
    case props::INT:
    case props::LONG:
        object->setValue(node->getIntValue());
        break;
    case props::FLOAT:
    case props::DOUBLE:
        object->setValue(node->getFloatValue());
        break;
    default:
        info->text = node->getStringValue();
        object->setValue(info->text.c_str());
        break;
    }
}


static void
copy_from_pui (puObject *object, SGPropertyNode *node)
{
    using namespace simgear;
    // puText objects are immutable, so should not be copied out
    if (object->getType() & PUCLASS_TEXT)
        return;

    switch (node->getType()) {
    case props::BOOL:
    case props::INT:
    case props::LONG:
        node->setIntValue(object->getIntegerValue());
        break;
    case props::FLOAT:
    case props::DOUBLE:
        node->setFloatValue(object->getFloatValue());
        break;
    default:
        const char *s = object->getStringValue();
        if (s)
            node->setStringValue(s);
        break;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGDialog.
////////////////////////////////////////////////////////////////////////

FGPUIDialog::FGPUIDialog (SGPropertyNode *props) :
    FGDialog(props),
    _object(0),
    _gui((NewGUI *)globals->get_subsystem("gui")),
    _props(props),
    _needsRelayout(false)
{
    _module = string("__dlg:") + props->getStringValue("name", "[unnamed]");
        
    SGPropertyNode *nasal = props->getNode("nasal");
    if (nasal) {
        _nasal_close = nasal->getNode("close");
        SGPropertyNode *open = nasal->getNode("open");
        if (open) {
            const char *s = open->getStringValue();
            FGNasalSys *nas = (FGNasalSys *)globals->get_subsystem("nasal");
            if (nas)
                nas->createModule(_module.c_str(), _module.c_str(), s, strlen(s), props);
        }
    }
    display(props);
}

FGPUIDialog::~FGPUIDialog ()
{
    int x, y;
    _object->getAbsolutePosition(&x, &y);
    _props->setIntValue("lastx", x);
    _props->setIntValue("lasty", y);

    FGNasalSys *nas = (FGNasalSys *)globals->get_subsystem("nasal");
    if (nas) {
        if (_nasal_close) {
            const char *s = _nasal_close->getStringValue();
            nas->createModule(_module.c_str(), _module.c_str(), s, strlen(s), _props);
        }
        nas->deleteModule(_module.c_str());
    }

    puDeleteObject(_object);

    unsigned int i;
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
FGPUIDialog::updateValues (const char *objectName)
{
    if (objectName && !objectName[0])
        objectName = 0;

  for (unsigned int i = 0; i < _propertyObjects.size(); i++) {
    const string &name = _propertyObjects[i]->name;
    if (objectName && name != objectName) {
      continue;
    }
    
    puObject *widget = _propertyObjects[i]->object;
    int widgetType = widget->getType();
    if (widgetType & PUCLASS_LIST) {
      GUI_ID* gui_id = dynamic_cast<GUI_ID *>(widget);
      if (gui_id && (gui_id->id & FGCLASS_LIST)) {
        fgList *pl = static_cast<fgList*>(widget);
        pl->update();
      } else {
        copy_to_pui(_propertyObjects[i]->node, widget);
      }
    } else if (widgetType & PUCLASS_COMBOBOX) {
      fgComboBox* combo = static_cast<fgComboBox*>(widget);
      combo->update();
    } else {
      copy_to_pui(_propertyObjects[i]->node, widget);
    }
  } // of property objects iteration
}

void
FGPUIDialog::applyValues (const char *objectName)
{
    if (objectName && !objectName[0])
        objectName = 0;

    for (unsigned int i = 0; i < _propertyObjects.size(); i++) {
        const string &name = _propertyObjects[i]->name;
        if (objectName && name != objectName)
            continue;

        copy_from_pui(_propertyObjects[i]->object,
                      _propertyObjects[i]->node);
    }
}

void
FGPUIDialog::update ()
{
    for (unsigned int i = 0; i < _liveObjects.size(); i++) {
        puObject *obj = _liveObjects[i]->object;
        if (obj->getType() & PUCLASS_INPUT && ((puInput *)obj)->isAcceptingInput())
            continue;

        copy_to_pui(_liveObjects[i]->node, obj);
    }
    
  for (unsigned int j=0; j < _conditionalObjects.size(); ++j) {
    _conditionalObjects[j]->update(this);
  }
  
  if (_needsRelayout) {
    relayout();
  }
}

void
FGPUIDialog::display (SGPropertyNode *props)
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

    // Let the layout widget work in the same property subtree.
    LayoutWidget wid(props);

    SGPropertyNode *fontnode = props->getNode("font");
    if (fontnode) {
        FGFontCache *fc = globals->get_fontcache();
        _font = fc->get(fontnode);
    } else {
        _font = _gui->getDefaultFont();
    }
    wid.setDefaultFont(_font, int(_font->getPointSize()));

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

    // Use the dimension and location properties as specified in the
    // configuration file or from the layout widget.
    _object = makeObject(props, screenw, screenh);

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

    if (_object != 0) {
        _object->reveal();
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "Widget "
               << props->getStringValue("name", "[unnamed]")
               << " does not contain a proper GUI definition");
    }
}

puObject *
FGPUIDialog::makeObject (SGPropertyNode *props, int parentWidth, int parentHeight)
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
        puPopup *obj;
        bool draggable = props->getBoolValue("draggable", true);
        bool resizable = props->getBoolValue("resizable", false);
        if (props->getBoolValue("modal", false))
            obj = new puDialogBox(x, y);
        else
            obj = new fgPopup(x, y, resizable, draggable);
        setupGroup(obj, props, width, height, true);
        setColor(obj, props);
        return obj;

    } else if (type == "group") {
        puGroup *obj = new puGroup(x, y);
        setupGroup(obj, props, width, height, false);
        setColor(obj, props);
        return obj;

    } else if (type == "frame") {
        puGroup *obj = new puGroup(x, y);
        setupGroup(obj, props, width, height, true);
        setColor(obj, props);
        return obj;

    } else if (type == "hrule" || type == "vrule") {
        puFrame *obj = new puFrame(x, y, x + width, y + height);
        obj->setBorderThickness(0);
        setupObject(obj, props);
        setColor(obj, props, BACKGROUND|FOREGROUND|HIGHLIGHT);
        return obj;

    } else if (type == "list") {
        int slider_width = props->getIntValue("slider", 20);
        fgList *obj = new fgList(x, y, x + width, y + height, props, slider_width);
        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props);
        return obj;

    } else if (type == "airport-list") {
        AirportList *obj = new AirportList(x, y, x + width, y + height);
        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props);
        return obj;

    } else if (type == "property-list") {
        PropertyList *obj = new PropertyList(x, y, x + width, y + height, globals->get_props());
        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props);
        return obj;

    } else if (type == "input") {
        puInput *obj = new puInput(x, y, x + width, y + height);
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND|LABEL);
        return obj;

    } else if (type == "text") {
        puText *obj = new puText(x, y);
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
        puButton *obj;
        obj = new puButton(x, y, x + width, y + height, PUBUTTON_XCHECK);
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND|LABEL);
        return obj;

    } else if (type == "radio") {
        puButton *obj;
        obj = new puButton(x, y, x + width, y + height, PUBUTTON_CIRCLE);
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND|LABEL);
        return obj;

    } else if (type == "button") {
        puButton *obj;
        const char *legend = props->getStringValue("legend", "[none]");
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
        return mapWidget;
    } else if (type == "combo") {
        fgComboBox *obj = new fgComboBox(x, y, x + width, y + height, props,
                props->getBoolValue("editable", false));
        setupObject(obj, props);
        setColor(obj, props, EDITFIELD);
        return obj;

    } else if (type == "slider") {
        bool vertical = props->getBoolValue("vertical", false);
        puSlider *obj = new puSlider(x, y, (vertical ? height : width), vertical);
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
        setColor(obj, props, FOREGROUND|LABEL);
        return obj;

    } else if (type == "dial") {
        puDial *obj = new puDial(x, y, width);
        obj->setMinValue(props->getFloatValue("min", 0.0));
        obj->setMaxValue(props->getFloatValue("max", 1.0));
        obj->setWrap(props->getBoolValue("wrap", true));
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND|LABEL);
        return obj;

    } else if (type == "textbox") {
        int slider_width = props->getIntValue("slider", 20);
        int wrap = props->getBoolValue("wrap", true);
#if PLIB_VERSION > 185
        puaLargeInput *obj = new puaLargeInput(x, y,
                x + width, x + height, 11, slider_width, wrap);
#else
        puaLargeInput *obj = new puaLargeInput(x, y,
                x + width, x + height, 2, slider_width, wrap);
#endif

        if (props->getBoolValue("editable"))
            obj->enableInput();
        else
            obj->disableInput();

        if (presetSize)
            obj->setSize(width, height);
        setupObject(obj, props);
        setColor(obj, props, FOREGROUND|LABEL);

        int top = props->getIntValue("top-line", 0);
        obj->setTopLineInWindow(top < 0 ? unsigned(-1) >> 1 : top);
        return obj;

    } else if (type == "select") {
        fgSelectBox *obj = new fgSelectBox(x, y, x + width, y + height, props);
        setupObject(obj, props);
        setColor(obj, props, EDITFIELD);
        return obj;
    } else if (type == "waypointlist") {
        ScrolledWaypointList* obj = new ScrolledWaypointList(x, y, width, height);
        setupObject(obj, props);
        return obj;
    } else {
        return 0;
    }
}

void
FGPUIDialog::setupObject (puObject *object, SGPropertyNode *props)
{
    GUIInfo *info = new GUIInfo(this);
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
        object->setBorderThickness( props->getIntValue("border", 2) );

    if (SGPropertyNode *nft = props->getNode("font", false)) {
       FGFontCache *fc = globals->get_fontcache();
       puFont *lfnt = fc->get(nft);
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
        const char *format = props->getStringValue("format", 0);
        if (format) {
            info->fmt_type = validate_format(format);
            if (info->fmt_type != f_INVALID)
                info->format = format;
            else
                SG_LOG(SG_GENERAL, SG_ALERT, "DIALOG: invalid <format> '"
                        << format << '\'');
        }
    }

    if (props->hasValue("property")) {
        const char *name = props->getStringValue("name");
        if (name == 0)
            name = "";
        const char *propname = props->getStringValue("property");
        SGPropertyNode_ptr node = fgGetNode(propname, true);
        if (type == "map") {
          // mapWidget binds to a sub-tree of properties, and
          // ignores the puValue mechanism, so special case things here
          MapWidget* mw = static_cast<MapWidget*>(object);
          mw->setProperty(node);
        } else {
            // normal widget, creating PropertyObject
            copy_to_pui(node, object);
            PropertyObject *po = new PropertyObject(name, object, node);
            _propertyObjects.push_back(po);
            if (props->getBoolValue("live"))
                _liveObjects.push_back(po);
        }
        

    }

    SGPropertyNode *dest = fgGetNode("/sim/bindings/gui", true);
    vector<SGPropertyNode_ptr> bindings = props->getChildren("binding");
    if (bindings.size() > 0) {
        info->key = props->getIntValue("keynum", -1);
        if (props->hasValue("key"))
            info->key = getKeyCode(props->getStringValue("key", ""));

        for (unsigned int i = 0; i < bindings.size(); i++) {
            unsigned int j = 0;
            SGPropertyNode_ptr binding;
            while (dest->getChild("binding", j))
                j++;

            const char *cmd = bindings[i]->getStringValue("command");
            if (!strcmp(cmd, "nasal"))
                bindings[i]->setStringValue("module", _module.c_str());

            binding = dest->getChild("binding", j, true);
            copyProperties(bindings[i], binding);
            info->bindings.push_back(new SGBinding(binding, globals->get_props()));
        }
        object->setCallback(action_callback);
    }
}

void
FGPUIDialog::setupGroup(puGroup *group, SGPropertyNode *props,
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

void
FGPUIDialog::setColor(puObject *object, SGPropertyNode *props, int which)
{
    string type = props->getName();
    if (type.empty())
        type = "dialog";
    if (type == "textbox" && props->getBoolValue("editable"))
        type += "-editable";

    FGColor c(_gui->getColor("background"));
    c.merge(_gui->getColor(type));
    c.merge(props->getNode("color"));
    if (c.isValid())
        object->setColourScheme(c.red(), c.green(), c.blue(), c.alpha());

    const struct {
        int mask;
        int id;
        const char *name;
        const char *cname;
    } pucol[] = {
        { BACKGROUND, PUCOL_BACKGROUND, "background", "color-background" },
        { FOREGROUND, PUCOL_FOREGROUND, "foreground", "color-foreground" },
        { HIGHLIGHT,  PUCOL_HIGHLIGHT,  "highlight",  "color-highlight" },
        { LABEL,      PUCOL_LABEL,      "label",      "color-label" },
        { LEGEND,     PUCOL_LEGEND,     "legend",     "color-legend" },
        { MISC,       PUCOL_MISC,       "misc",       "color-misc" },
        { EDITFIELD,  PUCOL_EDITFIELD,  "editfield",  "color-editfield" },
    };

    const int numcol = sizeof(pucol) / sizeof(pucol[0]);

    for (int i = 0; i < numcol; i++) {
        bool dirty = false;
        c.clear();
        c.setAlpha(1.0);

        dirty |= c.merge(_gui->getColor(type + '-' + pucol[i].name));
        if (which & pucol[i].mask)
            dirty |= c.merge(props->getNode("color"));

        if ((pucol[i].mask == LABEL) && !c.isValid())
            dirty |= c.merge(_gui->getColor("label"));

        dirty |= c.merge(props->getNode(pucol[i].cname));

        if (c.isValid() && dirty)
            object->setColor(pucol[i].id, c.red(), c.green(), c.blue(), c.alpha());
    }
}


static struct {
    const char *name;
    int key;
} keymap[] = {
    {"backspace", 8},
    {"tab", 9},
    {"return", 13},
    {"enter", 13},
    {"esc", 27},
    {"escape", 27},
    {"space", ' '},
    {"&amp;", '&'},
    {"and", '&'},
    {"&lt;", '<'},
    {"&gt;", '>'},
    {"f1", PU_KEY_F1},
    {"f2", PU_KEY_F2},
    {"f3", PU_KEY_F3},
    {"f4", PU_KEY_F4},
    {"f5", PU_KEY_F5},
    {"f6", PU_KEY_F6},
    {"f7", PU_KEY_F7},
    {"f8", PU_KEY_F8},
    {"f9", PU_KEY_F9},
    {"f10", PU_KEY_F10},
    {"f11", PU_KEY_F11},
    {"f12", PU_KEY_F12},
    {"left", PU_KEY_LEFT},
    {"up", PU_KEY_UP},
    {"right", PU_KEY_RIGHT},
    {"down", PU_KEY_DOWN},
    {"pageup", PU_KEY_PAGE_UP},
    {"pagedn", PU_KEY_PAGE_DOWN},
    {"home", PU_KEY_HOME},
    {"end", PU_KEY_END},
    {"insert", PU_KEY_INSERT},
    {0, -1},
};

int
FGPUIDialog::getKeyCode(const char *str)
{
    enum {
        CTRL = 0x1,
        SHIFT = 0x2,
        ALT = 0x4,
    };

    while (*str == ' ')
        str++;

    char *buf = new char[strlen(str) + 1];
    strcpy(buf, str);
    char *s = buf + strlen(buf);
    while (s > str && s[-1] == ' ')
        s--;
    *s = 0;
    s = buf;

    int mod = 0;
    while (1) {
        if (!strncmp(s, "Ctrl-", 5) || !strncmp(s, "CTRL-", 5))
            s += 5, mod |= CTRL;
        else if (!strncmp(s, "Shift-", 6) || !strncmp(s, "SHIFT-", 6))
            s += 6, mod |= SHIFT;
        else if (!strncmp(s, "Alt-", 4) || !strncmp(s, "ALT-", 4))
            s += 4, mod |= ALT;
        else
            break;
    }

    int key = -1;
    if (strlen(s) == 1 && isascii(*s)) {
        key = *s;
        if (mod & SHIFT)
            key = toupper(key);
        if (mod & CTRL)
            key = toupper(key) - '@';
        /* if (mod & ALT)
            ;   // Alt not propagated to the gui
        */
    } else {
        for (char *t = s; *t; t++)
            *t = tolower(*t);
        for (int i = 0; keymap[i].name; i++) {
            if (!strcmp(s, keymap[i].name)) {
                key = keymap[i].key;
                break;
            }
        }
    }
    delete[] buf;
    return key;
}

voidFGPUIDialog::relayout()
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
  wid.setDefaultFont(_font, int(_font->getPointSize()));

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

  applySize(_object);
  
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

void
FGPUIDialog::applySize(puObject *object)
{
  // compound plib widgets use setUserData() for internal purposes, so refuse
  // to descend into anything that has other bits set than the following
  const int validUserData = PUCLASS_VALUE|PUCLASS_OBJECT|PUCLASS_GROUP|PUCLASS_INTERFACE
          |PUCLASS_FRAME|PUCLASS_TEXT|PUCLASS_BUTTON|PUCLASS_ONESHOT|PUCLASS_INPUT
          |PUCLASS_ARROW|PUCLASS_DIAL|PUCLASS_POPUP;

  int type = object->getType();
  if ((type & PUCLASS_GROUP) && !(type & ~validUserData)) {
    puObject* c = ((puGroup *)object)->getFirstChild();
    for (; c != NULL; c = c->getNextObject()) {
      applySize(c);
    } // of child iteration
  } // of group object case
  
  GUIInfo *info = (GUIInfo *)object->getUserData();
  if (!info)
    return;

  SGPropertyNode *n = info->node;
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

////////////////////////////////////////////////////////////////////////
// Implementation of FGDialog::PropertyObject.
////////////////////////////////////////////////////////////////////////

FGPUIDialog::PropertyObject::PropertyObject(const char *n,
        puObject *o, SGPropertyNode_ptr p) :
    name(n),
    object(o),
    node(p)
{
}




////////////////////////////////////////////////////////////////////////
// Implementation of fgValueList and derived pui widgets
////////////////////////////////////////////////////////////////////////


fgValueList::fgValueList(SGPropertyNode *p) :
    _props(p)
{
    make_list();
}

void
fgValueList::update()
{
    destroy_list();
    make_list();
}

fgValueList::~fgValueList()
{
    destroy_list();
}

void
fgValueList::make_list()
{
  SGPropertyNode_ptr values = _props;
  const char* vname = "value";
  
  if (_props->hasChild("properties")) {
    // dynamic values, read from a property's children
    const char* path = _props->getStringValue("properties");
    values = fgGetNode(path, true);
  }
  
  if (_props->hasChild("property-name")) {
    vname = _props->getStringValue("property-name");
  }
  
  vector<SGPropertyNode_ptr> value_nodes = values->getChildren(vname);
  _list = new char *[value_nodes.size() + 1];
  unsigned int i;
  for (i = 0; i < value_nodes.size(); i++) {
    _list[i] = strdup((char *)value_nodes[i]->getStringValue());
  }
  _list[i] = 0;
}

void
fgValueList::destroy_list()
{
    for (int i = 0; _list[i] != 0; i++)
        if (_list[i])
            free(_list[i]);
    delete[] _list;
}



void
fgList::update()
{
    fgValueList::update();
    int top = getTopItem();
    newList(_list);
    setTopItem(top);
}

void fgComboBox::update()
{
  if (_inHit) {
    return;
  }
  
  std::string curValue(getStringValue());
  fgValueList::update();
  newList(_list);
  int currentItem = puaComboBox::getCurrentItem();

  
// look for the previous value, in the new list
  for (int i = 0; _list[i] != 0; i++) {
    if (_list[i] == curValue) {
    // don't set the current item again; this is important to avoid
    // recursion here if the combo callback ultimately causes another dialog-update
      if (i != currentItem) {
        puaComboBox::setCurrentItem(i);
      }
      
      return;
    }
  } // of list values iteration
  
// cound't find current item, default to first
  if (currentItem != 0) {
    puaComboBox::setCurrentItem(0);
  }
}

int fgComboBox::checkHit(int b, int up, int x, int y)
{
  _inHit = true;
  int r = puaComboBox::checkHit(b, up, x, y);
  _inHit = false;
  return r;
}

void fgComboBox::setSize(int w, int h)
{
  puaComboBox::setSize(w, h);
  recalc_bbox();
}

void fgComboBox::recalc_bbox()
{
// bug-fix for issue #192
// http://code.google.com/p/flightgear-bugs/issues/detail?id=192
// puaComboBox is including the height of its popupMenu in the height
// computation, which breaks layout computations.
// this implementation skips popup-menus
  
  puBox contents;
  contents.empty();
  
  for (puObject *bo = dlist; bo != NULL; bo = bo -> getNextObject()) {
    if (bo->getType() & PUCLASS_POPUPMENU) {
      continue;
    }
    
    contents.extend (bo -> getBBox()) ;
  }
  
  abox.max[0] = abox.min[0] + contents.max[0] ;
  abox.max[1] = abox.min[1] + contents.max[1] ;

  puObject::recalc_bbox () ;

}

// end of FGPUIDialog.cxx
