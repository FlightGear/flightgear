
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
    pre_str(node->getStringValue("pre_label_string")),
    post_str(node->getStringValue("post_label_string")),
    fontSize(fgGetInt("/sim/startup/xsize") > 1000 ? HUD_FONT_LARGE : HUD_FONT_SMALL),	// FIXME
    blink(node->getIntValue("blinking")),
    lat(node->getBoolValue("latitude", false)),
    lon(node->getBoolValue("longitude", false)),
    lbox(node->getBoolValue("label_box", false)),
    lon_node(fgGetNode("/position/longitude-string", true)),
    lat_node(fgGetNode("/position/latitude-string", true))
{
    SG_LOG(SG_INPUT, SG_INFO, "Done reading instr_label instrument "
            << node->getStringValue("name", "[unnamed]"));

    set_data_source(get_func(node->getStringValue("data_source")));

    int just = node->getIntValue("justification");
    if (just == 0)
        justify = LEFT_JUST;
    else if (just == 1)
        justify = CENTER_JUST;
    else if (just == 2)
        justify = RIGHT_JUST;

    if (!strcmp(pre_str, "NULL"))
        pre_str = NULL;
    else if (!strcmp(pre_str, "blank"))
        pre_str = " ";

    const char *units = strcmp(fgGetString("/sim/startup/units"), "feet") ? " m" : " ft";  // FIXME

    if (!strcmp(post_str, "blank"))
        post_str = " ";
    else if (!strcmp(post_str, "NULL"))
        post_str = NULL;
    else if (!strcmp(post_str, "units"))
        post_str = units;


    if (pre_str != NULL) {
        if (post_str != NULL)
            sprintf(format_buffer, "%s%s%s", pre_str, pformat, post_str);
        else
            sprintf(format_buffer, "%s%s", pre_str, pformat);

    } else if (post_str != NULL) {
            sprintf(format_buffer, "%s%s", pformat, post_str);
    } else {
            strcpy(format_buffer, pformat);			// FIXME
    }
}


void instr_label::draw(void)
{
    char label_buffer[80];
    int posincr;
    int lenstr;
    RECT scrn_rect = get_location();

    if (data_available()) {
        if (lat)
            snprintf(label_buffer, 80, format_buffer, lat_node->getStringValue());
        else if (lon)
            snprintf(label_buffer, 80, format_buffer, lon_node->getStringValue());
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
            sprintf(label_buffer, format_buffer, get_value() * data_scaling());
        }

    } else {
        sprintf(label_buffer, format_buffer);
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


