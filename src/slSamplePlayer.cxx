
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


void slSamplePlayer::read ( int nframes, Uchar *dst, int next_env )
{
  /*
    WARNING:

       CO-RECURSIVE!
  */

  /* Find the next envelope */

  while ( next_env < SL_MAX_ENVELOPES && env [ next_env ] == NULL )
    next_env++ ;

  /*
    If there are no fancy envelopes to process then return
    the raw data.
  */

  if ( next_env >= SL_MAX_ENVELOPES ) /* No fancy envelopes left */
  {
    low_read ( nframes, dst ) ;
    return ;
  }

  /*
    Envelope processing required...

    Process the next envelope using data read recursively through
    the remaining envelopes.
  */

  switch ( env_type [ next_env ] )
  {
    /* For Volume envelopes, SRC and DST can be the same buffer */

    case SL_INVERSE_VOLUME_ENVELOPE:
      read ( nframes, dst, next_env+1 ) ;
      env[ next_env ]->applyToInvVolume ( dst,dst,nframes,env_start_time[ next_env ] ) ;
      break ;

    case SL_VOLUME_ENVELOPE :
      read ( nframes, dst, next_env+1 ) ;
      env[ next_env ]->applyToVolume ( dst,dst,nframes,env_start_time[ next_env ] ) ;
      break ;

    case SL_INVERSE_PITCH_ENVELOPE :
      env[ next_env ]->applyToInvPitch ( dst,this,nframes,env_start_time[ next_env ], next_env+1 ) ;
      break ;

    case SL_PITCH_ENVELOPE  :
      env[ next_env ]->applyToPitch ( dst,this,nframes,env_start_time[ next_env ], next_env+1 ) ;
      break ;

    case SL_INVERSE_FILTER_ENVELOPE:
    case SL_FILTER_ENVELOPE :
      read ( nframes, dst, next_env+1 ) ;
      break ;

    case SL_INVERSE_PAN_ENVELOPE   :
    case SL_PAN_ENVELOPE    :
      read ( nframes, dst, next_env+1 ) ;
      break ;

    case SL_INVERSE_ECHO_ENVELOPE  :
    case SL_ECHO_ENVELOPE   :
      read ( nframes, dst, next_env+1 ) ;
      break ;
  }
}


void slSamplePlayer::low_read ( int nframes, Uchar *dst )
{
  if ( isWaiting() ) start () ;

  if ( bufferPos == NULL )  /* Run out of sample & no repeats */
  {
    memset ( dst, 0x80, nframes ) ;
    return ;
  }

  while ( SL_TRUE )
  {
    /*
      If we can satisfy this request in one read (with data left in
      the sample buffer ready for next time around) - then we are done...
    */

    if ( nframes < lengthRemaining )
    {
      memcpy ( dst, bufferPos, nframes ) ;
      bufferPos       += nframes ;
      lengthRemaining -= nframes ;
      return ;
    }

    memcpy ( dst, bufferPos, lengthRemaining ) ;
    bufferPos       += lengthRemaining ;
    dst             += lengthRemaining ;
    nframes         -= lengthRemaining ;
    lengthRemaining  = 0 ;

    if ( replay_mode == SL_SAMPLE_ONE_SHOT )
    {
      stop () ;
      memset ( dst, 0x80, nframes ) ;
      return ;
    }
    else
    {
      slScheduler::getCurrent() -> addCallBack ( callback, sample, SL_EVENT_LOOPED, magic ) ;
      start () ;
    }
  }
}



