/**************************************************************************
 * hud.cxx -- hud defines and prototypes
 *
 * Written by Michele America, started September 1997.
 *
 * Copyright (C) 1997  Michele F. America  - micheleamerica@geocities.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_VALUES_H
#  include <values.h>  // for MAXINT
#endif

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

// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

//using namespace std;

/*

class instr_ptr {
  private:
    instr_item *pHI;

  public:
    instr_ptr ( instr_item * pHudInstr = 0) : pHI( pHudInstr){}
    ~instr_ptr() { if( pHI ) delete pHI; }
    instr_ptr & operator = (const instr_ptr & rhs);
    instr_ptr( const instr_ptr & image );
}

instr_ptr &
instr_ptr ::
operator = ( const instr_ptr & rhs )
{
  if( !(this == &rhs )) {
    pHI = new instr_item( *(rhs.pHI));
    }
  return *this;
}

instr_ptr ::
instr_ptr ( const instr_ptr & image ) :
{
  pHI = new instr_item( *(image.pHI));
}
*/
deque< instr_item * > HUD_deque;

class locRECT {
  public:
    RECT rect;

    locRECT( UINT left, UINT top, UINT right, UINT bottom);
    RECT get_rect(void) { return rect;}
};

locRECT :: locRECT( UINT left, UINT top, UINT right, UINT bottom)
{
  rect.left   =  left;
  rect.top    =  top;
  rect.right  =  right;
  rect.bottom =  bottom;

}
// #define DEBUG

// #define drawOneLine(x1,y1,x2,y2)  glBegin(GL_LINES);
//   glVertex2f ((x1),(y1)); glVertex2f ((x2),(y2)); glEnd();
//

void drawOneLine( UINT x1, UINT y1, UINT x2, UINT y2)
{
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}

void drawOneLine( RECT &rect)
{
  glBegin(GL_LINES);
  glVertex2f(rect.left, rect.top);
  glVertex2f(rect.right, rect.bottom);
  glEnd();
}

//
// The following code deals with painting the "instrument" on the display
//
   /* textString - Bitmap font string */

static void textString(int x, int y, char *msg, void *font)
{
	glRasterPos2f(x, y);
	while (*msg) {
		glutBitmapCharacter(font, *msg);
		msg++;
    }
}

/* strokeString - Stroke font string */
/*
static void strokeString(int x, int y, char *msg, void *font)
{
	glPushMatrix();
	glTranslatef(x, y, 0);
	glScalef(.04, .04, .04);
	while (*msg) {
		glutStrokeCharacter(font, *msg);
		msg++;
	}
	glPopMatrix();
}
*/



// Abstract Base Class instr_item
//
UINT instr_item :: instances;  // Initial value of zero

// constructor    ( No default provided )

instr_item  ::
   instr_item( RECT             scr_pos,
               DBLFNPTR         data_source,
               ReadOriented     orientation,
               bool             working) :
                      handle         ( ++instances  ),
                      scrn_pos       ( scr_pos      ),
                      load_value_fn  ( data_source  ),
                      oriented       ( orientation  ),
                      is_enabled     ( working      ),
                      broken         ( FALSE        ),
                      brightness     ( BRT_DARK     )
{
int hitemp, widetemp;
int xtemp;

         // Check for inverted rect coords.  We must have top/right > bottom
         // left corners or derived classes will draw their boxes very oddly.
         // This is the only place we can reliably validate this data. Note
         // that the box may be entirely or partly located off the screen.
         // This needs consideration and possibly a member function to move
         // the box, a keen idea for supporting "editing" of the HUD.
  if( scrn_pos.top < scrn_pos.bottom ) {
    xtemp           = scrn_pos.bottom;
    scrn_pos.bottom = scrn_pos.top;
    scrn_pos.top    = xtemp;
    }
  if( scrn_pos.left > scrn_pos.right ) {
    xtemp           = scrn_pos.left;
    scrn_pos.left   = scrn_pos.right;
    scrn_pos.right  = xtemp;
    }

         // Insure that the midpoint marker will fall exactly at the
         // middle of the bar.
  if( !((scr_pos.top-scr_pos.bottom) % 2)) {
    scrn_pos.top++;
    }
  if( !((scr_pos.right - scr_pos.left ) % 2)) {
    scrn_pos.right++;
    }
         // Set up convenience values for centroid of the box and
         // the span values according to orientation

  hitemp   = scr_pos.top - scr_pos.bottom;
  widetemp = scr_pos.right - scr_pos.left;
  if((orientation == ReadTOP) || (orientation == ReadBOTTOM)) {
    scr_span = widetemp;
    }
  else {
    scr_span = hitemp;
    }
         // Here we work out the centroid for the corrected box.
  mid_span.x = scr_pos.left   + ((scr_pos.right - scr_pos.left) >> 1);
  mid_span.y = scr_pos.bottom + ((scr_pos.top - scr_pos.bottom) >> 1);
}

// copy constructor
instr_item  ::
     instr_item ( const instr_item & image ):
                         handle       ( ++instances        ),
                         scrn_pos     ( image.scrn_pos     ),
                         load_value_fn( image.load_value_fn),
                         oriented     ( image.oriented     ),
                         is_enabled   ( image.is_enabled   ),
                         broken       ( image.broken       ),
                         brightness   ( image.brightness   ),
                         scr_span     ( image.scr_span     ),
                         mid_span     ( image.mid_span     )
{
}

// assignment operator

instr_item & instr_item :: operator = ( const instr_item & rhs )
{
  if( !(this == &rhs )) { // Not an identity assignment
    scrn_pos      = rhs.scrn_pos;
    load_value_fn = rhs.load_value_fn;
    oriented      = rhs.oriented;
    is_enabled    = rhs.is_enabled;
    broken        = rhs.broken;
    brightness    = rhs.brightness;
    }
  return *this;
}

// destructor

instr_item :: ~instr_item ()
{
  if( instances ) {
    instances--;
    }
}

void instr_item ::
    update( void )
{
}

// break_display       This is emplaced to provide hooks for making
//                     instruments unreliable. The default behavior is
// to simply not display, but more sophisticated behavior is available
// by over riding the function which is virtual in this class.

void instr_item ::
    break_display ( bool bad )
{
  broken = !!bad;
  is_enabled = FALSE;
}

void instr_item ::
    SetBrightness  ( int level  )
{
  brightness = level;   // This is all we will do for now. Later the
                        // brightness levels will be sensitive both to
                        // the control knob and the outside light levels
                        // to emulated night vision effects.
}

UINT instr_item :: get_Handle( void )
{
  return handle;
}

//======================= Top of instr_label class =========================
instr_label ::
         instr_label( RECT          region,
                      DBLFNPTR      data_source,
                      const char   *label_format,
                      const char   *pre_label_string,
                      const char   *post_label_string,
                      ReadOriented  orientation,
                      fgLabelJust   justification,
                      int           font_size,
                      int           blinking,
                      bool          working ):
                           instr_item( region, data_source,
                                            orientation, working ),
                           pformat  ( label_format      ),
                           pre_str  ( pre_label_string  ),
                           post_str ( post_label_string ),
                           justify  ( justification     ),
                           fontSize ( font_size         ),
                           blink    ( blinking          )
{
}

// I put this in to make it easy to construct a class member using the current
// C code.


instr_label :: ~instr_label()
{
}

// Copy constructor
instr_label :: instr_label( const instr_label & image) :
                              instr_item((const instr_item &)image),
                              pformat    ( image.pformat    ),
                              pre_str  ( image.pre_str  ),
                              post_str ( image.post_str ),
                              blink    ( image.blink    )
{
}

instr_label & instr_label ::operator = (const instr_label & rhs )
{
  if( !(this == &rhs)) {
    instr_item::operator = (rhs);
    pformat      = rhs.pformat;
    fontSize   = rhs.fontSize;
    blink      = rhs.blink;
    justify    = rhs.justify;
    pre_str    = rhs.pre_str;
    post_str   = rhs.post_str;
    }
	return *this;
}


//
// draw                    Draws a label anywhere in the HUD
//
//
void instr_label ::
draw( void )       // Required method in base class
{
  char format_buffer[80];
  char label_buffer[80];
  int posincr;
  int lenstr;
  RECT  scrn_rect = get_location();

  if( pre_str != NULL) {
    if( post_str != NULL ) {
      sprintf( format_buffer, "%s%s%s", pre_str, pformat, post_str );
      }
    else {
      sprintf( format_buffer, "%s%s",   pre_str, pformat );
      }
    }
  else {
    if( post_str != NULL ) {
      sprintf( format_buffer, "%s%s",   pformat, post_str );
      }
    } // else do nothing if both pre and post strings are nulls. Interesting.

  sprintf( label_buffer, format_buffer, get_value() );
#ifdef DEBUGHUD
	fgPrintf( FG_COCKPIT, FG_DEBUG,  format_buffer );
	fgPrintf( FG_COCKPIT, FG_DEBUG,  "\n" );
	fgPrintf( FG_COCKPIT, FG_DEBUG, label_buffer );
	fgPrintf( FG_COCKPIT, FG_DEBUG, "\n" );
#endif
  lenstr = strlen( label_buffer );

  posincr = 0;   //  default to RIGHT_JUST ... center located calc: -lenstr*8;

  if( justify == CENTER_JUST ) {
    posincr =  - (lenstr << 2); //  -lenstr*4;
    }
  else {
    if( justify == LEFT_JUST ) {
      posincr = - (lenstr << 8);  // 0;
      }
    }

  if( fontSize == SMALL ) {
    textString( scrn_rect.left + posincr, scrn_rect.bottom,
                label_buffer, GLUT_BITMAP_8_BY_13);
    }
  else  {
    if( fontSize == LARGE ) {
      textString( scrn_rect.left + posincr, scrn_rect.bottom,
                  label_buffer, GLUT_BITMAP_9_BY_15);
      }
    }
}

//============== Top of instr_scale class memeber definitions ===============
//
// Notes:
// 1. instr_scales divide the specified location into half and then
//    the half opposite the read direction in half again. A bar is
//    then drawn along the second divider. Scale ticks are drawn
//    between the middle and quarter section lines (minor division
//    markers) or just over the middle line.
//
// 2.  This class was not intended to be instanciated. See moving_scale
//     and guage_instr classes.
//============================================================================
instr_scale ::
instr_scale ( RECT         the_box,
              DBLFNPTR     load_fn,
              ReadOriented orient,
              int          show_range,
              int          maxValue,
              int          minValue,
              UINT         major_divs,
              UINT         minor_divs,
              UINT         rollover,
              bool         working ) :
                instr_item( the_box, load_fn, orient, working),
                range_shown  ( show_range ),
                Maximum_value( maxValue   ),
                Minimum_value( minValue   ),
                Maj_div      ( major_divs ),
                Min_div      ( minor_divs ),
                Modulo       ( rollover   )
{
int temp;

  scale_factor   = (double)get_span() / range_shown;
  if( show_range < 0 ) {
    range_shown = -range_shown;
    }
  temp = (Maximum_value - Minimum_value) / 100;
  if( range_shown < temp ) {
    range_shown = temp;
    }
}

instr_scale ::
  instr_scale( const instr_scale & image ) :
            instr_item( (const instr_item &) image),
            range_shown  ( image.range_shown   ),
            Maximum_value( image.Maximum_value ),
            Minimum_value( image.Minimum_value ),
            Maj_div      ( image.Maj_div       ),
            Min_div      ( image.Min_div       ),
            Modulo       ( image.Modulo        ),
            scale_factor ( image.scale_factor  )
{
}

instr_scale & instr_scale :: operator = (const instr_scale & rhs )
{
  if( !(this == &rhs)) {
    instr_item::operator = (rhs);
    Minimum_value = rhs.Minimum_value;
    Maximum_value = rhs.Maximum_value;
    Maj_div       = rhs.Maj_div;
    Min_div       = rhs.Min_div;
    Modulo        = rhs.Modulo;
    scale_factor  = rhs.scale_factor;
    range_shown   = rhs.range_shown;
    }
  return *this;
}

instr_scale :: ~ instr_scale () {}

//========== Top of moving_scale_instr class member definitions =============

moving_scale ::
moving_scale( RECT         the_box,
              DBLFNPTR     data_source,
              ReadOriented orientation,
              int          max_value,
              int          min_value,
              UINT         major_divs,
              UINT         minor_divs,
              UINT         modulus,
              double       value_span,
              bool         working) :
                instr_scale( the_box,
                             data_source, orientation,
                             (int)value_span,
                             max_value, min_value,
                             major_divs, minor_divs, modulus,
                             working),
                val_span   ( value_span)
{
  half_width_units = range_to_show() / 2.0;
}

moving_scale ::
~moving_scale() { }

moving_scale ::
moving_scale( const moving_scale & image):
      instr_scale( (const instr_scale & ) image)
{
}

moving_scale & moving_scale ::
operator = (const moving_scale & rhs )
{
  if( !( this == &rhs)){
    instr_scale::operator = (rhs);
    }
  return *this;
}

void moving_scale ::
draw( void ) //  (HUD_scale * pscale )
{
  double vmin, vmax;
  int marker_x;
  int marker_y;
  register i;
  char TextScale[80];
  int condition;
  int disp_val;
  POINT mid_scr = get_centroid();
  POINT bias_bar;                  // eliminates some orientation checks
  double cur_value = get_value();
  RECT   scrn_rect = get_location();
  ReadOriented orientation = get_orientation();

  vmin = cur_value - half_width_units; // width units == needle travel
  vmax = cur_value + half_width_units; // or picture unit span.


  if( (orientation == ReadLEFT) ||( orientation == ReadRIGHT))  // Vertical scale
    {
    if( orientation == ReadLEFT ) {    // Calculate x marker offset
      marker_x = scrn_rect.left - 6;
      bias_bar.x = ((scrn_rect.right - mid_scr.x)>>1) + mid_scr.x;
      }
    else {                             // We'll default this for now.
      marker_x = mid_scr.x;            // scrn_rect.right;
      bias_bar.x = ((mid_scr.x - scrn_rect.left)>>1) + scrn_rect.left;
      }

    // Draw the basic markings for the scale...

    drawOneLine( bias_bar.x,    // Vertical scale bar
                 scrn_rect.bottom,
                 bias_bar.x,
                 scrn_rect.top );


    if( orientation == ReadLEFT )
      {

      drawOneLine( bias_bar.x,     // Bottom tick bar
                   scrn_rect.bottom,
                   mid_scr.x,
                   scrn_rect.bottom );

      drawOneLine( bias_bar.x,     // Top tick bar
                   scrn_rect.top,
                   mid_scr.x,
                   scrn_rect.top );

      drawOneLine( scrn_rect.right,       // Middle tick bar /Index
                   mid_scr.y,
                   bias_bar.x,
                   mid_scr.y );

      }
    else       { // ReadRight
      drawOneLine( bias_bar.x,
                   scrn_rect.bottom,
                   mid_scr.x,
                   scrn_rect.bottom );

      drawOneLine( bias_bar.x,
                   scrn_rect.top,
                   mid_scr.x,
                   scrn_rect.top );

      drawOneLine( scrn_rect.left,
                   mid_scr.y,
                   bias_bar.x,
                   mid_scr.y );
      }

    // Work through from bottom to top of scale. Calculating where to put
    // minor and major ticks.

    for( i = (int)vmin; i <= (int)vmax; i++ )
      {
//      if( sub_type == LIMIT ) {           // Don't show ticks
        condition = (i >= min_val()); // below Minimum value.
//        }
//      else {
//        if( sub_type == NOLIMIT ) {
//          condition = 1;
//          }
//        }
      if( condition )   // Show a tick if necessary
        {
        // Calculate the location of this tick
        marker_y = scrn_rect.bottom + (int)((i - vmin) * factor());

        // Block calculation artifact from drawing ticks below min coordinate.
        // Calculation here accounts for text height.

        if( marker_y < (scrn_rect.bottom + 4)) {  // Magic number!!!
          continue;
          }
        if( div_min()) {
          if( (i%div_min()) == 0) {
//            if( orientation == ReadLEFT )
//              {
//              drawOneLine( marker_x + 3, marker_y, marker_x + 6, marker_y );
              drawOneLine( bias_bar.x, marker_y, mid_scr.x, marker_y );
//              }
//            else {
//              if( orientation == ReadRIGHT )
//                {
//                drawOneLine( marker_x, marker_y, marker_x + 3, marker_y );
//                drawOneLine( bias_bar.x, marker_y, mid_scr.x, marker_y );
//                }
//              }
            }
          }
        if( div_max()) {
          if( (i%div_max()) == 0 )            {
//            drawOneLine( marker_x,     marker_y,
//                         marker_x + 6, marker_y );
            if(modulo()) {
              disp_val = i % modulo();
              if( disp_val < 0) {
                disp_val += modulo();
                }
              }
            else {
              disp_val = i;
              }
            sprintf( TextScale, "%d", disp_val );
            if( orientation == ReadLEFT )              {
              drawOneLine( mid_scr.x - 3, marker_y, bias_bar.x, marker_y );

              textString( marker_x -  8 * strlen(TextScale) - 2, marker_y - 4,
                          TextScale, GLUT_BITMAP_8_BY_13 );
              }
            else  {
              drawOneLine( mid_scr.x + 3, marker_y, bias_bar.x, marker_y );
              textString( marker_x + 10,                         marker_y - 4,
                          TextScale, GLUT_BITMAP_8_BY_13 );
              } // Else read oriented right
            } // End if modulo division by major interval is zero
          }  // End if major interval divisor non-zero
        } // End if condition
      } // End for range of i from vmin to vmax
    }  // End if VERTICAL SCALE TYPE
  else {                                // Horizontal scale by default
    {
    if( orientation == ReadTOP ) {
      bias_bar.y = ((mid_scr.y - scrn_rect.bottom)>>1 ) + scrn_rect.bottom;
      marker_y = bias_bar.y;  // Don't use now
      }
    else {  // We will assume no other possibility at this time.
      bias_bar.y = ((scrn_rect.top - mid_scr.y)>>1 ) + mid_scr.y;
      marker_y = bias_bar.y;  // Don't use now
      }

    drawOneLine( scrn_rect.left,
                 bias_bar.y,
                 scrn_rect.right,
                 bias_bar.y );

    if( orientation == ReadTOP )
      {
      drawOneLine( scrn_rect.left,
                   bias_bar.y,
                   scrn_rect.left,
                   mid_scr.y);

      drawOneLine( scrn_rect.right,
                   bias_bar.y,
                   scrn_rect.right,
                   mid_scr.y );

      drawOneLine( mid_scr.x,
                   scrn_rect.bottom,
                   mid_scr.x,
                   bias_bar.y );

      }
    else {
      if( orientation == ReadBOTTOM )
        {
        drawOneLine( scrn_rect.left,
                     bias_bar.y,
                     scrn_rect.left,
                     mid_scr.y );

        drawOneLine( scrn_rect.right,
                     bias_bar.y,
                     scrn_rect.right,
                     mid_scr.y );

        drawOneLine( mid_scr.x,
                     scrn_rect.top,
                     mid_scr.x,
                     bias_bar.y );
        }
      }

    for( i = (int)vmin; i <= (int)vmax; i++ )  // increment is faster than addition
      {
//      if( sub_type == LIMIT ) {
        condition = (i >= min_val());
//        }
//      else {
//        if( sub_type == NOLIMIT ) {
//          condition = 1;
//          }
//        }
      if( condition )        {
        marker_x = scrn_rect.left + (int)((i - vmin) * factor());
        if( div_min()){
          if( (i%(int)div_min()) == 0 ) {
//            if( orientation == ReadTOP )
//              {
//              drawOneLine( marker_x, marker_y, marker_x, marker_y + 3 );
              drawOneLine( marker_x, bias_bar.y, marker_x, mid_scr.y );
//              }
//            else {
//            drawOneLine( marker_x, marker_y + 3, marker_x, marker_y + 6 );
//              drawOneLine( marker_x, bias_bar.y, marker_x, mid_scr.y );
//              }
            }
           }
        if( div_max()) {
          if( (i%(int)div_max())==0 )
            {
            if(modulo()) {
              disp_val = i % modulo();
              if( disp_val < 0) {
                disp_val += modulo();
                }
              }
            else {
              disp_val = i;
              }
              sprintf( TextScale, "%d", disp_val );
            if( orientation == ReadTOP )
              {
//              drawOneLine( marker_x, marker_y, marker_x, marker_y+6 );
              drawOneLine( marker_x, mid_scr.y + 3,marker_x, bias_bar.y );
              textString ( marker_x - 4 * strlen(TextScale), marker_y + 14,
                           TextScale, GLUT_BITMAP_8_BY_13 );
              }
            else {
//            drawOneLine( marker_x, marker_y, marker_x, marker_y+6 );
              drawOneLine( marker_x, mid_scr.y - 3,marker_x, bias_bar.y );
              textString ( marker_x - 4 * strlen(TextScale), mid_scr.y - 14,
                           TextScale, GLUT_BITMAP_8_BY_13 );
              }
            }
          }  
        }
      }
    }
   }
}

//============== Top of guage_instr class member definitions ==============

guage_instr ::
    guage_instr( RECT         the_box,
                 DBLFNPTR     load_fn,
                 ReadOriented readway,
                 int          maxValue,
                 int          minValue,
                 UINT         major_divs,
                 UINT         minor_divs,
                 UINT         modulus,
                 bool         working) :
           instr_scale( the_box,
                        load_fn, readway,
                        maxValue, maxValue, minValue,
                        major_divs, minor_divs,
                        modulus,
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
  int marker_x;
  int marker_y;
  register i;
  char TextScale[80];
//  int condition;
  int disp_val;
  double vmin              = min_val();
  double vmax              = max_val();
  POINT mid_scr            = get_centroid();
//  double cur_value         = get_value();
  RECT   scrn_rect         = get_location();
  ReadOriented orientation = get_orientation();

  if( (orientation == ReadLEFT) ||( orientation == ReadRIGHT))  // Vertical scale
    {
    mid_scr = get_centroid();
    if( orientation == ReadLEFT )     // Calculate x marker offset
      marker_x = scrn_rect.left - 6;
    else                              // We'll default this for now.
      marker_x = scrn_rect.right;

    // Draw the basic markings for the scale...

    if( orientation == ReadLEFT )
      {

      drawOneLine( scrn_rect.right - 3,     // Bottom tick bar
                   scrn_rect.bottom,
                   scrn_rect.right,
                   scrn_rect.bottom );

      drawOneLine( scrn_rect.right - 3,     // Top tick bar
                   scrn_rect.top,
                   scrn_rect.right,
                   scrn_rect.top );
      }
    else       {
      drawOneLine( scrn_rect.right,
                   scrn_rect.bottom,
                   scrn_rect.right+3,
                   scrn_rect.bottom );

      drawOneLine( scrn_rect.right,
                   scrn_rect.top,
                   scrn_rect.right+3,
                   scrn_rect.top );
      }

    // Work through from bottom to top of scale. Calculating where to put
    // minor and major ticks.

    for( i = (int)vmin; i <= (int)vmax; i++ )
      {
        // Calculate the location of this tick
      marker_y = scrn_rect.bottom + (int)((i - vmin) * factor());

      if( div_min()) {
        if( (i%div_min()) == 0) {
          if( orientation == ReadLEFT )
            {
            drawOneLine( marker_x + 3, marker_y, marker_x + 6, marker_y );
            }
          else {
            drawOneLine( marker_x, marker_y, marker_x + 3, marker_y );
            }
          }
        }
      if( div_max()) {
        if( (i%div_max()) == 0 )            {
          drawOneLine( marker_x,     marker_y,
                       marker_x + 6, marker_y );
          if(modulo()) {
            disp_val = i % modulo();
            if( disp_val < 0) {
              disp_val += modulo();
              }
            }
          else {
            disp_val = i;
            }
          sprintf( TextScale, "%d", disp_val );
          if( orientation == ReadLEFT )              {
            textString( marker_x -  8 * strlen(TextScale) - 2, marker_y - 4,
                        TextScale, GLUT_BITMAP_8_BY_13 );
            }
          else  {
            if( orientation == ReadRIGHT )              {
            textString( marker_x + 10,                         marker_y - 4,
                        TextScale, GLUT_BITMAP_8_BY_13 );
              }
            }
          }
        } // End if condition
      } // End for range of i from vmin to vmax
    }  // End if VERTICAL SCALE TYPE
  else {                                // Horizontal scale by default
    {
    if( orientation == ReadTOP ) {
      marker_y = scrn_rect.bottom;
      }
    else {
      if( orientation == ReadBOTTOM ) {
        marker_y = scrn_rect.bottom - 6;
        }
      }
    drawOneLine( scrn_rect.left,
                 scrn_rect.bottom,
                 scrn_rect.right,
                 scrn_rect.bottom );

    if( orientation == ReadTOP )
      {
      drawOneLine( scrn_rect.left,
                   scrn_rect.bottom,
                   scrn_rect.left,
                   scrn_rect.bottom + 3 );

      drawOneLine( scrn_rect.right,
                   scrn_rect.bottom,
                   scrn_rect.right,
                   scrn_rect.bottom + 6 );
      }
    else {
      if( orientation == ReadBOTTOM )
        {
        drawOneLine( scrn_rect.left,
                     scrn_rect.bottom,
                     scrn_rect.left,
                     scrn_rect.bottom - 6 );

        drawOneLine( scrn_rect.right,
                     scrn_rect.bottom,
                     scrn_rect.right,
                     scrn_rect.bottom - 6 );
        }
      }

    for( i = (int)vmin; i <= (int)vmax; i++ )  // increment is faster than addition
      {
      marker_x = scrn_rect.left + (int)((i - vmin) * factor());
      if( div_min()) {
        if( (i%div_min()) == 0 ) {
          if( orientation == ReadTOP )
            {
            drawOneLine( marker_x, marker_y, marker_x, marker_y + 3 );
            }
          else {
            if( orientation == ReadBOTTOM )
              {
              drawOneLine( marker_x, marker_y + 3, marker_x, marker_y + 6 );
              }
            }
          }  // End if minor tick called for.
        }  // End if minor ticks are of interest
      if( div_max()) {
        if( (i%div_max())==0 )
          {
          // Modulo implies a "clock" style instrument. Needs work.
          if(modulo()) {
            disp_val = i % modulo();
            if( disp_val < 0) {
              disp_val += modulo();
              }
            }
          else {   // Scale doesn't roll around.
            disp_val = i;
            }
            sprintf( TextScale, "%d", disp_val );
          if( orientation == ReadTOP )
            {
            drawOneLine( marker_x, marker_y, marker_x, marker_y+6 );
            textString ( marker_x - 4 * strlen(TextScale), marker_y + 14,
                         TextScale, GLUT_BITMAP_8_BY_13 );
            }
          else {
            if( orientation == ReadBOTTOM )
              {
              drawOneLine( marker_x, marker_y, marker_x, marker_y+6 );
              textString ( marker_x - 4 * strlen(TextScale), marker_y - 14,
                           TextScale, GLUT_BITMAP_8_BY_13 );
              } // End if bottom
            } // End else if not ReadTOP
          } // End if major division point.
        } // End if major divisions of interest.
      }
    }
   }
}

//============ Top of dual_instr_item class member definitions ============

dual_instr_item ::
  dual_instr_item ( RECT         the_box,
                    DBLFNPTR     chn1_source,
                    DBLFNPTR     chn2_source,
                    bool         working,
                    ReadOriented readway ):
                  instr_item( the_box, chn1_source, readway, working),
                  alt_data_source( chn2_source )
{
}

dual_instr_item ::
  dual_instr_item( const dual_instr_item & image) :
                 instr_item ((instr_item &) image ),
                 alt_data_source( image.alt_data_source)
{
}

dual_instr_item & dual_instr_item ::
  operator = (const dual_instr_item & rhs )
{
  if( !(this == &rhs)) {
    instr_item::operator = (rhs);
    alt_data_source = rhs.alt_data_source;
    }
  return *this;
}

//============ Top of fgTBI_instr class member definitions ==============

fgTBI_instr ::
fgTBI_instr( RECT      the_box,
             DBLFNPTR  chn1_source,
             DBLFNPTR  chn2_source,
             UINT      maxBankAngle,
             UINT      maxSlipAngle,
             UINT      gap_width,
             bool      working ) :
               dual_instr_item( the_box,
                                chn1_source,
                                chn2_source,
                                working,
                                ReadTOP),
               BankLimit      (maxBankAngle),
               SlewLimit      (maxSlipAngle),
               scr_hole       (gap_width   )
{
}

fgTBI_instr :: ~fgTBI_instr() {}

fgTBI_instr :: fgTBI_instr( const fgTBI_instr & image):
                 dual_instr_item( (const dual_instr_item &) image),
                 BankLimit( image.BankLimit),
                 SlewLimit( image.SlewLimit),
                 scr_hole ( image.scr_hole )
{
}

fgTBI_instr & fgTBI_instr ::
operator = (const fgTBI_instr & rhs )
{
  if( !(this == &rhs)) {
    dual_instr_item::operator = (rhs);
    BankLimit = rhs.BankLimit;
    SlewLimit = rhs.SlewLimit;
    scr_hole  = rhs.scr_hole;
    }
   return *this;
}

//
//	Draws a Turn Bank Indicator on the screen
//

 void fgTBI_instr :: draw( void )
{
  int x_inc1, y_inc1;
  int x_inc2, y_inc2;
  int x_t_inc1, y_t_inc1;

  int d_bottom_x, d_bottom_y;
  int d_right_x, d_right_y;
  int d_top_x, d_top_y;
  int d_left_x, d_left_y;

  int inc_b_x, inc_b_y;
  int inc_r_x, inc_r_y;
  int inc_t_x, inc_t_y;
  int inc_l_x, inc_l_y;
  RECT My_box = get_location();
  POINT centroid = get_centroid();
  int tee_height = My_box.top - My_box.bottom; // Hack, hack.
  
//	struct fgFLIGHT *f = &current_aircraft.flight;
  double sin_bank, cos_bank;
  double bank_angle, sideslip_angle;
  double ss_const; // sideslip angle pixels per rad

  bank_angle     = current_ch2();  // Roll limit +/- 30 degrees
  if( bank_angle < -FG_PI_2/3 ) {
    bank_angle = -FG_PI_2/3;
  }else if( bank_angle > FG_PI_2/3 ) {
    bank_angle = FG_PI_2/3;
  }
  sideslip_angle = current_ch1(); // Sideslip limit +/- 20 degrees
  if( sideslip_angle < -FG_PI/9 ) {
    sideslip_angle = -FG_PI/9;
  } else if( sideslip_angle > FG_PI/9 ) {
    sideslip_angle = FG_PI/9;
  }

	// sin_bank = sin( FG_2PI-FG_Phi );
	// cos_bank = cos( FG_2PI-FG_Phi );
  sin_bank = sin(FG_2PI-bank_angle);
  cos_bank = cos(FG_2PI-bank_angle);
  
  x_inc1 = (int)(get_span() * cos_bank);
  y_inc1 = (int)(get_span() * sin_bank);
  x_inc2 = (int)(scr_hole  * cos_bank);
  y_inc2 = (int)(scr_hole  * sin_bank);

  x_t_inc1 = (int)(tee_height * sin_bank);
  y_t_inc1 = (int)(tee_height * cos_bank);
  
  d_bottom_x = 0;
  d_bottom_y = (int)(-scr_hole);
  d_right_x  = (int)(scr_hole);
  d_right_y  = 0;
  d_top_x    = 0;
  d_top_y    = (int)(scr_hole);
  d_left_x   = (int)(-scr_hole);
  d_left_y   = 0;

  ss_const = (get_span()*2)/(FG_2PI/9);  // width represents 40 degrees

  d_bottom_x += (int)(sideslip_angle*ss_const);
  d_right_x  += (int)(sideslip_angle*ss_const);
  d_left_x   += (int)(sideslip_angle*ss_const);
  d_top_x    += (int)(sideslip_angle*ss_const);
  
  inc_b_x = (int)(d_bottom_x*cos_bank-d_bottom_y*sin_bank);
  inc_b_y = (int)(d_bottom_x*sin_bank+d_bottom_y*cos_bank);
  inc_r_x = (int)(d_right_x*cos_bank-d_right_y*sin_bank);
  inc_r_y = (int)(d_right_x*sin_bank+d_right_y*cos_bank);
  inc_t_x = (int)(d_top_x*cos_bank-d_top_y*sin_bank);
  inc_t_y = (int)(d_top_x*sin_bank+d_top_y*cos_bank);
  inc_l_x = (int)(d_left_x*cos_bank-d_left_y*sin_bank);
  inc_l_y = (int)(d_left_x*sin_bank+d_left_y*cos_bank);

  if( scr_hole == 0 )
    {
    drawOneLine( centroid.x - x_inc1, centroid.y - y_inc1, \
                 centroid.x + x_inc1, centroid.y + y_inc1 );
    }
  else
    {
    drawOneLine( centroid.x - x_inc1, centroid.y - y_inc1, \
                 centroid.x - x_inc2, centroid.y - y_inc2 );
    drawOneLine( centroid.x + x_inc2, centroid.y + y_inc2, \
                 centroid.x + x_inc1, centroid.y + y_inc1 );
    }

  // draw teemarks
  drawOneLine( centroid.x + x_inc2,            \
                 centroid.y + y_inc2,          \
               centroid.x + x_inc2 + x_t_inc1, \
                 centroid.y + y_inc2 - y_t_inc1 );
  drawOneLine( centroid.x - x_inc2,            \
                 centroid.y - y_inc2,          \
               centroid.x - x_inc2 + x_t_inc1, \
                 centroid.y - y_inc2 - y_t_inc1 );
               
  // draw sideslip diamond (it is not yet positioned correctly )
  drawOneLine( centroid.x + inc_b_x, \
               centroid.y + inc_b_y, \
               centroid.x + inc_r_x, \
               centroid.y + inc_r_y );
  drawOneLine( centroid.x + inc_r_x, \
               centroid.y + inc_r_y, \
               centroid.x + inc_t_x, \
               centroid.y + inc_t_y );
  drawOneLine( centroid.x + inc_t_x, \
               centroid.y + inc_t_y, \
               centroid.x + inc_l_x, \
               centroid.y + inc_l_y );
  drawOneLine( centroid.x + inc_l_x, \
               centroid.y + inc_l_y, \
               centroid.x + inc_b_x, \
               centroid.y + inc_b_y );

}

//====================== Top of HudLadder Class =======================
HudLadder ::
  HudLadder(  RECT      the_box,
              DBLFNPTR  ptch_source,
              DBLFNPTR  roll_source,
              UINT      span_units,
              int       major_div,
              UINT      minor_div,
              UINT      screen_hole,
              UINT      lbl_pos,
              bool      working) :
               dual_instr_item( the_box,
                                ptch_source,
                                roll_source,
                                working,
                                ReadRIGHT ),
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
  double roll_value, pitch_value;
//  int marker_x;
  int marker_y;
  int scr_min;
//  int scr_max;
  int x_ini, x_end;
  int y_ini, y_end;
  int new_x_ini, new_x_end;
  int new_y_ini, new_y_end;
  register i;
  POINT centroid = get_centroid();
  RECT  box      = get_location();
  int half_span  = (box.right - box.left) >> 1;
  char TextLadder[80];
  int condition;

  roll_value  = current_ch2();
  pitch_value = current_ch1() * RAD_TO_DEG;

  vmin        = (int)(pitch_value - (double)width_units/2.0);
  vmax        = (int)(pitch_value + (double)width_units/2.0);

  scr_min     = box.bottom; // centroid.y - ((box.top - box.bottom) >> 1);
//  scr_max     = box.top;    // scr_min    + (box.top - box.bottom);
//  marker_x    = centroid.x - half_span;

// Box the target.
  drawOneLine( centroid.x - 5, centroid.y,     centroid.x,     centroid.y + 5);
  drawOneLine( centroid.x,     centroid.y + 5, centroid.x + 5, centroid.y);
  drawOneLine( centroid.x + 5, centroid.y,     centroid.x,     centroid.y - 5);
  drawOneLine( centroid.x,     centroid.y - 5, centroid.x - 5, centroid.y);

  for( i=(int)vmin; i<=(int)vmax; i+=1 )
    {
    condition = 1;
    if( condition )
      {
      marker_y = scr_min + (int)((i-vmin)*factor);
      if( div_units ) {
        if( i%div_units==0 )
          {
          sprintf( TextLadder, "%d", i );
          if( scr_hole == 0 )
            {
            if( i ) {
              x_ini = centroid.x - half_span;
              }
            else {
              x_ini = centroid.x - half_span - 10;
              }
            y_ini = marker_y;
            x_end = centroid.x + half_span;
            y_end = marker_y;
            new_x_ini = centroid.x + (int)(                     \
                       (x_ini - centroid.x) * cos(roll_value) - \
                       (y_ini - centroid.y) * sin(roll_value));
            new_y_ini = centroid.y + (int)(                     \
                       (x_ini - centroid.x) * sin(roll_value) + \
                       (y_ini - centroid.y) * cos(roll_value));
            new_x_end = centroid.x + (int)(                       \
                       (x_end - centroid.x) * cos(roll_value) - \
                       (y_end - centroid.y) * sin(roll_value));
            new_y_end = centroid.y + (int)(                            \
                       (x_end - centroid.x) * sin(roll_value) + \
                       (y_end - centroid.y) * cos(roll_value));

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
            textString( new_x_ini -  8 * strlen(TextLadder) - 8,
                        new_y_ini -  4,
                        TextLadder, GLUT_BITMAP_8_BY_13 );
            textString( new_x_end + 10,
                        new_y_end -  4,
                        TextLadder, GLUT_BITMAP_8_BY_13 );
            }
          else
            {
            if( i != 0 )  {
              x_ini = centroid.x - half_span;
              }
            else          {
              x_ini = centroid.x - half_span - 10;
              }
            y_ini = marker_y;
            x_end = centroid.x - half_span + scr_hole/2;
            y_end = marker_y;
            new_x_ini = centroid.x+ (int)(                      \
                        (x_ini - centroid.x) * cos(roll_value) -\
                        (y_ini - centroid.y) * sin(roll_value));
            new_y_ini = centroid.y+  (int)(                     \
                        (x_ini - centroid.x) * sin(roll_value) +\
                        (y_ini - centroid.y) * cos(roll_value));
            new_x_end = centroid.x+  (int)(                     \
                        (x_end - centroid.x) * cos(roll_value) -\
                        (y_end - centroid.y) * sin(roll_value));
            new_y_end = centroid.y+ (int)(                      \
                        (x_end - centroid.x) * sin(roll_value) +\
                        (y_end - centroid.y) * cos(roll_value));

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
            textString( new_x_ini - 8 * strlen(TextLadder) - 8,
                        new_y_ini - 4,
                        TextLadder, GLUT_BITMAP_8_BY_13 );

            x_ini = centroid.x + half_span - scr_hole/2;
            y_ini = marker_y;
            if( i != 0 )  {
              x_end = centroid.x + half_span;
              }
            else          {
              x_end = centroid.x + half_span + 10;
              }
            y_end = marker_y;
            new_x_ini = centroid.x + (int)(                 \
                        (x_ini-centroid.x)*cos(roll_value) -\
                        (y_ini-centroid.y)*sin(roll_value));
            new_y_ini = centroid.y + (int)(                 \
                        (x_ini-centroid.x)*sin(roll_value) +\
                        (y_ini-centroid.y)*cos(roll_value));
            new_x_end = centroid.x + (int)(                 \
                        (x_end-centroid.x)*cos(roll_value) -\
                        (y_end-centroid.y)*sin(roll_value));
            new_y_end = centroid.y +  (int)(                  \
                        (x_end-centroid.x)*sin(roll_value) +\
                        (y_end-centroid.y)*cos(roll_value));

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
            textString( new_x_end+10, new_y_end-4,
                        TextLadder, GLUT_BITMAP_8_BY_13 );
            }
          }
         }
            /* if( i%div_max()==0 )
            {
            	drawOneLine( marker_x, marker_y, marker_x+6, marker_y );
                sprintf( TextScale, "%d", i );
                if( orientation == LEFT )
                {
                	textString( marker_x-8*strlen(TextScale)-2, marker_y-4,
                              TextScale, GLUT_BITMAP_8_BY_13 );
                }
                else if( orientation == RIGHT )
                {
                	textString( marker_x+10, marker_y-4,
                              TextScale, GLUT_BITMAP_8_BY_13 );
                }
            } */
        }
    }

}

//========================= End of Class Implementations===================
// fgHUDInit
//
// Constructs a HUD object and then adds in instruments. At the present
// the instruments are hard coded into the routine. Ultimately these need
// to be defined by the aircraft's instrumentation records so that the
// display for a Piper Cub doesn't show the speed range of a North American
// mustange and the engine readouts of a B36!
//

#define INSTRDEFS 16

int fgHUDInit( fgAIRCRAFT * /* current_aircraft */ )
{
  instr_item *HIptr;
  int index;
  RECT loc;

  fgPrintf( FG_COCKPIT, FG_INFO, "Initializing current aircraft HUD\n" );

  HUD_deque.erase( HUD_deque.begin(), HUD_deque.end());  // empty the HUD deque

//  hud->code = 1;
//  hud->status = 0;

  // For now lets just hardcode the hud here.
  // In the future, hud information has to come from the same place
  // aircraft information came from.

//  fgHUDSetTimeMode( hud, NIGHT );
//  fgHUDSetBrightness( hud, BRT_LIGHT );

  for( index = 0; index <= INSTRDEFS; index++) {
    switch ( index ) {
      case 0:     // TBI
      //  fgHUDAddHorizon( hud, 330, 100, 40, 5, 10, get_roll, get_sideslip );
        loc.left   =  330 - 40;
        loc.top    =  110;
        loc.right  =  330 + 40;
        loc.bottom =  100;
        HIptr = (instr_item *) new fgTBI_instr( loc );
        break;

      case 1:     // Artificial Horizon
      //  fgHUDAddLadder ( hud, 330, 285, 120, 180, 70, 10,
      //                   NONE, 45, get_roll, get_pitch );
        loc.left   =  270; // 330 -  60
        loc.top    =  375; // 285 +  90
        loc.right  =  390; // 330 +  60
        loc.bottom =  195; // 285 -  90
        HIptr = (instr_item *) new HudLadder( loc );
        break;

      case 2:    // KIAS
      //  fgHUDAddScale  ( hud, VERTICAL,     LIMIT, 200, 180, 380,  5,  10,
      //                      LEFT,     0,  100,   50,   0, get_speed );
        loc.left   =  160;
        loc.top    =  380;
        loc.right  =  200;
        loc.bottom =  180;
        HIptr = (instr_item *) new moving_scale( loc,
                                                 get_speed,
                                                 ReadLEFT,
                                                 200, 0,
                                                 10,  5,
                                                 0,
                                                 50.0,
                                                 TRUE);
        break;

      case 3:    // Angle of Attack
      //  fgHUDAddScale  ( hud, HORIZONTAL, NOLIMIT, 180, 250, 410,  1,   5,
      //                      BOTTOM, -40,   50,   21,   0, get_aoa );
        loc.left   =  250;
        loc.top    =  190;
        loc.right  =  410;
        loc.bottom =  160;
        HIptr = (instr_item *) new moving_scale( loc,
                                                 get_aoa,
                                                 ReadBOTTOM,
                                                 50, -40,
                                                 5,    1,
                                                 0,
                                                 21.0,
                                                 TRUE);
        break;

      case 4:    // GYRO COMPASS
      // fgHUDAddScale  ( hud, HORIZONTAL, NOLIMIT, 380, 200, 460,  5,  10,
      //                      TOP,      0,   50,   50, 360, get_heading );
        loc.left   =  200;
        loc.top    =  410;
        loc.right  =  460;
        loc.bottom =  380;
        HIptr = (instr_item *) new moving_scale( loc,
                                                 get_heading,
                                                 ReadTOP,
                                                 360, 0,
                                                 10,   5,
                                                 360,
                                                 50,
                                                 TRUE);
        break;

      case 5:    // AMSL
      //  fgHUDAddScale  ( hud, VERTICAL,     LIMIT, 460, 180, 380, 25, 100,
      //                      RIGHT,    0, 15000, 250,   0, get_altitude);
        loc.left   =  460;
        loc.top    =  380;
        loc.right  =  490;
        loc.bottom =  180;
        HIptr = (instr_item *) new moving_scale( loc,
                                                 get_altitude,
                                                 ReadRIGHT,
                                                 15000, 0,
                                                 100,  25,
                                                 0,
                                                 250,
                                                 TRUE);
        break;

      case 6:    // Digital KIAS
      //  fgHUDAddLabel  ( hud, 160, 150, SMALL, NOBLINK,
      //                 RIGHT_JUST, NULL, " Kts",      "%5.0f", get_speed );
        loc.left   =  160;
        loc.top    =  180; // Ignore
        loc.right  =  200; // Ignore
        loc.bottom =  150;
        HIptr = (instr_item *) new instr_label ( loc,
                                                 get_speed,
                                                 "%5.0f",
                                                 NULL,
                                                 " Kts",
                                                 ReadTOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 7:    // Digital Altimeter
      //  fgHUDAddLabel  ( hud, 160, 135, SMALL, NOBLINK,
      //              RIGHT_JUST, NULL, " m",        "%5.0f", get_altitude );
        loc.left   =  160;
        loc.top    =  145; // Ignore
        loc.right  =  200; // Ignore
        loc.bottom =  135;
        HIptr = (instr_item *) new instr_label ( loc,
                                                 get_altitude,
                                                 "MSL  %5.0f",
                                                 NULL,
                                                 " m",
                                                 ReadTOP,
                                                 LEFT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 8:    // Roll indication diagnostic
      // fgHUDAddLabel  ( hud, 160, 120, SMALL, NOBLINK,
      //                  RIGHT_JUST, NULL, " Roll",     "%5.2f", get_roll );
        loc.left   =  160;
        loc.top    =  130; // Ignore
        loc.right  =  200; // Ignore
        loc.bottom =  120;
        HIptr = (instr_item *) new instr_label ( loc,
                                                 get_roll,
                                                 "%5.2f",
                                                 " Roll",
                                                 " Deg",
                                                 ReadTOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 9:    // Angle of attack diagnostic
      //  fgHUDAddLabel  ( hud, 440, 150, SMALL, NOBLINK,
      //                   RIGHT_JUST, NULL, " AOA",      "%5.2f", get_aoa );
        loc.left   =  440;
        loc.top    =  160; // Ignore
        loc.right  =  500; // Ignore
        loc.bottom =  150;
        HIptr = (instr_item *) new instr_label ( loc,
                                                 get_aoa,
                                                 "    %5.2f",
                                                 " AOA",
                                                 " Deg",
                                                 ReadTOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 10:
      //  fgHUDAddLabel  ( hud, 440, 135, SMALL, NOBLINK,
      //               RIGHT_JUST, NULL, " Heading",  "%5.0f", get_heading );
        loc.left   =  440;
        loc.top    =  145; // Ignore
        loc.right  =  500; // Ignore
        loc.bottom =  135;
        HIptr = (instr_item *) new instr_label ( loc,
                                                 get_heading,
                                                 "%5.0f",
                                                 "Heading",
                                                 " Deg",
                                                 ReadTOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 11:
      //  fgHUDAddLabel  ( hud, 440, 120, SMALL, NOBLINK,
      //              RIGHT_JUST, NULL, " Sideslip", "%5.2f", get_sideslip );
        loc.left   =  440;
        loc.top    =  130; // Ignore
        loc.right  =  500; // Ignore
        loc.bottom =  120;
        HIptr = (instr_item *) new instr_label ( loc,
                                                 get_sideslip,
                                                 "%5.2f",
                                                 "Sideslip ",
                                                 NULL,
                                                 ReadTOP,
                                                 RIGHT_JUST,
                                                 SMALL,
                                                 0,
                                                 TRUE );
        break;

      case 12:
        loc.left   = 440;
        loc.top    =  90; // Ignore
        loc.right  = 440; // Ignore
        loc.bottom =  90;
        HIptr = (instr_item *) new instr_label( loc, get_throttleval,
                                                "%.2f",
                                                "Throttle ",
                                                NULL,
                                                ReadTOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      case 13:
        loc.left   = 440;
        loc.top    =  70; // Ignore
        loc.right  = 500; // Ignore
        loc.bottom =  75;
        HIptr = (instr_item *) new instr_label( loc, get_elevatorval,
                                                "%5.2f",
                                                "Elevator",
                                                NULL,
                                                ReadTOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      case 14:
        loc.left   = 440;
        loc.top    = 100; // Ignore
        loc.right  = 500; // Ignore
        loc.bottom =  60;
        HIptr = (instr_item *) new instr_label( loc, get_aileronval,
                                                "%5.2f",
                                                "Aileron",
                                                NULL,
                                                ReadTOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      case 15:
        loc.left   = 10;
        loc.top    = 100; // Ignore
        loc.right  = 500; // Ignore
        loc.bottom =  10;
        HIptr = (instr_item *) new instr_label( loc, get_frame_rate,
                                                "%.1f",
                                                "Frame rate = ",
                                                NULL,
                                                ReadTOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      case 16:
        loc.left   = 10;
        loc.top    = 100; // Ignore
        loc.right  = 500; // Ignore
        loc.bottom =  25;
        HIptr = (instr_item *) new instr_label( loc, get_fov,
                                                "%.1f",
                                                "FOV = ",
                                                NULL,
                                                ReadTOP,
                                                RIGHT_JUST,
                                                SMALL,
                                                0,
                                                TRUE );
        break;

      //  fgHUDAddControlSurfaces( hud, 10, 10, NULL );
//        loc.left   =  250;
//        loc.top    =  190;
//        loc.right  =  410;
//        loc.bottom =  180;
//        HIptr = (instr_item *) new
//        break;

      default:;
      }
    if( HIptr ) {                   // Anything to install?
      HUD_deque.insert( HUD_deque.begin(), HIptr);
      }
    }

//  fgHUDAddControl( hud, HORIZONTAL, 50,  25, get_aileronval  ); // was 10, 10
//  fgHUDAddControl( hud, VERTICAL,   150, 25, get_elevatorval ); // was 10, 10
//  fgHUDAddControl( hud, HORIZONTAL, 250, 25, get_rudderval   ); // was 10, 10
  return 0;  // For now. Later we may use this for an error code.
}


// fgUpdateHUD
//
// Performs a once around the list of calls to instruments installed in
// the HUD object with requests for redraw. Kinda. It will when this is
// all C++.
//
int global_day_night_switch = DAY;

void fgUpdateHUD( void ) {
  int i;
  int brightness;
//  int day_night_sw = current_aircraft.controls->day_night_switch;
  int day_night_sw = global_day_night_switch;
  int hud_displays = HUD_deque.size();
  instr_item *pHUDInstr;

  if( !hud_displays ) {  // Trust everyone, but ALWAYS cut the cards!
    return;
    }

  pHUDInstr = HUD_deque[0];
  brightness = pHUDInstr->get_brightness();
//  brightness = HUD_deque.at(0)->get_brightness();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glLoadIdentity();
  gluOrtho2D(0, 640, 0, 480);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor3f(1.0, 1.0, 1.0);
  glIndexi(7);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glLineWidth(1);

  for( i = hud_displays; i; --i) { // Draw everything
//    if( HUD_deque.at(i)->enabled()) {
    pHUDInstr = HUD_deque[i - 1];
    if( pHUDInstr->enabled()) {
                                   // We should to respond to a dial instead
                                   // or as well to the of time of day. Of
                                   // course, we have no dial!
      if( day_night_sw == DAY) {
        switch (brightness) {
          case BRT_LIGHT:
            glColor3f (0.1, 0.9, 0.1);
            break;

          case BRT_MEDIUM:
            glColor3f (0.1, 0.7, 0.0);
            break;

          case BRT_DARK:
            glColor3f (0.0, 0.5, 0.0);
            }
          }
        else {
          if( day_night_sw == NIGHT) {
            switch (brightness) {
              case BRT_LIGHT:
                glColor3f (0.9, 0.1, 0.1);
                break;

              case BRT_MEDIUM:
                glColor3f (0.7, 0.0, 0.1);
                break;

              case BRT_DARK:
              default:
                glColor3f (0.5, 0.0, 0.0);
              }
            }
          else {     // Just in case default
            glColor3f (0.1, 0.9, 0.1);
            }
          }
    //  fgPrintf( FG_COCKPIT, FG_DEBUG, "HUD Code %d  Status %d\n",
    //            hud->code, hud->status );
      pHUDInstr->draw();
//      HUD_deque.at(i)->draw(); // Responsible for broken or fixed variants.
                              // No broken displays honored just now.
      }
    }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

/* $Log$
/* Revision 1.9  1998/05/16 13:04:14  curt
/* New updates from Charlie Hotchkiss.
/*
 * Revision 1.8  1998/05/13 18:27:54  curt
 * Added an fov to hud display.
 *
 * Revision 1.7  1998/05/11 18:13:11  curt
 * Complete C++ rewrite of all cockpit code by Charlie Hotchkiss.
 *
 * Revision 1.22  1998/04/18 04:14:02  curt
 * Moved fg_debug.c to it's own library.
 *
 * Revision 1.21  1998/04/03 21:55:28  curt
 * Converting to Gnu autoconf system.
 * Tweaks to hud.c
 *
 * Revision 1.20  1998/03/09 22:48:40  curt
 * Minor "formatting" tweaks.
 *
 * Revision 1.19  1998/02/23 20:18:28  curt
 * Incorporated Michele America's hud changes.
 *
 * Revision 1.18  1998/02/21 14:53:10  curt
 * Added Charlie's HUD changes.
 *
 * Revision 1.17  1998/02/20 00:16:21  curt
 * Thursday's tweaks.
 *
 * Revision 1.16  1998/02/19 13:05:49  curt
 * Incorporated some HUD tweaks from Michelle America.
 * Tweaked the sky's sunset/rise colors.
 * Other misc. tweaks.
 *
 * Revision 1.15  1998/02/16 13:38:39  curt
 * Integrated changes from Charlie Hotchkiss.
 *
 * Revision 1.14  1998/02/12 21:59:41  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.12  1998/02/09 15:07:48  curt
 * Minor tweaks.
 *
 * Revision 1.11  1998/02/07 15:29:34  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.10  1998/02/03 23:20:14  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.9  1998/01/31 00:43:04  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.8  1998/01/27 00:47:51  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.7  1998/01/19 18:40:20  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.6  1997/12/15 23:54:34  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.5  1997/12/10 22:37:39  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.4  1997/09/23 00:29:32  curt
 * Tweaks to get things to compile with gcc-win32.
 *
 * Revision 1.3  1997/09/05 14:17:26  curt
 * More tweaking with stars.
 *
 * Revision 1.2  1997/09/04 02:17:30  curt
 * Shufflin' stuff.
 *
 * Revision 1.1  1997/08/29 18:03:22  curt
 * Initial revision.
 *
 */
