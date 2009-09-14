
#include "puLocal.h"

void puOneShot::doHit ( int button, int updown, int x, int y )
{
  puButton::doHit ( button, updown, x, y ) ;
  setValue ( 0 ) ;
}

