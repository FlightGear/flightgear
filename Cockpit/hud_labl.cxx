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

//======================= Top of instr_label class =========================
instr_label ::
         instr_label( int           x,
                      int           y,
                      UINT          width,
                      UINT          height,
                      DBLFNPTR      data_source,
                      const char   *label_format,
                      const char   *pre_label_string,
                      const char   *post_label_string,
                      double        scale_data,
                      UINT          options,
                      fgLabelJust   justification,
                      int           font_size,
                      int           blinking,
                      bool          working ):
                           instr_item( x, y, width, height,
                                       data_source, scale_data,options, working ),
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

  if( data_available() ) {
    sprintf( label_buffer, format_buffer, get_value() );
    }
  else {
    sprintf( label_buffer, format_buffer );
    }
    
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
    textString( scrn_rect.left + posincr, scrn_rect.top,
                label_buffer, GLUT_BITMAP_8_BY_13);
    }
  else  {
    if( fontSize == LARGE ) {
      textString( scrn_rect.left + posincr, scrn_rect.top,
                  label_buffer, GLUT_BITMAP_9_BY_15);
      }
    }
}

