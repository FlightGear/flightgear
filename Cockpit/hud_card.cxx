#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <Aircraft/aircraft.hxx>
#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>


#include "hud.hxx"
//========== Top of hud_card class member definitions =============

hud_card ::
hud_card( int       x,
          int       y,
          UINT      width,
          UINT      height,
          DBLFNPTR  data_source,
          UINT      options,
          double    max_value,
          double    min_value,
          double    disp_scaling,
          UINT      major_divs,
          UINT      minor_divs,
          UINT      modulus,
          int       dp_showing,
          double    value_span,
          bool      working) :
                instr_scale( x,y,width,height,
                             data_source, options,
                             value_span,
                             max_value, min_value, disp_scaling,
                             major_divs, minor_divs, modulus,
                             working),
                val_span   ( value_span)
{
  half_width_units = range_to_show() / 2.0;
}

hud_card ::
~hud_card() { }

hud_card ::
hud_card( const hud_card & image):
      instr_scale( (const instr_scale & ) image),
      val_span( image.val_span),
      half_width_units (image.half_width_units)
{
}

hud_card & hud_card ::
operator = (const hud_card & rhs )
{
  if( !( this == &rhs)){
    instr_scale::operator = (rhs);
    val_span = rhs.val_span;
    half_width_units = rhs.half_width_units;
    }
  return *this;
}

void hud_card ::
draw( void ) //  (HUD_scale * pscale )
{
  double vmin, vmax;
  int marker_xs;
  int marker_xe;
  int marker_ys;
  int marker_ye;
  /* register */ int i;
  char TextScale[80];
  bool condition;
  int disp_val = 0;
  POINT mid_scr    = get_centroid();
  double cur_value = get_value();
  RECT   scrn_rect = get_location();
  UINT options     = get_options();

  vmin = cur_value - half_width_units; // width units == needle travel
  vmax = cur_value + half_width_units; // or picture unit span.
    // Draw the basic markings for the scale...

  if( options & HUDS_VERT ) { // Vertical scale
    drawOneLine( scrn_rect.left,     // Bottom tick bar
                 scrn_rect.top,
                 scrn_rect.left + scrn_rect.right,
                 scrn_rect.top);

    drawOneLine( scrn_rect.left,    // Top tick bar
                 scrn_rect.top  + scrn_rect.bottom,
                 scrn_rect.left + scrn_rect.right,
                 scrn_rect.top  + scrn_rect.bottom );

    marker_xs = scrn_rect.left;
    marker_xe = scrn_rect.left + scrn_rect.right;

    // We do not use else in the following so that combining the two
    // options produces a "caged" display with double carrots. The
    // same is done for horizontal card indicators.

    if( options & HUDS_LEFT ) {    // Calculate x marker offset
      drawOneLine( scrn_rect.left + scrn_rect.right,
                   scrn_rect.top,
                   scrn_rect.left + scrn_rect.right,
                   scrn_rect.top + scrn_rect.bottom); // Cap right side

      marker_xs  = marker_xe - scrn_rect.right / 3;   // Adjust tick xs
                                                      // Indicator carrot
      drawOneLine( marker_xs, mid_scr.y,
                   marker_xe, mid_scr.y + scrn_rect.right / 6);
      drawOneLine( marker_xs, mid_scr.y,
                   marker_xe, mid_scr.y - scrn_rect.right / 6);

      }
    if( options & HUDS_RIGHT ) {  // We'll default this for now.
      drawOneLine( scrn_rect.left,
                   scrn_rect.top,
                   scrn_rect.left,
                   scrn_rect.top + scrn_rect.bottom);  // Cap left side

      marker_xe = scrn_rect.left + scrn_rect.right / 3;     // Adjust tick xe
                                                       // Indicator carrot
      drawOneLine( scrn_rect.left, mid_scr.y +  scrn_rect.right / 6,
                   marker_xe, mid_scr.y );
      drawOneLine( scrn_rect.left, mid_scr.y -  scrn_rect.right / 6,
                   marker_xe, mid_scr.y);
      }

    // At this point marker x_start and x_end values are transposed.
    // To keep this from confusing things they are now interchanged.
    if(( options & HUDS_BOTH) == HUDS_BOTH) {
      marker_ye = marker_xs;
      marker_xs = marker_xe;
      marker_xe = marker_ye;
      }

    // Work through from bottom to top of scale. Calculating where to put
    // minor and major ticks.

    for( i = (int)vmin; i <= (int)vmax; i++ )      {
      condition = true;
      if( !modulo()) {
        if( i < min_val()) {
          condition = false;
          }
        }

      if( condition ) {  // Show a tick if necessary
                         // Calculate the location of this tick
        marker_ys = scrn_rect.top + (int)((i - vmin) * factor() + .5);

        // Block calculation artifact from drawing ticks below min coordinate.
        // Calculation here accounts for text height.

        if(( marker_ys < (scrn_rect.top + 4)) |
           ( marker_ys > (scrn_rect.top + scrn_rect.bottom - 4))) {
            // Magic numbers!!!
          continue;
          }
        if( div_min()) {
          if( (i%div_min()) == 0) {
            if((( marker_ys - 5) > scrn_rect.top ) &&
               (( marker_ys + 5) < (scrn_rect.top + scrn_rect.bottom))){
              if( (options & HUDS_BOTH) == HUDS_BOTH ) {
                drawOneLine( scrn_rect.left, marker_ys,
                             marker_xs,      marker_ys );
                drawOneLine( marker_xe,      marker_ys,
                             scrn_rect.left + scrn_rect.right,  marker_ys );
                }
              else {
                if( options & HUDS_LEFT ) {
                  drawOneLine( marker_xs + 4, marker_ys,
                               marker_xe,     marker_ys );
                  }
                else {
                  drawOneLine( marker_xs,     marker_ys,
                               marker_xe - 4, marker_ys );
                  }
                }
              }
            }
          }
        if( div_max()) {
          if( !(i%(int)div_max())){
            if(modulo()) {
              if( disp_val < 0) {
                disp_val += modulo();
                }
              else {
                disp_val = i % modulo();
                }
              }
            else {
              disp_val = i;
              }
            sprintf( TextScale, "%d", (int)(disp_val  * data_scaling() +.5));
            if(( (marker_ys - 8 ) > scrn_rect.top ) &&
               ( (marker_ys + 8) < (scrn_rect.top + scrn_rect.bottom))){
              if( (options & HUDS_BOTH) == HUDS_BOTH) {
                drawOneLine( scrn_rect.left, marker_ys,
                             marker_xs,      marker_ys);
                drawOneLine( marker_xs,                  marker_ys,
                             scrn_rect.left + scrn_rect.right,
                             marker_ys);
                if( !(options & HUDS_NOTEXT)) {
                  textString ( marker_xs + 2,  marker_ys,
                               TextScale,  GLUT_BITMAP_8_BY_13 );
                  }
                }
              else {
                drawOneLine( marker_xs, marker_ys, marker_xe, marker_ys );
                if( !(options & HUDS_NOTEXT)) {
                  if( options & HUDS_LEFT )              {
                    textString( marker_xs -  8 * strlen(TextScale) - 2,
                                marker_ys - 4,
                                TextScale, GLUT_BITMAP_8_BY_13 );
                    }
                  else  {
                    textString( marker_xe + 3 * strlen(TextScale),
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
    drawOneLine( scrn_rect.left,     // left tick bar
                 scrn_rect.top,
                 scrn_rect.left,
                 scrn_rect.top + scrn_rect.bottom);

    drawOneLine( scrn_rect.left + scrn_rect.right,    // right tick bar
                 scrn_rect.top,
                 scrn_rect.left + scrn_rect.right,
                 scrn_rect.top  + scrn_rect.bottom );

    marker_ys = scrn_rect.top;                       // Starting point for
    marker_ye = scrn_rect.top + scrn_rect.bottom;    // tick y location calcs

    if( options & HUDS_TOP ) {
      drawOneLine( scrn_rect.left,
                   scrn_rect.top,
                   scrn_rect.left + scrn_rect.right,
                   scrn_rect.top);                    // Bottom box line

      marker_ye  = scrn_rect.top + scrn_rect.bottom / 2;   // Tick point adjust
                                                      // Bottom arrow
      drawOneLine( mid_scr.x, marker_ye,
                   mid_scr.x - scrn_rect.bottom / 4, scrn_rect.top);
      drawOneLine( mid_scr.x, marker_ye,
                   mid_scr.x + scrn_rect.bottom / 4, scrn_rect.top);
      }
    if( options & HUDS_BOTTOM) {
      drawOneLine( scrn_rect.left,
                   scrn_rect.top + scrn_rect.bottom,
                   scrn_rect.left + scrn_rect.right,
                   scrn_rect.top + scrn_rect.bottom);  // Top box line
                                                       // Tick point adjust
      marker_ys = scrn_rect.top +
                  scrn_rect.bottom - scrn_rect.bottom  / 2;
                                                       // Top arrow
      drawOneLine( mid_scr.x + scrn_rect.bottom / 4,
                          scrn_rect.top + scrn_rect.bottom,
                   mid_scr.x ,
                          marker_ys );
      drawOneLine( mid_scr.x - scrn_rect.bottom / 4,
                          scrn_rect.top + scrn_rect.bottom,
                   mid_scr.x ,
                          marker_ys );
      }

//    if(( options & HUDS_BOTTOM) == HUDS_BOTTOM ) {
//      marker_xe = marker_ys;
//      marker_ys = marker_ye;
//      marker_ye = marker_xe;
//      }

    // printf("vmin = %d  vmax = %d\n", (int)vmin, (int)vmax);
    for( i = (int)vmin; i <= (int)vmax; i++ )     {
      // printf("<*> i = %d\n", i);
      condition = true;
      if( !modulo()) {
        if( i < min_val()) {
          condition = false;
          }
        }
      // printf("<**> i = %d\n", i);
      if( condition )        {
        marker_xs = scrn_rect.left + (int)((i - vmin) * factor() + .5);
        if( div_min()){
          if( (i%(int)div_min()) == 0 ) {
            // draw in ticks only if they aren't too close to the edge.
            if((( marker_xs - 5) > scrn_rect.left ) &&
               (( marker_xs + 5 )< (scrn_rect.left + scrn_rect.right))){

              if( (options & HUDS_BOTH) == HUDS_BOTH ) {
                drawOneLine( marker_xs, scrn_rect.top,
                             marker_xs, marker_ys - 4);
                drawOneLine( marker_xs, marker_ye + 4,
                             marker_xs, scrn_rect.top + scrn_rect.bottom);
                }
              else {
                if( options & HUDS_TOP) {
                  drawOneLine( marker_xs, marker_ys,
                               marker_xs, marker_ye - 4);
                  }
                else {
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
          if( (i%(int)div_max())==0 ) {
            if(modulo()) {
              if( disp_val < 0) {
                disp_val += modulo();
                }
              else {
                disp_val = i % modulo();
                }
              }
            else {
              disp_val = i;
              }
	    // printf("disp_val = %d\n", disp_val);
	    // printf("%d\n", (int)(disp_val  * (double)data_scaling() + 0.5));
            sprintf( TextScale, "%d", (int)(disp_val  * data_scaling() +.5));
            // Draw major ticks and text only if far enough from the edge.
            if(( (marker_xs - 10)> scrn_rect.left ) &&
               ( (marker_xs + 10) < (scrn_rect.left + scrn_rect.right))){
              if( (options & HUDS_BOTH) == HUDS_BOTH) {
                drawOneLine( marker_xs, scrn_rect.top,
                             marker_xs, marker_ys);
                drawOneLine( marker_xs, marker_ye,
                             marker_xs, scrn_rect.top + scrn_rect.bottom);

                if( !(options & HUDS_NOTEXT)) {
                  textString ( marker_xs - 4 * strlen(TextScale),
                               marker_ys + 4,
                               TextScale,  GLUT_BITMAP_8_BY_13 );
                  }
                }
              else {
                drawOneLine( marker_xs, marker_ys,
                             marker_xs, marker_ye );
                if( !(options & HUDS_NOTEXT)) {
                  if( options & HUDS_TOP )              {
                    textString ( marker_xs - 4 * strlen(TextScale),
                                 scrn_rect.top + scrn_rect.bottom - 10,
                                 TextScale, GLUT_BITMAP_8_BY_13 );
                    }
                  else  {
                    textString( marker_xs - 4 * strlen(TextScale),
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
}

