#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <math.h>
#include <GL/glut.h>
#include "pu.h"

/***********************************\
*                                   *
* These are the PUI widget pointers *
*                                   *
\***********************************/

puMenuBar   *main_menu_bar ;
puButton    *hide_menu_button ;
puDialogBox *dialog_box ;
puText      *dialog_box_message ;
puOneShot   *dialog_box_ok_button ;
puText      *timer_text ;
puSlider    *rspeedSlider;


/***********************************\
*                                   *
*  This is a generic tumbling cube  *
*                                   *
\***********************************/

GLfloat light_diffuse [] = {1.0, 0.0, 0.0, 1.0} ;  /* Red diffuse light. */
GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0} ;  /* Infinite light location. */

GLfloat cube_n[6][3] =  /* Normals */
{
 {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
 { 0.0,-1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0,-1.0}
} ;

GLint cube_i[6][4] =  /* Vertex indices */
{
  {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
  {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3}
} ;

GLfloat cube_v[8][3] =  /* Vertices */
{
  {-1.0,-1.0, 1.0}, {-1.0,-1.0,-1.0}, {-1.0, 1.0,-1.0}, {-1.0, 1.0, 1.0},
  { 1.0,-1.0, 1.0}, { 1.0,-1.0,-1.0}, { 1.0, 1.0,-1.0}, { 1.0, 1.0, 1.0}
} ;


static int firsttime;

void drawCube (void)
{

  if ( firsttime )
  {
    /*
      Deliberately do this only once - it's a better test of
      PUI's attempts to leave the OpenGL state undisturbed
    */

    firsttime = FALSE ;
    glLightfv      ( GL_LIGHT0, GL_DIFFUSE , light_diffuse  ) ;
    glLightfv      ( GL_LIGHT0, GL_POSITION, light_position ) ;
    glEnable       ( GL_LIGHT0     ) ;
    glEnable       ( GL_LIGHTING   ) ;
    glEnable       ( GL_DEPTH_TEST ) ;
    glMatrixMode   ( GL_PROJECTION ) ;
    gluPerspective ( 40.0, 1.0, 1.0, 10.0 ) ;
    glMatrixMode   ( GL_MODELVIEW ) ;
    gluLookAt      ( 0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 ) ;
    glTranslatef   ( 0.0, 0.0, -1.0 ) ;
    glRotatef      ( 60.0, 1.0, 0.0, 0.0 ) ;
  }

  glCullFace     ( GL_FRONT ) ;
  glEnable       ( GL_CULL_FACE ) ;
  //  glRotatef ( 1.0f, 0.0, 0.0, 1.0 ) ;  /* Tumble that cube! */

  glBegin ( GL_QUADS ) ;

  for ( int i = 0 ; i < 6 ; i++ )
  {
    glNormal3fv ( &cube_n[i][0] ) ;
    glVertex3fv ( cube_v[cube_i[i][0]] ) ; glVertex3fv ( cube_v[cube_i[i][1]] ) ;
    glVertex3fv ( cube_v[cube_i[i][2]] ) ; glVertex3fv ( cube_v[cube_i[i][3]] ) ;
  }

  glEnd () ;
}

/********************************\
*                                *
* End of cube renderer in OpenGL * 
*                                *
\********************************/


/**************************************\
*                                      *
* These three functions capture mouse  *
* and keystrokes (special and mundane) *
* from GLUT and pass them on to PUI.   *
*                                      *
\**************************************/

static void specialfn ( int key, int, int )
{
  puKeyboard ( key + PU_KEY_GLUT_SPECIAL_OFFSET, PU_DOWN ) ;
  glutPostRedisplay () ;
}

static void keyfn ( unsigned char key, int, int )
{
  puKeyboard ( key, PU_DOWN ) ;
  glutPostRedisplay () ;
}

static void motionfn ( int x, int y )
{
  puMouse ( x, y ) ;
  glutPostRedisplay () ;
}

static void mousefn ( int button, int updown, int x, int y )
{
  puMouse ( button, updown, x, y ) ;
  glutPostRedisplay () ;
}

/**************************************\
*                                      *
* This function redisplays the PUI and *
* the tumbling cube, flips the double  *
* buffer and then asks GLUT to post a  *
* redisplay command - so we re-render  *
* at maximum rate.                     *
*                                      *
\**************************************/

static void displayfn (void)
{
  /* Clear the screen */

  glClearColor ( 0.0, 0.0, 0.0, 1.0 ) ;
  glClear      ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

  /* Draw the tumbling cube */

  float val ; rspeedSlider->getValue ( &val ) ;

  glRotatef( 4*val, 15.0, 10.0 , 5.0 );

  drawCube  () ;

  /* Update the 'timer' */

  time_t t = time ( NULL ) ;
  timer_text -> setLabel ( ctime ( & t ) ) ;

  /* Make PUI redraw */

  puDisplay () ;
  
  /* Off we go again... */

  glutSwapBuffers   () ;
  glutPostRedisplay () ;
}


/***********************************\
*                                   *
* Here are the PUI widget callback  *
* functions.                        *
*                                   *
\***********************************/

void hide_menu_cb ( puObject *cb )
{
  if ( cb -> getValue () )
  {
    main_menu_bar -> reveal () ;
    hide_menu_button->setLegend ( "Hide Menu" ) ;
  }
  else
  {
    main_menu_bar -> hide   () ;
    hide_menu_button->setLegend ( "Show Menu" ) ;
  }
}


void go_away_cb ( puObject * )
{
  /*
    Delete the dialog box when its 'OK' button is pressed.

    This seems to crash on MSVC compilers - probably because
    I delete dialog_box - whose member function is calling
    this function. Hence we return to something that is
    in a distinctly 'iffy' state.
  */

  delete dialog_box ;
  dialog_box = NULL ;
}

void mk_dialog ( char *txt )
{
  dialog_box = new puDialogBox ( 150, 50 ) ;
  {
    new puFrame ( 0, 0, 400, 100 ) ;
    dialog_box_message   = new puText         ( 10, 70 ) ;
    dialog_box_message   -> setLabel          ( txt ) ;
    dialog_box_ok_button = new puOneShot      ( 180, 10, 240, 50 ) ;
    dialog_box_ok_button -> setLegend         ( "OK" ) ;
    dialog_box_ok_button -> makeReturnDefault ( TRUE ) ;
    dialog_box_ok_button -> setCallback       ( go_away_cb ) ;
  }
  dialog_box -> close  () ;
  dialog_box -> reveal () ;
}

void ni_cb ( puObject * )
{
  mk_dialog ( "Sorry, that function isn't implemented" ) ;
}

void about_cb ( puObject * )
{
  mk_dialog ( "This is the PUI 'complex' program" ) ;
}

void help_cb ( puObject * )
{
  mk_dialog ( "Sorry, no help is available for this demo" ) ;
}

void edit_cb ( puObject * )
{
}

void exit_cb ( puObject * )
{
  fprintf ( stderr, "Exiting PUI demo program.\n" ) ;
  exit ( 1 ) ;
}

/* Menu bar entries: */

char      *file_submenu    [] = {  "Exit", "Close", "--------", "Print", "--------", "Save", "New", NULL } ;
puCallback file_submenu_cb [] = { exit_cb, exit_cb,       NULL, ni_cb  ,       NULL,  ni_cb, ni_cb, NULL } ;

char      *edit_submenu    [] = { "Edit text", NULL } ;
puCallback edit_submenu_cb [] = {     edit_cb, NULL } ;

char      *help_submenu    [] = { "About...",  "Help", NULL } ;
puCallback help_submenu_cb [] = {   about_cb, help_cb, NULL } ;


void sliderCB( puObject *sliderObj)
{
  glutPostRedisplay();
}

int main ( int argc, char **argv )
{

  firsttime = TRUE;

  glutInitWindowSize    ( 640, 480 ) ;
  glutInit              ( &argc, argv ) ;
  glutInitDisplayMode   ( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH ) ;
  glutCreateWindow      ( "Complex PUI Application"  ) ;
  glutDisplayFunc       ( displayfn ) ;
  glutKeyboardFunc      ( keyfn     ) ;
  glutSpecialFunc       ( specialfn ) ;
  glutMouseFunc         ( mousefn   ) ;
  glutMotionFunc        ( motionfn  ) ;
  glutPassiveMotionFunc ( motionfn  ) ;
  glutIdleFunc          ( displayfn ) ;

  puInit () ;

#ifdef USING_3DFX
  puShowCursor () ;
#endif

  puSetDefaultStyle        ( PUSTYLE_SMALL_SHADED ) ;
  puSetDefaultColourScheme ( 0.8, 0.2, 0.2 ) ;

  timer_text = new puText ( 300, 10 ) ;
  timer_text -> setColour ( PUCOL_LABEL, 1.0, 1.0, 1.0 ) ;

  /* Make a button to hide the menu bar */

  hide_menu_button = new puButton     ( 10, 10, 150, 50 ) ;
  hide_menu_button->setValue          (    TRUE      ) ;
  hide_menu_button->setLegend         ( "Hide Menu"  ) ;
  hide_menu_button->setCallback       ( hide_menu_cb ) ;
  hide_menu_button->makeReturnDefault (    TRUE      ) ;

  /* Make the menu bar */

  main_menu_bar = new puMenuBar () ;
  {
    main_menu_bar -> add_submenu ( "File", file_submenu, file_submenu_cb ) ;
    main_menu_bar -> add_submenu ( "Edit", edit_submenu, edit_submenu_cb ) ;
    main_menu_bar -> add_submenu ( "Help", help_submenu, help_submenu_cb ) ;
  }
  main_menu_bar -> close () ; 

  rspeedSlider = new puSlider (20,80,150,TRUE);
  rspeedSlider->setDelta(0.1);
  rspeedSlider->setCBMode( PUSLIDER_DELTA );
  rspeedSlider->setCallback(sliderCB);

  glutMainLoop () ;
  return 0 ;
}


