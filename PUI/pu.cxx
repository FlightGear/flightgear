
#include "puLocal.h"

#define PU_STRING_X_FUDGE 6
#define PU_STRING_Y_FUDGE 6

int puRefresh = TRUE ;

puColour _puDefaultColourTable[] =
{
  { 0.5, 0.5, 0.5, 1.0 }, /* PUCOL_FOREGROUND */
  { 0.3, 0.3, 0.3, 1.0 }, /* PUCOL_BACKGROUND */
  { 0.7, 0.7, 0.7, 1.0 }, /* PUCOL_HIGHLIGHT  */
  { 0.0, 0.0, 0.0, 1.0 }, /* PUCOL_LABEL      */
  { 1.0, 1.0, 1.0, 1.0 }, /* PUCOL_TEXT       */

  { 0.0, 0.0, 0.0, 0.0 }  /* ILLEGAL */
} ;
 

puValue::~puValue () {}  

static int _puCursor_enable = FALSE ;
static int _puCursor_x      = 0 ;
static int _puCursor_y      = 0 ;
static float _puCursor_bgcolour [4] = { 1.0f, 1.0f, 1.0f, 1.0f } ; 
static float _puCursor_fgcolour [4] = { 0.0f, 0.0f, 0.0f, 1.0f } ;  

void   puHideCursor ( void ) { _puCursor_enable = FALSE ; }
void   puShowCursor ( void ) { _puCursor_enable = TRUE  ; }
int    puCursorIsHidden ( void ) { return ! _puCursor_enable ; }

void puCursor ( int x, int y )
{
  _puCursor_x = x ;
  _puCursor_y = y ;
}

int puGetStringDescender ( void *fnt )
{
  if ( fnt == NULL )
    fnt = GLUT_BITMAP_9_BY_15 ;

  if ( fnt == GLUT_BITMAP_8_BY_13        ) return 2 ;
  if ( fnt == GLUT_BITMAP_9_BY_15        ) return 3 ;
  if ( fnt == GLUT_BITMAP_TIMES_ROMAN_10 ) return 2 ;
  if ( fnt == GLUT_BITMAP_TIMES_ROMAN_24 ) return 5 ;
  if ( fnt == GLUT_BITMAP_HELVETICA_10   ) return 2 ;
  if ( fnt == GLUT_BITMAP_HELVETICA_12   ) return 3 ;
  if ( fnt == GLUT_BITMAP_HELVETICA_18   ) return 4 ;

  return 0 ;
}

int puGetStringHeight ( void *fnt )
{
  /* Height *excluding* descender */

  if ( fnt == NULL )
    fnt = GLUT_BITMAP_9_BY_15 ;

  if ( fnt == GLUT_BITMAP_8_BY_13        ) return  9 ;
  if ( fnt == GLUT_BITMAP_9_BY_15        ) return 10 ;
  if ( fnt == GLUT_BITMAP_TIMES_ROMAN_10 ) return  7 ;
  if ( fnt == GLUT_BITMAP_TIMES_ROMAN_24 ) return 17 ;
  if ( fnt == GLUT_BITMAP_HELVETICA_10   ) return  8 ;
  if ( fnt == GLUT_BITMAP_HELVETICA_12   ) return  9 ;
  if ( fnt == GLUT_BITMAP_HELVETICA_18   ) return 14 ;

  return 0 ;
}

int puGetStringWidth ( void *fnt, char *str )
{
  if ( str == NULL )
    return 0 ;

  if ( fnt == NULL )
    fnt = GLUT_BITMAP_9_BY_15 ;

  int res = 0 ;

  while ( *str != '\0' )
  {
    res += glutBitmapWidth ( fnt, *str ) ;
    str++ ;
  }

  return res ;
}


void puDrawString ( void *fnt, char *str, int x, int y )
{
  if ( str == NULL )
    return ;

  if ( fnt == NULL )
    fnt = GLUT_BITMAP_9_BY_15 ;

  glRasterPos2f ( x, y ) ;

  while ( *str != '\0' )
  {
    glutBitmapCharacter ( fnt, *str ) ;
    str++ ;
  }
}


static void puDrawCursor ( int x, int y )
{
  glColor4fv ( _puCursor_bgcolour ) ;  

  glBegin    ( GL_TRIANGLES ) ;
  glVertex2i ( x, y ) ;
  glVertex2i ( x + 13, y -  4 ) ;
  glVertex2i ( x +  4, y - 13 ) ;

  glVertex2i ( x +  8, y -  3 ) ;
  glVertex2i ( x + 17, y - 12 ) ;
  glVertex2i ( x + 12, y - 17 ) ;

  glVertex2i ( x + 12, y - 17 ) ;
  glVertex2i ( x +  3, y -  8 ) ;
  glVertex2i ( x +  8, y -  3 ) ;
  glEnd      () ;

  glColor4fv ( _puCursor_fgcolour ) ;  

  glBegin    ( GL_TRIANGLES ) ;
  glVertex2i ( x+1, y-1 ) ;
  glVertex2i ( x + 11, y -  4 ) ;
  glVertex2i ( x +  4, y - 11 ) ;

  glVertex2i ( x +  8, y -  5 ) ;
  glVertex2i ( x + 15, y - 12 ) ;
  glVertex2i ( x + 12, y - 15 ) ;

  glVertex2i ( x + 12, y - 15 ) ;
  glVertex2i ( x +  5, y -  8 ) ;
  glVertex2i ( x +  8, y -  5 ) ;
  glEnd      () ;
}

void  puInit ( void )
{
  static int firsttime = TRUE ;

  if ( firsttime )
  {
    puInterface *base_interface = new puInterface ( 0, 0 ) ;
    puPushInterface     ( base_interface ) ;
    puPushLiveInterface ( base_interface ) ;
    firsttime = FALSE ;
  }
}

static void puSetOpenGLState ( void )
{
  int w = glutGet ( (GLenum) GLUT_WINDOW_WIDTH  ) ;
  int h = glutGet ( (GLenum) GLUT_WINDOW_HEIGHT ) ;

  glPushAttrib   ( GL_ENABLE_BIT | GL_VIEWPORT_BIT | GL_TRANSFORM_BIT ) ;
  glDisable      ( GL_LIGHTING   ) ;
  glDisable      ( GL_FOG        ) ;
  glDisable      ( GL_TEXTURE_2D ) ;
  glDisable      ( GL_DEPTH_TEST ) ;
  glDisable      ( GL_CULL_FACE  ) ;
 
  glViewport     ( 0, 0, w, h ) ;
  glMatrixMode   ( GL_PROJECTION ) ;
  glPushMatrix   () ;
  glLoadIdentity () ;
  gluOrtho2D     ( 0, w, 0, h ) ;
  glMatrixMode   ( GL_MODELVIEW ) ;
  glPushMatrix   () ;
  glLoadIdentity () ;
}

static void puRestoreOpenGLState ( void )
{
  glMatrixMode   ( GL_PROJECTION ) ;
  glPopMatrix    () ;
  glMatrixMode   ( GL_MODELVIEW ) ;
  glPopMatrix    () ;
  glPopAttrib    () ;
}


void  puDisplay ( void )
{
  puSetOpenGLState () ;
  puGetUltimateLiveInterface () -> draw ( 0, 0 ) ;

  if ( _puCursor_enable )
    puDrawCursor ( _puCursor_x,
                   glutGet((GLenum)GLUT_WINDOW_HEIGHT) - _puCursor_y ) ;

  puRestoreOpenGLState () ;
}

int puKeyboard ( int key, int updown )
{
  return puGetBaseLiveInterface () -> checkKey ( key, updown ) ;
}


static int last_buttons = 0 ;

int puMouse ( int button, int updown, int x, int y )
{
  puCursor ( x, y ) ;

  if ( updown == PU_DOWN )
    last_buttons |=  ( 1 << button ) ;
  else
    last_buttons &= ~( 1 << button ) ;

  return puGetBaseLiveInterface () -> checkHit ( button, updown, x,
                                 glutGet((GLenum)GLUT_WINDOW_HEIGHT) - y ) ;
}

int puMouse ( int x, int y )
{
  puCursor ( x, y ) ;

  if ( last_buttons == 0 )
    return FALSE ;

  int button = (last_buttons & (1<<PU_LEFT_BUTTON  )) ?  PU_LEFT_BUTTON   :
               (last_buttons & (1<<PU_MIDDLE_BUTTON)) ?  PU_MIDDLE_BUTTON :
               (last_buttons & (1<<PU_RIGHT_BUTTON )) ?  PU_RIGHT_BUTTON  : 0 ;

  return puGetBaseLiveInterface () -> checkHit ( button, PU_DRAG, x,
                                 glutGet((GLenum)GLUT_WINDOW_HEIGHT) - y ) ;
}

