#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/math/fg_random.h>
#include <simgear/math/polar3d.hxx>

#include <Aircraft/aircraft.hxx>
#include <GUI/gui.h>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>

#include "hud.hxx"


#ifdef USE_HUD_TextList
#define textString( x , y, text, font )  TextString( text, x , y )
#else
#define textString( x , y, text, font )  puDrawString ( guiFnt, text, x, y );
#endif

//======================= Top of instr_label class =========================
lon_label ::
         lon_label( int           x,
                      int           y,
                      UINT          width,
                      UINT          height,
                      FLTFNPTR      data_source,
//                      DBLFNPTR      data_source,					
                      const char   *label_format,
                      const char   *pre_label_string,
                      const char   *post_label_string,
                      float        scale_data,
//                      double        scale_data,					
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

}

// I put this in to make it easy to construct a class member using the current
// C code.


lon_label :: ~lon_label()
{
}

// Copy constructor
lon_label :: lon_label( const lon_label & image) :
                              instr_item((const instr_item &)image),
                              pformat    ( image.pformat    ),
                              pre_str  ( image.pre_str  ),
                              post_str ( image.post_str ),
                              blink    ( image.blink    )
{
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

}

lon_label & lon_label ::operator = (const lon_label & rhs )
{
  if( !(this == &rhs)) {
    instr_item::operator = (rhs);
    pformat      = rhs.pformat;
    fontSize   = rhs.fontSize;
    blink      = rhs.blink;
    justify    = rhs.justify;
    pre_str    = rhs.pre_str;
    post_str   = rhs.post_str;
    strcpy(format_buffer,rhs.format_buffer);	
    }
  return *this;
}

//
// draw                    Draws a label anywhere in the HUD
//
//
void lon_label ::
draw( void )       // Required method in base class
{
//  char format_buffer[80];
  char label_buffer[80];
  int posincr;
  int lenstr;
  RECT  scrn_rect = get_location();
  float lon = get_value();
//  double lon = get_value();  
  if( data_available() ) {
//	char *lonstring = coord_format_lon(lon);
    lenstr = sprintf( label_buffer, format_buffer, coord_format_lon(lon) );
    }
  else {
    lenstr = sprintf( label_buffer, format_buffer );
    }
    
#ifdef DEBUGHUD
  fgPrintf( FG_COCKPIT, FG_DEBUG,  format_buffer );
  fgPrintf( FG_COCKPIT, FG_DEBUG,  "\n" );
  fgPrintf( FG_COCKPIT, FG_DEBUG, label_buffer );
  fgPrintf( FG_COCKPIT, FG_DEBUG, "\n" );
#endif
	
	lenstr = getStringWidth(label_buffer);
						
  if( justify == RIGHT_JUST ) {
	  posincr = scrn_rect.right - lenstr;
  }else if( justify == CENTER_JUST ) {
	  posincr = get_span() - (lenstr/2); //  -lenstr*4;
  }  else {
      //  justify == LEFT_JUST
      posincr = 0;  // 0;
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

