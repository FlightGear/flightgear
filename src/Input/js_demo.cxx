#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <string.h>		// plib/js.h should really include this !!!!!!
#include <plib/js.h>

#define Z 8

int main ( int, char ** )
{
  jsJoystick *js[Z] ;
  float      *ax[Z] ;
  int i, j, t, useful[Z];

  jsInit();

  for ( i = 0; i < Z; i++ )
      js[i] = new jsJoystick ( i ) ;

  printf ( "Joystick test program.\n" ) ;
  printf ( "~~~~~~~~~~~~~~~~~~~~~~\n" ) ;

  t = 0;
  for ( i = 0; i < Z; i++ )
  { useful[i] = ! ( js[i]->notWorking () );
    if ( useful[i] ) {
         t++;
         printf ( "Joystick %i: \"%s\"\n", i, js[i]->getName() ) ;
    } else printf ( "Joystick %i not detected\n", i ) ;
  }
  if ( t == 0 ) exit ( 1 ) ;

  for ( i = 0; i < Z; i++ )
    if ( useful[i] )
       ax[i] = new float [ js[i]->getNumAxes () ] ;

  for ( i = 0 ; i < Z ; i++ )
    if ( useful[i] )
       printf ( "+--------------------JS.%d----------------------", i ) ;

  printf ( "+\n" ) ;

  for ( i = 0 ; i < Z ; i++ )
   if ( useful[i] )
   {
    if ( js[i]->notWorking () )
      printf ( "|           ~~~ Not Detected ~~~             " ) ;
    else
    {
      printf ( "| Btns " ) ;

      for ( j = 0 ; j < js[i]->getNumAxes () ; j++ )
        printf ( "Ax:%1d ", j ) ;

      for ( ; j < 8 ; j++ )
        printf ( "     " ) ;
    }
   }

  printf ( "|\n" ) ;

  for ( i = 0 ; i < Z ; i++ )
    if ( useful[i] )
      printf ( "+----------------------------------------------" ) ;

  printf ( "+\n" ) ;

  while (1)
  {
    for ( i = 0 ; i < Z ; i++ )
    if ( useful[i] )
     {
      if ( js[i]->notWorking () )
        printf ( "|  .   .   .   .   .   .   .   .   .   .   . " ) ;
      else
      {
        int b ;

        js[i]->read ( &b, ax[i] ) ;

        printf ( "| %04x ", b ) ;

	for ( j = 0 ; j < js[i]->getNumAxes () ; j++ )
	  printf ( "%+.1f ", ax[i][j] ) ;

	for ( ; j < 8 ; j++ )
	  printf ( "  .  " ) ;
      }
     }

    printf ( "|\r" ) ;
    fflush ( stdout ) ;

    /* give other processes a chance */

#ifdef _WIN32
    Sleep ( 1 ) ;
#elif defined(sgi)
    sginap ( 1 ) ;
#else
    usleep ( 1000 ) ;
#endif
  }

  return 0 ;
}


