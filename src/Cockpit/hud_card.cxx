
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <plib/sg.h>
#include "hud.hxx"

#ifdef USE_HUD_TextList
#define textString(x, y, text, digit)  TextString(text, x , y ,digit)
#else
#define textString(x, y, text, digit)  puDrawString(guiFnt, text, x, y)
#endif

//========== Top of hud_card class member definitions =============
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


hud_card::hud_card(
          int       x,
          int       y,
          UINT      width,
          UINT      height,
          FLTFNPTR  data_source,
          UINT      options,
          float     max_value, // 360
          float     min_value, // 0
          float     disp_scaling,
          UINT      major_divs,
          UINT      minor_divs,
          UINT      modulus,  // 360
          int       dp_showing,
          float     value_span,
          string    card_type,
          bool      tick_bottom,
          bool      tick_top,
          bool      tick_right,
          bool      tick_left,
          bool      cap_bottom,
          bool      cap_top,
          bool      cap_right,
          bool      cap_left,
          float     mark_offset,
          bool      pointer_enable,
          string    type_pointer,
          string    type_tick,//hud
          string    length_tick,//hud
          bool      working,
          float     rad,
          int       divs,
          int       zooms) :
    instr_scale(x,y,width,height,
                 data_source, options,
                 value_span,
                 max_value, min_value, disp_scaling,
                 major_divs, minor_divs, modulus,
                 working),
    val_span         (value_span),
    type             (card_type),
    draw_tick_bottom (tick_bottom),
    draw_tick_top    (tick_top),
    draw_tick_right  (tick_right),
    draw_tick_left   (tick_left),
    draw_cap_bottom  (cap_bottom),
    draw_cap_top     (cap_top),
    draw_cap_right   (cap_right),
    draw_cap_left    (cap_left),
    marker_offset    (mark_offset),
    pointer          (pointer_enable),
    pointer_type     (type_pointer),
    tick_type        (type_tick), //hud
    tick_length      (length_tick), //hud
    Maj_div          (major_divs),
    Min_div          (minor_divs)

{
    half_width_units = range_to_show() / 2.0;
    radius = rad;
    maxValue = max_value;
    minValue = min_value;
    divisions = divs;
    zoom = zooms;

    //  UINT options     = get_options();
    //  huds_both = (options & HUDS_BOTH) == HUDS_BOTH;
    //  huds_right = options & HUDS_RIGHT;
    //  huds_left = options & HUDS_LEFT;
    //  huds_vert = options & HUDS_VERT;
    //  huds_notext = options & HUDS_NOTEXT;
    //  huds_top = options & HUDS_TOP;
    //  huds_bottom = options & HUDS_BOTTOM;
}


hud_card::~hud_card() { }


hud_card::hud_card(const hud_card & image) :
    instr_scale((const instr_scale &) image),
    val_span(image.val_span),
    type(image.type),
    half_width_units (image.half_width_units),
    draw_tick_bottom (image.draw_tick_bottom),
    draw_tick_top (image.draw_tick_top),
    draw_tick_right (image.draw_tick_right),
    draw_tick_left (image.draw_tick_left),
    draw_cap_bottom (image.draw_cap_bottom),
    draw_cap_top (image.draw_cap_top),
    draw_cap_right (image.draw_cap_right),
    draw_cap_left (image.draw_cap_left),
    marker_offset (image.marker_offset),
    pointer (image.pointer),
    pointer_type (image.pointer_type),
    tick_type(image.tick_type), //hud
    tick_length(image.tick_length), //hud
    Maj_div(image.Maj_div),
    Min_div(image.Min_div)

{
    //  UINT options     = get_options();
    //  huds_both = (options & HUDS_BOTH) == HUDS_BOTH;
    //  huds_right = options & HUDS_RIGHT;
    //  huds_left = options & HUDS_LEFT;
    //  huds_vert = options & HUDS_VERT;
    //  huds_notext = options & HUDS_NOTEXT;
    //  huds_top = options & HUDS_TOP;
    //  huds_bottom = options & HUDS_BOTTOM;
}


hud_card & hud_card::operator=(const hud_card & rhs)
{
    if (!(this == &rhs)) {
        instr_scale::operator = (rhs);
        val_span = rhs.val_span;
        half_width_units = rhs.half_width_units;
        draw_tick_bottom = rhs.draw_tick_bottom;
        draw_tick_top    = rhs.draw_tick_top;
        draw_tick_right  = rhs.draw_tick_right;
        draw_tick_left   = rhs.draw_tick_left;
        draw_cap_bottom  = rhs.draw_cap_bottom;
        draw_cap_top     = rhs.draw_cap_top;
        draw_cap_right   = rhs.draw_cap_right;
        draw_cap_left    = rhs.draw_cap_left;
        marker_offset    = rhs.marker_offset;
        type             = rhs.type;
        pointer          = rhs.pointer;
        pointer_type     = rhs.pointer_type;
        tick_type        = rhs.tick_type;
        tick_length      = rhs.tick_length;
        Maj_div          = rhs.Maj_div;
        Min_div          = rhs.Min_div;
    }

    return *this;
}


void hud_card::draw(void) //  (HUD_scale * pscale)
{
    float vmin = 0.0, vmax = 0.0;
    float marker_xs;
    float marker_xe;
    float marker_ys;
    float marker_ye;
    int text_x = 0, text_y = 0;
    int lenstr;
    int height, width;
    int i, last;
    char TextScale[80];
    bool condition;
    int disp_val = 0;
    int oddtype, k; //odd or even values for ticks

    POINT mid_scr    = get_centroid();
    float cur_value  = get_value();

    if (!((int)maxValue%2))
        oddtype = 0; //draw ticks at even values
    else
        oddtype = 1; //draw ticks at odd values

    RECT scrn_rect = get_location();
    UINT options = get_options();

    height = scrn_rect.top + scrn_rect.bottom;
    width = scrn_rect.left + scrn_rect.right;

    // if type=gauge then display dial
    if (type == "gauge") {
        float x,y;
        float i;
        y = (float)(scrn_rect.top);
        x = (float)(scrn_rect.left);
        glEnable(GL_POINT_SMOOTH);
        glPointSize(3.0);

        float incr = 360.0 / divisions;
        for (i = 0.0; i < 360.0; i += incr) {
            float i1 = i * SGD_DEGREES_TO_RADIANS;
            float x1 = x + radius*cos(i1);
            float y1 = y + radius*sin(i1);

            glBegin(GL_POINTS);
            glVertex2f(x1, y1);
            glEnd();
        }
        glPointSize(1.0);
        glDisable(GL_POINT_SMOOTH);


        if (data_available()) {
            float offset = 90.0 * SGD_DEGREES_TO_RADIANS;
            float r1 = 10.0; //size of carrot
            float theta = get_value();

            float theta1 = -theta*SGD_DEGREES_TO_RADIANS+offset;
            float x1 = x+radius*cos(theta1);
            float y1 = y+radius*sin(theta1);
            float x2 = x1-r1*cos(theta1-30.0*SGD_DEGREES_TO_RADIANS);
            float y2 = y1-r1*sin(theta1-30.0*SGD_DEGREES_TO_RADIANS);
            float x3 = x1-r1*cos(theta1+30.0*SGD_DEGREES_TO_RADIANS);
            float y3 = y1-r1*sin(theta1+30.0*SGD_DEGREES_TO_RADIANS);

            // draw carrot
            drawOneLine(x1,y1,x2,y2);
            drawOneLine(x1,y1,x3,y3);
            sprintf(TextScale,"%3.1f\n",theta);

            // draw value
            int l = abs((int)theta);
            if (l) {
                if (l < 10)
                    textString(x, y, TextScale, 0);
                else if (l < 100)
                    textString(x-1.0, y, TextScale, 0);
                else if (l<360)
                    textString(x-2.0, y, TextScale, 0);
            }
        } //end type=gauge

    } else {
        // if its not explicitly a gauge default to tape
        if (pointer) {
            if (pointer_type=="moving") {
                vmin = minValue;
                vmax = maxValue;

            } else {
                // default to fixed
                vmin   = cur_value - half_width_units; // width units == needle travel
                vmax   = cur_value + half_width_units; // or picture unit span.
                text_x = mid_scr.x;
                text_y = mid_scr.y;
            }

        } else {
            vmin   = cur_value - half_width_units; // width units == needle travel
            vmax   = cur_value + half_width_units; // or picture unit span.
            text_x = mid_scr.x;
            text_y = mid_scr.y;
        }

        // Draw the basic markings for the scale...

        if (huds_vert(options)) { // Vertical scale
            if (draw_tick_bottom) {
                drawOneLine(scrn_rect.left,     // Bottom tick bar
                            scrn_rect.top,
                            width,
                            scrn_rect.top);
            } // endif draw_tick_bottom

            if (draw_tick_top) {
                drawOneLine(scrn_rect.left,    // Top tick bar
                            height,
                            width,
                            height);
            } // endif draw_tick_top

            marker_xs = scrn_rect.left;  // x start
            marker_xe = width;           // x extent
            marker_ye = height;

            //    glBegin(GL_LINES);

            // Bottom tick bar
            //    glVertex2f(marker_xs, scrn_rect.top);
            //    glVertex2f(marker_xe, scrn_rect.top);

            // Top tick bar
            //    glVertex2f(marker_xs, marker_ye);
            //    glVertex2f(marker_xe, marker_ye);
            //    glEnd();


            // We do not use else in the following so that combining the
            // two options produces a "caged" display with double
            // carrots. The same is done for horizontal card indicators.

            // begin vertical/left
            //First draw capping lines and pointers
            if (huds_left(options)) {    // Calculate x marker offset

                if (draw_cap_right) {
                    drawOneLine(marker_xe, scrn_rect.top,
                                 marker_xe, marker_ye); // Cap right side
                } //endif cap_right

                marker_xs  = marker_xe - scrn_rect.right / 3;   // Adjust tick xs

                // drawOneLine(marker_xs, mid_scr.y,
                //              marker_xe, mid_scr.y + scrn_rect.right / 6);
                // drawOneLine(marker_xs, mid_scr.y,
                //              marker_xe, mid_scr.y - scrn_rect.right / 6);

                // draw pointer
                if (pointer) {
                    if (pointer_type=="moving") {
                        if (zoom == 0) {
                            //Code for Moving Type Pointer
                            float ycentre, ypoint,xpoint;
                            int range,wth;
                            if (cur_value > maxValue)
                                cur_value = maxValue;
                            if (cur_value < minValue)
                                cur_value = minValue;

                            if (minValue >= 0.0)
                                ycentre = scrn_rect.top;
                            else if (maxValue + minValue == 0.0)
                                ycentre = mid_scr.y;
                            else
                                if (oddtype == 1)
                                    ycentre = scrn_rect.top + (1.0-minValue)*scrn_rect.bottom/(maxValue-minValue);
                                else
                                    ycentre = scrn_rect.top + minValue*scrn_rect.bottom/(maxValue-minValue);

                            range = scrn_rect.bottom;
                            wth   = scrn_rect.left + scrn_rect.right;

                            if (oddtype == 1)
                                ypoint = ycentre + ((cur_value-1.0) * range / val_span);
                            else
                                ypoint = ycentre + (cur_value * range / val_span);

                            xpoint = wth + marker_offset;
                            drawOneLine(xpoint,ycentre,xpoint,ypoint);
                            drawOneLine(xpoint,ypoint,xpoint-marker_offset,ypoint);
                            drawOneLine(xpoint-marker_offset,ypoint,xpoint-5.0,ypoint+5.0);
                            drawOneLine(xpoint-marker_offset,ypoint,xpoint-5.0,ypoint-5.0);
                        } //zoom=0

                    } else {
                        // default to fixed
                        fixed(marker_offset+marker_xe, text_y + scrn_rect.right / 6,
                              marker_offset+marker_xs, text_y,marker_offset+marker_xe,
                              text_y - scrn_rect.right / 6);
                    }//end pointer type
                } //if pointer
            } //end vertical/left

            // begin vertical/right
            //First draw capping lines and pointers
            if (huds_right(options)) {  // We'll default this for now.
                if (draw_cap_left) {
                    drawOneLine(scrn_rect.left, scrn_rect.top,
                                 scrn_rect.left, marker_ye);  // Cap left side
                } //endif cap_left

                marker_xe = scrn_rect.left + scrn_rect.right / 3;     // Adjust tick xe
                // Indicator carrot
                // drawOneLine(scrn_rect.left, mid_scr.y +  scrn_rect.right / 6,
                //              marker_xe, mid_scr.y);
                // drawOneLine(scrn_rect.left, mid_scr.y -  scrn_rect.right / 6,
                //              marker_xe, mid_scr.y);

                // draw pointer
                if (pointer) {
                    if (pointer_type == "moving") {
                        if (zoom == 0) {
                            //type-fixed & zoom=1, behaviour to be defined
                            // Code for Moving Type Pointer
                            float ycentre, ypoint,xpoint;
                            int range;

                            if (cur_value > maxValue)
                                cur_value = maxValue;
                            if (cur_value < minValue)
                                cur_value = minValue;

                            if (minValue >= 0.0)
                                ycentre = scrn_rect.top;
                            else if (maxValue + minValue == 0.0)
                                ycentre = mid_scr.y;
                            else
                                if (oddtype == 1)
                                    ycentre = scrn_rect.top + (1.0-minValue)*scrn_rect.bottom/(maxValue-minValue);
                                else
                                    ycentre = scrn_rect.top + minValue*scrn_rect.bottom/(maxValue-minValue);

                            range = scrn_rect.bottom;

                            if (oddtype == 1)
                                ypoint = ycentre + ((cur_value-1.0) * range / val_span);
                            else
                                ypoint = ycentre + (cur_value * range / val_span);

                            xpoint = scrn_rect.left - marker_offset;
                            drawOneLine(xpoint,ycentre,xpoint,ypoint);
                            drawOneLine(xpoint,ypoint,xpoint+marker_offset,ypoint);
                            drawOneLine(xpoint+marker_offset,ypoint,xpoint+5.0,ypoint+5.0);
                            drawOneLine(xpoint+marker_offset,ypoint,xpoint+5.0,ypoint-5.0);
                        }

                    } else {
                        // default to fixed
                        fixed(-marker_offset+scrn_rect.left, text_y +  scrn_rect.right / 6,
                               -marker_offset+marker_xe, text_y,-marker_offset+scrn_rect.left,
                               text_y -  scrn_rect.right / 6);
                    }
                } //if pointer
            }  //end vertical/right

            // At this point marker x_start and x_end values are transposed.
            // To keep this from confusing things they are now interchanged.
            if (huds_both(options)) {
                marker_ye = marker_xs;
                marker_xs = marker_xe;
                marker_xe = marker_ye;
            }

            // Work through from bottom to top of scale. Calculating where to put
            // minor and major ticks.

            // draw scale or tape

//            last = FloatToInt(vmax)+1;
//            i = FloatToInt(vmin);
            last = (int)vmax + 1; // N
            i = (int)vmin; // N

            if (zoom == 1) {
                zoomed_scale((int)vmin, (int)vmax);
            } else {
                for (; i < last; i++) {
                    condition = true;
                    if (!modulo()) {
                        if (i < min_val()) {
                            condition = false;
                        }
                    }

                    if (condition) {  // Show a tick if necessary
                        // Calculate the location of this tick
                        marker_ys = scrn_rect.top + ((i - vmin) * factor()/*+.5f*/);
                        // marker_ys = scrn_rect.top + (int)((i - vmin) * factor() + .5);
                        // Block calculation artifact from drawing ticks below min coordinate.
                        // Calculation here accounts for text height.

                        if ((marker_ys < (scrn_rect.top + 4))
                                | (marker_ys > (height - 4))) {
                            // Magic numbers!!!
                            continue;
                        }

                        if (oddtype == 1)
                            k = i+1; //enable ticks at odd values
                        else
                            k = i;

                        // Minor ticks
                        if (div_min()) {
                            // if ((i%div_min()) == 0) {
                            if (!(k%(int)div_min())) {
                                if (((marker_ys - 5) > scrn_rect.top)
                                        && ((marker_ys + 5) < (height))) {

                                    //vertical/left OR vertical/right
                                    if (huds_both(options)) {
                                        if (tick_type=="line") {
                                            if (tick_length=="variable") {
                                                drawOneLine(scrn_rect.left, marker_ys,
                                                            marker_xs,      marker_ys);
                                                drawOneLine(marker_xe,      marker_ys,
                                                            width,  marker_ys);
                                            } else {
                                                drawOneLine(scrn_rect.left, marker_ys,
                                                            marker_xs,      marker_ys);
                                                drawOneLine(marker_xe,      marker_ys,
                                                            width,  marker_ys);
                                            }
                                        } else if (tick_type == "circle")
                                            circles(scrn_rect.left,(float)marker_ys,3.0);

                                        else {
                                            // if neither line nor circle draw default as line
                                            drawOneLine(scrn_rect.left, marker_ys,
                                                        marker_xs,      marker_ys);
                                            drawOneLine(marker_xe,      marker_ys,
                                                        width,  marker_ys);
                                        }
                                        // glBegin(GL_LINES);
                                        // glVertex2f(scrn_rect.left, marker_ys);
                                        // glVertex2f(marker_xs,      marker_ys);
                                        // glVertex2f(marker_xe,      marker_ys);
                                        // glVertex2f(scrn_rect.left + scrn_rect.right,  marker_ys);
                                        // glEnd();
                                        // anything other than huds_both

                                    } else {
                                        if (huds_left(options)) {
                                            if (tick_type=="line") {
                                                if (tick_length=="variable") {
                                                    drawOneLine(marker_xs + 4, marker_ys,
                                                                marker_xe,     marker_ys);
                                                } else {
                                                    drawOneLine(marker_xs , marker_ys,
                                                                marker_xe,     marker_ys);
                                                }
                                            } else if (tick_type=="circle")
                                                circles((float)marker_xs + 4, (float)marker_ys,3.0);

                                            else {
                                                drawOneLine(marker_xs + 4, marker_ys,
                                                            marker_xe,     marker_ys);
                                            }
                                        }  else {
                                            if (tick_type=="line") {
                                                if (tick_length=="variable") {
                                                    drawOneLine(marker_xs,     marker_ys,
                                                                marker_xe - 4, marker_ys);
                                                } else {
                                                    drawOneLine(marker_xs,     marker_ys,
                                                                marker_xe , marker_ys);
                                                }
                                            } else if (tick_type=="circle")
                                                circles((float)marker_xe - 4, (float)marker_ys,3.0);
                                            else {
                                                drawOneLine(marker_xs,     marker_ys,
                                                            marker_xe - 4, marker_ys);
                                            }
                                        }
                                    } //end huds both
                                }
                            } //end draw minor ticks
                        }  //end minor ticks

                        // Major ticks
                        if (div_max()) {
                            if (!(k%(int)div_max())) {
                                if (modulo()) {
                                    disp_val = i % (int) modulo(); // ?????????
                                    if (disp_val < 0) {
                                        while (disp_val < 0)
                                            disp_val += modulo();
                                    }
                                } else {
                                    disp_val = i;
                                }

                                lenstr = sprintf(TextScale, "%d",
                                                 FloatToInt(disp_val * data_scaling()/*+.5*/));
                                // (int)(disp_val  * data_scaling() +.5));
                                /* if (((marker_ys - 8) > scrn_rect.top) &&
                                   ((marker_ys + 8) < (height))){ */
                                // huds_both
                                if (huds_both(options)) {
                                    // drawOneLine(scrn_rect.left, marker_ys,
                                    //              marker_xs,      marker_ys);
                                    // drawOneLine(marker_xs, marker_ys,
                                    //              scrn_rect.left + scrn_rect.right,
                                    //              marker_ys);
                                    if (tick_type == "line") {
                                        glBegin(GL_LINE_STRIP);
                                        glVertex2f(scrn_rect.left, marker_ys);
                                        glVertex2f(marker_xs, marker_ys);
                                        glVertex2f(width, marker_ys);
                                        glEnd();
                                    } else if (tick_type == "circle") {
                                        circles(scrn_rect.left, (float)marker_ys,5.0);
                                    } else {
                                        glBegin(GL_LINE_STRIP);
                                        glVertex2f(scrn_rect.left, marker_ys);
                                        glVertex2f(marker_xs, marker_ys);
                                        glVertex2f(width, marker_ys);
                                        glEnd();
                                    }

                                    if (!huds_notext(options)) {
                                        textString (marker_xs + 2,  marker_ys,
                                                    TextScale,0);
                                    }

                                } else {
                                    /* Changes are made to draw a circle when tick_type="circle" */
                                    // anything other than huds_both
                                    if (tick_type=="line") {
                                        drawOneLine(marker_xs, marker_ys, marker_xe, marker_ys);
                                    } else if (tick_type=="circle") {
                                        circles((float)marker_xs + 4, (float)marker_ys,5.0);
                                    } else {
                                        drawOneLine(marker_xs, marker_ys, marker_xe, marker_ys);
                                    }

                                    if (!huds_notext(options)) {
                                        if (huds_left(options)) {
                                            textString(marker_xs -  8 * lenstr - 2,
                                                       marker_ys - 4,
                                                       TextScale, 0);
                                        } else {
                                            textString(marker_xe + 3 * lenstr,
                                                       marker_ys - 4,
                                                       TextScale, 0);
                                        } //End if huds_left
                                    } //End if !huds_notext
                                }  //End if huds-both
                            }  // End if draw major ticks
                        }   // End if major ticks
                    }  // End condition
                }  // End for
            }  //end of zoom
            // End if VERTICAL SCALE TYPE (tape loop yet to be closed)

        } else {
            // Horizontal scale by default
            // left tick bar
            if (draw_tick_left) {
                drawOneLine(scrn_rect.left, scrn_rect.top,
                            scrn_rect.left, height);
            }  // endif draw_tick_left
            // right tick bar
            if (draw_tick_right) {
                drawOneLine(width, scrn_rect.top,
                            width,
                            height);
            }  // endif draw_tick_right

            marker_ys = scrn_rect.top;           // Starting point for
            marker_ye = height;                  // tick y location calcs
            marker_xe = width;
            marker_xs = scrn_rect.left + ((cur_value - vmin) * factor() /*+ .5f*/);

            //    glBegin(GL_LINES);
            // left tick bar
            //    glVertex2f(scrn_rect.left, scrn_rect.top);
            //    glVertex2f(scrn_rect.left, marker_ye);

            // right tick bar
            //    glVertex2f(marker_xe, scrn_rect.top);
            //    glVertex2f(marker_xe, marker_ye);
            //    glEnd();

            if (huds_top(options)) {
                if (draw_cap_bottom) {
                    // Bottom box line
                    drawOneLine(scrn_rect.left,
                                scrn_rect.top,
                                width,
                                scrn_rect.top);
                }  //endif cap_bottom

                // Tick point adjust
                marker_ye  = scrn_rect.top + scrn_rect.bottom / 2;
                // Bottom arrow
                // drawOneLine(mid_scr.x, marker_ye,
                //              mid_scr.x - scrn_rect.bottom / 4, scrn_rect.top);
                // drawOneLine(mid_scr.x, marker_ye,
                //              mid_scr.x + scrn_rect.bottom / 4, scrn_rect.top);
                // draw pointer
                if (pointer) {
                    if (pointer_type == "moving") {
                        if (zoom == 0) {
                            //Code for Moving Type Pointer
                            // static float xcentre,xpoint,ypoint;
                            // static int range;
                            if (cur_value > maxValue)
                                cur_value = maxValue;
                            if (cur_value < minValue)
                                cur_value = minValue;

                            float xcentre = mid_scr.x;
                            int range = scrn_rect.right;
                            float xpoint = xcentre + (cur_value * range / val_span);
                            float ypoint = scrn_rect.top - marker_offset;
                            drawOneLine(xcentre, ypoint,xpoint,ypoint);
                            drawOneLine(xpoint,ypoint,xpoint,ypoint+marker_offset);
                            drawOneLine(xpoint,ypoint+marker_offset,xpoint+5.0,ypoint+5.0);
                            drawOneLine(xpoint,ypoint+marker_offset,xpoint-5.0,ypoint+5.0);
                        }
                    } else {
                        //default to fixed
                        fixed(marker_xs - scrn_rect.bottom / 4, scrn_rect.top,
                               marker_xs, marker_ye, marker_xs + scrn_rect.bottom / 4,scrn_rect.top);
                    }
                }  //if pointer
            } //End Horizontal scale/top

            if (huds_bottom(options)) {
                // Top box line
                if (draw_cap_top) {
                    drawOneLine(scrn_rect.left, height,
                                 width, height);
                }  //endif cap_top

                // Tick point adjust
                marker_ys = height - scrn_rect.bottom / 2;
                // Top arrow
                //      drawOneLine(mid_scr.x + scrn_rect.bottom / 4,
                //                   scrn_rect.top + scrn_rect.bottom,
                //                   mid_scr.x, marker_ys);
                //      drawOneLine(mid_scr.x - scrn_rect.bottom / 4,
                //                   scrn_rect.top + scrn_rect.bottom,
                //                   mid_scr.x , marker_ys);

                // draw pointer
                if (pointer) {
                    if (pointer_type =="moving") {
                        if (zoom == 0) {
                            //Code for Moving Type Pointer
                            // static float xcentre,xpoint,ypoint;
                            // static int range,hgt;
                            if (cur_value > maxValue)
                                cur_value = maxValue;
                            if (cur_value < minValue)
                                cur_value = minValue;

                            float xcentre = mid_scr.x ;
                            int range = scrn_rect.right;
                            int hgt   = scrn_rect.top + scrn_rect.bottom;
                            float xpoint = xcentre + (cur_value * range / val_span);
                            float ypoint = hgt + marker_offset;
                            drawOneLine(xcentre, ypoint,xpoint,ypoint);
                            drawOneLine(xpoint,ypoint,xpoint,ypoint-marker_offset);
                            drawOneLine(xpoint,ypoint-marker_offset,xpoint+5.0,ypoint-5.0);
                            drawOneLine(xpoint,ypoint-marker_offset,xpoint-5.0,ypoint-5.0);
                        }
                    } else {
                        fixed(marker_xs + scrn_rect.bottom / 4, height, marker_xs, marker_ys,
                              marker_xs - scrn_rect.bottom / 4, height);
                    }
                } //if pointer
            }  //end horizontal scale bottom

            //    if ((options & HUDS_BOTTOM) == HUDS_BOTTOM) {
            //      marker_xe = marker_ys;
            //      marker_ys = marker_ye;
            //      marker_ye = marker_xe;
            //      }

            // printf("vmin = %d  vmax = %d\n", (int)vmin, (int)vmax);

            //  last = FloatToInt(vmax)+1;
            //  i    = FloatToInt(vmin);

            if (zoom == 1) {
                zoomed_scale((int)vmin,(int)vmax);
            } else {
                //default to zoom=0
                last = (int)vmax + 1;
                i = (int)vmin;
                for (; i < last; i++) {
                    // for (i = (int)vmin; i <= (int)vmax; i++)     {
                    // printf("<*> i = %d\n", i);
                    condition = true;
                    if (!modulo()) {
                        if (i < min_val()) {
                            condition = false;
                        }
                    }
                    // printf("<**> i = %d\n", i);
                    if (condition) {
                        // marker_xs = scrn_rect.left + (int)((i - vmin) * factor() + .5);
                        marker_xs = scrn_rect.left + (((i - vmin) * factor()/*+ .5f*/));

                        if (oddtype == 1)
                            k = i+1; //enable ticks at odd values
                        else
                            k = i;

                        if (div_min()) {
                            //          if ((i%(int)div_min()) == 0) {
                            //draw minor ticks
                            if (!(k%(int)div_min())) {
                                // draw in ticks only if they aren't too close to the edge.
                                if (((marker_xs - 5) > scrn_rect.left)
                                        && ((marker_xs + 5)< (scrn_rect.left + scrn_rect.right))) {

                                    if (huds_both(options)) {
                                        if (tick_length == "variable") {
                                            drawOneLine(marker_xs, scrn_rect.top,
                                                        marker_xs, marker_ys - 4);
                                            drawOneLine(marker_xs, marker_ye + 4,
                                                        marker_xs, height);
                                        } else {
                                            drawOneLine(marker_xs, scrn_rect.top,
                                                        marker_xs, marker_ys);
                                            drawOneLine(marker_xs, marker_ye,
                                                        marker_xs, height);
                                        }
                                        // glBegin(GL_LINES);
                                        // glVertex2f(marker_xs, scrn_rect.top);
                                        // glVertex2f(marker_xs, marker_ys - 4);
                                        // glVertex2f(marker_xs, marker_ye + 4);
                                        // glVertex2f(marker_xs, scrn_rect.top + scrn_rect.bottom);
                                        // glEnd();

                                    } else {
                                        if (huds_top(options)) {
                                            //draw minor ticks
                                            if (tick_length=="variable")
                                                drawOneLine(marker_xs,marker_ys,marker_xs,marker_ye-4);
                                            else
                                                drawOneLine(marker_xs,marker_ys,marker_xs,marker_ye);

                                        } else
                                            if (tick_length=="variable")
                                                drawOneLine(marker_xs,marker_ys+4,marker_xs,marker_ye);
                                            else
                                                drawOneLine(marker_xs,marker_ys,marker_xs,marker_ye);
                                        }
                                    }
                                } //end draw minor ticks
                            } //end minor ticks

                            //major ticks
                            if (div_max()) {
                                // printf("i = %d\n", i);
                                // if ((i%(int)div_max())==0) {
                                //     draw major ticks

                                if (!(k%(int)div_max())) {
                                    if (modulo()) {
                                        disp_val = i % (int) modulo(); // ?????????
                                        if (disp_val < 0) {
                                            while (disp_val<0)
                                                disp_val += modulo();
                                        }
                                    } else {
                                        disp_val = i;
                                    }
                                    // printf("disp_val = %d\n", disp_val);
                                    // printf("%d\n", (int)(disp_val  * (double)data_scaling() + 0.5));
                                    lenstr = sprintf(TextScale, "%d",
                                            // (int)(disp_val  * data_scaling() +.5));
                                            FloatToInt(disp_val * data_scaling()/*+.5*/));

                                    // Draw major ticks and text only if far enough from the edge.
                                    if (((marker_xs - 10)> scrn_rect.left)
                                            && ((marker_xs + 10) < (scrn_rect.left + scrn_rect.right))) {
                                        if (huds_both(options)) {
                                            // drawOneLine(marker_xs, scrn_rect.top,
                                            //              marker_xs, marker_ys);
                                            // drawOneLine(marker_xs, marker_ye,
                                            //              marker_xs, scrn_rect.top + scrn_rect.bottom);
                                            glBegin(GL_LINE_STRIP);
                                            glVertex2f(marker_xs, scrn_rect.top);
                                            glVertex2f(marker_xs, marker_ye);
                                            glVertex2f(marker_xs, height);
                                            glEnd();

                                            if (!huds_notext(options)) {
                                                textString(marker_xs - 4 * lenstr,
                                                           marker_ys + 4, TextScale, 0);
                                            }
                                        } else {
                                            drawOneLine(marker_xs, marker_ys, marker_xs, marker_ye);

                                        if (!huds_notext(options)) {
                                            if (huds_top(options)) {
                                                textString(marker_xs - 4 * lenstr,
                                                           height - 10, TextScale, 0);

                                            }  else  {
                                                textString(marker_xs - 4 * lenstr,
                                                           scrn_rect.top, TextScale, 0);
                                            }
                                        }
                                    }
                                }
                            }  //end draw major ticks
                        } //endif major ticks
                    }   //end condition
                } //end for
            }  //end zoom
        } //end horizontal/vertical scale
    } // end of type tape
} //draw



void hud_card::circles(float x,float y,float size)
{
    glEnable(GL_POINT_SMOOTH);
    glPointSize(size);

    glBegin(GL_POINTS);
    glVertex2f(x,y);
    glEnd();

    glPointSize(1.0);
    glDisable(GL_POINT_SMOOTH);
}


void hud_card::fixed(float x1, float y1, float x2, float y2, float x3, float y3)
{
    glBegin(GL_LINE_STRIP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}


void hud_card::zoomed_scale(int first, int last)
{
    POINT mid_scr    = get_centroid();
    RECT   scrn_rect = get_location();
    UINT options     = get_options();
    char TextScale[80];
    // int disp_val = 0;
    int data[80];

    float x,y,w,h,bottom;
    float cur_value=get_value();
    if (cur_value > maxValue)
        cur_value = maxValue;
    if (cur_value < minValue)
        cur_value = minValue;

    int a=0;

    while (first <= last) {
        if ((first % (int)Maj_div) == 0) {
            data[a] = first;
            a++ ;
        }
        first++;
    }
    int centre =a/2;

    if (huds_vert(options)) {
        x=scrn_rect.left;
        y=scrn_rect.top;
        w=scrn_rect.left+scrn_rect.right;
        h=scrn_rect.top+scrn_rect.bottom;
        bottom=scrn_rect.bottom;

        float xstart, yfirst, ycentre, ysecond;

        float hgt = bottom * 20.0 /100.0;  // 60% of height should be zoomed
        yfirst = mid_scr.y - hgt;
        ycentre = mid_scr.y;
        ysecond = mid_scr.y + hgt;
        float range = hgt * 2;

        int i;
        float factor = range /10.0;

        float hgt1 = bottom * 30.0 /100.0;
        int  incrs = ((int)val_span - (Maj_div * 2)) / Maj_div ;
        int  incr = incrs / 2;
        float factors = hgt1 / incr;

        // begin
        //this is for moving type pointer
        static float ycent, ypoint,xpoint;
        static int wth;

        ycent = mid_scr.y;
        wth=scrn_rect.left+scrn_rect.right;

        if (cur_value <= data[centre+1])
            if (cur_value > data[centre]) {
                ypoint = ycent + ((cur_value - data[centre]) * hgt/Maj_div);
            }

        if (cur_value >= data[centre-1])
            if (cur_value <= data[centre]) {
                ypoint = ycent - ((data[centre]-cur_value) * hgt/Maj_div);
            }

        if (cur_value < data[centre-1])
            if (cur_value >= minValue) {
                float diff  = minValue  - data[centre-1];
                float diff1 = cur_value - data[centre-1];
                float val = (diff1 * hgt1) / diff;

                ypoint = ycent - hgt -  val;
            }

        if (cur_value > data[centre+1])
            if (cur_value <= maxValue) {
                float diff  = maxValue  - data[centre+1];
                float diff1 = cur_value - data[centre+1];
                float val = (diff1 * hgt1) / diff;

                ypoint = ycent + hgt +  val;
            }

        if (huds_left(options)) {
            xstart = w;

            drawOneLine(xstart, ycentre, xstart - 5.0, ycentre); //centre tick

            sprintf(TextScale,"%3.0f\n",(float)(data[centre] * data_scaling()));

            if (!huds_notext(options)) {
                textString (x, ycentre, TextScale, 0);
            }

            for (i = 1; i < 5; i++) {
                yfirst  += factor;
                ycentre += factor;
                circles(xstart-2.5,yfirst, 3.0);
                circles(xstart-2.5,ycentre,3.0);
            }

            yfirst = mid_scr.y - hgt;

            for (i = 0; i <= incr; i++) {
                drawOneLine(xstart, yfirst, xstart - 5.0, yfirst);
                drawOneLine(xstart,ysecond, xstart - 5.0,ysecond);

                sprintf(TextScale,"%3.0f\n",(float)(data[centre-i-1] * data_scaling()));

                if (!huds_notext(options))
                    textString (x, yfirst, TextScale, 0);

                sprintf(TextScale,"%3.0f\n",(float)(data[centre+i+1] * data_scaling()));

                if (!huds_notext(options))
                    textString (x, ysecond, TextScale, 0);

                yfirst    -= factors;
                ysecond   += factors;

            }

            //to draw moving type pointer for left option
            //begin
            xpoint = wth + 10.0;

            if (pointer_type == "moving") {
                drawOneLine(xpoint,ycent,xpoint,ypoint);
                drawOneLine(xpoint,ypoint,xpoint-10.0,ypoint);
                drawOneLine(xpoint-10.0,ypoint,xpoint-5.0,ypoint+5.0);
                drawOneLine(xpoint-10.0,ypoint,xpoint-5.0,ypoint-5.0);
            }
            //end

        } else {
            //huds_right
            xstart = (x+w)/2;

            drawOneLine(xstart, ycentre, xstart + 5.0, ycentre); //centre tick

            sprintf(TextScale,"%3.0f\n",(float)(data[centre] * data_scaling()));

            if (!huds_notext(options)) {
                textString (w, ycentre, TextScale, 0);
            }

            for (i = 1; i < 5; i++) {
                yfirst  += factor;
                ycentre += factor;
                circles(xstart+2.5,yfirst, 3.0);
                circles(xstart+2.5,ycentre,3.0);
            }

            yfirst = mid_scr.y - hgt;

            for (i = 0; i <= incr; i++) {
                drawOneLine(xstart, yfirst, xstart + 5.0, yfirst);
                drawOneLine(xstart,ysecond, xstart + 5.0,ysecond);

                sprintf(TextScale,"%3.0f\n",(float)(data[centre-i-1] * data_scaling()));

                if (!huds_notext(options))
                    textString(w, yfirst, TextScale, 0);

                sprintf(TextScale,"%3.0f\n",(float)(data[centre+i+1] * data_scaling()));

                if (!huds_notext(options))
                    textString(w, ysecond, TextScale, 0);

                yfirst -= factors;
                ysecond += factors;

            }

            // to draw moving type pointer for right option
            //begin
            xpoint = scrn_rect.left;

            if (pointer_type == "moving") {
                drawOneLine(xpoint,ycent,xpoint,ypoint);
                drawOneLine(xpoint,ypoint,xpoint+10.0,ypoint);
                drawOneLine(xpoint+10.0,ypoint,xpoint+5.0,ypoint+5.0);
                drawOneLine(xpoint+10.0,ypoint,xpoint+5.0,ypoint-5.0);
            }
            //end
        }//end huds_right /left
        //end of vertical scale

    } else {
        //horizontal scale
        x=scrn_rect.left;
        y=scrn_rect.top;
        w=scrn_rect.left+scrn_rect.right;
        h=scrn_rect.top+scrn_rect.bottom;
        bottom=scrn_rect.right;

        float ystart, xfirst, xcentre, xsecond;

        float hgt = bottom * 20.0 /100.0;  // 60% of height should be zoomed
        xfirst = mid_scr.x - hgt;
        xcentre = mid_scr.x;
        xsecond = mid_scr.x + hgt;
        float range = hgt * 2;

        int i;
        float factor = range /10.0;

        float hgt1 = bottom * 30.0 /100.0;
        int  incrs = ((int)val_span - (Maj_div * 2)) / Maj_div ;
        int  incr = incrs / 2;
        float factors = hgt1 / incr;


        //Code for Moving Type Pointer
        //begin
        static float xcent,xpoint,ypoint;

        xcent = mid_scr.x;

        if (cur_value <= data[centre+1])
            if (cur_value > data[centre]) {
                xpoint = xcent + ((cur_value - data[centre]) * hgt/Maj_div);
            }

        if (cur_value >= data[centre-1])
            if (cur_value <= data[centre]) {
                xpoint = xcent - ((data[centre]-cur_value) * hgt/Maj_div);
            }

        if (cur_value < data[centre-1])
            if (cur_value >= minValue) {
                float diff  = minValue  - data[centre-1];
                float diff1 = cur_value - data[centre-1];
                float val = (diff1 * hgt1) / diff;

                xpoint = xcent - hgt -  val;
            }


        if (cur_value > data[centre+1])
            if (cur_value <= maxValue) {
                float diff  = maxValue  - data[centre+1];
                float diff1 = cur_value - data[centre+1];
                float val = (diff1 * hgt1) / diff;

                xpoint = xcent + hgt +  val;
            }

        //end
        if (huds_top(options)) {

            ystart =h;

            drawOneLine(xcentre, ystart, xcentre, ystart - 5.0); //centre tick

            sprintf(TextScale,"%3.0f\n",(float)(data[centre] * data_scaling()));

            if (!huds_notext(options)) {
                textString (xcentre-10.0, y, TextScale, 0);
            }

            for (i = 1; i < 5; i++) {
                xfirst  += factor;
                xcentre += factor;
                circles(xfirst,  ystart-2.5, 3.0);
                circles(xcentre, ystart-2.5, 3.0);
            }

            xfirst = mid_scr.x - hgt;

            for (i = 0; i <= incr; i++) {
                drawOneLine(xfirst,  ystart, xfirst,  ystart - 5.0);
                drawOneLine(xsecond, ystart, xsecond, ystart - 5.0);

                sprintf(TextScale,"%3.0f\n",(float)(data[centre-i-1] * data_scaling()));

                if (!huds_notext(options))
                    textString (xfirst-10.0, y, TextScale, 0);

                sprintf(TextScale,"%3.0f\n",(float)(data[centre+i+1] * data_scaling()));

                if (!huds_notext(options))
                    textString (xsecond-10.0, y, TextScale, 0);


                xfirst -= factors;
                xsecond += factors;
            }
            //this is for moving pointer for top option
            //begin

            ypoint = scrn_rect.top + scrn_rect.bottom + 10.0;

            if (pointer_type == "moving") {
                drawOneLine(xcent, ypoint,xpoint,ypoint);
                drawOneLine(xpoint,ypoint,xpoint,ypoint-10.0);
                drawOneLine(xpoint,ypoint-10.0,xpoint+5.0,ypoint-5.0);
                drawOneLine(xpoint,ypoint-10.0,xpoint-5.0,ypoint-5.0);
            }
            //end of top option

        } else {
            //else huds_bottom
            ystart = (y + h) / 2;

            //drawOneLine(xstart, yfirst,  xstart - 5.0, yfirst);
            drawOneLine(xcentre, ystart, xcentre, ystart + 5.0); //centre tick

            sprintf(TextScale,"%3.0f\n",(float)(data[centre] * data_scaling()));

            if (!huds_notext(options)) {
                textString (xcentre-10.0, h, TextScale, 0);
            }

            for (i = 1; i < 5; i++) {
                xfirst  += factor;
                xcentre += factor;
                circles(xfirst,  ystart+2.5, 3.0);
                circles(xcentre, ystart+2.5,3.0);
            }

            xfirst = mid_scr.x - hgt;

            for (i = 0; i <= incr; i++) {
                drawOneLine(xfirst, ystart, xfirst, ystart + 5.0);
                drawOneLine(xsecond,ystart, xsecond,ystart + 5.0);

                sprintf(TextScale,"%3.0f\n",(float)(data[centre-i-1] * data_scaling()));

                if (!huds_notext(options))
                    textString (xfirst-10.0,h, TextScale, 0);

                sprintf(TextScale,"%3.0f\n",(float)(data[centre+i+1] * data_scaling()));

                if (!huds_notext(options))
                    textString (xsecond-10.0,h, TextScale, 0);


                xfirst -= factors;
                xsecond   += factors;

            }
            //this is for movimg pointer for bottom option
            //begin

            ypoint = scrn_rect.top - 10.0;
            if (pointer_type == "moving") {
                drawOneLine(xcent, ypoint,xpoint,ypoint);
                drawOneLine(xpoint,ypoint,xpoint,ypoint+10.0);
                drawOneLine(xpoint,ypoint+10.0,xpoint+5.0,ypoint+5.0);
                drawOneLine(xpoint,ypoint+10.0,xpoint-5.0,ypoint+5.0);
            }
        }//end hud_top or hud_bottom
    }  //end of horizontal/vertical scales
}//end draw


