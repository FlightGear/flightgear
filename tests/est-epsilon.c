#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdio.h>

#include <simgear/compiler.h>

#include SG_GLUT_H


int main() {
    GLfloat a, t;

    a = 1.0;

    do {
	printf("a = %.10f\n", a);
	a = a / 2.0;
	t = 1.0 + a;
    } while ( t > 1.0 );

    a = a + a;

    printf("Estimated GLfloat epsilon = %.10f\n", a);

    return(0);
}
