
#include "puLocal.h"

#define PUMENU_BUTTON_HEIGHT       25
#define PUMENU_BUTTON_EXTRA_WIDTH  25

puObject *puPopupMenu::add_item ( char *str, puCallback cb )
{
  int w, h ;
  getSize ( &w, &h ) ;
  puOneShot *b = new puOneShot ( 0, h, str ) ;
  b->setStyle        ( PUSTYLE_PLAIN ) ;
  b->setColourScheme ( colour[PUCOL_FOREGROUND][0],
		       colour[PUCOL_FOREGROUND][1],
		       colour[PUCOL_FOREGROUND][2],
		       colour[PUCOL_FOREGROUND][3] ) ;
  b->setCallback     ( cb ) ;
  recalc_bbox () ;
  return b ;
}

void puPopupMenu::close ( void )
{
  puPopup::close () ;

  int widest = 0 ;
  puObject *ob = dlist ;

  for ( ob = dlist ; ob != NULL ; ob = ob -> next )
  {
    int w, h ;

    ob -> getSize ( &w, &h ) ;

    if ( w > widest ) widest = w ;
  }

  for ( ob = dlist ; ob != NULL ; ob = ob -> next )
    ob -> setSize ( widest, PUMENU_BUTTON_HEIGHT ) ;

  recalc_bbox () ;
}


int puPopupMenu::checkKey ( int key, int updown )
{
  if ( dlist == NULL || ! isVisible () || ! isActive () )
    return FALSE ;

  if ( updown == PU_DOWN )
  {
    hide () ;

    /* Turn everything off ready for next time. */

    for ( puObject *bo = dlist ; bo != NULL ; bo = bo->next )
      bo -> clrValue () ;
  }

  puObject *bo ;

  /*
    We have to walk the list backwards to ensure that
    the click order is the same as the DRAW order.
  */

  for ( bo = dlist ; bo->next != NULL ; bo = bo->next )
    /* Find the last object in our list. */ ;

  for ( ; bo != NULL ; bo = bo->prev )
    if ( bo -> checkKey ( key, updown ) )
      return TRUE ;

  return FALSE ;
}


int puPopupMenu::checkHit ( int button, int updown, int x, int y )
{
  if ( dlist == NULL || ! isVisible () || ! isActive () )
    return FALSE ;

  /* Must test 'isHit' before making the menu invisible! */

  int hit = isHit ( x, y ) ;

  if ( updown == active_mouse_edge || active_mouse_edge == PU_UP_AND_DOWN )
  {
    hide () ;

    /* Turn everything off ready for next time. */

    for ( puObject *bo = dlist ; bo != NULL ; bo = bo->next )
      bo -> clrValue () ;
  }

  if ( ! hit )
    return FALSE ;

  /*
    This might be a bit redundant - but it's too hard to keep
    track of changing abox sizes when daughter objects are
    changing sizes.
  */

  recalc_bbox () ;

  puObject *bo ;

  x -= abox.min[0] ;
  y -= abox.min[1] ;

  /*
    We have to walk the list backwards to ensure that
    the click order is the same as the DRAW order.
  */

  for ( bo = dlist ; bo->next != NULL ; bo = bo->next )
    /* Find the last object in our list. */ ;

  for ( ; bo != NULL ; bo = bo->prev )
    if ( bo -> checkHit ( button, updown, x, y ) )
      return TRUE ;

  return FALSE ;
}

