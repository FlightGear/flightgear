
#include "puLocal.h"

void puInput::normalize_cursors ( void )
{
  char val [ PUSTRING_MAX ] ;
  getValue ( val ) ;
  int sl = strlen ( val ) ;

  /* Clamp the positions to the limits of the text.  */

  if ( cursor_position       <  0 ) cursor_position       =  0 ;
  if ( select_start_position <  0 ) select_start_position =  0 ;
  if ( select_end_position   <  0 ) select_end_position   =  0 ;
  if ( cursor_position       > sl ) cursor_position       = sl ;
  if ( select_start_position > sl ) select_start_position = sl ;
  if ( select_end_position   > sl ) select_end_position   = sl ;

  /* Swap the ends of the select window if they get crossed over */

  if ( select_end_position < select_start_position )
  {
    int tmp = select_end_position ;     
    select_end_position = select_start_position ;     
    select_start_position = tmp ;
  }
}

void puInput::draw ( int dx, int dy )
{
  normalize_cursors () ;

  if ( !visible ) return ;

  /* 3D Input boxes look nicest if they are always in inverse style. */

  abox . draw ( dx, dy, (style==PUSTYLE_SMALL_BEVELLED) ? -style :
                        (accepting ? -style : style ), colour, FALSE ) ;

  int xx = puGetStringWidth ( legendFont, " " ) ;
  int yy = ( abox.max[1] - abox.min[1] - puGetStringHeight(legendFont) ) / 2 ;

  if ( accepting )
  {
    char val [ PUSTRING_MAX ] ;
    getValue ( val ) ;

    /* Highlight the select area */

    if ( select_end_position > 0 &&
         select_end_position != select_start_position )    
    {
      val [ select_end_position ] = '\0' ;
      int cpos2 = puGetStringWidth ( legendFont, val ) + xx + dx + abox.min[0] ;
      val [ select_start_position ] = '\0' ;
      int cpos1 = puGetStringWidth ( legendFont, val ) + xx + dx + abox.min[0] ;

      glColor3f ( 1.0, 1.0, 0.7 ) ;
      glRecti ( cpos1, dy + abox.min[1] + 6 ,
                cpos2, dy + abox.max[1] - 6 ) ;
    }
  }

  /* Draw the text */

  {
    /* If greyed out then halve the opacity when drawing the label and legend */

    if ( active )
      glColor4fv ( colour [ PUCOL_LEGEND ] ) ;
    else
      glColor4f ( colour [ PUCOL_LEGEND ][0],
		  colour [ PUCOL_LEGEND ][1],
		  colour [ PUCOL_LEGEND ][2],
		  colour [ PUCOL_LEGEND ][3] / 2.0 ) ; /* 50% more transparent */

    char val [ PUSTRING_MAX ] ;
    getValue ( val ) ;

    puDrawString ( legendFont, val,
                  dx + abox.min[0] + xx,
                  dy + abox.min[1] + yy ) ;

    draw_label ( dx, dy ) ;
  }

  if ( accepting )
  { 
    char val [ PUSTRING_MAX ] ;
    getValue ( val ) ;

    /* Draw the 'I' bar cursor. */

    if ( cursor_position >= 0 )
    {
      val [ cursor_position ] = '\0' ;

      int cpos = puGetStringWidth ( legendFont, val ) + xx + dx + abox.min[0] ;

      glColor3f ( 0.1, 0.1, 1.0 ) ;
      glBegin   ( GL_LINES ) ;
      glVertex2i ( cpos    , dy + abox.min[1] + 7 ) ;
      glVertex2i ( cpos    , dy + abox.max[1] - 7 ) ;
      glVertex2i ( cpos - 1, dy + abox.min[1] + 7 ) ;
      glVertex2i ( cpos - 1, dy + abox.max[1] - 7 ) ;
      glVertex2i ( cpos - 4, dy + abox.min[1] + 7 ) ;
      glVertex2i ( cpos + 3, dy + abox.min[1] + 7 ) ;
      glVertex2i ( cpos - 4, dy + abox.max[1] - 7 ) ;
      glVertex2i ( cpos + 3, dy + abox.max[1] - 7 ) ;
      glEnd      () ;
    }
  }
}


void puInput::doHit ( int button, int updown, int x, int /* y */ )
{
  if ( button == PU_LEFT_BUTTON )
  {
    /* Most GUI's activate a button on button-UP not button-DOWN. */

    if ( updown == active_mouse_edge || active_mouse_edge == PU_UP_AND_DOWN )
    {
      lowlight () ;

      char *strval ;
      getValue ( & strval ) ;
      char *tmpval = new char [ strlen(strval) + 1 ] ;
      strcpy ( tmpval, strval ) ;

      int i = strlen ( tmpval ) ;

      while ( x <= puGetStringWidth ( legendFont, tmpval ) + abox.min[0] &&
              i >= 0 )
        tmpval[--i] = '\0' ;
    
      accepting = TRUE ;
      cursor_position = i ;
      normalize_cursors () ;
      invokeCallback () ;
    }
    else
      highlight () ;
  }
  else
    lowlight () ;
}

int puInput::checkKey ( int key, int updown )
{
  (updown,updown);

  if ( ! isAcceptingInput() || ! isActive () || ! isVisible () )
    return FALSE ;

  normalize_cursors () ;

  char *p ;

  switch ( key )
  {
    case PU_KEY_PAGE_UP   :
    case PU_KEY_PAGE_DOWN :
    case PU_KEY_INSERT    : return FALSE ; 

    case PU_KEY_UP   :
    case PU_KEY_DOWN :
    case 0x1B /* ESC */ :
    case '\t' :
    case '\r' :
    case '\n' : /* Carriage return/Line Feed/TAB  -- End of input */
      rejectInput () ;
      normalize_cursors () ;
      invokeCallback () ;
      break ;

    case '\b' : /* Backspace */
      if ( cursor_position > 0 ) 
        for ( p = & string [ --cursor_position ] ; *p != '\0' ; p++ )
          *p = *(p+1) ;
      break ;

    case 0x7F : /* DEL */
      if ( select_start_position != select_end_position )
      {
        char *p1 = & string [ select_start_position ] ;
        char *p2 = & string [ select_end_position   ] ;

        while ( *p1 != '\0' )
          *p1++ = *p2++ ;

        select_end_position = select_start_position ;
      }
      else
	for ( p = & string [ cursor_position ] ; *p != '\0' ; p++ )
	  *p = *(p+1) ;
      break ;

    case 0x15 /* ^U */ : string [ 0 ] = '\0' ; break ;
    case PU_KEY_HOME   : cursor_position = 0 ; break ;
    case PU_KEY_END    : cursor_position = PUSTRING_MAX ; break ;
    case PU_KEY_LEFT   : cursor_position-- ; break ;
    case PU_KEY_RIGHT  : cursor_position++ ; break ;

    default:
      if ( key < ' ' || key > 127 ) return FALSE ;

      if ( strlen ( string ) >= PUSTRING_MAX )
        return FALSE ;

      for ( p = & string [ strlen(string) ] ;
               p != &string[cursor_position] ; p-- )
        *(p+1) = *p ;

      *p = key ;
      cursor_position++ ;
      break ;
  }

  setValue ( string ) ;
  normalize_cursors () ;
  return TRUE ;
}


