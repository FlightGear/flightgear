
#include "hud.hxx"

#ifdef USE_HUD_TextList
#define textString( x , y, text, font )  TextString( text, x , y )
#else
#define textString( x , y, text, font )  puDrawString ( guiFnt, text, x, y );
#endif

//========== Top of hud_card class member definitions =============

hud_card ::
hud_card( int       x,
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
          bool      working) :
                instr_scale( x,y,width,height,
                             data_source, options,
                             value_span,
                             max_value, min_value, disp_scaling,
                             major_divs, minor_divs, modulus,
                             working),
                val_span		 ( value_span),
                type             ( card_type),
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
				pointer_type     (type_pointer)

{
  half_width_units = range_to_show() / 2.0;
//  UINT options     = get_options();
//  huds_both = (options & HUDS_BOTH) == HUDS_BOTH;
//  huds_right = options & HUDS_RIGHT;
//  huds_left = options & HUDS_LEFT;
//  huds_vert = options & HUDS_VERT;
//  huds_notext = options & HUDS_NOTEXT;
//  huds_top = options & HUDS_TOP;
//  huds_bottom = options & HUDS_BOTTOM;
}

hud_card ::
~hud_card() { }

hud_card ::
hud_card( const hud_card & image):
    instr_scale( (const instr_scale & ) image),
    val_span( image.val_span),
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
    pointer_type (image.pointer_type)

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

hud_card & hud_card ::
operator = (const hud_card & rhs )
{
    if( !( this == &rhs)){
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
	pointer			 = rhs.pointer;
	pointer_type     = rhs.pointer_type;
    }

    return *this;
}

void hud_card ::
draw( void ) //  (HUD_scale * pscale )
{

    float vmin = 0.0, vmax = 0.0;
    int marker_xs;
    int marker_xe;
    int marker_ys;
    int marker_ye;
    int text_x = 0, text_y = 0;
    int lenstr;
    int height, width;
    int i, last;
    char TextScale[80];
    bool condition;
    int disp_val = 0;

    POINT mid_scr    = get_centroid();
    float cur_value  = get_value();
    RECT   scrn_rect = get_location();
    UINT options     = get_options();

    height = scrn_rect.top  + scrn_rect.bottom;
    width = scrn_rect.left + scrn_rect.right;

    if(type=="guage") {

        vmin   = min_val();
        vmax   = max_val();
        text_y = scrn_rect.top + FloatToInt((cur_value - vmin) * factor() /*+.5f*/);
        text_x = marker_xs;
    } else {
        if(type=="tape") {
		
            vmin   = cur_value - half_width_units; // width units == needle travel
            vmax   = cur_value + half_width_units; // or picture unit span.
            text_x = mid_scr.x;
            text_y = mid_scr.y;
        }
    }
  
    // Draw the basic markings for the scale...
  
    if( huds_vert(options) ) { // Vertical scale
        if (draw_tick_bottom) {
            drawOneLine( scrn_rect.left,     // Bottom tick bar
                         scrn_rect.top,
                         width,
                         scrn_rect.top);
        } // endif draw_tick_bottom
        if (draw_tick_top) {
            drawOneLine( scrn_rect.left,    // Top tick bar
                         height,
                         width,
                         height );
        } // endif draw_tick_top
      
        marker_xs = scrn_rect.left;  // x start
        marker_xe = width;  // x extent
        marker_ye = height;

	//    glBegin(GL_LINES);
      
        // Bottom tick bar
	//    glVertex2f( marker_xs, scrn_rect.top);
	//    glVertex2f( marker_xe, scrn_rect.top);

		  // Top tick bar
	//    glVertex2f( marker_xs, marker_ye);
	//    glVertex2f( marker_xe, marker_ye );
	//    glEnd();


        // We do not use else in the following so that combining the
        // two options produces a "caged" display with double
        // carrots. The same is done for horizontal card indicators.

        if( huds_left(options) ) {    // Calculate x marker offset
			
            if (draw_cap_right) {
			  
                drawOneLine( marker_xe, scrn_rect.top,
                             marker_xe, marker_ye); // Cap right side
            } //endif cap_right

            marker_xs  = marker_xe - scrn_rect.right / 3;   // Adjust tick xs

            // drawOneLine( marker_xs, mid_scr.y,
            //              marker_xe, mid_scr.y + scrn_rect.right / 6);
            // drawOneLine( marker_xs, mid_scr.y,
            //              marker_xe, mid_scr.y - scrn_rect.right / 6);

            // draw pointer
            if(pointer)	{
                if(pointer_type=="fixed")	{
                    glBegin(GL_LINE_STRIP);
                    glVertex2f( marker_offset+marker_xe, text_y + scrn_rect.right / 6);
                    glVertex2f( marker_offset+marker_xs, text_y);
                    glVertex2f( marker_offset+marker_xe, text_y - scrn_rect.right / 6);
                    glEnd();
                } else {
                    if(pointer_type=="moving")	{
                        //Code for Moving Type Pointer to be included.
                    }
                }
            }
        }
		
        if( huds_right(options) ) {  // We'll default this for now.
            if (draw_cap_left) {
                drawOneLine( scrn_rect.left, scrn_rect.top,
                             scrn_rect.left, marker_ye );  // Cap left side
            } //endif cap_left

            marker_xe = scrn_rect.left + scrn_rect.right / 3;     // Adjust tick xe
            // Indicator carrot
            // drawOneLine( scrn_rect.left, mid_scr.y +  scrn_rect.right / 6,
            //              marker_xe, mid_scr.y );
            // drawOneLine( scrn_rect.left, mid_scr.y -  scrn_rect.right / 6,
            //              marker_xe, mid_scr.y);
			
            // draw pointer
            if(pointer) {
                if(pointer_type=="fixed")	{
                    glBegin(GL_LINE_STRIP);
                    glVertex2f( -marker_offset+scrn_rect.left, text_y +  scrn_rect.right / 6);
                    glVertex2f( -marker_offset+marker_xe, text_y );
                    glVertex2f( -marker_offset+scrn_rect.left, text_y -  scrn_rect.right / 6);
                    glEnd();
                }
                else {
                    if(pointer_type=="moving")	{
                        // Code for Moving Type Pointer to be included.
                    }
                }
            }
        }

        // At this point marker x_start and x_end values are transposed.
        // To keep this from confusing things they are now interchanged.
        if(huds_both(options)) {
            marker_ye = marker_xs;
            marker_xs = marker_xe;
            marker_xe = marker_ye;
        }

        // Work through from bottom to top of scale. Calculating where to put
        // minor and major ticks.

        // draw scale or tape
	//  last = FloatToInt(vmax)+1;
	//  i = FloatToInt(vmin);
        last = (int)vmax + 1;
        i = (int)vmin;
        for( ; i <last ; i++ ) {
            condition = true;
            if( !modulo()) {
                if( i < min_val()) {
                    condition = false;
                }
            }

            if( condition ) {  // Show a tick if necessary
                // Calculate the location of this tick
                marker_ys = scrn_rect.top + FloatToInt(((i - vmin) * factor()/*+.5f*/));
                // marker_ys = scrn_rect.top + (int)((i - vmin) * factor() + .5);
                // Block calculation artifact from drawing ticks below min coordinate.
                // Calculation here accounts for text height.

                if(( marker_ys < (scrn_rect.top + 4)) |
                   ( marker_ys > (height - 4)))
                {
                    // Magic numbers!!!
                    continue;
                }
                if( div_min()) {
                    // if( (i%div_min()) == 0) {
                    if( !(i%(int)div_min())) {            
                        if((( marker_ys - 5) > scrn_rect.top ) &&
                           (( marker_ys + 5) < (height))){
                            if( huds_both(options) ) {
                                drawOneLine( scrn_rect.left, marker_ys,
                                             marker_xs,      marker_ys );
                                drawOneLine( marker_xe,      marker_ys,
                                             width,  marker_ys );
                                // glBegin(GL_LINES);
                                // glVertex2f( scrn_rect.left, marker_ys );
                                // glVertex2f( marker_xs,      marker_ys );
                                // glVertex2f( marker_xe,      marker_ys);
                                // glVertex2f( scrn_rect.left + scrn_rect.right,  marker_ys );
                                // glEnd();
                            } else {
                                if( huds_left(options) ) {
                                    drawOneLine( marker_xs + 4, marker_ys,
                                                 marker_xe,     marker_ys );
                                } else {
                                    drawOneLine( marker_xs,     marker_ys,
                                                 marker_xe - 4, marker_ys );
                                }
                            }
                        }
                    }
                }

                if( div_max() ) {
                    if( !(i%(int)div_max()) ) {
                        if(modulo()) {
                            if( disp_val < 0) {
                                while(disp_val < 0)
                                    disp_val += modulo();
                            // } else {
                            //   disp_val = i % (int)modulo();
                            }
                            disp_val = i % (int) modulo(); // ?????????
                        } else {
                            disp_val = i;
                        }

                        lenstr = sprintf( TextScale, "%d",
                                          FloatToInt(disp_val * data_scaling()/*+.5*/));
                        // (int)(disp_val  * data_scaling() +.5));
                        if(( (marker_ys - 8 ) > scrn_rect.top ) &&
                           ( (marker_ys + 8) < (height))){
                            if( huds_both(options) ) {
                                // drawOneLine( scrn_rect.left, marker_ys,
                                //              marker_xs,      marker_ys);
                                // drawOneLine( marker_xs, marker_ys,
                                //              scrn_rect.left + scrn_rect.right,
                                //              marker_ys);
                                glBegin(GL_LINE_STRIP);
                                glVertex2f( scrn_rect.left, marker_ys );
                                glVertex2f( marker_xs, marker_ys);
                                glVertex2f( width, marker_ys);
                                glEnd();
                                if( !huds_notext(options)) {
                                    textString ( marker_xs + 2,  marker_ys,
                                                 TextScale,  GLUT_BITMAP_8_BY_13 );
                                }
                            } else {
                                drawOneLine( marker_xs, marker_ys, marker_xe, marker_ys );
                                if( !huds_notext(options) ) {
                                    if( huds_left(options) )              {
                                        textString( marker_xs -  8 * lenstr - 2,
                                                    marker_ys - 4,
                                                    TextScale, GLUT_BITMAP_8_BY_13 );
                                    } else  {
                                        textString( marker_xe + 3 * lenstr,
                                                    marker_ys - 4,
                                                    TextScale, GLUT_BITMAP_8_BY_13 );
                                    }
                                }
                            }
                        } // Else read oriented right
                    } // End if modulo division by major interval is zero
                }  // End if major interval divisor non-zero
            } // End if condition
        } // End for range of i from vmin to vmax
    }  // End if VERTICAL SCALE TYPE
    else {                                // Horizontal scale by default
        // left tick bar
	if (draw_tick_left) {
            drawOneLine( scrn_rect.left, scrn_rect.top,
                         scrn_rect.left, height);
	} // endif draw_tick_left
        // right tick bar
	if (draw_tick_right) {
            drawOneLine( width, scrn_rect.top,
                         width,
                         height );
	} // endif draw_tick_right
      
        marker_ys = scrn_rect.top;           // Starting point for
        marker_ye = height;                  // tick y location calcs
        marker_xe = width;
        marker_xs = scrn_rect.left + FloatToInt((cur_value - vmin) * factor() /*+ .5f*/);
		  

	//    glBegin(GL_LINES);
		  // left tick bar
	//    glVertex2f( scrn_rect.left, scrn_rect.top);
	//    glVertex2f( scrn_rect.left, marker_ye);

		  // right tick bar
	//    glVertex2f( marker_xe, scrn_rect.top);
	//    glVertex2f( marker_xe, marker_ye );
	//    glEnd();

	if( huds_top(options) ) {
            if (draw_cap_bottom) {
                // Bottom box line
                drawOneLine( scrn_rect.left,
                             scrn_rect.top,
                             width,
                             scrn_rect.top);
            } //endif cap_bottom

            // Tick point adjust
            marker_ye  = scrn_rect.top + scrn_rect.bottom / 2;
            // Bottom arrow
            // drawOneLine( mid_scr.x, marker_ye,
            //              mid_scr.x - scrn_rect.bottom / 4, scrn_rect.top);
            // drawOneLine( mid_scr.x, marker_ye,
            //              mid_scr.x + scrn_rect.bottom / 4, scrn_rect.top);
            // draw pointer
            if(pointer) {
                if(pointer_type=="fixed")	{
                    glBegin(GL_LINE_STRIP);
                    glVertex2f( marker_xs - scrn_rect.bottom / 4, scrn_rect.top);
                    glVertex2f( marker_xs, marker_ye);
                    glVertex2f( marker_xs + scrn_rect.bottom / 4, scrn_rect.top);
                    glEnd();
                } else {
                    if(pointer_type=="moving")	{
                        // Code for Moving type Pointer to be included.
                    }
                }
            }
	}

	if( huds_bottom(options) ) {
            // Top box line
            if (draw_cap_top) {
                drawOneLine( scrn_rect.left, height,
                             width, height);
            } //endif cap_top

            // Tick point adjust
            marker_ys = height - scrn_rect.bottom  / 2;
            // Top arrow
            //      drawOneLine( mid_scr.x + scrn_rect.bottom / 4,
            //                   scrn_rect.top + scrn_rect.bottom,
            //                   mid_scr.x, marker_ys );
            //      drawOneLine( mid_scr.x - scrn_rect.bottom / 4,
            //                   scrn_rect.top + scrn_rect.bottom,
            //                   mid_scr.x , marker_ys );
            
            // draw pointer
            if(pointer) {
                if(pointer_type=="fixed")	{
                    glBegin(GL_LINE_STRIP);
                    glVertex2f( marker_xs + scrn_rect.bottom / 4, height);
                    glVertex2f( marker_xs, marker_ys );
                    glVertex2f( marker_xs - scrn_rect.bottom / 4, height);
                    glEnd();
                } else {
                    if(pointer_type=="moving")	{
                        // Code for Moving Type Pointer to be included.
                    }
                }
            }//if pointer

	}

	//    if(( options & HUDS_BOTTOM) == HUDS_BOTTOM ) {
	//      marker_xe = marker_ys;
	//      marker_ys = marker_ye;
	//      marker_ye = marker_xe;
	//      }

		// printf("vmin = %d  vmax = %d\n", (int)vmin, (int)vmax);

	//  last = FloatToInt(vmax)+1;
	//  i    = FloatToInt(vmin);
        last = (int)vmax + 1;
        i = (int)vmin;
        for(; i <last ; i++ ) {
            // for( i = (int)vmin; i <= (int)vmax; i++ )     {
            // printf("<*> i = %d\n", i);
            condition = true;
            if( !modulo()) {
                if( i < min_val()) {
                    condition = false;
                }
            }
            // printf("<**> i = %d\n", i);
            if( condition )        {
                // marker_xs = scrn_rect.left + (int)((i - vmin) * factor() + .5);
                marker_xs = scrn_rect.left + FloatToInt(((i - vmin) * factor()/*+ .5f*/));
                if( div_min()){
                    //          if( (i%(int)div_min()) == 0 ) {
                    if( !(i%(int)div_min() )) {           
                        // draw in ticks only if they aren't too close to the edge.
                        if((( marker_xs - 5) > scrn_rect.left ) &&
                           (( marker_xs + 5 )< (scrn_rect.left + scrn_rect.right))){
                            
                            if( huds_both(options) ) {
                                drawOneLine( marker_xs, scrn_rect.top,
                                             marker_xs, marker_ys - 4);
                                drawOneLine( marker_xs, marker_ye + 4,
                                             marker_xs, height);
                                // glBegin(GL_LINES);
                                // glVertex2f( marker_xs, scrn_rect.top);
                                // glVertex2f( marker_xs, marker_ys - 4);
                                // glVertex2f( marker_xs, marker_ye + 4);
                                // glVertex2f( marker_xs, scrn_rect.top + scrn_rect.bottom);
                                // glEnd();
                            } else {
                                if( huds_top(options)) {
                                    // draw minor ticks
                                    drawOneLine( marker_xs, marker_ys,
                                                 marker_xs, marker_ye - 4);
                                } else {
                                    drawOneLine( marker_xs, marker_ys + 4,
                                                 marker_xs, marker_ye);
                                }
                            }
                        }
                    }
                }
		// printf("<***> i = %d\n", i);
                if( div_max()) {
                    // printf("i = %d\n", i);
                    //          if( (i%(int)div_max())==0 ) {
                    if( !(i%(int)div_max()) ) {           
                        if(modulo()) {
                            if( disp_val < 0) {
                                while(disp_val<0)
                                    disp_val += modulo();
                            }
                            disp_val = i % (int) modulo(); // ?????????
                        } else {
                            disp_val = i;
                        }
			// printf("disp_val = %d\n", disp_val);
			// printf("%d\n", (int)(disp_val  * (double)data_scaling() + 0.5));
                        lenstr = sprintf( TextScale, "%d",
                                          // (int)(disp_val  * data_scaling() +.5));
                                          FloatToInt(disp_val * data_scaling()/*+.5*/));
                        // Draw major ticks and text only if far enough from the edge.
                        if(( (marker_xs - 10)> scrn_rect.left ) &&
                           ( (marker_xs + 10) < (scrn_rect.left + scrn_rect.right))){
                            if( huds_both(options) ) {
                                // drawOneLine( marker_xs, scrn_rect.top,
                                //              marker_xs, marker_ys);
                                // drawOneLine( marker_xs, marker_ye,
                                //              marker_xs, scrn_rect.top + scrn_rect.bottom);
                                glBegin(GL_LINE_STRIP);
                                glVertex2f( marker_xs, scrn_rect.top);
                                glVertex2f( marker_xs, marker_ye);
                                glVertex2f( marker_xs, height);
                                glEnd();
                                if( !huds_notext(options) ) {
                                    textString ( marker_xs - 4 * lenstr,
                                                 marker_ys + 4,
                                                 TextScale,  GLUT_BITMAP_8_BY_13 );
                                }
                            } else {
                                drawOneLine( marker_xs, marker_ys,
                                             marker_xs, marker_ye );
                                if( !huds_notext(options)) {
                                    if( huds_top(options) )              {
                                        textString ( marker_xs - 4 * lenstr,
                                                     height - 10,
                                                     TextScale, GLUT_BITMAP_8_BY_13 );
                                    } else  {
                                        textString( marker_xs - 4 * lenstr,
                                                    scrn_rect.top,
                                                    TextScale, GLUT_BITMAP_8_BY_13 );
                                    }
                                }
                            }
                        }
                    }
                }
		// printf("<****> i = %d\n", i);
            }
        }
    }
} //draw
