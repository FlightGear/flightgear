#include "hud.hxx"

#ifdef USE_HUD_TextList
#define textString(x, y, text, digit)  TextString(text, x , y ,digit)
#else
#define textString(x, y, text, digit)  puDrawString(guiFnt, text, x, y)
#endif

FLTFNPTR get_func(const char *name);   // FIXME

gauge_instr::gauge_instr(const SGPropertyNode *node) :
    instr_scale(
            node->getIntValue("x"),
            node->getIntValue("y"),
            node->getIntValue("width"),
            node->getIntValue("height"),
            0 /*load_fn*/,
            node->getIntValue("options"),
            node->getFloatValue("maxValue") - node->getFloatValue("minValue"), // Always shows span?
            node->getFloatValue("maxValue"),
            node->getFloatValue("minValue"),
            node->getFloatValue("disp_scaling"),
            node->getIntValue("major_divs"),
            node->getIntValue("minor_divs"),
            node->getIntValue("modulator"), // "rollover"
            0, /* hud.cxx: static int dp_shoing = 0; */    // FIXME
            node->getBoolValue("working", true))
{
    SG_LOG(SG_INPUT, SG_BULK, "Done reading gauge instrument "
            << node->getStringValue("name", "[unnamed]"));

    set_data_source(get_func(node->getStringValue("loadfn")));
}


// As implemented, draw only correctly draws a horizontal or vertical
// scale. It should contain a variation that permits clock type displays.
// Now is supports "tickless" displays such as control surface indicators.
// This routine should be worked over before using. Current value would be
// fetched and not used if not commented out. Clearly that is intollerable.

void gauge_instr::draw(void)
{
    float marker_xs, marker_xe;
    float marker_ys, marker_ye;
    int text_x, text_y;
    int width, height, bottom_4;
    int lenstr;
    int i;
    char TextScale[80];
    bool condition;
    int disp_val = 0;
    float vmin = min_val();
    float vmax = max_val();
    POINT mid_scr = get_centroid();
    float cur_value = get_value();
    RECT  scrn_rect = get_location();
    UINT options = get_options();

    width = scrn_rect.left + scrn_rect.right;
    height = scrn_rect.top + scrn_rect.bottom;
    bottom_4 = scrn_rect.bottom / 4;

    // Draw the basic markings for the scale...
    if (huds_vert(options)) { // Vertical scale
        // Bottom tick bar
        drawOneLine(scrn_rect.left, scrn_rect.top, width, scrn_rect.top);

        // Top tick bar
        drawOneLine( scrn_rect.left, height, width, height);

        marker_xs = scrn_rect.left;
        marker_xe = width;

        if (huds_left(options)) { // Read left, so line down right side
            drawOneLine(width, scrn_rect.top, width, height);
            marker_xs  = marker_xe - scrn_rect.right / 3.0;   // Adjust tick
        }

        if (huds_right(options)) {     // Read  right, so down left sides
            drawOneLine(scrn_rect.left, scrn_rect.top, scrn_rect.left, height);
            marker_xe = scrn_rect.left + scrn_rect.right / 3.0;   // Adjust tick
        }

        // At this point marker x_start and x_end values are transposed.
        // To keep this from confusing things they are now interchanged.
        if (huds_both(options)) {
            marker_ye = marker_xs;
            marker_xs = marker_xe;
            marker_xe = marker_ye;
        }

        // Work through from bottom to top of scale. Calculating where to put
        // minor and major ticks.

        if (!huds_noticks(options)) {    // If not no ticks...:)
            // Calculate x marker offsets
            int last = (int)vmax + 1;    // float_to_int(vmax)+1;
            i = (int)vmin; //float_to_int(vmin);

            for (; i < last; i++) {
                // Calculate the location of this tick
                marker_ys = scrn_rect.top + (i - vmin) * factor()/* +.5f*/;

                // We compute marker_ys even though we don't know if we will use
                // either major or minor divisions. Simpler.

                if (div_min()) {                  // Minor tick marks
                    if (!(i % (int)div_min())) {
                        if (huds_left(options) && huds_right(options)) {
                            drawOneLine(scrn_rect.left, marker_ys, marker_xs - 3, marker_ys);
                            drawOneLine(marker_xe + 3, marker_ys, width, marker_ys);

                        } else if (huds_left(options)) {
                            drawOneLine(marker_xs + 3, marker_ys, marker_xe, marker_ys);
                        } else {
                            drawOneLine(marker_xs, marker_ys, marker_xe - 3, marker_ys);
                        }
                    }
                }

                // Now we work on the major divisions. Since these are also labeled
                // and no labels are drawn otherwise, we label inside this if
                // statement.

                if (div_max()) {                  // Major tick mark
                    if (!(i % (int)div_max())) {
                        if (huds_left(options) && huds_right(options)) {
                            drawOneLine(scrn_rect.left, marker_ys, marker_xs, marker_ys);
                            drawOneLine(marker_xe, marker_ys, width, marker_ys);
                        } else {
                            drawOneLine(marker_xs, marker_ys, marker_xe, marker_ys);
                        }

                        if (!huds_notext(options)) {
                            disp_val = i;
                            sprintf(TextScale, "%d",
                                    float_to_int(disp_val * data_scaling()/*+.5*/));

                            lenstr = getStringWidth(TextScale);

                            if (huds_left(options) && huds_right(options)) {
                                text_x = mid_scr.x -  lenstr/2 ;

                            } else if (huds_left(options)) {
                                text_x = float_to_int(marker_xs - lenstr);
                            } else {
                                text_x = float_to_int(marker_xe - lenstr);
                            }
                            // Now we know where to put the text.
                            text_y = float_to_int(marker_ys);
                            textString(text_x, text_y, TextScale, 0);
                        }
                    }
                }
            }
        }

        // Now that the scale is drawn, we draw in the pointer(s). Since labels
        // have been drawn, text_x and text_y may be recycled. This is used
        // with the marker start stops to produce a pointer for each side reading

        text_y = scrn_rect.top + float_to_int((cur_value - vmin) * factor() /*+.5f*/);
        //    text_x = marker_xs - scrn_rect.left;

        if (huds_right(options)) {
            glBegin(GL_LINE_STRIP);
            glVertex2f(scrn_rect.left, text_y + 5);
            glVertex2f(float_to_int(marker_xe), text_y);
            glVertex2f(scrn_rect.left, text_y - 5);
            glEnd();
        }
        if (huds_left(options)) {
            glBegin(GL_LINE_STRIP);
            glVertex2f(width, text_y + 5);
            glVertex2f(float_to_int(marker_xs), text_y);
            glVertex2f(width, text_y - 5);
            glEnd();
        }
        // End if VERTICAL SCALE TYPE

    } else {                             // Horizontal scale by default
        // left tick bar
        drawOneLine(scrn_rect.left, scrn_rect.top, scrn_rect.left, height);

        // right tick bar
        drawOneLine(width, scrn_rect.top, width, height );

        marker_ys = scrn_rect.top;                       // Starting point for
        marker_ye = height;                              // tick y location calcs
        marker_xs = scrn_rect.left + (cur_value - vmin) * factor() /*+ .5f*/;

        if (huds_top(options)) {
            // Bottom box line
            drawOneLine(scrn_rect.left, scrn_rect.top, width, scrn_rect.top);

            marker_ye = scrn_rect.top + scrn_rect.bottom / 2.0;   // Tick point adjust
            // Bottom arrow
            glBegin(GL_LINE_STRIP);
            glVertex2f(marker_xs - bottom_4, scrn_rect.top);
            glVertex2f(marker_xs, marker_ye);
            glVertex2f(marker_xs + bottom_4, scrn_rect.top);
            glEnd();
        }

        if (huds_bottom(options)) {
            // Top box line
            drawOneLine(scrn_rect.left, height, width, height);
            // Tick point adjust
            marker_ys = height - scrn_rect.bottom  / 2.0;

            // Top arrow
            glBegin(GL_LINE_STRIP);
            glVertex2f(marker_xs + bottom_4, height);
            glVertex2f(marker_xs, marker_ys );
            glVertex2f(marker_xs - bottom_4, height);
            glEnd();
        }


        int last = (int)vmax + 1; //float_to_int(vmax)+1;
        i = (int)vmin; //float_to_int(vmin);
        for (; i <last ; i++) {
            condition = true;
            if (!modulo() && i < min_val())
                    condition = false;

            if (condition) {
                marker_xs = scrn_rect.left + (i - vmin) * factor()/* +.5f*/;
                //        marker_xs = scrn_rect.left + (int)((i - vmin) * factor() + .5f);
                if (div_min()) {
                    if (!(i % (int)div_min())) {
                        // draw in ticks only if they aren't too close to the edge.
                        if (((marker_xs + 5) > scrn_rect.left)
                               || ((marker_xs - 5) < (width))) {

                            if (huds_both(options)) {
                                drawOneLine(marker_xs, scrn_rect.top, marker_xs, marker_ys - 4);
                                drawOneLine(marker_xs, marker_ye + 4, marker_xs, height);

                            } else if (huds_top(options)) {
                                drawOneLine(marker_xs, marker_ys, marker_xs, marker_ye - 4);
                            } else {
                                drawOneLine(marker_xs, marker_ys + 4, marker_xs, marker_ye);
                            }
                        }
                    }
                }

                if (div_max()) {
                    if (!(i % (int)div_max())) {
                        if (modulo()) {
                            if (disp_val < 0) {
                                while (disp_val < 0)
                                    disp_val += modulo();
                            }
                            disp_val = i % (int)modulo();
                        } else {
                            disp_val = i;
                        }
                        sprintf(TextScale, "%d",
                                float_to_int(disp_val * data_scaling()/* +.5*/));
                        lenstr = getStringWidth(TextScale);

                        // Draw major ticks and text only if far enough from the edge.
                        if (((marker_xs - 10) > scrn_rect.left)
                                && ((marker_xs + 10) < width)) {
                            if (huds_both(options)) {
                                drawOneLine(marker_xs, scrn_rect.top, marker_xs, marker_ys);
                                drawOneLine(marker_xs, marker_ye, marker_xs, height);

                                if (!huds_notext(options))
                                    textString(marker_xs - lenstr, marker_ys + 4, TextScale, 0);

                            } else {
                                drawOneLine(marker_xs, marker_ys, marker_xs, marker_ye);

                                if (!huds_notext(options)) {
                                    if (huds_top(options))
                                        textString(marker_xs - lenstr, height - 10, TextScale, 0);
                                    else
                                        textString(marker_xs - lenstr, scrn_rect.top, TextScale, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


