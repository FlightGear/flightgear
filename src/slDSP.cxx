
#include "sl.h"

static int init_bytes ;

#ifdef SL_USING_OSS_AUDIO

/* ------------------------------------------------------------ */
/* OSSAUDIO - Linux, FreeBSD, etc                               */
/* ------------------------------------------------------------ */

void slDSP::open ( char *device, int _rate, int _stereo, int _bps )
{
  fd = ::open ( device, O_WRONLY ) ;

  if ( fd < 0 )
  {
    perror ( "slDSP: open" ) ;
    error = SL_TRUE ;

    stereo     = SL_FALSE ;
    bps        =    1     ;
    rate       =  8000    ;
    init_bytes =    0     ;
  }
  else
  {
    error = SL_FALSE ;

    /* Set up a driver fragment size of 1024 (ie 2^10) */

    ioctl ( SNDCTL_DSP_SETFRAGMENT, 0x7FFF000A ) ;

    stereo = ioctl ( SOUND_PCM_WRITE_CHANNELS, _stereo ? 2 : 1 ) >= 2 ; 
    bps    = ioctl ( SOUND_PCM_WRITE_BITS, _bps ) ; 
    rate   = ioctl ( SOUND_PCM_WRITE_RATE, _rate ) ; 

    getBufferInfo () ;
    init_bytes = buff_info.bytes ;
  }
}


void slDSP::close ()
{
  if ( fd >= 0 )
    ::close ( fd ) ;
}


int slDSP::getDriverBufferSize ()
{
  if ( error )
    return 0 ;

  getBufferInfo () ;
  return buff_info.fragsize ;
}

void slDSP::getBufferInfo ()
{
  if ( error )
    return ;

  if ( ::ioctl ( fd, SNDCTL_DSP_GETOSPACE, & buff_info ) < 0 )
  {
    perror ( "slDSP: getBufferInfo" ) ;
    error = SL_TRUE ;
    return ;
  }
}


void slDSP::write ( void *buffer, size_t length )
{
  if ( error || length <= 0 )
    return ;

  int nwritten = ::write ( fd, (const char *) buffer, length ) ;

  if ( nwritten < 0 )
    perror ( "slDSP: write" ) ;
  else
  if ( nwritten != length )
    perror ( "slDSP: short write" ) ;
}


float slDSP::secondsRemaining ()
{
  if ( error )
    return 0.0f ;

  getBufferInfo () ;

  int samples_left = buff_info.fragments * buff_info.fragsize ;

  if (   stereo  ) samples_left /= 2 ;
  if ( bps == 16 ) samples_left /= 2 ;
  return   (float) samples_left / (float) rate ;
}


float slDSP::secondsUsed ()
{
  if ( error )
    return 0.0f ;

  getBufferInfo () ;

  int samples_used = init_bytes - buff_info.bytes ;

  if (  stereo   ) samples_used /= 2 ;
  if ( bps == 16 ) samples_used /= 2 ;
  return   (float) samples_used / (float) rate ;
}


void slDSP::sync ()
{ 
   if ( !error) ::ioctl ( fd, SOUND_PCM_SYNC , 0 ) ; 
}

void slDSP::stop ()
{ 
   if ( !error) ::ioctl ( fd, SOUND_PCM_RESET, 0 ) ; 
}

#endif

#ifdef WIN32

/* ------------------------------------------------------------ */
/* win32                                                        */
/* ------------------------------------------------------------ */

static	void	wperror(MMRESULT num)
{
   char	buffer[0xff];  // yes, this is hardcoded :-)

   waveOutGetErrorText( num, buffer, sizeof(buffer)-1);

   fprintf( stderr, "SlDSP: %s\n", buffer );
   fflush ( stderr );
}



void CALLBACK waveOutProc( HWAVEOUT hwo, UINT uMsg,	
      DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{   
   switch( uMsg )
   {
   case    WOM_CLOSE:
      break;

   case    WOM_OPEN:
      break;

   case    WOM_DONE:
      waveOutUnprepareHeader( (HWAVEOUT)dwParam1, 
         (LPWAVEHDR)dwParam2, sizeof( WAVEHDR ));
      break;
   }
}


void slDSP::open ( char *device, int _rate, int _stereo, int _bps )
{
   MMRESULT	result;

   hWaveOut   	= NULL;
   curr_header	= 0;
   counter     = 0;

   Format.wFormatTag       = WAVE_FORMAT_PCM;		
   Format.nChannels        = _stereo ? 2 : 1;
   Format.nSamplesPerSec   = _rate;
   Format.wBitsPerSample   = _bps;
   Format.nBlockAlign      = 1;
   Format.nAvgBytesPerSec  = _rate * Format.nChannels;
   Format.cbSize           = 0;

   result = waveOutOpen( & hWaveOut, WAVE_MAPPER, & Format, NULL, 
      0L, WAVE_FORMAT_QUERY );

   if ( result != MMSYSERR_NOERROR )
   {
      wperror( result);

      error      = SL_TRUE  ;
      stereo     = SL_FALSE ;
      bps        = _bps     ;
      rate       = _rate    ;
      init_bytes =    0     ;
      
      return;
   }

   // Now the hwaveouthandle "should" be valid 

   if ( ( result = waveOutOpen( & hWaveOut, WAVE_MAPPER, 
         (WAVEFORMATEX *)& Format, (DWORD)waveOutProc, 
         0L, CALLBACK_FUNCTION )) != MMSYSERR_NOERROR )
   {
      wperror( result);

      error      = SL_TRUE ;
      stereo     = SL_FALSE ;
      bps        = _bps     ;
      rate       = _rate    ;
      init_bytes =    0     ;
      return;
   }
   else
   {
      error  = SL_FALSE ;
      stereo = _stereo;
      bps    = _bps;
      rate   = _rate;

      /* hmm ?! */ 

      init_bytes = 1024*8;
   }
}


void slDSP::close ()
{
   if ( hWaveOut != NULL )
   {
      waveOutClose( hWaveOut );
      hWaveOut = NULL;
   }
}

int slDSP::getDriverBufferSize ()
{
   if ( error )
      return 0 ;

   /* hmm ?! */

   return    1024*8;
}

void slDSP::getBufferInfo ()
{
    return ;
}


void slDSP::write ( void *buffer, size_t length )
{
   MMRESULT	result;

   if ( error || length <= 0 )
      return ;

   wavehdr[curr_header].lpData          = (LPSTR) buffer;
   wavehdr[curr_header].dwBufferLength  = (long) length;
   wavehdr[curr_header].dwBytesRecorded = 0L;
   wavehdr[curr_header].dwUser          = NULL;
   wavehdr[curr_header].dwFlags         = 0;
   wavehdr[curr_header].dwLoops         = 0;
   wavehdr[curr_header].lpNext          = NULL;
   wavehdr[curr_header].reserved        = 0;


   result = waveOutPrepareHeader( hWaveOut, & wavehdr[curr_header], 
      sizeof(WAVEHDR));

   if ( result != MMSYSERR_NOERROR ) 
   {
      wperror ( result );
      error = SL_TRUE;
   }

   result = waveOutWrite(hWaveOut, & wavehdr[curr_header], 
      sizeof(WAVEHDR));
   if ( result != MMSYSERR_NOERROR )
   {
      wperror ( result );
      error = SL_TRUE;
   }
   
   counter ++; 
   
   curr_header = ( curr_header + 1 ) % 3;
}


float slDSP::secondsRemaining ()
{
   if ( error )
      return 0.0f ;
   
   return 0.0f ;
}


float slDSP::secondsUsed ()
{
   int      samples_used;
   MMRESULT	result;
   float    samp_time;

   if ( error )
      return 0.0f ;

   mmt.wType = TIME_BYTES;

   result = waveOutGetPosition( hWaveOut, &mmt, sizeof( mmt ));

   if ( mmt.u.cb == 0 || counter == 0)
      return    (float)0.0;

   samples_used = ( init_bytes * counter ) - mmt.u.cb;

   if (  stereo   ) samples_used /= 2 ;
   if ( bps == 16 ) samples_used /= 2 ;

   samp_time  = (float) samples_used / (float) rate ;

   //printf("%0.2f position=%ld total written=%ld\n", 
   // samp_time, mmt.u.cb, init_bytes * counter );
   
   return   samp_time;
}


void slDSP::sync ()
{
}

void slDSP::stop ()
{
   if ( error )
     return ;

   waveOutReset( hWaveOut );
}

/* ------------------------------------------------------------ */
/* OpenBSD 2.3 this should be very close to SUN Audio           */
/* ------------------------------------------------------------ */

#elif defined(__OpenBSD__)
void slDSP::open ( char *device, int _rate, int _stereo, int _bps )
{

  counter = 0;
  
  fd = ::open ( device, O_RDWR ) ;
    
  if ( fd < 0 )
  {
    perror ( "slDSP: open" ) ;
    error = SL_TRUE ;
  }
  else
  {    
  
    if( ::ioctl( fd, AUDIO_GETINFO, &ainfo) == -1)
    {
      perror("slDSP: open - getinfo");
      stereo     = SL_FALSE ;
      bps        =    8     ;
      rate       =  8000    ;
      init_bytes =    0     ;
      
      return;
    }
      
    ainfo.play.sample_rate  = _rate;
    ainfo.play.precision    = _bps;    
    ainfo.play.channels     = _stereo ? 2 : 1;
    
    ainfo.play.encoding     = AUDIO_ENCODING_ULINEAR;

    if( :: ioctl(fd, AUDIO_SETINFO, &ainfo) == -1)
    {
      perror("slDSP: open - setinfo");
      stereo     = SL_FALSE ;
      bps        =    8     ;
      rate       =  8000    ;
      init_bytes =    0     ;
      return;
    }

    rate    = _rate;
    stereo  = _stereo;
    bps     = _bps;

    error = SL_FALSE ;

    getBufferInfo ();
    
    // I could not change the size, 
    // so let's try this ...
    
    init_bytes = 1024 * 8;
  }
}


void slDSP::close ()
{
  if ( fd >= 0 )
    ::close ( fd ) ;
}


int slDSP::getDriverBufferSize ()
{
  if ( error )
    return 0 ;

  getBufferInfo () ;
  
  // HW buffer is 0xffff on my box
  //return ainfo.play.buffer_size;
  
  return  1024 * 8;
}

void slDSP::getBufferInfo ()
{
  if ( error )
    return ;

  if( ::ioctl( fd, AUDIO_GETINFO, &ainfo) < 0)
  {
    perror ( "slDSP: getBufferInfo" ) ;
    error = SL_TRUE ;
    return ;
  }
    
  if( ::ioctl( fd, AUDIO_GETOOFFS, &audio_offset ) < 0)
  {
    perror ( "slDSP: getBufferInfo" ) ;
    error = SL_TRUE ;
    return ;
  }
}


void slDSP::write ( void *buffer, size_t length )
{
  if ( error || length <= 0 )
    return ;
  
  int nwritten = ::write ( fd, (const char *) buffer, length ) ;

  if ( nwritten < 0 )
    perror ( "slDSP: write" ) ;
  else if ( nwritten != length )
      perror ( "slDSP: short write" ) ;
      
  counter ++; /* hmmm */
}


float slDSP::secondsRemaining ()
{
    return 0.0f ;
}


float slDSP::secondsUsed ()
{
  /*
   * original formula from Steve:
   * -----------------------------
   *
   * int samples_used = init_bytes - buff_info.bytes ;
   *                    |            |
   *                    |            +--- current available
   *                    |                 space in bytes !
   *                    +---------------- available space
   *                                      when empty;
   * 
   * sample_used contains the number of bytes which are
   * "used" or in the DSP "pipeline".
   */


  int samples_used;
  
  if ( error )
    return 0.0f ;

  getBufferInfo () ;

  //This is wrong: this is the hw queue in the kernel !
  //samples_used   = ainfo.play.buffer_size - audio_offset.offset ;

  // This is: all data written minus where we are now in the queue
  
  if ( counter == 0 )
      return 0.0;

  samples_used = ( counter * init_bytes ) - audio_offset.samples;
  
  if (  stereo   ) samples_used /= 2 ;
  if ( bps == 16 ) samples_used /= 2 ;

  return   (float) samples_used / (float) rate ;
}


void slDSP::sync ()
{ 
   if ( !error) ::ioctl ( fd, AUDIO_FLUSH , 0 ) ; 
}

void slDSP::stop ()
{ 
   // nothing found yet 
}

/* ------------------------------------------------------------ */
/* SGI IRIX audio                                               */
/* ------------------------------------------------------------ */

#elif defined(sgi)

void slDSP::open ( char *device, int _rate, int _stereo, int _bps )
{
  if ( _bps != 8 )
  {
    perror ( "slDSP: supports only 8bit audio for sgi" ) ;
    error = SL_TRUE;
    return;
  }

  init_bytes = 1024 * 8;

  config  = ALnewconfig();
  
  ALsetchannels (  config, _stereo ? AL_STEREO : AL_MONO );
  ALsetwidth    (  config, _bps == 8 ? AL_SAMPLE_8 : AL_SAMPLE_16 );
  ALsetqueuesize(  config, init_bytes );

  port = ALopenport( device, "w", config );
    
  if ( port == NULL )
  {
    perror ( "slDSP: open" ) ;
    error = SL_TRUE ;
  }
  else
  {    
    long params[2] = {AL_OUTPUT_RATE, 0 };

    params[1] = _rate;

    if ( ALsetparams(AL_DEFAULT_DEVICE, params, 2) != 0 )
	{
       perror ( "slDSP: open - ALsetparams" ) ;
       error = SL_TRUE ;
       return;
	}
  
    rate    = _rate;
    stereo  = _stereo;
    bps     = _bps;

    error = SL_FALSE ;

  }
}


void slDSP::close ()
{
  if ( port != NULL )
  {
     ALcloseport ( port   );
     ALfreeconfig( config );
     port = NULL;
  }
}


int slDSP::getDriverBufferSize ()
{
  if ( error )
    return 0 ;

  return  ALgetqueuesize( config );
}

void slDSP::getBufferInfo ()
{
  if ( error )
    return ;
}


void slDSP::write ( void *buffer, size_t length )
{
  char *buf = (char *)buffer;
  int  i;

  if ( error || length <= 0 )
    return ;

  // Steve: is this a problem ??

  for ( i = 0; i < length; i ++ )
    buf[i] = buf[i] >> 1;

  ALwritesamps(port, (void *)buf, length );
}


float slDSP::secondsRemaining ()
{
  int   samples_remain;

  if ( error )
    return 0.0f ;

  samples_remain = ALgetfillable(port);

  if (  stereo   ) samples_remain /= 2 ;
  if ( bps == 16 ) samples_remain /= 2 ;

  return   (float) samples_remain / (float) rate ;
}


float slDSP::secondsUsed ()
{
  int   samples_used;
  
  if ( error )
    return 0.0f ;

  samples_used = ALgetfilled(port);

  if (  stereo   ) samples_used /= 2 ;
  if ( bps == 16 ) samples_used /= 2 ;

  return   (float) samples_used / (float) rate ;
}


void slDSP::sync ()
{ 
   if ( error )
     return ;

  /* found this in the header file - but no description
   * or example for the long parameter.
   */

  // ALflush(ALport, long);
}

void slDSP::stop ()
{ 
}


#endif

