#include <errno.h>
#include <math.h>

main() {
  double x;

  x = sqrt(-1);
  
  if ( errno == EDOM ) {
     printf("domain error\n");
  }

  printf("x = %.2f\n", x);

  if ( x < 0 ) {
	printf ("x < 0\n");
  }
  if ( x > 0 ) {
	printf ("x > 0\n");
  }
  if ( x > -9999.0 ) {
	printf ("x > 0\n");
  }
}
