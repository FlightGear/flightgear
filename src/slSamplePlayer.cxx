
#include "sl.h"

void slSamplePlayer::addEnvelope ( int i, slEnvelope *_env, slEnvelopeType _type )
{
  if ( i < 0 || i >= SL_MAX_ENVELOPES ) return ;

  if ( env [ i ] != NULL )
    env [ i ] -> unRef () ;

  env [ i ] = _env ;

  if ( _env != NULL )
    env [ i ] -> ref () ;

  env_type [ i ] = _type ;
  env_start_time [ i ] = slScheduler::getCurrent() -> getTimeNow () ;
}

int slSamplePlayer::preempt ( int delay )
{
  slScheduler::getCurrent() -> addCallBack ( callback, sample, SL_EVENT_PREEMPTED, magic ) ;

  switch ( preempt_mode )
  {
    case SL_SAMPLE_CONTINUE: if ( isRunning() )
			       return SL_FALSE ;
			     /* FALLTHROUGH! */
    case SL_SAMPLE_DELAY   :                   break ;
    case SL_SAMPLE_MUTE    : skip  ( delay ) ; break ;
    case SL_SAMPLE_ABORT   : stop  ()        ; break ;
    case SL_SAMPLE_RESTART : reset ()        ; break ;
  }

  return SL_TRUE ;
}

slSamplePlayer::~slSamplePlayer ()
{
  if ( sample )
    sample -> unRef () ;

  slScheduler::getCurrent() -> addCallBack ( callback, sample, SL_EVENT_COMPLETE, magic ) ;
}

void slSamplePlayer::skip ( int nframes )
{
  if ( nframes < lengthRemaining )
  {
    lengthRemaining -= nframes ;
    bufferPos       += nframes ;
  }
  else 
  if ( replay_mode == SL_SAMPLE_LOOP )
  {
    slScheduler::getCurrent() -> addCallBack ( callback, sample, SL_EVENT_LOOPED, magic ) ;

    nframes -= lengthRemaining ;

    while ( nframes >= sample->getLength () )
      nframes -= sample->getLength () ;

    lengthRemaining = sample->getLength() - nframes ;
    bufferPos = & ( sample->getBuffer() [ nframes ] ) ;
  }
  else
    stop () ;
}


Uchar *slSamplePlayer::read ( int nframes, Uchar *spare1, Uchar *spare2 )
{
  if ( isWaiting() ) start () ;

  if ( nframes > lengthRemaining ) /* This is an error */
  {
    fprintf ( stderr, "slSamplePlayer: FATAL ERROR - Mixer Requested too much data.\n" ) ;
    abort () ;
  }

  Uchar *src = bufferPos ;
  Uchar *dst = spare1 ;

  for ( int i = 0 ; i < SL_MAX_ENVELOPES ; i++ )
  {
    if ( env[i] )
    {
      switch ( env_type [ i ] )
      {
        case SL_INVERSE_PITCH_ENVELOPE :
        case SL_PITCH_ENVELOPE  :
          memcpy ( dst, src, nframes ) /* Tricky! */ ;
          break ;

        case SL_INVERSE_VOLUME_ENVELOPE:
          env[i]->applyToInvVolume ( dst,src,nframes,env_start_time[i] ) ;
          break ;

        case SL_VOLUME_ENVELOPE :
          env[i]->applyToVolume ( dst,src,nframes,env_start_time[i] ) ;
          break ;

        case SL_INVERSE_FILTER_ENVELOPE:
        case SL_FILTER_ENVELOPE :
          memcpy ( dst, src, nframes ) /* Tricky! */ ;
          break ;

        case SL_INVERSE_PAN_ENVELOPE   :
        case SL_PAN_ENVELOPE    :
          memcpy ( dst, src, nframes ) /* Tricky! */ ;
          break ;

        case SL_INVERSE_ECHO_ENVELOPE  :
        case SL_ECHO_ENVELOPE   :
          memcpy ( dst, src, nframes ) /* Tricky! */ ;
          break ;
      }

      if ( dst == spare1 )
      {
        src = spare1 ;
        dst = spare2 ;
      }
      else
      {
        dst = spare1 ;
        src = spare2 ;
      }
    }
  }
 
  if ( nframes < lengthRemaining ) /* Less data than there is left...  */
  {
    lengthRemaining -= nframes ;
    bufferPos       += nframes ;
  }
  else  /* Read it all */
  {
    if ( replay_mode == SL_SAMPLE_ONE_SHOT )
      stop () ;
    else
    {
      slScheduler::getCurrent() -> addCallBack ( callback, sample, SL_EVENT_LOOPED, magic ) ;
      start () ;
    }
  }

  return src ;
}

