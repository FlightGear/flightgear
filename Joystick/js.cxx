
#include "js.hxx"

jsJoystick js0 ( 0 ) ;
jsJoystick js1 ( 1 ) ;

int main ()
{
  printf ( "Joystick test program.\n" ) ;
  printf ( "~~~~~~~~~~~~~~~~~~~~~~\n" ) ;

  if ( js0 . notWorking () ) printf ( "Joystick 0 not detected\n" ) ;
  if ( js1 . notWorking () ) printf ( "Joystick 1 not detected\n" ) ;
  if ( js0 . notWorking () && js1 . notWorking () ) exit ( 1 ) ;

  js0 . setDeadBand ( 0, 0.1 ) ;
  js0 . setDeadBand ( 1, 0.1 ) ;
  js1 . setDeadBand ( 0, 0.1 ) ;
  js1 . setDeadBand ( 1, 0.1 ) ;

  float *ax0 = new float [ js0.getNumAxes () ] ;
  float *ax1 = new float [ js1.getNumAxes () ] ;

  while (1)
  {
    int b ;

    if ( ! js0 . notWorking () )
    {
      js0 . read ( &b, ax0 ) ;

      printf ( "JS0: b0:%s b1:%s X:%1.3f Y:%1.3f   ",
         ( b & 1 ) ? "on " : "off", ( b & 2 ) ? "on " : "off", ax0[0], ax0[1] ) ;
    }

    if ( ! js1 . notWorking () )
    {
      js1 . read ( &b, ax1 ) ;

      printf ( "JS1: b0:%s b1:%s X:%1.3f Y:%1.3f   ",
       ( b & 1 ) ? "on " : "off", ( b & 2 ) ? "on " : "off", ax1[0], ax1[1] ) ;
    }

    printf ( "\r" ) ;
    fflush ( stdout ) ;

    /* give other processes a chance */

    usleep ( 1 ) ;
  }

  exit ( 0 ) ;
}

