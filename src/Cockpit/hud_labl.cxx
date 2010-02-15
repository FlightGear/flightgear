
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "hud.hxx"

#ifdef USE_HUD_TextList
#define textString(x, y, text, digit)  TextString(text, x , y ,digit)
#else
#define textString(x, y, text, digit)  puDrawString(guiFnt, text, x, y)
#endif

FLTFNPTR get_func(const char *name);   // FIXME

instr_label::instr_label(const SGPropertyNode *node) :
    instr_item(
            node->getIntValue("x"),
            node->getIntValue("y"),
            node->getIntValue("width"),
            node->getIntValue("height"),
            NULL /* node->getStringValue("data_source") */,	// FIXME
            node->getFloatValue("scale_data"),
            node->getIntValue("options"),
            node->getBoolValue("working", true),
            node->getIntValue("digits")),
    pformat(node->getStringValue("label_format")),
    fontSize(fgGetInt("/sim/startup/xsize") > 1000 ? HUD_FONT_LARGE : HUD_FONT_SMALL),	// FIXME
    blink(node->getIntValue("blinking")),
    lat(node->getBoolValue("latitude", false)),
    lon(node->getBoolValue("longitude", false)),
    lbox(node->getBoolValue("label_box", false)),
    lon_node(fgGetNode("/position/longitude-string", true)),
    lat_node(fgGetNode("/position/latitude-string", true))
{
    SG_LOG(SG_INPUT, SG_BULK, "Done reading instr_label instrument "
            << node->getStringValue("name", "[unnamed]"));

    set_data_source(get_func(node->getStringValue("data_source")));

    int just = node->getIntValue("justification");
    if (just == 0)
        justify = LEFT_JUST;
    else if (just == 1)
        justify = CENTER_JUST;
    else if (just == 2)
        justify = RIGHT_JUST;

    string pre_str(node->getStringValue("pre_label_string"));
    if (pre_str== "NULL")
        pre_str.clear();
    else if (pre_str == "blank")
        pre_str = " ";

    const char *units = strcmp(fgGetString("/sim/startup/units"), "feet") ? " m" : " ft";  // FIXME

    string post_str(node->getStringValue("post_label_string"));
    if (post_str== "NULL")
        post_str.clear();
    else if (post_str == "blank")
        post_str = " ";
    else if (post_str == "units")
        post_str = units;

    format_buffer = pre_str + pformat;
    format_buffer += post_str;
}


void instr_label::draw(void)
{
    char label_buffer[80];
    int posincr;
    int lenstr;
    RECT scrn_rect = get_location();

    memset( label_buffer, 0, sizeof( label_buffer));
    if (data_available()) {
        if (lat)
            snprintf(label_buffer, sizeof( label_buffer)-1, format_buffer.c_str(), lat_node->getStringValue());
        else if (lon)
            snprintf(label_buffer, sizeof( label_buffer)-1, format_buffer.c_str(), lon_node->getStringValue());
        else {
            if (lbox) {// Box for label
                float x = scrn_rect.left;
                float y = scrn_rect.top;
                float w = scrn_rect.right;
                float h = HUD_TextSize;

                glPushMatrix();
                glLoadIdentity();

                glBegin(GL_LINES);
                glVertex2f(x - 2.0,  y - 2.0);
                glVertex2f(x + w + 2.0, y - 2.0);
                glVertex2f(x + w + 2.0, y + h + 2.0);
                glVertex2f(x - 2.0,  y + h + 2.0);
                glEnd();

                glEnable(GL_LINE_STIPPLE);
                glLineStipple(1, 0xAAAA);

                glBegin(GL_LINES);
                glVertex2f(x + w + 2.0, y - 2.0);
                glVertex2f(x + w + 2.0, y + h + 2.0);
                glVertex2f(x - 2.0, y + h + 2.0);
                glVertex2f(x - 2.0, y - 2.0);
                glEnd();

                glDisable(GL_LINE_STIPPLE);
                glPopMatrix();
            }
            snprintf(label_buffer, sizeof(label_buffer)-1, format_buffer.c_str(), get_value() * data_scaling());
        }

    } else {
        snprintf(label_buffer, sizeof( label_buffer) -1, format_buffer.c_str());
    }

    lenstr = getStringWidth(label_buffer);

#ifdef DEBUGHUD
    fgPrintf( SG_COCKPIT, SG_DEBUG, format_buffer);
    fgPrintf( SG_COCKPIT, SG_DEBUG, "\n");
    fgPrintf( SG_COCKPIT, SG_DEBUG, label_buffer);
    fgPrintf( SG_COCKPIT, SG_DEBUG, "\n");
#endif
    lenstr = strlen(label_buffer);

    if (justify == RIGHT_JUST)
        posincr = scrn_rect.right - lenstr;
    else if (justify == CENTER_JUST)
        posincr = get_span() - (lenstr / 2); //  -lenstr*4;
    else // justify == LEFT_JUST
        posincr = 0;

    if (fontSize == HUD_FONT_SMALL) {
        textString(scrn_rect.left + posincr, scrn_rect.top,
                label_buffer, get_digits());

    } else if (fontSize == HUD_FONT_LARGE) {
        textString(scrn_rect.left + posincr, scrn_rect.top,
                label_buffer, get_digits());
    }
}
