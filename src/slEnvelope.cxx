
#include "sl.h"

float slEnvelope::getValue ( float _time )
{
  float delta ;
  int   step = getStepDelta ( &_time, &delta ) ;

  return delta * (_time - time[step]) + value[step] ;
}



int slEnvelope::getStepDelta ( float *_time, float *delta )
{
  float tt ;

  if ( replay_mode == SL_SAMPLE_LOOP )
  {
    tt = floor ( *_time / time [ nsteps-1 ] ) ; 
    *_time -= tt * time [ nsteps-1 ] ;
  }

  tt = *_time ;

  if ( tt <= time[    0   ] ) { *delta = 0.0f ; return 0 ; }
  if ( tt >= time[nsteps-1] ) { *delta = 0.0f ; return nsteps-1 ; }

  for ( int i = 1 ; i <= nsteps-1 ; i++ )
    if ( tt <= time[i] )
    {
      float t1 = time[i-1] ; float v1 = value[i-1] ;
      float t2 = time[ i ] ; float v2 = value[ i ] ;

      if ( t1 == t2 )
      {
        *delta = 0.0f ;
	return i ;
      }

      *delta = (v2-v1) / (t2-t1) ;
      return i-1 ;
    }

  *delta = 0.0f ;
  return nsteps - 1 ;
}

void slEnvelope::applyToVolume ( Uchar *dst, Uchar *src,
                                 int nframes, int start )
{
  float  delta ;
  float  _time = slScheduler::getCurrent() -> getElapsedTime ( start ) ;
  int     step = getStepDelta ( &_time, &delta ) ;
  float _value = delta * (_time - time[step]) + value[step] ;

  delta /= (float) slScheduler::getCurrent() -> getRate () ;

  while ( nframes-- )
  {
    register int res = (int)( (float)((int)*(src++)-0x80) * _value ) + 0x80 ;

    _value += delta ;

    *(dst++) = ( res > 255 ) ? 255 : ( res < 0 ) ? 0 : res ;
  }
}

void slEnvelope::applyToInvVolume ( Uchar *dst, Uchar *src,
                                    int nframes, int start )
{
  float  delta ;
  float  _time = slScheduler::getCurrent() -> getElapsedTime ( start ) ;
  int     step = getStepDelta ( &_time, &delta ) ;
  float _value = delta * (_time - time[step]) + value[step] ;

  delta /= (float) slScheduler::getCurrent() -> getRate () ;

  delta = - delta ;
  _value = 1.0 - _value ;

  while ( nframes-- )
  {
    register int res = (int)( (float)((int)*(src++)-0x80) * _value ) + 0x80 ;

    _value += delta ;

    *(dst++) = ( res > 255 ) ? 255 : ( res < 0 ) ? 0 : res ;
  }
}


