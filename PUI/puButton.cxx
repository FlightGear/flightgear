
#include "puLocal.h"

void puButton::draw ( int dx, int dy )
{
  if ( !visible ) return ;

  /* If button is pushed or highlighted - use inverse style for button itself */

  abox . draw ( dx, dy, ( getValue() ^ highlighted ) ? -style : style, colour,
                                            isReturnDefault() ) ;

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


void puButton::doHit ( int button, int updown, int, int )
{
  if ( button == PU_LEFT_BUTTON )
  {
    if ( updown == active_mouse_edge || active_mouse_edge == PU_UP_AND_DOWN )
    {
      lowlight () ;
      setValue ( (int) ! getValue () ) ;
      invokeCallback () ;
    }
    else
      highlight () ;
  }
  else
    lowlight () ;
}

