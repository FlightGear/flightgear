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
#include <Math/polar3d.h>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>
#include <Weather/weather.h>


#include "hud.hxx"
//====================== Top of HudLadder Class =======================
HudLadder ::
  HudLadder(  int       x,
              int       y,
              UINT      width,
              UINT      height,
              DBLFNPTR  ptch_source,
              DBLFNPTR  roll_source,
              double    span_units,
              double    major_div,
              double    minor_div,
              UINT      screen_hole,
              UINT      lbl_pos,
              bool      working) :
               dual_instr_item( x, y, width, height,
                                ptch_source,
                                roll_source,
                                working,
                                HUDS_RIGHT),
               width_units    ( span_units   ),
               div_units      ( major_div < 0? -major_div: major_div ),
               minor_div      ( minor_div    ),
               label_pos      ( lbl_pos      ),
               scr_hole       ( screen_hole  ),
               vmax           ( span_units/2 ),
               vmin           ( -vmax        )
{
  if( !width_units ) {
    width_units = 45;
    }
  factor = (double)get_span() / (double) width_units;
}

HudLadder ::
  ~HudLadder()
{
}

HudLadder ::
  HudLadder( const HudLadder & image ) :
        dual_instr_item( (dual_instr_item &) image),
        width_units    ( image.width_units   ),
        div_units      ( image.div_units     ),
        label_pos      ( image.label_pos     ),
        scr_hole       ( image.scr_hole      ),
        vmax           ( image.vmax ),
        vmin           ( image.vmin ),
        factor         ( image.factor        )
{
}
HudLadder & HudLadder ::
  operator = ( const HudLadder & rhs )
{
  if( !(this == &rhs)) {
    (dual_instr_item &)(*this) = (dual_instr_item &)rhs;
    width_units  = rhs.width_units;
    div_units    = rhs.div_units;
    label_pos    = rhs.label_pos;
    scr_hole     = rhs.scr_hole;
    vmax         = rhs.vmax;
    vmin         = rhs.vmin;
    factor       = rhs.factor;
    }
  return *this;
}

//
//	Draws a climb ladder in the center of the HUD
//

void HudLadder :: draw( void )
{
  double roll_value;
  double pitch_value;
  int    marker_y;
  int    x_ini;
  int    x_end;
  int    y_ini;
  int    y_end;
  int    new_x_ini;
  int    new_x_end;
  int    new_y_ini;
  int    new_y_end;
  int    i;
  POINT  centroid   = get_centroid();
  RECT   box        = get_location();
  int    scr_min    = box.top;
  int    half_span  = box.right >> 1;
  char   TextLadder[80];
  int    condition;
  int    label_length;
  roll_value        = current_ch2();
  GLfloat sinRoll   = sin( roll_value );
  GLfloat cosRoll   = cos( roll_value );

  pitch_value       = current_ch1() * RAD_TO_DEG;
  vmin              = pitch_value - (double)width_units/2.0;
  vmax              = pitch_value + (double)width_units/2.0;

// Box the target.
  drawOneLine( centroid.x - 5, centroid.y,     centroid.x,     centroid.y + 5);
  drawOneLine( centroid.x,     centroid.y + 5, centroid.x + 5, centroid.y);
  drawOneLine( centroid.x + 5, centroid.y,     centroid.x,     centroid.y - 5);
  drawOneLine( centroid.x,     centroid.y - 5, centroid.x - 5, centroid.y);

  for( i=(int)vmin; i<=(int)vmax; i++ )  {  // Through integer pitch values...
    condition = 1;
    if( condition )      {
      marker_y = centroid.y + (int)(((double)(i - pitch_value) * factor) + .5);
      if( div_units ) {
        if( !(i % div_units ))    {        //  At integral multiple of div
          sprintf( TextLadder, "%d", i );
          label_length = strlen( TextLadder );
          if( scr_hole == 0 )           {
            if( i ) {
              x_ini = centroid.x - half_span;
              }
            else {                         // Make zero point wider on left
              x_ini = centroid.x - half_span - 10;
              }
            y_ini = marker_y;
            x_end = centroid.x + half_span;
            y_end = marker_y;
            new_x_ini = centroid.x + (int)(
                       (x_ini - centroid.x) * cosRoll -
                       (y_ini - centroid.y) * sinRoll);
            new_y_ini = centroid.y + (int)(             \
                       (x_ini - centroid.x) * sinRoll + \
                       (y_ini - centroid.y) * cosRoll);
            new_x_end = centroid.x + (int)(             \
                       (x_end - centroid.x) * cosRoll - \
                       (y_end - centroid.y) * sinRoll);
            new_y_end = centroid.y + (int)(             \
                       (x_end - centroid.x) * sinRoll + \
                       (y_end - centroid.y) * cosRoll);

            if( i >= 0 ) { // Above zero draw solid lines
              drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
              }
            else         { // Below zero draw dashed lines.
              glEnable(GL_LINE_STIPPLE);
              glLineStipple( 1, 0x00FF );
              drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
              glDisable(GL_LINE_STIPPLE);
              }
            // Calculate the position of the left text and write it.
            new_x_ini = centroid.x + (int)(
                        (x_ini - 8 * label_length- 4 - centroid.x) * cosRoll -
                        (y_ini - 4 ) * sinRoll);
            new_y_ini = centroid.y + (int)(
                        (x_ini - 8 * label_length- 4 - centroid.x) * sinRoll +
                        (y_ini - 4 - centroid.y) * cosRoll);
            strokeString( new_x_ini , new_y_ini ,
                        TextLadder, GLUT_STROKE_ROMAN,
                        roll_value );

            // Calculate the position of the right text and write it.
            new_x_end = centroid.x + (int)(                  \
                       (x_end + 24 - 8 * label_length - centroid.x) * cosRoll - \
                       (y_end -  4 - centroid.y) * sinRoll);
            new_y_end = centroid.y + (int)(                  \
                       (x_end + 24 - 8 * label_length - centroid.x) * sinRoll + \
                       (y_end -  4 - centroid.y) * cosRoll);
            strokeString( new_x_end,  new_y_end,
                          TextLadder, GLUT_STROKE_ROMAN,
                          roll_value );
            }
          else   {  // Draw ladder with space in the middle of the lines
                    // Start by calculating the points and drawing the
                    // left side lines.
            if( i != 0 )  {
              x_ini = centroid.x - half_span;
              }
            else          {
              x_ini = centroid.x - half_span - 10;
              }
            y_ini = marker_y;
            x_end = centroid.x - half_span + scr_hole/2;
            y_end = marker_y;

            new_x_end = centroid.x+  (int)(             \
                        (x_end - centroid.x) * cosRoll -\
                        (y_end - centroid.y) * sinRoll);
            new_y_end = centroid.y+ (int)(              \
                        (x_end - centroid.x) * sinRoll +\
                        (y_end - centroid.y) * cosRoll);
            new_x_ini = centroid.x + (int)(              \
                        (x_ini - centroid.x) * cosRoll -\
                        (y_ini - centroid.y) * sinRoll);
            new_y_ini = centroid.y + (int)(             \
                        (x_ini - centroid.x) * sinRoll +\
                        (y_ini - centroid.y) * cosRoll);

            if( i >= 0 )
              {
              drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
              }
            else  {
              glEnable(GL_LINE_STIPPLE);
              glLineStipple( 1, 0x00FF );
              drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
              glDisable(GL_LINE_STIPPLE);
              }
            // Now calculate the location of the left side label using
            // the previously calculated start of the left side line.

            x_ini = x_ini - (label_length + 32 + centroid.x);
            if( i < 0) {
              x_ini -= 8;
              }
            else {
              if( i == 0 ) {
                x_ini += 20;
                }
              }
            y_ini = y_ini - ( 4 + centroid.y);

            new_x_ini = centroid.x + (int)(x_ini * cosRoll - y_ini * sinRoll);
            new_y_ini = centroid.y + (int)(x_ini * sinRoll + y_ini * cosRoll);
            strokeString( new_x_ini , new_y_ini ,
                          TextLadder, GLUT_STROKE_MONO_ROMAN,
                          roll_value );

            // Now calculate and draw the right side line location
            x_ini = centroid.x + half_span - scr_hole/2;
            y_ini = marker_y;
            if( i != 0 )  {
              x_end = centroid.x + half_span;
              }
            else          {
              x_end = centroid.x + half_span + 10;
              }
            y_end = marker_y;

            new_x_ini = centroid.x + (int)(         \
                        (x_ini-centroid.x)*cosRoll -\
                        (y_ini-centroid.y)*sinRoll);
            new_y_ini = centroid.y + (int)(         \
                        (x_ini-centroid.x)*sinRoll +\
                        (y_ini-centroid.y)*cosRoll);
            new_x_end = centroid.x + (int)(         \
                        (x_end-centroid.x)*cosRoll -\
                        (y_end-centroid.y)*sinRoll);
            new_y_end = centroid.y +  (int)(        \
                        (x_end-centroid.x)*sinRoll +\
                        (y_end-centroid.y)*cosRoll);

            if( i >= 0 )
              {
              drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
              }
            else
              {
              glEnable(GL_LINE_STIPPLE);
              glLineStipple( 1, 0x00FF );
              drawOneLine( new_x_ini, new_y_ini, new_x_end, new_y_end );
              glDisable(GL_LINE_STIPPLE);
              }

            // Calculate the location and draw the right side label
            // using the end of the line as previously calculated.
            x_end -= centroid.x + label_length - 24;
            if( i < 0 ) {
              x_end -= 8;
              }
            y_end  = marker_y - ( 4 + centroid.y);
            new_x_end = centroid.x + (int)( (GLfloat)x_end * cosRoll -
                                            (GLfloat)y_end * sinRoll);
            new_y_end = centroid.y + (int)( (GLfloat)x_end * sinRoll +
                                            (GLfloat)y_end * cosRoll);
            strokeString( new_x_end,  new_y_end,
                          TextLadder, GLUT_STROKE_MONO_ROMAN,
                          roll_value );
            }
          }
        }
      }
    }
}

