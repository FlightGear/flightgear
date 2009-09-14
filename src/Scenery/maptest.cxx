// a simple STL test

#include <map>    // stl
#include <string> // stl
#include <stdio.h>

using namespace std;

main() {
    map < string, int, less<string> > mymap;
    string name;
    int value;

    mymap["curt"] = 1;
    mymap["doug"] = 2;
    mymap["doug"] = 3;
    mymap["dan"] = 4;

    printf("curt = %d\n", mymap["curt"]);
    printf("doug = %d\n", mymap["doug"]);
}
