#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>

#include <simgear/compiler.h>
#include <osg/GL>
#include <plib/pw.h>
#include <plib/pu.h>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include "layout.hxx"

// Takes a property file on the command line, lays it out, and writes
// the resulting tree back to stdout.  Requires that the
// "Helvetica.txf" font file from the base package be in the current
// directory.

// g++ -Wall -g -o layout layout.cxx layout-props.cxx layout-test.cxx
// -I/fg/include -L/fg/lib -I.. -lsgprops -lsgdebug -lsgstructure
// -lsgmisc -lsgxml -lplibpw -lplibpu -lplibfnt -lplibul -lGL

// We can't load a plib fntTexFont without a GL context, so we use the
// PW library to initialize things.  The callbacks are required, but
// just stubs.
void exitCB(){ pwCleanup(); exit(0); }
void resizeCB(int w, int h){ }
void mouseMotionCB(int x, int y){ puMouse(x, y); }
void mouseButtonCB(int button, int updn, int x, int y){ puMouse(button, updn, x, y); }
void keyboardCB(int key, int updn, int x, int y){ puKeyboard(key, updn, x, y); }

const char* FONT_FILE = "Helvetica.txf";

int main(int argc, char** argv)
{
    FILE* tmp;
    if(!(tmp = fopen(FONT_FILE, "r"))) {
        fprintf(stderr, "Could not open %s for reading.\n", FONT_FILE);
        exit(1);
    }
    fclose(tmp);

    pwInit(0, 0, 600, 400, 0, const_cast<char*>("Layout Test"), true, 0);
    pwSetCallbacks(keyboardCB, mouseButtonCB, mouseMotionCB,
                   resizeCB, exitCB);

    fntTexFont helv;
    helv.load(FONT_FILE);
    puFont puhelv(&helv);

    LayoutWidget::setDefaultFont(&puhelv, 15);
    SGPropertyNode props;
    readProperties(argv[1], &props);
    LayoutWidget w(&props);
    int pw=0, ph=0;
    w.calcPrefSize(&pw, &ph);
    w.layout(0, 0, pw, ph);
    writeProperties(std::cout, &props, true);
}
