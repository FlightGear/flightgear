
#include "puLocal.h"

puButtonBox::puButtonBox ( int minx, int miny, int maxx, int maxy,
                char **labels, int one_button ) :
                    puObject ( minx, miny, maxx, maxy )
{
  type |= PUCLASS_BUTTONBOX ;
  one_only = one_button ;

  button_labels = labels ;

  for ( num_kids = 0 ; button_labels [ num_kids ] != NULL ; num_kids++ )
    /* Count number of labels */ ;
}


int puButtonBox::checkKey ( int key, int updown )
{
  if ( updown == PU_UP ||
       ! isReturnDefault() ||
       ( key != '\r' && key != '\n' ) )
    return FALSE ;

  int v = getValue () ;

  if ( ! one_only )
    v = ~v ;
  else
  if ( v++ > num_kids )
    v = 0 ;

  setValue ( v ) ;
  invokeCallback() ;
  return TRUE ;
}


int puButtonBox::checkHit ( int button, int updown, int x, int y )
{
  if ( ! isHit ( x, y ) ||
       ( updown != active_mouse_edge &&
         active_mouse_edge != PU_UP_AND_DOWN ) )
    return FALSE ;

  int i = num_kids - 1 - (( y - abox.min[1] - PUSTR_BGAP ) * num_kids ) /
                          ( abox.max[1] - abox.min[1] - PUSTR_BGAP - PUSTR_TGAP ) ;

  if ( i < 0 ) i = 0 ;
  if ( i >= num_kids ) i = num_kids - 1 ;

  if ( one_only )
    setValue ( i ) ;
  else
    setValue ( getValue () ^ ( 1 << i ) ) ;

  invokeCallback () ;
  return TRUE ;
}


void puButtonBox::draw ( int dx, int dy )
{
  if ( !visible ) return ;

  abox . draw ( dx, dy, style, colour, isReturnDefault() ) ;

  for ( int i = 0 ; i < num_kids ; i++ )
  {
    puBox tbox ;

    tbox . min [ 0 ] = abox.min [ 0 ] + PUSTR_LGAP + PUSTR_LGAP ;
    tbox . min [ 1 ] = abox.min [ 1 ] + ((abox.max[1]-abox.min[1]-PUSTR_TGAP-PUSTR_BGAP)/num_kids) * (num_kids-1-i) ;
    tbox . max [ 0 ] = tbox.min [ 0 ] ;
    tbox . max [ 1 ] = tbox.min [ 1 ] ;

    if (( one_only && i == getValue() ) ||
        ( !one_only && ((1<<i) & getValue() ) != 0 ) ) 
      tbox . draw ( dx, dy + PUSTR_BGAP + PUSTR_BGAP, -PUSTYLE_RADIO, colour, FALSE ) ;
    else
      tbox . draw ( dx, dy + PUSTR_BGAP + PUSTR_BGAP, PUSTYLE_RADIO, colour, FALSE ) ;

    /* If greyed out then halve the opacity when drawing the label and legend */

    if ( active )
      glColor4fv ( colour [ PUCOL_LEGEND ] ) ;
    else
      glColor4f ( colour [ PUCOL_LEGEND ][0],
                  colour [ PUCOL_LEGEND ][1],
                  colour [ PUCOL_LEGEND ][2],
                  colour [ PUCOL_LEGEND ][3] / 2.0f ) ; /* 50% more transparent */

    puDrawString ( legendFont, button_labels[i],
                   dx + tbox.min[0] + PU_RADIO_BUTTON_SIZE + PUSTR_LGAP,
                   dy + tbox.min[1] + puGetStringDescender(legendFont) + PUSTR_BGAP  + PUSTR_BGAP) ;
  }

  draw_label ( dx, dy ) ;
}

