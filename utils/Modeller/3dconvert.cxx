#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <iostream>

#include <GL/glut.h>

#include <plib/ssg.h>

using std::cerr;
using std::endl;


int
main (int ac, char ** av)
{
    if (ac != 3) {
        cerr << "Usage: " << av[0] << " <file_in> <file_out>" << endl;
        return 1;
    }

    int fakeac = 1;
    char * fakeav[] = { "3dconvert",
                        "Convert a 3D Model",
                        0 };
    glutInit(&fakeac, fakeav);
    glutCreateWindow(fakeav[1]);

    ssgInit();
    ssgEntity * object = ssgLoad(av[1]);
    if (object == 0) {
        cerr << "Failed to load " << av[1] << endl;
        return 2;
    }

    ssgSave(av[2], object);
}
