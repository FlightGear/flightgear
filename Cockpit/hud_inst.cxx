// Abstract Base Class instr_item
//
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

UINT instr_item :: instances = 0;  // Initial value of zero
int  instr_item :: brightness = BRT_MEDIUM;
glRGBTRIPLE instr_item :: color = {0.1, 0.7, 0.0};

// constructor    ( No default provided )
instr_item  ::
   instr_item( int              x,
               int              y,
               UINT             width,
               UINT             height,
               DBLFNPTR         data_source,
               double           data_scaling,
               UINT             options,
               bool             working) :
                      handle         ( ++instances  ),
                      load_value_fn  ( data_source  ),
                      disp_factor    ( data_scaling ),
                      opts           ( options      ),
                      is_enabled     ( working      ),
                      broken         ( FALSE        )
{
  scrn_pos.left   = x;
  scrn_pos.top    = y;
  scrn_pos.right  = width;
  scrn_pos.bottom = height;

         // Set up convenience values for centroid of the box and
         // the span values according to orientation

  if( opts & HUDS_VERT) { // Vertical style
         // Insure that the midpoint marker will fall exactly at the
         // middle of the bar.
    if( !(scrn_pos.bottom % 2)) {
      scrn_pos.bottom++;
      }
    scr_span = scrn_pos.bottom;
    }
  else {
         // Insure that the midpoint marker will fall exactly at the
         // middle of the bar.
    if( !(scrn_pos.right % 2)) {
      scrn_pos.right++;
      }
    scr_span = scrn_pos.right;
    }
         // Here we work out the centroid for the corrected box.
  mid_span.x = scrn_pos.left   + (scrn_pos.right  >> 1);
  mid_span.y = scrn_pos.top + (scrn_pos.bottom >> 1);
}


// copy constructor
instr_item  ::
     instr_item ( const instr_item & image ):
                         handle       ( ++instances        ),
                         scrn_pos     ( image.scrn_pos     ),
                         load_value_fn( image.load_value_fn),
                         disp_factor  ( image.disp_factor  ),
                         opts         ( image.opts         ),
                         is_enabled   ( image.is_enabled   ),
                         broken       ( image.broken       ),
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
    disp_factor   = rhs.disp_factor;
    opts          = rhs.opts;
    is_enabled    = rhs.is_enabled;
    broken        = rhs.broken;
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

