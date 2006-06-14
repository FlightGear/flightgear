
#include "hud.hxx"


#ifdef USE_HUD_TextList
#define textString(x, y, text, digit)  TextString(text, x , y ,digit)
#else
#define textString(x, y, text, digit)  puDrawString(guiFnt, text, x, y)
#endif

//======================= Top of instr_label class =========================
instr_label::instr_label(
        int           x,
        int           y,
        UINT          width,
        UINT          height,
        FLTFNPTR      data_source,
        const char   *label_format,
        const char   *pre_label_string,
        const char   *post_label_string,
        float         scale_data,
        UINT          options,
        fgLabelJust   justification,
        int           font_size,
        int           blinking,
        bool          latitude,
        bool          longitude,
        bool          label_box,
        bool          working,
        int           digit) :
    instr_item( x, y, width, height,
                data_source,scale_data,options, working, digit),
    pformat  ( label_format      ),
    pre_str  ( pre_label_string  ),
    post_str ( post_label_string ),
    justify  ( justification     ),
    fontSize ( font_size         ),
    blink    ( blinking          ),
    lat      ( latitude          ),
    lon      ( longitude         ),
    lbox     ( label_box         )
{
    if (pre_str != NULL) {
        if (post_str != NULL )
            sprintf( format_buffer, "%s%s%s", pre_str, pformat, post_str );
        else
            sprintf( format_buffer, "%s%s",   pre_str, pformat );

    } else if (post_str != NULL) {
            sprintf( format_buffer, "%s%s",   pformat, post_str );
    } // else do nothing if both pre and post strings are nulls. Interesting.

}


instr_label::~instr_label()
{
}


// Copy constructor
instr_label::instr_label(const instr_label & image) :
    instr_item((const instr_item &)image),
    pformat    ( image.pformat  ),
    pre_str    ( image.pre_str  ),
    post_str   ( image.post_str ),
    blink      ( image.blink    ),
    lat        ( image.lat      ),
    lon        ( image.lon      ),
    lbox       ( image.lbox     )

{
    if (pre_str != NULL) {
        if (post_str != NULL)
            sprintf( format_buffer, "%s%s%s", pre_str, pformat, post_str );
        else
            sprintf( format_buffer, "%s%s",   pre_str, pformat );

    } else if (post_str != NULL) {
            sprintf( format_buffer, "%s%s",   pformat, post_str );
    } // else do nothing if both pre and post strings are nulls. Interesting.

}


//
// draw                    Draws a label anywhere in the HUD
//
//
void instr_label::draw( void )
{
    char label_buffer[80];
    int posincr;
    int lenstr;
    RECT  scrn_rect = get_location();

    if (data_available()) {
        if (lat)
            sprintf( label_buffer, format_buffer, coord_format_lat(get_value()) );
        else if (lon)
            sprintf( label_buffer, format_buffer, coord_format_lon(get_value()) );
        else {
            if (lbox) {// Box for label
                float x = scrn_rect.left;
                float y = scrn_rect.top;
                float w = scrn_rect.right;
                float h = HUD_TextSize;

                glPushMatrix();
                glLoadIdentity();

                glBegin(GL_LINES);
                glVertex2f( x - 2.0,  y - 2.0);
                glVertex2f( x + w + 2.0, y - 2.0);
                glVertex2f( x + w + 2.0, y + h + 2.0);
                glVertex2f( x - 2.0,  y + h + 2.0);
                glEnd();

                glEnable(GL_LINE_STIPPLE);
                glLineStipple( 1, 0xAAAA );

                glBegin(GL_LINES);
                glVertex2f( x + w + 2.0, y - 2.0);
                glVertex2f( x + w + 2.0, y + h + 2.0);
                glVertex2f( x - 2.0,  y + h + 2.0);
                glVertex2f( x - 2.0,  y - 2.0);
                glEnd();

                glDisable(GL_LINE_STIPPLE);
                glPopMatrix();
            }
            sprintf( label_buffer, format_buffer, get_value()*data_scaling() );
        }

    } else {
//    sprintf( label_buffer, format_buffer );
    }

    lenstr = getStringWidth( label_buffer );


#ifdef DEBUGHUD
    fgPrintf( SG_COCKPIT, SG_DEBUG,  format_buffer );
    fgPrintf( SG_COCKPIT, SG_DEBUG,  "\n" );
    fgPrintf( SG_COCKPIT, SG_DEBUG, label_buffer );
    fgPrintf( SG_COCKPIT, SG_DEBUG, "\n" );
#endif
    lenstr = strlen( label_buffer );

    if (justify == RIGHT_JUST) {
        posincr = scrn_rect.right - lenstr;
    } else if (justify == CENTER_JUST) {
        posincr = get_span() - (lenstr/2); //  -lenstr*4;
    } else {
        //  justify == LEFT_JUST
        posincr = 0;  // 0;
    }

    if (fontSize == HUD_FONT_SMALL) {
        textString( scrn_rect.left + posincr, scrn_rect.top,
                    label_buffer, get_digits());

    } else if (fontSize == HUD_FONT_LARGE) {
        textString( scrn_rect.left + posincr, scrn_rect.top,
                    label_buffer, get_digits());
    }
}

