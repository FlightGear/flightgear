#include <stdio.h>
#include <Debug/fg_debug.h>

#include "interpolater.hxx"

main() {
    fgINTERPTABLE test("test.table");

    fgInitDebug();

    printf("-1.0 = %.2f\n", test.interpolate(-1.0));
    printf("0.0 = %.2f\n", test.interpolate(0.0));
    printf("2.9 = %.2f\n", test.interpolate(2.9));
    printf("3.0 = %.2f\n", test.interpolate(3.0));
    printf("3.5 = %.2f\n", test.interpolate(3.5));
    printf("4.0 = %.2f\n", test.interpolate(4.0));
    printf("4.5 = %.2f\n", test.interpolate(4.5));
    printf("5.2 = %.2f\n", test.interpolate(5.2));
    printf("8.0 = %.2f\n", test.interpolate(8.0));
    printf("8.5 = %.2f\n", test.interpolate(8.5));
    printf("9.0 = %.2f\n", test.interpolate(9.0));
    printf("10.0 = %.2f\n", test.interpolate(10.0));
}
