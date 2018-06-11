#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined( unix ) || defined( __CYGWIN__ )
#  include <unistd.h>		// for gethostname()
#endif

#include <stdio.h>

int main() {
#if defined( unix ) || defined( __CYGWIN__ )
  char name[256];
  gethostname( name, 256 );
  printf("hostname = %s\n", name);
#endif
  return 0;
}
