
#include "puLocal.h"

#ifdef PU_NOT_USING_GLUT
#include <assert.h>
#include <iostream.h>
#endif

#define PU_STRING_X_FUDGE 6
#define PU_STRING_Y_FUDGE 6

int puRefresh = TRUE ;

#ifdef PU_NOT_USING_GLUT

static int puWindowWidth  = 400 ;
static int puWindowHeight = 400 ;

int puGetWindowHeight () { return puWindowHeight ; }
int puGetWindowWidth  () { return puWindowWidth  ; }

void puSetWindowSize ( int width, int height )
{
  puWindowWidth  = width  ;
  puWindowHeight = height ;
}

static int fontBase = 0;
static int fontSize[257];
#else

int puGetWindowHeight () { return glutGet ( (GLenum) GLUT_WINDOW_HEIGHT ) ; }
int puGetWindowWidth  () { return glutGet ( (GLenum) GLUT_WINDOW_WIDTH  ) ; }

void puSetWindowSize ( int width, int height )
{
  fprintf ( stderr, "PUI: puSetWindowSize shouldn't be used with GLUT.\n" ) ;
}

#endif

puColour _puDefaultColourTable[] =
{
  { 0.5f, 0.5f, 0.5f, 1.0f }, /* PUCOL_FOREGROUND */
  { 0.3f, 0.3f, 0.3f, 1.0f }, /* PUCOL_BACKGROUND */
  { 0.7f, 0.7f, 0.7f, 1.0f }, /* PUCOL_HIGHLIGHT  */
  { 0.0f, 0.0f, 0.0f, 1.0f }, /* PUCOL_LABEL      */
  { 1.0f, 1.0f, 1.0f, 1.0f }, /* PUCOL_TEXT       */

  { 0.0f, 0.0f, 0.0f, 0.0f }  /* ILLEGAL */
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
    fnt = PUFONT_9_BY_15 ;

  if ( fnt == PUFONT_8_BY_13        ) return 2 ;
  if ( fnt == PUFONT_9_BY_15        ) return 3 ;
  if ( fnt == PUFONT_TIMES_ROMAN_10 ) return 2 ;
  if ( fnt == PUFONT_TIMES_ROMAN_24 ) return 5 ;
  if ( fnt == PUFONT_HELVETICA_10   ) return 2 ;
  if ( fnt == PUFONT_HELVETICA_12   ) return 3 ;
  if ( fnt == PUFONT_HELVETICA_18   ) return 4 ;

  return 0 ;
}

int puGetStringHeight ( void *fnt )
{
  /* Height *excluding* descender */
  if ( fnt == NULL )
    fnt = PUFONT_9_BY_15 ;

  if ( fnt == PUFONT_8_BY_13        ) return  9 ;
  if ( fnt == PUFONT_9_BY_15        ) return 10 ;
  if ( fnt == PUFONT_TIMES_ROMAN_10 ) return  7 ;
  if ( fnt == PUFONT_TIMES_ROMAN_24 ) return 17 ;
  if ( fnt == PUFONT_HELVETICA_10   ) return  8 ;
  if ( fnt == PUFONT_HELVETICA_12   ) return  9 ;
  if ( fnt == PUFONT_HELVETICA_18   ) return 14 ;

  return 0 ;
}

int puGetStringWidth ( void *fnt, char *str )
{

  if ( str == NULL )
    return 0 ;

  int res = 0 ;

#ifdef PU_NOT_USING_GLUT
  while ( *str != '\0' )
  {
    res += fontSize [ *str ] ;
    str++ ;
  }
#else
  if ( fnt == NULL )
    fnt = PUFONT_9_BY_15 ;

  while ( *str != '\0' )
  {
    res += glutBitmapWidth ( fnt, *str ) ;
    str++ ;
  }
#endif

  return res ;
}


void puDrawString ( void *fnt, char *str, int x, int y )
{
  if ( str == NULL )
    return ;

  glRasterPos2f((float)x, (float)y); 

#ifdef PU_NOT_USING_GLUT
  /*
    Display a string: 
       indicate start of glyph display lists 
  */

  glListBase (fontBase);

  /* Now draw the characters in a string */

  int len = strlen(str);
  glCallLists(len, GL_UNSIGNED_BYTE, str); 
  glListBase(0);
#else
  if ( fnt == NULL )
    fnt = PUFONT_9_BY_15 ;

  while ( *str != '\0' )
  {
    glutBitmapCharacter ( fnt, *str ) ;
    str++ ;
  }
#endif
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
#ifdef PU_NOT_USING_GLUT

    /* Create bitmaps for the device context font's first 256 glyphs */

    fontBase = glGenLists(256);
    assert(fontBase);
    HDC hdc = wglGetCurrentDC();

    /* Make the system font the device context's selected font */

    SelectObject (hdc, GetStockObject (SYSTEM_FONT)); 

    int *tempSize = &fontSize[1];

    if ( ! GetCharWidth32 ( hdc, 1, 255, tempSize ) )
    {
      LPVOID lpMsgBuf ;

      FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM,
		      NULL,
		      GetLastError(),
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		      (LPTSTR) &lpMsgBuf,
		      0, NULL ) ;

      fprintf ( stderr, "PUI: Error: %s\n" (char *)lpMsgBuf ) ;
      LocalFree ( lpMsgBuf ) ;
    }
    wglUseFontBitmaps ( hdc, 0, 256, fontBase ) ;
#endif
  }
}

static void puSetOpenGLState ( void )
{
  int w = puGetWindowWidth  () ;
  int h = puGetWindowHeight () ;

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

  int h = puGetWindowHeight () ;

  if ( _puCursor_enable )
    puDrawCursor ( _puCursor_x,
                   h - _puCursor_y ) ;

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

  int h = puGetWindowHeight () ;

  if ( updown == PU_DOWN )
    last_buttons |=  ( 1 << button ) ;
  else
    last_buttons &= ~( 1 << button ) ;

  return puGetBaseLiveInterface () -> checkHit ( button, updown, x,
                                 h - y ) ;
}

int puMouse ( int x, int y )
{
  puCursor ( x, y ) ;

  if ( last_buttons == 0 )
    return FALSE ;

  int button = (last_buttons & (1<<PU_LEFT_BUTTON  )) ?  PU_LEFT_BUTTON   :
               (last_buttons & (1<<PU_MIDDLE_BUTTON)) ?  PU_MIDDLE_BUTTON :
               (last_buttons & (1<<PU_RIGHT_BUTTON )) ?  PU_RIGHT_BUTTON  : 0 ;

  int h = puGetWindowHeight () ;

  return puGetBaseLiveInterface () -> checkHit ( button, PU_DRAG, x,
                                 h - y ) ;
}

