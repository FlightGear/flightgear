
#include <plib/js.h>

int main ( int, char ** )
{
  jsJoystick *js[2] ;
  float      *ax[2] ;

  js[0] = new jsJoystick ( 0 ) ;
  js[1] = new jsJoystick ( 1 ) ;

  printf ( "Joystick test program.\n" ) ;
  printf ( "~~~~~~~~~~~~~~~~~~~~~~\n" ) ;

  if ( js[0]->notWorking () ) printf ( "Joystick 0 not detected\n" ) ;
  if ( js[1]->notWorking () ) printf ( "Joystick 1 not detected\n" ) ;
  if ( js[0]->notWorking () && js[1]->notWorking () ) exit ( 1 ) ;

  ax[0] = new float [ js[0]->getNumAxes () ] ;
  ax[1] = new float [ js[1]->getNumAxes () ] ;

  int i, j ;

  for ( i = 0 ; i < 2 ; i++ )
    printf ( "+---------------JS.%d-----------------", i ) ;

  printf ( "+\n" ) ;

  for ( i = 0 ; i < 2 ; i++ )
  {
    if ( js[i]->notWorking () )
      printf ( "|       ~~~ Not Detected ~~~         " ) ;
    else
    {
      printf ( "| Btns " ) ;

      for ( j = 0 ; j < js[i]->getNumAxes () ; j++ )
        printf ( "Ax:%d ", j ) ;

      for ( ; j < 6 ; j++ )
        printf ( "     " ) ;
    }
  }

  printf ( "|\n" ) ;

  for ( i = 0 ; i < 2 ; i++ )
    printf ( "+------------------------------------" ) ;

  printf ( "+\n" ) ;

  while (1)
  {
    for ( i = 0 ; i < 2 ; i++ )
    {
      if ( js[i]->notWorking () )
        printf ( "|  .   .   .   .   .   .   .   .   . " ) ;
      else
      {
        int b ;

        js[i]->read ( &b, ax[i] ) ;

        printf ( "| %04x ", b ) ;

	for ( j = 0 ; j < js[i]->getNumAxes () ; j++ )
	  printf ( "%+.1f ", ax[i][j] ) ;

	for ( ; j < 6 ; j++ )
	  printf ( "  .  " ) ;
      }
    }

    printf ( "|\r" ) ;
    fflush ( stdout ) ;

    /* give other processes a chance */

#ifdef WIN32
    Sleep ( 1 ) ;
#elif defined(sgi)
    sginap ( 1 ) ;
#else
    usleep ( 1000 ) ;
#endif
  }

  return 0 ;
}


