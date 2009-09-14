#ifndef _PU_H_
#define _PU_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef PU_NOT_USING_GLUT
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GL/glut.h>
#endif

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

/*
  Webster's Dictionary (for American English) permits
  Color or Colour as acceptable spellings - but
  The Oxford English Dictionary (for English) only
  permits Colour.

  Hence, the logical thing to do is to use 'colour',
  which *ought* to be acceptable on both sides of
  the atlantic.

  However, as a concession to the illogical:
*/

#define setColorScheme          setColourScheme
#define setColor                setColour
#define getColor                getColour
#define puColor                 puColour
#define puSetColor              puSetColour
#define puSetDefaultColorScheme puSetDefaultColourScheme
#define puGetDefaultColorScheme puGetDefaultColourScheme


typedef void *puFont ;

#ifdef PU_NOT_USING_GLUT
#define PU_LEFT_BUTTON		0
#define PU_LEFT_BUTTON		0
#define PU_MIDDLE_BUTTON	1
#define PU_RIGHT_BUTTON		2
#define PU_DOWN			0
#define PU_UP		        1

#define PUFONT_8_BY_13        ((void*)3)
#define PUFONT_9_BY_15        ((void*)2)
#define PUFONT_TIMES_ROMAN_10 ((void*)4)
#define PUFONT_TIMES_ROMAN_24 ((void*)5)
#define PUFONT_HELVETICA_10   ((void*)6)
#define PUFONT_HELVETICA_12   ((void*)7)
#define PUFONT_HELVETICA_18   ((void*)8)

#else

#define PUFONT_8_BY_13        GLUT_BITMAP_8_BY_13
#define PUFONT_9_BY_15        GLUT_BITMAP_9_BY_15
#define PUFONT_TIMES_ROMAN_10 GLUT_BITMAP_TIMES_ROMAN_10
#define PUFONT_TIMES_ROMAN_24 GLUT_BITMAP_TIMES_ROMAN_24
#define PUFONT_HELVETICA_10   GLUT_BITMAP_HELVETICA_10
#define PUFONT_HELVETICA_12   GLUT_BITMAP_HELVETICA_12
#define PUFONT_HELVETICA_18   GLUT_BITMAP_HELVETICA_18

#define PU_LEFT_BUTTON      GLUT_LEFT_BUTTON
#define PU_MIDDLE_BUTTON    GLUT_MIDDLE_BUTTON
#define PU_RIGHT_BUTTON     GLUT_RIGHT_BUTTON
#define PU_DOWN             GLUT_DOWN
#define PU_UP               GLUT_UP
#endif	// PU_NOT_USING_GLUT

#define PU_UP_AND_DOWN   254
#define PU_DRAG          255
#define PU_CONTINUAL     PU_DRAG

#define PU_KEY_GLUT_SPECIAL_OFFSET  256

#ifdef PU_NOT_USING_GLUT
#define PU_KEY_F1        (1		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F2        (2		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F3        (3		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F4        (4	    + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F5        (5		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F6        (6		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F7        (7		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F8        (8		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F9        (9		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F10       (10		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F11       (11		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F12       (12		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_LEFT      (100		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_UP        (101		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_RIGHT     (102		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_DOWN      (103		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_PAGE_UP   (104		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_PAGE_DOWN (105		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_HOME      (106		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_END       (107		+ PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_INSERT    (108		+ PU_KEY_GLUT_SPECIAL_OFFSET)

#else
#define PU_KEY_F1        (GLUT_KEY_F1        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F2        (GLUT_KEY_F2        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F3        (GLUT_KEY_F3        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F4        (GLUT_KEY_F4        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F5        (GLUT_KEY_F5        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F6        (GLUT_KEY_F6        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F7        (GLUT_KEY_F7        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F8        (GLUT_KEY_F8        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F9        (GLUT_KEY_F9        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F10       (GLUT_KEY_F10       + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F11       (GLUT_KEY_F11       + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_F12       (GLUT_KEY_F12       + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_LEFT      (GLUT_KEY_LEFT      + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_UP        (GLUT_KEY_UP        + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_RIGHT     (GLUT_KEY_RIGHT     + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_DOWN      (GLUT_KEY_DOWN      + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_PAGE_UP   (GLUT_KEY_PAGE_UP   + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_PAGE_DOWN (GLUT_KEY_PAGE_DOWN + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_HOME      (GLUT_KEY_HOME      + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_END       (GLUT_KEY_END       + PU_KEY_GLUT_SPECIAL_OFFSET)
#define PU_KEY_INSERT    (GLUT_KEY_INSERT    + PU_KEY_GLUT_SPECIAL_OFFSET)
#endif	// PU_NOT_USING_GLUT

#define PUPLACE_DEFAULT  PUPLACE_RIGHT
#define PUPLACE_ABOVE    0
#define PUPLACE_BELOW    1
#define PUPLACE_LEFT     2
#define PUPLACE_RIGHT    3

#define PUCOL_FOREGROUND 0
#define PUCOL_BACKGROUND 1
#define PUCOL_HIGHLIGHT  2
#define PUCOL_LABEL      3
#define PUCOL_LEGEND     4
#define PUCOL_MAX        5

#define PUSLIDER_CLICK   0
#define PUSLIDER_ALWAYS  1
#define PUSLIDER_DELTA   2

/* These styles may be negated to get 'highlighted' graphics */

#define PUSTYLE_DEFAULT    PUSTYLE_SHADED
#define PUSTYLE_NONE       0
#define PUSTYLE_PLAIN      1
#define PUSTYLE_BEVELLED   2
#define PUSTYLE_BOXED      3
#define PUSTYLE_DROPSHADOW 4
#define PUSTYLE_SPECIAL_UNDERLINED 5
#define PUSTYLE_SMALL_BEVELLED     6
#define PUSTYLE_RADIO      7
#define PUSTYLE_SHADED     8
#define PUSTYLE_SMALL_SHADED   9
#define PUSTYLE_MAX        10

/* These are the gaps that we try to leave around text objects */

#define PUSTR_TGAP   5
#define PUSTR_BGAP   5
#define PUSTR_LGAP   5
#define PUSTR_RGAP   5
#define PUSTR_MAX_HEIGHT  ( 25 + PUSTR_TGAP + PUSTR_BGAP )

#define PU_RADIO_BUTTON_SIZE 16

extern int puRefresh ;

#define PUCLASS_VALUE            0x00000001
#define PUCLASS_OBJECT           0x00000002
#define PUCLASS_INTERFACE        0x00000004
#define PUCLASS_FRAME            0x00000008
#define PUCLASS_TEXT             0x00000010
#define PUCLASS_BUTTON           0x00000020
#define PUCLASS_ONESHOT          0x00000040
#define PUCLASS_POPUP            0x00000080
#define PUCLASS_POPUPMENU        0x00000100
#define PUCLASS_MENUBAR          0x00000200
#define PUCLASS_INPUT            0x00000400
#define PUCLASS_BUTTONBOX        0x00000800
#define PUCLASS_SLIDER           0x00001000
#define PUCLASS_DIALOGBOX        0x00002000

/* This function is not required for GLUT programs */
void puSetWindowSize ( int width, int height ) ;

int  puGetWindowHeight () ;
int  puGetWindowWidth  () ;

class puValue            ;
class puObject           ;
class puInterface        ;
class puButtonBox        ;
class puFrame            ;
class puText             ;
class puButton           ;
class puOneShot          ;
class puPopup            ;
class puPopupMenu        ;
class puMenuBar          ;
class puInput            ;
class puSlider           ;

typedef float puColour [ 4 ] ;  /* RGBA */

struct puBox
{
  int min [ 2 ] ;
  int max [ 2 ] ;

  void draw   ( int dx, int dy, int style, puColour colour[], int am_default ) ;
  void extend ( puBox *bx ) ;

  void empty   ( void ) { min[0]=min[1]=1000000 ; max[0]=max[1]=-1000000 ; }
  int  isEmpty ( void ) { return min[0]>max[0] || min[1]>max[1] ; }
} ;

#define PUSTRING_MAX 80

/* If you change - or add to these, be sure to change _puDefaultColourTable */

extern puColour _puDefaultColourTable[] ;


inline void puSetColour ( puColour dst, puColour src )
{
  dst[0] = src[0] ; dst[1] = src[1] ; dst[2] = src[2] ; dst[3] = src[3] ;
}

inline void puSetColour ( puColour c, float r, float g, float b, float a = 1.0f )
{
  c [ 0 ] = r ; c [ 1 ] = g ; c [ 2 ] = b ; c [ 3 ] = a ;
}


void  puInit           ( void ) ;
void  puDisplay        ( void ) ;
int   puMouse          ( int button, int updown, int x, int y ) ;
int   puMouse          ( int x, int y ) ;
int   puKeyboard       ( int key, int updown ) ;
void  puHideCursor     ( void ) ;
void  puShowCursor     ( void ) ;
int   puCursorIsHidden ( void ) ;

void puDrawString         ( puFont fnt, char *str, int x, int y ) ;
int  puGetStringWidth     ( puFont fnt, char *str ) ;
int  puGetStringHeight    ( puFont fnt = NULL ) ;
int  puGetStringDescender ( puFont fnt = NULL ) ;

class puValue
{
protected:
  int   type    ;
  int   integer ;
  float floater ;
  char  string  [ PUSTRING_MAX ] ;
public:
  puValue () { type = PUCLASS_VALUE ; clrValue () ; }

  virtual ~puValue () ;

  int  getType ( void ) { return type ; }
  char *getTypeString ( void ) ;
  void clrValue ( void ) { setValue ( "" ) ; }

  void setValue ( puValue *pv )
  {
    integer = pv -> integer ;
    floater = pv -> floater ;
    strcpy ( string, pv -> string ) ;
    puRefresh = TRUE ;
  }

  void setValue ( int   i ) { integer = i ; floater = (float) i ; sprintf ( string, "%d", i ) ; puRefresh = TRUE ; }
  void setValue ( float f ) { integer = (int) f ; floater = f ; sprintf ( string, "%g", f ) ; puRefresh = TRUE ; }
  void setValue ( char *s ) { 
                              if ( s == NULL || s[0] == '\0' )
                              {
                                integer = 0 ;
                                floater = 0.0f ;
                                s = "" ;
                              }
                              else
                              {
                                integer = atoi(s) ;
                                floater = (float)atof(s) ;

                                if ( string != s ) strcpy ( string, s ) ;
                              }
                              puRefresh = TRUE ;
                            }

  void getValue ( int   *i ) { *i = integer ; }
  void getValue ( float *f ) { *f = floater ; }
  void getValue ( char **s ) { *s = string  ; }
  void getValue ( char  *s ) { strcpy ( s, string ) ; }

  int  getValue ( void ) { return integer ; }
} ;

typedef void (*puCallback)(class puObject *) ;

void puSetDefaultStyle ( int  style ) ;
int  puGetDefaultStyle ( void ) ;
void puSetDefaultFonts ( puFont  legendFont, puFont  labelFont ) ;
void puGetDefaultFonts ( puFont *legendFont, puFont *labelFont ) ;
void puSetDefaultColourScheme ( float r, float g, float b, float a = 1.0 ) ;
void puGetDefaultColourScheme ( float *r, float *g, float *b, float *a = NULL );

class puObject : public puValue
{
protected:
  puValue default_value ;

  puBox bbox ;   /* Bounding box of entire Object */
  puBox abox ;   /* Active (clickable) area */
  puColour colour [ PUCOL_MAX ] ;
  puInterface *parent ;

  int active_mouse_edge ; /* is it PU_UP or PU_DOWN (or both) that activates this? */
  int style       ;
  int visible     ;
  int active      ;
  int highlighted ;
  int am_default  ;

  char *label  ; puFont  labelFont ; int labelPlace ;
  char *legend ; puFont legendFont ;

  void *user_data ;
  puCallback cb ;

  virtual void draw_label  ( int dx, int dy ) ;
  virtual int  isHit ( int x, int y ) { return isVisible() && isActive() &&
                                               x >= abox.min[0] &&
                                               x <= abox.max[0] &&
                                               y >= abox.min[1] &&
                                               y <= abox.max[1] ; }
  virtual void doHit ( int button, int updown, int x, int y ) ;

public:
   puObject ( int minx, int miny, int maxx, int maxy ) ;
  ~puObject () ;

  puObject *next ;
  puObject *prev ;
 
  puBox *getBBox ( void ) { return & bbox ; }
  puBox *getABox ( void ) { return & abox ; }

  void setPosition ( int x, int y )
  {
    if ( abox.isEmpty() )
    {
      abox.max[0] = abox.min[0] = x ;
      abox.max[1] = abox.min[1] = y ;
    }
    else
    {
      abox.max[0] += x - abox.min[0] ;
      abox.max[1] += y - abox.min[1] ;
      abox.min[0]  = x ;
      abox.min[1]  = y ;
    }
    recalc_bbox() ; puRefresh = TRUE ;
  }

  void setSize ( int w, int h )
  {
    abox.max[0] = abox.min[0] + w ;
    abox.max[1] = abox.min[1] + h ;
    recalc_bbox() ; puRefresh = TRUE ;
  }

  void getPosition ( int *x, int *y )
  {
    if ( abox . isEmpty () )
    {
      if ( x ) *x = 0 ;
      if ( y ) *y = 0 ;
    }
    else
    {
      if ( x ) *x = abox.min[0] ;
      if ( y ) *y = abox.min[1] ;
    }
  }

  void getSize ( int *w, int *h )
  {
    if ( abox . isEmpty () )
    {
      if ( w ) *w = 0 ;
      if ( h ) *h = 0 ;
    }
    else
    {
      if ( w ) *w = abox.max[0] - abox.min[0] ;
      if ( h ) *h = abox.max[1] - abox.min[1] ;
    }
  }

  virtual void recalc_bbox ( void ) ;
  virtual int  checkHit ( int button, int updown, int x, int y ) ;
  virtual int  checkKey ( int key   , int updown ) ;
  virtual void draw ( int dx, int dy ) = 0 ;

  puInterface *getParent     ( void ) { return parent ; }
  puObject    *getNextObject ( void ) { return next   ; }
  puObject    *getPrevObject ( void ) { return prev   ; }

  void       setCallback ( puCallback c ) { cb = c ;    }
  puCallback getCallback ( void )               { return cb ; }
  void       invokeCallback ( void ) { if ( cb ) (*cb)(this) ; }

  void  makeReturnDefault ( int def ) { am_default = def ; }
  int   isReturnDefault   ( void )          { return am_default ; }

  void  setActiveDirn ( int e ) { active_mouse_edge = e ; }
  int   getActiveDirn ( void ) { return active_mouse_edge ; }

  void  setLegend ( char *l ) { legend = l ; recalc_bbox() ; puRefresh = TRUE ; }
  char *getLegend ( void ) { return legend ; }

  void  setLegendFont ( puFont f ) { legendFont = f ; recalc_bbox() ; puRefresh = TRUE ; }
  puFont getLegendFont ( void ) { return legendFont ; }

  void  setLabel ( char *l ) { label = l ; recalc_bbox() ; puRefresh = TRUE ; }
  char *getLabel ( void ) { return label ; }

  void  setLabelFont ( puFont f ) { labelFont = f ; recalc_bbox() ; puRefresh = TRUE ; }
  puFont getLabelFont ( void ) { return labelFont ; }

  void  setLabelPlace ( int lp ) { labelPlace = lp ; recalc_bbox() ; puRefresh = TRUE ; }
  int   getLabelPlace ( void ) { return labelPlace ; }

  void activate   ( void ) { if ( ! active  ) { active  = TRUE  ; puRefresh = TRUE ; } }
  void greyOut    ( void ) { if (   active  ) { active  = FALSE ; puRefresh = TRUE ; } }
  int  isActive   ( void ) { return active ; }

  void highlight  ( void ) { if ( ! highlighted ) { highlighted = TRUE  ; puRefresh = TRUE ; } }
  void lowlight   ( void ) { if (   highlighted ) { highlighted = FALSE ; puRefresh = TRUE ; } }
  int isHighlighted( void ){ return highlighted ; }

  void reveal     ( void ) { if ( ! visible ) { visible = TRUE  ; puRefresh = TRUE ; } }
  void hide       ( void ) { if (   visible ) { visible = FALSE ; puRefresh = TRUE ; } }
  int  isVisible  ( void ) { return visible ; }

  void setStyle ( int which )
  {
    style = which ;
    recalc_bbox () ;
    puRefresh = TRUE ;
  }

  int  getStyle ( void ) { return style ; }

  void setColourScheme ( float r, float g, float b, float a = 1.0f ) ;

  void setColour ( int which, float r, float g, float b, float a = 1.0f )
  {
    puSetColour ( colour [ which ], r, g, b, a ) ;
    puRefresh = TRUE ;
  }

  void getColour ( int which, float *r, float *g, float *b, float *a = NULL )
  {
    if ( r ) *r = colour[which][0] ;
    if ( g ) *g = colour[which][1] ;
    if ( b ) *b = colour[which][2] ;
    if ( a ) *a = colour[which][3] ;
  }

  void  setUserData ( void *data ) { user_data = data ; }
  void *getUserData ( void )             { return user_data ; }

  void defaultValue ( void ) { setValue ( & default_value ) ; }

  void setDefaultValue ( int    i ) { default_value . setValue ( i ) ; }
  void setDefaultValue ( float  f ) { default_value . setValue ( f ) ; }
  void setDefaultValue ( char  *s ) { default_value . setValue ( s ) ; }

  void getDefaultValue ( int   *i ) { default_value . getValue ( i ) ; }
  void getDefaultValue ( float *f ) { default_value . getValue ( f ) ; }
  void getDefaultValue ( char **s ) { default_value . getValue ( s ) ; }
  int  getDefaultValue ( void )    { return default_value . getValue () ; }
} ;

/*
  The 'live' interface stack is used for clicking and rendering.
*/

void         puPushLiveInterface        ( puInterface *in ) ;
void         puPopLiveInterface         ( void ) ;
int          puNoLiveInterface          ( void ) ;
puInterface *puGetBaseLiveInterface     ( void ) ;
puInterface *puGetUltimateLiveInterface ( void ) ;

/*
  The regular interface stack is used for adding widgets
*/

void         puPushInterface        ( puInterface *in ) ;
void         puPopInterface         ( void ) ;
int          puNoInterface          ( void ) ;
puInterface *puGetCurrInterface     ( void ) ;

class puInterface : public puObject
{
protected:
  int num_children ;
  puObject *dlist ;

  void doHit       ( int button, int updown, int x, int y ) ;

public:

  puInterface ( int x, int y ) : puObject ( x, y, x, y )
  {
    type |= PUCLASS_INTERFACE ;
    dlist = NULL ;
    num_children = 0 ;
    puPushInterface ( this ) ;
    puPushLiveInterface ( this ) ;
  }

  ~puInterface () ;

  void recalc_bbox ( void ) ;
  virtual void add    ( puObject *new_object ) ;
  virtual void remove ( puObject *old_object ) ;

  void draw        ( int dx, int dy ) ;
  int  checkHit    ( int button, int updown, int x, int y ) ;
  int  checkKey    ( int key   , int updown ) ;

  puObject *getFirstChild ( void ) { return dlist ; }
  int getNumChildren ( void ) { return num_children ; }

  virtual void close ( void )
  {
    if ( puGetCurrInterface () != this )
      fprintf ( stderr, "PUI: puInterface::close() is mismatched!\n" ) ;
    else
      puPopInterface () ;
  }
} ;

class puFrame : public puObject
{
protected:
  virtual int  isHit ( int /* x */, int /* y */ ) { return FALSE ; }
public:
  void draw ( int dx, int dy ) ;
  puFrame ( int minx, int miny, int maxx, int maxy ) :
             puObject ( minx, miny, maxx, maxy )
  {
    type |= PUCLASS_FRAME ;
  }
} ;


class puText : public puObject
{
protected:
  virtual int  isHit ( int /* x */, int /* y */ ) { return FALSE ; }
public:
  void draw ( int dx, int dy ) ;
  puText ( int x, int y ) : puObject ( x, y, x, y )
  {
    type |= PUCLASS_TEXT ;
  }
} ;


class puButton : public puObject
{
protected:
public:
  void doHit ( int button, int updown, int x, int y ) ;
  void draw  ( int dx, int dy ) ;
  puButton   ( int minx, int miny, char *l ) :
                 puObject ( minx, miny,
                            minx + puGetStringWidth  ( NULL, l ) + PUSTR_LGAP + PUSTR_RGAP,
                            miny + puGetStringHeight () + puGetStringDescender () + PUSTR_TGAP + PUSTR_BGAP )
  {
    type |= PUCLASS_BUTTON ;
    setLegend ( l ) ;
  }

  puButton   ( int minx, int miny, int maxx, int maxy ) :
                 puObject ( minx, miny, maxx, maxy )
  {
    type |= PUCLASS_BUTTON ;
  }
} ;


class puSlider : public puObject
{
protected:
  int vert ;
  float last_cb_value ;
  float cb_delta ;
  int   cb_mode ;
  float slider_fraction ;
public:
  void doHit ( int button, int updown, int x, int y ) ;
  void draw  ( int dx, int dy ) ;
  puSlider ( int minx, int miny, int sz, int vertical = FALSE ) :
     puObject ( minx, miny, vertical ?
                             ( minx + puGetStringWidth ( NULL, "W" ) +
                                      PUSTR_LGAP + PUSTR_RGAP ) :
                             ( minx + sz ),
                            vertical ?
                             ( miny + sz ) :
                             ( miny + puGetStringHeight () +
                                      puGetStringDescender () +
                                      PUSTR_TGAP + PUSTR_BGAP )
                           )
  {
    type |= PUCLASS_SLIDER ;
    slider_fraction = 0.1f ;
    getValue ( & last_cb_value ) ;
    vert = vertical ;
    cb_delta = 0.1f ;
    cb_mode = PUSLIDER_ALWAYS ;
  }

  void setCBMode ( int m ) { cb_mode = m ; }
  float getCBMode ( void ) { return (float)cb_mode ; }

  int  isVertical ( void ) { return vert ; }

  void setDelta ( float f ) { cb_delta = (f<=0.0f) ? 0.1f : (f>=1.0f) ? 0.9f : f ; }
  float getDelta ( void ) { return cb_delta ; }

  void setSliderFraction ( float f ) { slider_fraction = (f<=0.0f) ? 0.1f : (f>=1.0f) ? 0.9f : f ; }
  float getSliderFraction ( void ) { return slider_fraction ; }
} ;



class puOneShot : public puButton
{
protected:
public:
  void doHit ( int button, int updown, int x, int y ) ;

  puOneShot ( int minx, int miny, char *l ) : puButton   ( minx, miny, l )
  {
    type |= PUCLASS_ONESHOT ;
  }

  puOneShot ( int minx, int miny, int maxx, int maxy ) :
                 puButton ( minx, miny, maxx, maxy )
  {
    type |= PUCLASS_ONESHOT ;
  }
} ;



class puPopup : public puInterface
{
protected:
public:
  puPopup ( int x, int y ) : puInterface ( x, y )
  {
    type |= PUCLASS_POPUP ;
    hide () ;
  }
} ;

class puPopupMenu : public puPopup
{
protected:
public:
  puPopupMenu ( int x, int y ) : puPopup ( x, y )
  {
    type |= PUCLASS_POPUPMENU ;
  }

  puObject *add_item ( char *str, puCallback cb ) ;
  int  checkHit ( int button, int updown, int x, int y ) ;
  int  checkKey ( int key   , int updown ) ;
  void close ( void ) ;
} ;

class puMenuBar : public puInterface
{
protected:
public:
  puMenuBar ( int h = -1 ) :

  puInterface ( 0, h < 0 ? puGetWindowHeight() -
                      ( puGetStringHeight() + PUSTR_TGAP + PUSTR_BGAP ) : h )
  {
    type |= PUCLASS_MENUBAR ;
  }

  void add_submenu ( char *str, char *items[], puCallback cb[] ) ;
  void close ( void ) ;
} ;


class puInput : public puObject
{
  int accepting ;
  int cursor_position ;
  int select_start_position ;
  int select_end_position ;

  void normalize_cursors ( void ) ;

public:
  void draw     ( int dx, int dy ) ;
  void doHit    ( int button, int updown, int x, int y ) ;
  int  checkKey ( int key, int updown ) ;

  int  isAcceptingInput ( void ) { return accepting ; }
  void rejectInput      ( void ) { accepting = FALSE ; }
  void acceptInput      ( void ) { accepting = TRUE ;
                             cursor_position = strlen ( string ) ;
                             select_start_position = select_end_position = -1 ; }

  int  getCursor ( void )        { return cursor_position ; }
  void setCursor ( int c ) { cursor_position = c ; }

  void setSelectRegion ( int s, int e )
  {
    select_start_position = s ;
    select_end_position   = e ;
  }

  void getSelectRegion ( int *s, int *e )
  {
    if ( s ) *s = select_start_position ;
    if ( e ) *e = select_end_position   ;
  }

  puInput ( int minx, int miny, int maxx, int maxy ) :
             puObject ( minx, miny, maxx, maxy )
  {
    type |= PUCLASS_INPUT ;

    accepting = FALSE ;

    cursor_position       =  0 ;
    select_start_position = -1 ;
    select_end_position   = -1 ;

    setColourScheme ( 0.8f, 0.7f, 0.7f ) ; /* Yeukky Pink */
  }
} ;


class puButtonBox : public puObject
{
protected:
  int one_only ;
  int num_kids ;
  char **button_labels ;

public:

  puButtonBox ( int minx, int miny, int maxx, int maxy, 
                char **labels, int one_button ) ;

  int isOneButton ( void ) { return one_only ; }

  int checkKey ( int key   , int updown ) ;
  int checkHit ( int button, int updown, int x, int y ) ;
  void draw    ( int dx, int dy ) ;
} ;



class puDialogBox : public puPopup
{
protected:
public:

  puDialogBox ( int x, int y ) : puPopup ( x, y )
  {
    type |= PUCLASS_DIALOGBOX ;
  }
} ;

#endif

