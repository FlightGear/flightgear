
#include "sl.h"

slScheduler *slScheduler::current = NULL ;

void slScheduler::init ()
{
  current = this ;

  if ( not_working () )
  {
    fprintf ( stderr, "slScheduler: soundcard init failed.\n" ) ;
    setError () ;
    return ;
  }

  if ( getBps() != 8 )
  {
    fprintf ( stderr, "slScheduler: Needs a sound card that supports 8 bits per sample.\n" ) ;
    setError () ;
    return ;
  }

  if ( getStereo() )
  {
    fprintf ( stderr, "slScheduler: Needs a sound card that supports monophonic replay.\n" ) ;
    setError () ;
    return ;
  }

  for ( int i = 0 ; i < SL_MAX_SAMPLES ; i++ )
    samplePlayer [ i ] = NULL ;

  amount_left = 0 ;
  now = 0 ;
  num_pending_callbacks = 0 ;
  safety_margin = 1.0 ;

  mixer = NULL ;

  mixer_buffer  = NULL ;
  spare_buffer0 = NULL ;
  spare_buffer1 = NULL ;
  spare_buffer2 = NULL ;

  initBuffers () ;
}

void slScheduler::initBuffers ()
{
  if ( not_working () ) return ;

  delete mixer_buffer ;
  delete spare_buffer0 ;
  delete spare_buffer1 ;
  delete spare_buffer2 ;

  mixer_buffer_size = getDriverBufferSize () ;

  mixer_buffer = new Uchar [ mixer_buffer_size ] ;
  memset ( mixer_buffer, 0x80, mixer_buffer_size ) ;

  spare_buffer0 = new Uchar [ mixer_buffer_size ] ;
  spare_buffer1 = new Uchar [ mixer_buffer_size ] ;
  spare_buffer2 = new Uchar [ mixer_buffer_size ] ;
}

slScheduler::~slScheduler ()
{
  if ( current == this )
    current = NULL ;

  delete mixer_buffer ;

  delete spare_buffer0 ;
  delete spare_buffer1 ;
  delete spare_buffer2 ;
}





void slScheduler::mixBuffer ( slSamplePlayer *spa, slSamplePlayer *spb )
{
  register int    l = mixer_buffer_size ;
  register Uchar *d = mixer_buffer ;

  register Uchar *a = spare_buffer0 ;
  register Uchar *b = spare_buffer1 ;

  spa -> read ( l, a ) ;
  spb -> read ( l, b ) ;

  while ( l-- ) *d++ = mix ( *a++, *b++ ) ;
}



void slScheduler::mixBuffer ( slSamplePlayer *spa, slSamplePlayer *spb,
                              slSamplePlayer *spc )
{
  register int    l = mixer_buffer_size ;
  register Uchar *d = mixer_buffer ;

  register Uchar *a = spare_buffer0 ;
  register Uchar *b = spare_buffer1 ;
  register Uchar *c = spare_buffer2 ;

  spa -> read ( l, a ) ;
  spb -> read ( l, b ) ;
  spc -> read ( l, c ) ;

  while ( l-- ) *d++ = mix ( *a++, *b++, *c++ ) ;
}


void slScheduler::realUpdate ( int dump_first )
{
  int i ;

  if ( not_working () )
    return ;

  while ( secondsUsed() <= safety_margin )
  {
    slSamplePlayer *psp [ 3 ] ;
    int             pri [ 3 ] ;

    pri [ 0 ] = pri [ 1 ] = pri [ 2 ] =  -1  ;

    for ( i = 0 ; i < SL_MAX_SAMPLES ; i++ )
    {
      if ( samplePlayer [ i ] == NULL )
	continue ;

      /* Clean up dead sample players */

      if ( samplePlayer [ i ] -> isDone () )
      {
	delete samplePlayer [ i ] ;
	samplePlayer [ i ] = NULL ;
	continue ;
      }

      if ( samplePlayer [ i ] -> isPaused () )
	continue ;

      int lowest = ( pri [0] <= pri [2] ) ?
		     (( pri [0] <= pri [1] ) ? 0 : 1 ) :
		     (( pri [1] <= pri [2] ) ? 1 : 2 ) ;

      if ( samplePlayer[i]->getPriority() > pri[lowest] )
      {
	psp[lowest] = samplePlayer[i] ;
	pri[lowest] = samplePlayer[i]->getPriority() ;
      }
    }

    for ( i = 0 ; i < SL_MAX_SAMPLES ; i++ )
    {
      if ( samplePlayer [ i ] == NULL )
	continue ;

      if ( ! samplePlayer [ i ] -> isPaused () &&
           samplePlayer [ i ] != psp[0] &&
           samplePlayer [ i ] != psp[1] &&
           samplePlayer [ i ] != psp[2] )
      {
        samplePlayer [ i ] -> preempt ( mixer_buffer_size ) ;
      }
    }

    if ( pri[0] < 0 )
    {
      memset ( mixer_buffer, 0x80, mixer_buffer_size ) ;
      amount_left = 0 ;
    }
    else
    if ( pri[1] < 0 )
      psp[0] -> read ( mixer_buffer_size, mixer_buffer ) ;
    else
    if ( pri[2] < 0 )
      mixBuffer ( psp[0], psp[1] ) ;
    else
      mixBuffer ( psp[0], psp[1], psp[2] ) ;

    if ( dump_first )
    {
      stop () ;
      dump_first = SL_FALSE ;
    }

    play ( mixer_buffer, mixer_buffer_size ) ;

    now += mixer_buffer_size ;
  }

  flushCallBacks () ;
}

void slScheduler::addCallBack ( slCallBack c, slSample *s, slEvent e, int m )
{
  if ( num_pending_callbacks >= SL_MAX_CALLBACKS )
  {
    fprintf ( stderr, "slScheduler: Too many pending callback events!\n" ) ;
    return ;
  }

  slPendingCallBack *p = & ( pending_callback [ num_pending_callbacks++ ] ) ;

  p -> callback = c ;
  p -> sample   = s ;
  p -> event    = e ;
  p -> magic    = m ;
}

void slScheduler::flushCallBacks ()
{
  /*
    Execute all the callbacks that we accumulated
    in this iteration.

    This is done at the end of 'update' to reduce the risk
    of nasty side-effects caused by 'unusual' activities
    in the application's callback function.
  */

  while ( num_pending_callbacks > 0 )
  {
    slPendingCallBack *p = & ( pending_callback [ --num_pending_callbacks ] ) ;

    if ( p -> callback )
      (*(p->callback))( p->sample, p->event, p->magic ) ;
  }
}


