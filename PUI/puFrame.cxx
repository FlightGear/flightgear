

#include "puLocal.h"

void puFrame::draw ( int dx, int dy )
{
  if ( !visible ) return ;

  abox . draw ( dx, dy, style, colour, FALSE ) ;

  /* If greyed out then halve the opacity when drawing the label and legend */

  if ( active )
    glColor4fv ( colour [ PUCOL_LEGEND ] ) ;
  else
    glColor4f ( colour [ PUCOL_LEGEND ][0],
                colour [ PUCOL_LEGEND ][1],
                colour [ PUCOL_LEGEND ][2],
                colour [ PUCOL_LEGEND ][3] / 2.0 ) ; /* 50% more transparent */

  int xx = ( abox.max[0] - abox.min[0] - puGetStringWidth ( legendFont, legend ) ) / 2 ;

  puDrawString ( legendFont, legend,
                  dx + abox.min[0] + xx,
                  dy + abox.min[1] + puGetStringDescender ( legendFont ) + PUSTR_BGAP ) ;

  draw_label ( dx, dy ) ;
}


