
#ifndef __SM_H__
#define __SM_H__ 1

#include "slPortability.h"

#ifdef SL_USING_OSS_AUDIO
#define SMMIXER_DEFAULT_DEVICE "/dev/mixer"
// static char *labels [] = SOUND_DEVICE_LABELS;
#elif defined(WIN32)
#define SMMIXER_DEFAULT_DEVICE "mixer"
#else
#endif


# define SM_TRUE  1
# define SM_FALSE 0

typedef unsigned char  Uchar  ;
typedef unsigned short Ushort ;


class smMixer
{
private:

  int devices ;
  int error ;
  int fd ;

#ifdef SL_USING_OSS_AUDIO
  // static char *labels [] = SOUND_DEVICE_LABELS ;

  int ioctl ( int cmd, int param = 0 )
  {
    if ( error ) return param ;

    if ( ::ioctl ( fd, cmd, & param ) == -1 )
    {
      perror ( "smMixer: ioctl" ) ;
      error = SM_TRUE ;
    }

    return param ;
  }
#endif
  void open ( char *device ) ;
  void close () ;

public:

  /* Tom */

  smMixer ();
  smMixer ( char *device );
 ~smMixer ();
  
  int not_working ();

  /* Volume controls are in integer percentages */

  int  getVolume        ( int channel             );
  void setVolume        ( int channel, int volume ); 

  void getVolume        ( int channel, int *left, int *right );
  void setVolume        ( int channel, int  left, int  right );

  void setTreble       ( int treble );
  void setBass         ( int bass   );

  void setMasterVolume ( int volume );
  void setSynthVolume  ( int volume );
  void setPCMVolume    ( int volume );
  void setSpeakerVolume( int volume );
  void setLineVolume   ( int volume );
  void setMicVolume    ( int volume );
  void setCDVolume     ( int volume );

  void setMasterVolume ( int left, int right );
  void setSynthVolume  ( int left, int right );
  void setPCMVolume    ( int left, int right );
  void setSpeakerVolume( int left, int right );
  void setLineVolume   ( int left, int right );
  void setMicVolume    ( int left, int right );
  void setCDVolume     ( int left, int right );
} ;

#endif

