
#include "puLocal.h"

void puSlider::draw ( int dx, int dy )
{
  if ( !visible ) return ;

  abox . draw ( dx, dy,
                style==PUSTYLE_BEVELLED ? -PUSTYLE_BOXED : -style,
                colour, FALSE ) ;

  int sd, od ;

  if ( isVertical() ) { sd = 1 ; od = 0 ; } else { sd = 0 ; od = 1 ; }

  int sz = abox.max [sd] - abox.min [sd] ;

  float val ;

  getValue ( & val ) ;

  if ( val < 0.0f ) val = 0.0f ;
  if ( val > 1.0f ) val = 1.0f ;

  val *= (float) sz * (1.0f - slider_fraction) ;

  puBox bx ;
    
  bx . min [ sd ] = abox . min [ sd ] + (int) val ;
  bx . max [ sd ] = (int) ( (float) bx . min [ sd ] + (float) sz * slider_fraction ) ;
  bx . min [ od ] = abox . min [ od ] + 2 ;
  bx . max [ od ] = abox . max [ od ] - 2 ;

  bx . draw ( dx, dy, PUSTYLE_SMALL_BEVELLED, colour, FALSE ) ;

  /* If greyed out then halve the opacity when drawing the label and legend */

  if ( active )
    glColor4fv ( colour [ PUCOL_LEGEND ] ) ;
  else
    glColor4f ( colour [ PUCOL_LEGEND ][0],
                colour [ PUCOL_LEGEND ][1],
                colour [ PUCOL_LEGEND ][2],
                colour [ PUCOL_LEGEND ][3] / 2.0 ) ; /* 50% more transparent */

  int xx = ( abox.max[0] - abox.min[0] - puGetStringWidth(legendFont,legend) ) / 2 ;
  int yy = ( abox.max[1] - abox.min[1] - puGetStringHeight(legendFont) ) / 2 ;

  puDrawString ( legendFont, legend,
                  dx + abox.min[0] + xx,
                  dy + abox.min[1] + yy ) ;

  draw_label ( dx, dy ) ;
}


void puSlider::doHit ( int button, int updown, int x, int y )
{
  if ( button == PU_LEFT_BUTTON )
  {
    int sd = isVertical() ;
    int sz = abox.max [sd] - abox.min [sd] ;
    int coord = isVertical() ? y : x ;

    float next_value ;

    if ( sz == 0 )
      next_value = 0.5f ;
    else
    {
      next_value = ( (float)coord - (float)abox.min[sd] - (float)sz * slider_fraction / 2.0f ) /
                   ( (float) sz * (1.0f - slider_fraction) ) ;
    }

    next_value = (next_value < 0.0f) ? 0.0f : (next_value > 1.0) ? 1.0f : next_value ;

    setValue ( next_value ) ;

    switch ( cb_mode )
    {
      case PUSLIDER_CLICK :
        if ( updown == active_mouse_edge )
        {
	  last_cb_value = next_value ;
	  invokeCallback () ;
        }
        break ;

      case PUSLIDER_DELTA :
        if ( fabs ( last_cb_value - next_value ) >= cb_delta )
        {
	  last_cb_value = next_value ;
	  invokeCallback () ;
        }
        break ;

      case PUSLIDER_ALWAYS :
      default :
        last_cb_value = next_value ;
        invokeCallback () ;
        break ;
    }
  }
}


