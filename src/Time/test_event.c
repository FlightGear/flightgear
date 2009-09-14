#include <stdio.h>

#include "event.h"

void a() {
    printf("Function a()\n");
    system("date");
}


void b() {
    printf("Function b()\n");
    system("date");
}


void c() {
    printf("Function c()\n");
    fgEventPrintStats();
}


void d() {
}


main() {
    fgEventInit();

    fgEventRegister("Function b()", b, FG_EVENT_READY, 1000);
    fgEventRegister("Function a()", a, FG_EVENT_READY, 2500);
    fgEventRegister("Function d()", d, FG_EVENT_READY, 500);
    fgEventRegister("Function c()", c, FG_EVENT_READY, 10000);

    while ( 1 ) {
        fgEventProcess();
    }
}
