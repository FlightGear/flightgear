
#include "puLocal.h"

void drop_down_the_menu ( puObject *b )
{
  puPopupMenu *p = (puPopupMenu *) b -> getUserData () ;

  if ( b -> getValue () )
    p->reveal () ;
  else
    p->hide () ;

  for ( puObject *child = b -> getParent () -> getFirstChild () ;
        child != NULL ; child = child -> next )
  {
    if (( child -> getType() & PUCLASS_BUTTON    ) != 0 && child != b ) child -> clrValue () ;
    if (( child -> getType() & PUCLASS_POPUPMENU ) != 0 && child != p ) child -> hide     () ;
  }
}

void puMenuBar::add_submenu ( char *str, char *items[], puCallback cb[] )
{
  int w, h ;
  getSize ( &w, &h ) ;

  puOneShot    *b = new puOneShot ( w+10, 0, str ) ;
  b -> setStyle ( PUSTYLE_SPECIAL_UNDERLINED ) ;
  b -> setColourScheme ( colour[PUCOL_FOREGROUND][0],
                         colour[PUCOL_FOREGROUND][1],
                         colour[PUCOL_FOREGROUND][2],
                         colour[PUCOL_FOREGROUND][3] ) ;
  b -> setCallback ( drop_down_the_menu ) ;
  b -> setActiveDirn ( PU_UP_AND_DOWN ) ;

  puPopupMenu *p = new puPopupMenu ( w+10, 0 ) ;

  b -> setUserData ( p ) ;

  for ( int i = 0 ; items[i] != NULL ; i++ )
    p -> add_item ( items[i], cb[i] ) ;

  p->close () ;
  recalc_bbox () ;
}

void puMenuBar::close (void)
{
  puInterface::close () ;

  if ( dlist == NULL )
    return ;

  int width = 0 ;
  puObject *ob ;

  /*
    Use alternate objects - which gets the puOneShot/puPopupMenu pairs
  */

  for ( ob = dlist ; ob != NULL ; ob = ob -> next )
  {
    int w, h ;

    /* Reposition the button so it looks nice */

    ob -> getSize ( &w, &h ) ;
    ob -> setPosition ( width, 0 ) ;
    ob = ob -> next ;

    /* Reposition the submenu so it sits under the button */

    int w2, h2 ;
    ob -> getSize ( &w2, &h2 ) ;
    ob -> setPosition ( width, -h2 ) ;

    /* Next please! */
    width += w ;
  }

  recalc_bbox () ;
}


