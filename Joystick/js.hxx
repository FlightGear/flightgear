#ifndef __INCLUDED_JS_H__
#define __INCLUDED_JS_H__ 1

#ifdef __linux__
#  include <stdio.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <linux/joystick.h>
#endif

#define JS_TRUE  1
#define JS_FALSE 0

/*
  This is all set up for the older Linux and BSD drivers
  which restrict you to two axes.
*/

#define _JS_MAX_AXES 2

class jsJoystick
{
  JS_DATA_TYPE js    ;
  char         fname [ 128 ] ;
  int          error ;
  int          fd    ;

  int          num_axes ;

  float dead_band [ _JS_MAX_AXES ] ;
  float center    [ _JS_MAX_AXES ] ;
  float max       [ _JS_MAX_AXES ] ;
  float min       [ _JS_MAX_AXES ] ;

  void open ()
  {
    num_axes = _JS_MAX_AXES ;

    fd = ::open ( fname, O_RDONLY ) ;

    error = ( fd < 0 ) ;

    if ( error )
      return ;

    int counter = 0 ;

    /*
      The Linux driver seems to return 512 for all axes
      when no stick is present - but there is a chance
      that could happen by accident - so it's gotta happen
      on both axes for at least 100 attempts.
    */

    do
    { 
      rawRead ( NULL, center ) ;
      counter++ ;
    } while ( counter < 100 && center[0] == 512.0f && center[1] == 512.0f ) ;
   
    if ( counter >= 100 )
      error = JS_TRUE ;

    for ( int i = 0 ; i < _JS_MAX_AXES ; i++ )
    {
      max [ i ] = center [ i ] * 2.0f ;
      min [ i ] = 0.0f ;
      dead_band [ i ] = 0.0f ;
    }
  }

  void close ()
  {
    if ( ! error )
      ::close ( fd ) ;
  }

  float fudge_axis ( float value, int axis )
  {
    if ( value < center[axis] )
    {
      float xx = ( value - center[ axis ] ) /
		 ( center [ axis ] - min [ axis ] ) ;

      xx = ( xx > -dead_band [ axis ] ) ? 0.0f :
                  ( ( xx + dead_band [ axis ] ) / ( 1.0f - dead_band [ axis ] ) ) ;

      return ( xx < -1.0f ) ? -1.0f : xx ;
    }
    else
    {
      float xx = ( value - center [ axis ] ) /
		 ( max [ axis ] - center [ axis ] ) ;

      xx = ( xx < dead_band [ axis ] ) ? 0.0f :
                  ( ( xx - dead_band [ axis ] ) / ( 1.0f - dead_band [ axis ] ) ) ;

      return ( xx > 1.0f ) ? 1.0f : xx ;
    }
  }

public:

  jsJoystick ( int id = 0 )
  {
    sprintf ( fname, "/dev/js%d", id ) ;
    open () ;
  }

  ~jsJoystick ()
  {
    close () ;
  }

  int  getNumAxes () { return num_axes ; }
  int  notWorking () { return error ;    }
  void setError   () { error = JS_TRUE ; }

  float getDeadBand ( int axis )           { return dead_band [ axis ] ; }
  void  setDeadBand ( int axis, float db ) { dead_band [ axis ] = db   ; }

  void setMinRange ( float *axes ) { memcpy ( min   , axes, num_axes * sizeof(float) ) ; }
  void setMaxRange ( float *axes ) { memcpy ( max   , axes, num_axes * sizeof(float) ) ; }
  void setCenter   ( float *axes ) { memcpy ( center, axes, num_axes * sizeof(float) ) ; }

  void getMinRange ( float *axes ) { memcpy ( axes, min   , num_axes * sizeof(float) ) ; }
  void getMaxRange ( float *axes ) { memcpy ( axes, max   , num_axes * sizeof(float) ) ; }
  void getCenter   ( float *axes ) { memcpy ( axes, center, num_axes * sizeof(float) ) ; }

  void read ( int *buttons, float *axes )
  {
    if ( error )
    {
      if ( buttons )
        *buttons = 0 ;

      if ( axes )
        for ( int i = 0 ; i < _JS_MAX_AXES ; i++ )
          axes[i] = 0.0f ;
    }

    float raw_axes [ _JS_MAX_AXES ] ;

    rawRead ( buttons, raw_axes ) ;

    if ( axes )
      for ( int i = 0 ; i < _JS_MAX_AXES ; i++ )
        axes[i] = ( i < num_axes ) ? fudge_axis ( raw_axes[i], i ) : 0.0f ; 
  }

  void rawRead ( int *buttons, float *axes )
  {
    if ( error )
    {
      if ( buttons ) *buttons =   0  ;
      if ( axes )
        for ( int i = 0 ; i < _JS_MAX_AXES ; i++ )
          axes[i] = 1500.0f ;

      return ;
    }

    int status = ::read ( fd, &js, JS_RETURN ) ;

    if ( status != JS_RETURN )
    {
      perror ( fname ) ;
      setError () ;
      return ;
    }

    if ( buttons )
      *buttons = js.buttons ;

    if ( axes )
    {
      axes[0] = (float) js.x ;
      axes[1] = (float) js.y ;
    }
  }
} ;

#endif

