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

smMixer::smMixer ()    {} 
smMixer::smMixer ( char * ) {} 
smMixer::~smMixer ()         {}
  
int smMixer::not_working () 
{ 
   return error ; 
}

  /* Volume controls are in integer percentages */

int  smMixer::getVolume ( int             ) { return 50 ; }
void smMixer::getVolume ( int, int *left, int *right )
{
  if ( left  ) *left  = 50 ;
  if ( right ) *right = 50 ;
}

void smMixer::setVolume ( int , int  ) {}
void smMixer::setVolume ( int , int , int  ){}
void smMixer::setTreble       ( int  ) {}
void smMixer::setBass         ( int    ) {}
void smMixer::setMasterVolume ( int  ) {}
void smMixer::setSynthVolume  ( int  ) {}
void smMixer::setPCMVolume    ( int  ) {}
void smMixer::setSpeakerVolume( int  ) {}
void smMixer::setLineVolume   ( int  ) {}
void smMixer::setMicVolume    ( int  ) {}
void smMixer::setCDVolume     ( int ) {}
void smMixer::setMasterVolume ( int, int ) {}
void smMixer::setSynthVolume  ( int, int ) {}
void smMixer::setPCMVolume    ( int, int ) {}
void smMixer::setSpeakerVolume( int, int ) {}
void smMixer::setLineVolume   ( int, int ) {}
void smMixer::setMicVolume    ( int, int ) {}
void smMixer::setCDVolume     ( int, int ) {} 


#else
/* ------------------------------------------------------------ */
/* win32                                                        */
/* ------------------------------------------------------------ */

void smMixer::open ( char * ) {}
  
void smMixer::close (){}

smMixer::smMixer ()    {} 
smMixer::smMixer ( char * ) {} 
smMixer::~smMixer ()         {}
  
int smMixer::not_working () 
{ 
   return error ; 
}

  /* Volume controls are in integer percentages */

int  smMixer::getVolume ( int             ) { return 50 ; }
void smMixer::getVolume ( int, int *left, int *right )
{
  if ( left  ) *left  = 50 ;
  if ( right ) *right = 50 ;
}

void smMixer::setVolume ( int, int ) {}
void smMixer::setVolume ( int, int, int ){}
void smMixer::setTreble       ( int ) {}
void smMixer::setBass         ( int   ) {}
void smMixer::setMasterVolume ( int ) {}
void smMixer::setSynthVolume  ( int ) {}
void smMixer::setPCMVolume    ( int ) {}
void smMixer::setSpeakerVolume( int ) {}
void smMixer::setLineVolume   ( int ) {}
void smMixer::setMicVolume    ( int ) {}
void smMixer::setCDVolume     ( int ) {}
void smMixer::setMasterVolume ( int, int ) {}
void smMixer::setSynthVolume  ( int, int ) {}
void smMixer::setPCMVolume    ( int, int ) {}
void smMixer::setSpeakerVolume( int, int ) {}
void smMixer::setLineVolume   ( int, int ) {}
void smMixer::setMicVolume    ( int, int ) {}
void smMixer::setCDVolume     ( int, int ) {} 


#endif
