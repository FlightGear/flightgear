#include "sm.h"

#ifdef SL_USING_OSS_AUDIO
/* ------------------------------------------------------------ */
/* OSSAUDIO - Linux, FreeBSD                                    */
/* ------------------------------------------------------------ */


void smMixer::open ( char *device )
{
  fd = ::open ( device, O_WRONLY ) ;

  if ( fd < 0 )
  {
    perror ( "smMixer: open" ) ;
    error = SM_TRUE ;
  }
  else
    error = SM_FALSE ;

  devices = ioctl ( SOUND_MIXER_READ_DEVMASK ) ;
}
  
void smMixer::close ()
{
  if ( fd >= 0 )
    ::close ( fd ) ;
}


smMixer::smMixer ()               
{ 
   open ( SMMIXER_DEFAULT_DEVICE ) ; 
} 

smMixer::smMixer ( char *device ) 
{ 
   open ( device ) ; 
} 

smMixer::~smMixer ()               
{ 
   close () ; 
}
  
int smMixer::not_working () 
{ 
   return error ; 
}

  /* Volume controls are in integer percentages */

int  smMixer::getVolume ( int channel             ) 
{ 
   return ioctl ( MIXER_READ  ( channel ) ) & 0xFF ; 
}


void smMixer::setVolume ( int channel, int volume ) 
{ 
   ioctl ( MIXER_WRITE ( channel ), (( volume & 255 ) << 8 ) |
                                     ( volume & 255 ) ) ; 
}

void smMixer::getVolume ( int channel, int *left, int *right )
{
   int vv = ioctl ( MIXER_READ ( channel ) ) ;

   if ( left  ) *left  =  vv     & 0xFF ;
   if ( right ) *right = (vv>>8) & 0xFF ;
}

void smMixer::setVolume ( int channel, int  left, int  right )
{
   ioctl ( MIXER_WRITE ( channel ), (( right & 255 ) << 8 ) | 
      ( left & 255 ) ) ;
}

void smMixer::setTreble       ( int treble ) 
{ 
   setVolume ( SOUND_MIXER_TREBLE , treble ) ; 
}

void smMixer::setBass         ( int bass   ) 
{ 
   setVolume ( SOUND_MIXER_TREBLE , bass   ) ; 
}

void smMixer::setMasterVolume ( int volume ) 
{ 
   setVolume ( SOUND_MIXER_VOLUME , volume ) ; 
}

void smMixer::setSynthVolume  ( int volume ) 
{ 
   setVolume ( SOUND_MIXER_SYNTH  , volume ) ; 
}

void smMixer::setPCMVolume    ( int volume ) 
{ 
   setVolume ( SOUND_MIXER_PCM    , volume ) ; 
}

void smMixer::setSpeakerVolume( int volume ) 
{ 
   setVolume ( SOUND_MIXER_SPEAKER, volume ) ; 
}

void smMixer::setLineVolume   ( int volume ) 
{ 
   setVolume ( SOUND_MIXER_LINE   , volume ) ; 
}

void smMixer::setMicVolume    ( int volume ) 
{ 
   setVolume ( SOUND_MIXER_MIC    , volume ) ; 
}

void smMixer::setCDVolume     ( int volume ) 
{ 
   setVolume ( SOUND_MIXER_CD     , volume ) ; 
}

void smMixer::setMasterVolume ( int left, int right ) 
{ 
   setVolume ( SOUND_MIXER_VOLUME , left, right ) ; 
}

void smMixer::setSynthVolume  ( int left, int right ) 
{  
   setVolume ( SOUND_MIXER_SYNTH  , left, right ) ; 
}

void smMixer::setPCMVolume    ( int left, int right ) 
{ 
   setVolume ( SOUND_MIXER_PCM    , left, right ) ; 
}

void smMixer::setSpeakerVolume( int left, int right ) 
{ 
   setVolume ( SOUND_MIXER_SPEAKER, left, right ) ; 
}

void smMixer::setLineVolume   ( int left, int right ) 
{ 
   setVolume ( SOUND_MIXER_LINE   , left, right ) ; 
}

void smMixer::setMicVolume    ( int left, int right ) 
{ 
   setVolume ( SOUND_MIXER_MIC    , left, right ) ; 
}

void smMixer::setCDVolume     ( int left, int right ) 
{ 
   setVolume ( SOUND_MIXER_CD     , left, right ) ; 
}

#elif defined(__OpenBSD__)

/* ------------------------------------------------------------ */
/* OpenBSD 2.3                                                  */
/* ------------------------------------------------------------ */

void smMixer::open ( char *device )
{
}
  
void smMixer::close (){}

smMixer::smMixer ()    { } 
smMixer::smMixer ( char *device ) { } 
smMixer::~smMixer ()         {}
  
int smMixer::not_working () 
{ 
   return error ; 
}

  /* Volume controls are in integer percentages */

int  smMixer::getVolume ( int channel             ) { return 50 ; }
void smMixer::getVolume ( int channel, int *left, int *right )
{
  if ( left  ) *left  = 50 ;
  if ( right ) *right = 50 ;
}

void smMixer::setVolume ( int channel, int volume ) {}
void smMixer::setVolume ( int channel, int  left, int  right ){}
void smMixer::setTreble       ( int treble ) {}
void smMixer::setBass         ( int bass   ) {}
void smMixer::setMasterVolume ( int volume ) {}
void smMixer::setSynthVolume  ( int volume ) {}
void smMixer::setPCMVolume    ( int volume ) {}
void smMixer::setSpeakerVolume( int volume ) {}
void smMixer::setLineVolume   ( int volume ) {}
void smMixer::setMicVolume    ( int volume ) {}
void smMixer::setCDVolume     ( int volume ) {}
void smMixer::setMasterVolume ( int left, int right ) {}
void smMixer::setSynthVolume  ( int left, int right ) {}
void smMixer::setPCMVolume    ( int left, int right ) {}
void smMixer::setSpeakerVolume( int left, int right ) {}
void smMixer::setLineVolume   ( int left, int right ) {}
void smMixer::setMicVolume    ( int left, int right ) {}
void smMixer::setCDVolume     ( int left, int right ) {} 


#else
/* ------------------------------------------------------------ */
/* win32                                                        */
/* ------------------------------------------------------------ */

void smMixer::open ( char *device )
{
}
  
void smMixer::close (){}

smMixer::smMixer ()    { } 
smMixer::smMixer ( char *device ) { } 
smMixer::~smMixer ()         {}
  
int smMixer::not_working () 
{ 
   return error ; 
}

  /* Volume controls are in integer percentages */

int  smMixer::getVolume ( int channel             ) { return 50 ; }
void smMixer::getVolume ( int channel, int *left, int *right )
{
  if ( left  ) *left  = 50 ;
  if ( right ) *right = 50 ;
}

void smMixer::setVolume ( int channel, int volume ) {}
void smMixer::setVolume ( int channel, int  left, int  right ){}
void smMixer::setTreble       ( int treble ) {}
void smMixer::setBass         ( int bass   ) {}
void smMixer::setMasterVolume ( int volume ) {}
void smMixer::setSynthVolume  ( int volume ) {}
void smMixer::setPCMVolume    ( int volume ) {}
void smMixer::setSpeakerVolume( int volume ) {}
void smMixer::setLineVolume   ( int volume ) {}
void smMixer::setMicVolume    ( int volume ) {}
void smMixer::setCDVolume     ( int volume ) {}
void smMixer::setMasterVolume ( int left, int right ) {}
void smMixer::setSynthVolume  ( int left, int right ) {}
void smMixer::setPCMVolume    ( int left, int right ) {}
void smMixer::setSpeakerVolume( int left, int right ) {}
void smMixer::setLineVolume   ( int left, int right ) {}
void smMixer::setMicVolume    ( int left, int right ) {}
void smMixer::setCDVolume     ( int left, int right ) {} 


#endif
