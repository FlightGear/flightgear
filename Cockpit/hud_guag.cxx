#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <Aircraft/aircraft.h>
#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>
#include <Weather/weather.h>


#include "hud.hxx"
//============== Top of guage_instr class member definitions ==============

guage_instr ::
    guage_instr( int          x,
                 int          y,
                 UINT         width,
                 UINT         height,
                 DBLFNPTR     load_fn,
                 UINT         options,
                 double       disp_scale,
                 double       maxValue,
                 double       minValue,
                 UINT         major_divs,
                 UINT         minor_divs,
                 int          dp_showing,
                 UINT         modulus,
                 bool         working) :
           instr_scale( x, y, width, height,
                        load_fn, options,
                        (maxValue - minValue), // Always shows span?
                        maxValue, minValue,
                        disp_scale,
                        major_divs, minor_divs,
                        modulus, dp_showing,
                        working)
{
}

guage_instr ::
   ~guage_instr()
{
}

guage_instr ::
    guage_instr( const guage_instr & image):
       instr_scale( (instr_scale &) image)
{
}

guage_instr & guage_instr ::
    operator = (const guage_instr & rhs )
{
  if( !(this == &rhs)) {
    instr_scale::operator = (rhs);
    }
  return *this;
}

// As implemented, draw only correctly draws a horizontal or vertical
// scale. It should contain a variation that permits clock type displays.
// Now is supports "tickless" displays such as control surface indicators.
// This routine should be worked over before using. Current value would be
// fetched and not used if not commented out. Clearly that is intollerable.

void guage_instr :: draw (void)
{
  int marker_xs, marker_xe;
  int marker_ys, marker_ye;
  int text_x, text_y;
  register i;
  char TextScale[80];
  bool condition;
  int disp_val;
  double vmin         = min_val();
  double vmax         = max_val();
  POINT mid_scr       = get_centroid();
  double cur_value    = get_value();
  RECT   scrn_rect    = get_location();
  UINT options        = get_options();

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

    if( options & HUDS_LEFT ) {     // Read left, so line down right side
      drawOneLine( scrn_rect.left + scrn_rect.right,
                   scrn_rect.top,
                   scrn_rect.left + scrn_rect.right,
                   scrn_rect.top + scrn_rect.bottom);

      marker_xs  = marker_xe - scrn_rect.right / 3;   // Adjust tick xs
      }
    if( options & HUDS_RIGHT ) {     // Read  right, so down left sides
      drawOneLine( scrn_rect.left,
                   scrn_rect.top,
                   scrn_rect.left,
                   scrn_rect.top + scrn_rect.bottom);
      marker_xe = scrn_rect.left + scrn_rect.right / 3;     // Adjust tick xe
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

    if( !(options & HUDS_NOTICKS )) {    // If not no ticks...:)
                                          // Calculate x marker offsets
      for( i = (int)vmin; i <= (int)vmax; i++ ) {

        // Calculate the location of this tick
        marker_ys = scrn_rect.top + (int)((i - vmin) * factor() + .5);

        // We compute marker_ys even though we don't know if we will use
        // either major or minor divisions. Simpler.

        if( div_min()) {                  // Minor tick marks
          if( (i%div_min()) == 0) {
            if((options & HUDS_LEFT) && (options && HUDS_RIGHT)) {
                drawOneLine( scrn_rect.left, marker_ys,
                             marker_xs - 3, marker_ys );
                drawOneLine( marker_xe + 3, marker_ys,
                             scrn_rect.left + scrn_rect.right, marker_ys );
              }
            else {
              if( options & HUDS_LEFT) {
                drawOneLine( marker_xs + 3, marker_ys, marker_xe, marker_ys );
                }
              else {
                drawOneLine( marker_xs, marker_ys, marker_xe - 3, marker_ys );
               }
              }
            }
          }

          // Now we work on the major divisions. Since these are also labeled
          // and no labels are drawn otherwise, we label inside this if
          // statement.

        if( div_max()) {                  // Major tick mark
          if( (i%div_max()) == 0 )            {
            if((options & HUDS_LEFT) && (options && HUDS_RIGHT)) {

                drawOneLine( scrn_rect.left, marker_ys,
                             marker_xs, marker_ys );
                drawOneLine( marker_xe, marker_ys,
                             scrn_rect.left + scrn_rect.right, marker_ys );
              }
            else {
              drawOneLine( marker_xs, marker_ys, marker_xe, marker_ys );
              }

            if( !(options & HUDS_NOTEXT)) {
              disp_val = i;
              sprintf( TextScale, "%d",disp_val  * (int)(data_scaling() +.5));

              if((options & HUDS_LEFT) && (options && HUDS_RIGHT)) {
                text_x = mid_scr.x -  2 - ((3 * strlen( TextScale ))>>1);
                }
              else {
                if( options & HUDS_LEFT )              {
                  text_x = marker_xs - 2 - 3 * strlen( TextScale);
                  }
                else {
                  text_x = marker_xe + 10 - strlen( TextScale );
                  }
                }
              // Now we know where to put the text.
              text_y = marker_ys;
              textString( text_x, text_y, TextScale, GLUT_BITMAP_8_BY_13 );
              }
            }
          }  //
        }  //
      }  //
    // Now that the scale is drawn, we draw in the pointer(s). Since labels
    // have been drawn, text_x and text_y may be recycled. This is used
    // with the marker start stops to produce a pointer for each side reading

    text_y = scrn_rect.top + (int)((cur_value - vmin) * factor() + .5);
//    text_x = marker_xs - scrn_rect.left;

    if( options & HUDS_RIGHT ) {
      drawOneLine(scrn_rect.left, text_y + 5,
                  marker_xe,      text_y);
      drawOneLine(scrn_rect.left, text_y - 5,
                  marker_xe,      text_y);
      }
    if( options & HUDS_LEFT ) {
      drawOneLine(scrn_rect.left + scrn_rect.right, text_y + 5,
                  marker_xs,                        text_y);
      drawOneLine(scrn_rect.left + scrn_rect.right, text_y - 5,
                  marker_xs,                        text_y);
      }
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
    marker_xs = scrn_rect.left + (int)((cur_value - vmin) * factor() + .5);

    if( options & HUDS_TOP ) {
      drawOneLine( scrn_rect.left,
                   scrn_rect.top,
                   scrn_rect.left + scrn_rect.right,
                   scrn_rect.top);                    // Bottom box line

      marker_ye  = scrn_rect.top + scrn_rect.bottom / 2;   // Tick point adjust
                                                      // Bottom arrow
      drawOneLine( marker_xs, marker_ye,
                   marker_xs - scrn_rect.bottom / 4, scrn_rect.top);
      drawOneLine( marker_xs, marker_ye,
                   marker_xs + scrn_rect.bottom / 4, scrn_rect.top);
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
      drawOneLine( marker_xs + scrn_rect.bottom / 4,
                          scrn_rect.top + scrn_rect.bottom,
                   marker_xs,
                          marker_ys );
      drawOneLine( marker_xs - scrn_rect.bottom / 4,
                          scrn_rect.top + scrn_rect.bottom,
                   marker_xs ,
                          marker_ys );
      }

    for( i = (int)vmin; i <= (int)vmax; i++ )   {
      condition = true;
      if( !modulo()) {
        if( i < min_val()) {
          condition = false;
          }
        }
      if( condition )        {
        marker_xs = scrn_rect.left + (int)((i - vmin) * factor() + .5);
        if( div_min()){
          if( (i%(int)div_min()) == 0 ) {
            // draw in ticks only if they aren't too close to the edge.
            if((( marker_xs + 5) > scrn_rect.left ) ||
               (( marker_xs - 5 )< (scrn_rect.left + scrn_rect.right))){

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
        if( div_max()) {
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
        }
      }
    }
}


