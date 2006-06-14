#include <Main/fg_props.hxx>

#include "hud.hxx"

#ifdef USE_HUD_TextList
#define textString(x, y, text, digit)  TextString(text, x , y ,digit)
#else
#define textString(x, y, text, digit)  puDrawString(guiFnt, text, x, y)
#endif

// FIXME
extern float get_aux1(), get_aux2(), get_aux3(), get_aux4(), get_aux5(), get_aux6();
extern float get_aux7(), get_aux8(), get_aux9(), get_aux10(), get_aux11(), get_aux12();
extern float get_aux13(), get_aux14(), get_aux15(), get_aux16(), get_aux17(), get_aux18();
extern float get_Ax(), get_speed(), get_mach(), get_altitude(), get_agl(), get_frame_rate();
extern float get_heading(), get_fov(), get_vfc_tris_culled(), get_vfc_tris_drawn(), get_aoa();
extern float get_latitude(), get_anzg(), get_longitude(), get_throttleval();

instr_label::instr_label(const SGPropertyNode *node) :
    instr_item(
            node->getIntValue("x"),
            node->getIntValue("y"),
            node->getIntValue("width"),
            node->getIntValue("height"),
            NULL /* node->getStringValue("data_source") */,	// FIXME
            node->getFloatValue("scale_data"),
            node->getIntValue("options"),
            node->getBoolValue("working"),
            node->getIntValue("digits")),
    pformat(node->getStringValue("label_format")),
    pre_str(node->getStringValue("pre_label_string")),
    post_str(node->getStringValue("post_label_string")),
    fontSize(fgGetInt("/sim/startup/xsize") > 1000 ? HUD_FONT_LARGE : HUD_FONT_SMALL),	// FIXME
    blink(node->getIntValue("blinking")),
    lat(node->getBoolValue("latitude", false)),
    lon(node->getBoolValue("longitude", false)),
    lbox(node->getBoolValue("label_box", false))
{
    SG_LOG(SG_INPUT, SG_INFO, "Done reading instr_label instrument "
            << node->getStringValue("name", "[none]"));

    string loadfn = node->getStringValue("data_source");	// FIXME
    float (*load_fn)(void);
#ifdef ENABLE_SP_FMDS
    if (loadfn == "aux1")
        load_fn = get_aux1;
    else if (loadfn == "aux2")
        load_fn = get_aux2;
    else if (loadfn == "aux3")
        load_fn = get_aux3;
    else if (loadfn == "aux4")
        load_fn = get_aux4;
    else if (loadfn == "aux5")
        load_fn = get_aux5;
    else if (loadfn == "aux6")
        load_fn = get_aux6;
    else if (loadfn == "aux7")
        load_fn = get_aux7;
    else if (loadfn == "aux8")
        load_fn = get_aux8;
    else if (loadfn == "aux9")
        load_fn = get_aux9;
    else if (loadfn == "aux10")
        load_fn = get_aux10;
    else if (loadfn == "aux11")
        load_fn = get_aux11;
    else if (loadfn == "aux12")
        load_fn = get_aux12;
    else if (loadfn == "aux13")
        load_fn = get_aux13;
    else if (loadfn == "aux14")
        load_fn = get_aux14;
    else if (loadfn == "aux15")
        load_fn = get_aux15;
    else if (loadfn == "aux16")
        load_fn = get_aux16;
    else if (loadfn == "aux17")
        load_fn = get_aux17;
    else if (loadfn == "aux18")
        load_fn = get_aux18;
    else
#endif
    if (loadfn == "ax")
        load_fn = get_Ax;
    else if (loadfn == "speed")
        load_fn = get_speed;
    else if (loadfn == "mach")
        load_fn = get_mach;
    else if (loadfn == "altitude")
        load_fn = get_altitude;
    else if (loadfn == "agl")
        load_fn = get_agl;
    else if (loadfn == "framerate")
        load_fn = get_frame_rate;
    else if (loadfn == "heading")
        load_fn = get_heading;
    else if (loadfn == "fov")
        load_fn = get_fov;
    else if (loadfn == "vfc_tris_culled")
        load_fn = get_vfc_tris_culled;
    else if (loadfn == "vfc_tris_drawn")
        load_fn = get_vfc_tris_drawn;
    else if (loadfn == "aoa")
        load_fn = get_aoa;
    else if (loadfn == "latitude")
        load_fn = get_latitude;
    else if (loadfn == "anzg")
        load_fn = get_anzg;
    else if (loadfn == "longitude")
        load_fn = get_longitude;
    else if (loadfn =="throttleval")
        load_fn = get_throttleval;
    else
        load_fn = 0;

    set_data_source(load_fn);

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


instr_label::~instr_label()
{
}


void instr_label::draw(void)
{
    char label_buffer[80];
    int posincr;
    int lenstr;
    RECT scrn_rect = get_location();

    if (data_available()) {
        if (lat)
            sprintf(label_buffer, format_buffer, coord_format_lat(get_value()));
        else if (lon)
            sprintf(label_buffer, format_buffer, coord_format_lon(get_value()));
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


