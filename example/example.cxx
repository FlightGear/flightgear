

#include "sl.h"
#include "sm.h"
#include <math.h>

/*
  Construct a sound scheduler and a mixer.
*/

slScheduler sched ( 8000 ) ;
smMixer mixer ;

int main ()
{
  mixer . setMasterVolume ( 30 ) ;
  sched . setSafetyMargin ( 0.128 ) ;

  /* Just for fun, let's make a one second synthetic engine sample... */

  Uchar buffer [ 8000 ] ;

  for ( int i = 0 ; i < 8000 ; i++ )
  {
    /* Sum some sin waves and convert to range 0..1 */

    float level = ( sin ( (double) i * 2.0 * M_PI / (8000.0/ 50.0) ) +
                    sin ( (double) i * 2.0 * M_PI / (8000.0/149.0) ) +
                    sin ( (double) i * 2.0 * M_PI / (8000.0/152.0) ) +
                    sin ( (double) i * 2.0 * M_PI / (8000.0/192.0) )
                  ) / 8.0f + 0.5f ;

    /* Convert to unsigned byte */

    buffer [ i ] = (Uchar) ( level * 255.0 ) ;
  }

  /* Set up four samples and a loop */

  slSample  *s = new slSample ( buffer, 8000 ) ;
  slSample *s1 = new slSample ( "scream.ub", & sched ) ;
  slSample *s2 = new slSample ( "zzap.wav" , & sched ) ;
  slSample *s3 = new slSample ( "cuckoo.au", & sched ) ;
  slSample *s4 = new slSample ( "wheeee.ub", & sched ) ;

  /* Mess about with some of the samples... */

  s1 -> adjustVolume ( 2.2  ) ;
  s2 -> adjustVolume ( 0.5  ) ;
  s3 -> adjustVolume ( 0.2  ) ;

  /* Play the engine sample continuously. */

  sched . loopSample ( s ) ;

  int tim = 0 ;  /* My periodic event timer. */

  slEnvelope pitch_envelope ( 3, SL_SAMPLE_LOOP ) ;
  slEnvelope p_envelope ( 1, SL_SAMPLE_ONE_SHOT ) ;
  slEnvelope volume_envelope ( 3, SL_SAMPLE_LOOP ) ;

  while ( SL_TRUE )
  {

   tim++ ;  /* Time passes */

    if ( tim % 200 == 0 ) sched.playSample ( s1 ) ;
    if ( tim % 180 == 0 ) sched.playSample ( s2 ) ;
    if ( tim % 150 == 0 ) sched.playSample ( s3 ) ;
    if ( tim % 120 == 0 ) sched.playSample ( s4 ) ;

    if ( tim == 60 ) {
	// introduce an envelope for our engine noise after 10 seconds

	pitch_envelope.setStep ( 0,  0.0, 1.0 ) ;
	pitch_envelope.setStep ( 1,  5.0, 2.0 ) ;
	pitch_envelope.setStep ( 2, 10.0, 1.0 ) ;

	p_envelope.setStep ( 0,  5.0, 2.0 ) ;

	volume_envelope.setStep ( 0,  0.0, 1.0 ) ;
	volume_envelope.setStep ( 1,  5.0, 2.0 ) ;
	volume_envelope.setStep ( 2, 10.0, 1.0 ) ;

	// scheduler -> playSample ( my_sample ) ;
	sched.addSampleEnvelope( s, 0, 0, &p_envelope, SL_PITCH_ENVELOPE );
	sched.addSampleEnvelope( s, 0, 1, &volume_envelope, SL_VOLUME_ENVELOPE);
    }


    /*
      For the sake of realism, I'll delay for 1/30th second to
      simulate a graphics update process.
    */

#ifdef WIN32
    Sleep ( 1000 / 30 ) ;      /* 30Hz */
#elif defined(sgi)
    sginap( 3 );               /* ARG */
#else
    usleep ( 1000000 / 30 ) ;  /* 30Hz */
#endif

    /*
      This would normally be called just before the graphics buffer swap
      - but it could be anywhere where it's guaranteed to get called
      fairly often.
    */

    sched . update () ;
  }
}

